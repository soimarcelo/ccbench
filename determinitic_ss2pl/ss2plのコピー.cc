// #include "gflags/gflags.h"
#include "../include/zipf.hh"
#include "../include/random.hh"

#include <iostream>
#include <vector>
#include <thread>
#include <functional>
#include <atomic>
#include <string>
#include <algorithm>

#define PAGE_SIZE 4096
#define THREAD_NUM 10
#define TUPLE_NUM 1000000
#define MAX_OPE 10
#define RW_RATE 50
#define EX_TIME 3
#define PRE_NUM 1000000

uint64_t tx_counter;

// DEFINE_uint64(tuple_num, 1000000, "Total number of records");

class Result
{
public:
    uint64_t commit_cnt_;
};

std::vector<Result> AllResult(THREAD_NUM);

enum class Ope
{
    READ,
    WRITE,
};
class Task
{
public:
    Ope ope_;
    uint64_t key_;

    Task() : ope_(Ope::READ), key_(0) {}

    Task(Ope ope, uint64_t key) : ope_(ope), key_(key) {}
};

class RWLock
{
public:
    std::atomic<int> counter;
    RWLock() { counter.store(0, std::memory_order_release); }
    bool r_try_lock()
    {
        int expected, desired;
        expected = counter.load(std::memory_order_acquire);
        for (;;)
        {
            if (expected != -1)
            {
                desired = expected + 1;
            }
            else
            {
                return false;
            }
            if (counter.compare_exchange_strong(
                    expected, desired, std::memory_order_acq_rel, std::memory_order_acquire))
            {
                return true;
            }
        }
    }

    bool w_try_lock()
    {
        int expected, desired(-1);
        expected = counter.load(std::memory_order_acquire);
        for (;;)
        {
            if (expected != 0)
            {
                return false;
            }

            if (counter.compare_exchange_strong(
                    expected, desired, std::memory_order_acq_rel, std::memory_order_acquire))
            {
                return true;
            }
        }
    }

    bool try_upgrade()
    {
        int expected, desired(-1);
        expected = counter.load(std::memory_order_acquire);
        for (;;)
        {
            if (expected != 1)
                return false;

            if (counter.compare_exchange_strong(expected, desired,
                                                std::memory_order_acq_rel))
                return true;
        }
    }

    void r_unlock()
    {
        counter--;
    }
    void w_unlock()
    {
        counter++;
    }
};

RWLock lock_for_locks;

class Tuple
{
public:
    RWLock lock_;
    uint64_t value_;
};

class ReadOperation
{
public:
    uint64_t key_;
    uint64_t value_;
    Tuple *tuple_;

    ReadOperation(uint64_t key, uint64_t value, Tuple *tuple) : key_(key), value_(value), tuple_(tuple) {}
};

class WriteOperation
{
public:
    uint64_t key_;
    uint64_t value_;
    Tuple *tuple_;

    WriteOperation(uint64_t key, uint64_t value, Tuple *tuple) : key_(key), value_(value), tuple_(tuple) {}
};

Tuple *Table;

enum class Status
{
    IN_FLIGHT,
    COMMITTED,
    ABORTED
};

class Pre
{
public:
    std::vector<Task> task_set_;
};

std::vector<Pre> Pre_tx_set(PRE_NUM);

class Transaction
{
public:
    Status status_;
    std::vector<Task> task_set_;
    std::vector<RWLock *> r_lock_list_;
    std::vector<RWLock *> w_lock_list_;
    std::vector<ReadOperation> read_set_;
    std::vector<WriteOperation> write_set_;
    Transaction() : status_(Status::IN_FLIGHT) {}

    void read(const uint64_t key)
    {

        Tuple *tuple = &Table[key];

        // tuple->lock_.read_lock();

        uint64_t read_value = __atomic_load_n(&tuple->value_, __ATOMIC_SEQ_CST);
        read_set_.emplace_back(key, read_value, tuple);
        return;
    }

