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
#define TUPLE_NUM 100000
#define MAX_OPE 16
#define SLEEP_POS 80
#define RW_RATE 50
#define EX_TIME 3
#define PRE_NUM 1000000
#define SLEEP_TIME 0
#define SKEW_PAR 0.8

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
    SLEEP,
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

    void
    upgrade()
    {
        int expected, desired(-1);
        expected = counter.load(std::memory_order_acquire);
        for (;;)
        {
            if (expected != 1)
            {
                expected = counter.load(std::memory_order_acquire);
                continue;
            }

            if (counter.compare_exchange_strong(
                    expected, desired, std::memory_order_acq_rel, std::memory_order_acquire))
                return;
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

        for (auto &itr : write_set_)
        {
            if (itr.key_ == key)
            {
                // std::cout << "read-write" << std::endl;
                return;
            }
        }
        for (auto &itr : read_set_)
        {
            if (itr.key_ == key)
            {
                // std::cout << "read-read" << std::endl;
                return;
            }
        }
        Tuple *tuple = &Table[key];

        if (!tuple->lock_.r_try_lock())
        {
            status_ = Status::ABORTED;
            // std::cout << "read abort" << key << std::endl;
            return;
        }
        // tuple->lock_.read_lock();
        r_lock_list_.emplace_back(&tuple->lock_);

        uint64_t read_value = __atomic_load_n(&tuple->value_, __ATOMIC_SEQ_CST);
        read_set_.emplace_back(key, read_value, tuple);
        return;
    }
    void write(const uint64_t key)
    {

        for (auto &itr : write_set_)
        {
            if (itr.key_ == key)
            {
                // std::cout << "write-write" << std::endl;
                return;
            }
        }

        for (auto itr = read_set_.begin(); itr != read_set_.end(); ++itr)
        {
            if ((*itr).key_ == key)
            {
                // std::cout << "read-write" << std::endl;
                // (*itr).tuple_->lock_.upgrade();
                if (!(*itr).tuple_->lock_.try_upgrade())
                {
                    // std::cout << "upgrade abort" << key << std::endl;
                    status_ = Status::ABORTED;
                    return;
                }
                else
                {
                    // std::cout << "upgrade success" << std::endl;
                    for (auto lock_itr = r_lock_list_.begin(); lock_itr != r_lock_list_.end(); ++lock_itr)
                    {
                        if (*lock_itr == &((*itr).tuple_->lock_))
                        {
                            // __atomic_store_n((*itr).tuple_->value_, 100, __ATOMIC_SEQ_CST);
                            write_set_.emplace_back(key, 100, (*itr).tuple_);
                            w_lock_list_.emplace_back(&(*itr).tuple_->lock_);
                            r_lock_list_.erase(lock_itr);
                            break;
                        }
                    }

                    read_set_.erase(itr);
                    return;
                }
            }
        }
        Tuple *tuple = &Table[key];

        if (!tuple->lock_.w_try_lock())
        {
            status_ = Status::ABORTED;
            // std::cout << "write abort" << key << std::endl;
            return;
        }
        // tuple->lock_.write_lock();
        w_lock_list_.emplace_back(&tuple->lock_);

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

        if (i == SLEEP_POS)
        {
            tasks.emplace_back(Ope::SLEEP, 0);
        }

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
    uint64_t tx_pos = PRE_NUM / THREAD_NUM * thread_id;

    __atomic_store_n(&ready, 1, __ATOMIC_SEQ_CST);

    while (!__atomic_load_n(&start, __ATOMIC_SEQ_CST))
    {
    }

    while (!__atomic_load_n(&quit, __ATOMIC_SEQ_CST))
    {
        // makeTask(trans.task_set_, rnd, zipf);
        Pre &work_tx = std::ref(Pre_tx_set[tx_pos]);
        trans.task_set_ = work_tx.task_set_;
        tx_pos++;

    RETRY:
        if (__atomic_load_n(&quit, __ATOMIC_SEQ_CST))
            break;
        trans.begin();
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
            case Ope::SLEEP:
                // std::cout << "sleeping" << std::endl;
                std::this_thread::sleep_for(std::chrono::microseconds(SLEEP_TIME));
                break;
            default:
                std::cout << "fail" << std::endl;
                break;
            }

            if (trans.status_ == Status::ABORTED)
            {
                trans.abort();
                // std::cout << "abort" << std::endl;
                goto RETRY;
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

    Xoroshiro128Plus rnd;
    FastZipf zipf(&rnd, SKEW_PAR, TUPLE_NUM);

    makeDB();

    bool start = false;
    bool quit = false;

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