    void write(const uint64_t key)
    {

        Tuple *tuple = &Table[key];

        __atomic_store_n(&tuple->value_, 100, __ATOMIC_SEQ_CST);
        write_set_.emplace_back(key, 100, tuple);
        return;
    }

    void begin()
    {
        status_ = Status::IN_FLIGHT;
        return;
    }

    void commit()
    {
        for (auto &lock : r_lock_list_)
        {
            lock->r_unlock();
        }
        for (auto &lock : w_lock_list_)
        {
            lock->w_unlock();
        }
        r_lock_list_.clear();
        w_lock_list_.clear();

        read_set_.clear();
        write_set_.clear();
        return;
    }
    void abort()
    {
        for (auto &lock : r_lock_list_)
        {
            lock->r_unlock();
        }
        for (auto &lock : w_lock_list_)
        {
            lock->w_unlock();
        }
        r_lock_list_.clear();
        w_lock_list_.clear();

        read_set_.clear();
        write_set_.clear();
        return;
    }
};

void makeTask(std::vector<Task> &tasks, Xoroshiro128Plus &rnd, FastZipf &zipf)
{
    tasks.clear();
    for (size_t i = 0; i < MAX_OPE; ++i)
    {
        uint64_t random_gen_key = zipf();
        // std::cout << random_gen_key << std::endl;
        assert(random_gen_key < TUPLE_NUM);

        if ((rnd.next() % 100) < RW_RATE)
        {
            // std::cout << "read" << std::endl;
            tasks.emplace_back(Ope::READ, random_gen_key);
        }
        else
        {
            // std::cout << "write" << std::endl;
            tasks.emplace_back(Ope::WRITE, random_gen_key);
        }
    }
    // std::sort(tasks.begin(), tasks.end());
}

void makeDB()
{
    posix_memalign((void **)&Table, PAGE_SIZE, TUPLE_NUM * sizeof(Tuple));
    for (int i = 0; i < TUPLE_NUM; i++)
    {
        Table[i].value_ = 0;
    }
}

void worker(int thread_id, int &ready, const bool &start, const bool &quit)
{
    Result &myres = std::ref(AllResult[thread_id]);

    Transaction trans;
    uint64_t tx_pos;

    __atomic_store_n(&ready, 1, __ATOMIC_SEQ_CST);

    while (!__atomic_load_n(&start, __ATOMIC_SEQ_CST))
    {
    }

POINT:

    while (!__atomic_load_n(&quit, __ATOMIC_SEQ_CST))
    {

        // aquire giant lock
        if (!lock_for_locks.w_try_lock())
        {
            goto POINT;
        }

        //取得すべきtxの現在地　ロック必要か
        tx_pos = __atomic_load_n(&tx_counter, __ATOMIC_SEQ_CST);
        __atomic_store_n(&tx_counter, tx_pos + 1, __ATOMIC_SEQ_CST);

    RETRY:
        if (__atomic_load_n(&quit, __ATOMIC_SEQ_CST))
            break;

        // pre_tx_setからコピー
        Pre &work_tx = std::ref(Pre_tx_set[tx_pos]);
        trans.task_set_ = work_tx.task_set_;

        trans.begin();
        // request all locks
        for (auto &item : trans.task_set_)
        {
            // search tuple
            Tuple *tuple = &Table[item.key_];
            // if the item has been alredy read or writen in the tx, skip the ope or upgrade the lock

            int count = 0;
            // check r_lock_list_ and w_lock_list_
            switch (item.ope_)
            {
            case Ope::READ:
                for (auto &r_lock : trans.r_lock_list_)
                {
                    count++;
                    if (r_lock == &tuple->lock_)
                    {
                        // delete from task.set
                        // trans.r_lock_list_.erase(trans.r_lock_list_.begin() + count - 1);
                        break;
                    }
                }
                for (auto &w_lock : trans.w_lock_list_)
                {
                    count++;
                    if (w_lock == &tuple->lock_)
                    {
                        // delete from task.set
                        // trans.w_lock_list_.erase(trans.w_lock_list_.begin() + count - 1);
                        break;
                    }
                }
                if (!tuple->lock_.r_try_lock())
                {
                    trans.status_ = Status::ABORTED;
                }
                else
                {
                    trans.r_lock_list_.emplace_back(&tuple->lock_);
                }
                break;
            case Ope::WRITE:
                for (auto r_lock : trans.r_lock_list_)
                {
                    count++;
                    if (&tuple->lock_ == r_lock)
                    {
                        // delete from task_set and upgrade
                        if (!tuple->lock_.try_upgrade())
                        {
                            // abort
                            trans.status_ = Status::ABORTED;
                        }
                        else
                        {
                            trans.w_lock_list_.emplace_back(&tuple->lock_);
                            // delete from read sets
                            trans.r_lock_list_.erase(trans.r_lock_list_.begin() + count - 1);
                        }
                        break;
                    }
                }
                for (auto w_lock : trans.w_lock_list_)
                {
                    count++;
                    if (&tuple->lock_ == w_lock)
                    {
                        // trans.w_lock_list_.erase(trans.w_lock_list_.begin() + count - 1);
                        break;
                    }
                }
                if (!tuple->lock_.w_try_lock())
                {
                    trans.status_ = Status::ABORTED;
                }
                else
                {
                    trans.w_lock_list_.emplace_back(&tuple->lock_);
                }
                break;
            default:
                std::cout << "lock logical error" << std::endl;
                break;
            }
            if (trans.status_ == Status::ABORTED)
            {
                trans.abort();
                // std::cout << "abort" << std::endl;
                goto RETRY;
            }
        }

        // release giant lock
        lock_for_locks.w_unlock();

        for (auto &task : trans.task_set_)
        {
            switch (task.ope_)
            {
            case Ope::READ:
                trans.read(task.key_);
                // std::cout << "read" << std::endl;
                break;
            case Ope::WRITE:
                trans.write(task.key_);
                // std::cout << "write" << std::endl;
                break;
            default:
                std::cout << "fail" << std::endl;
                break;
            }
        }
        trans.commit();
        myres.commit_cnt_++;
    }
}

int main(int argc, char *argv[])
{
    // gflags::ParseCommandLineFlags(&argc, &argv, true);
    // std::cout << "#FLAGS_tuple_num" << FLAGS_tuple_num << "\n";

    // initilize rnd and zipf
    Xoroshiro128Plus rnd;
    FastZipf zipf(&rnd, 0, TUPLE_NUM);

    makeDB();

    tx_counter = 0;

    bool start = false;
    bool quit = false;

    // transaction generate
    for (auto &pre : Pre_tx_set)
    {
        makeTask(pre.task_set_, rnd, zipf);
    }

    std::vector<int> readys;
    for (size_t i = 0; i < THREAD_NUM; ++i)
    {
        readys.emplace_back(0);
    }

    std::vector<std::thread> thv;
    for (size_t i = 0; i < THREAD_NUM; ++i)
    {
        thv.emplace_back(worker, i, std::ref(readys[i]), std::ref(start), std::ref(quit));
    }

    while (true)
    {
        bool failed = false;
        for (auto &re : readys)
        {
            if (!__atomic_load_n(&re, __ATOMIC_SEQ_CST))
            {
                failed = true;
                break;
            }
        }
        if (!failed)
        {
            break;
        }
    }
    __atomic_store_n(&start, true, __ATOMIC_SEQ_CST);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 * EX_TIME));

    __atomic_store_n(&quit, true, __ATOMIC_SEQ_CST);

    for (auto &th : thv)
    {
        th.join();
    }

    uint64_t total_count = 0;
    for (auto &re : AllResult)
    {
        total_count += re.commit_cnt_;
    }
    std::cout << "throughput:" << total_count / EX_TIME << std::endl;

    return 0;
}