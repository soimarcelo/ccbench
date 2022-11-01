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
// #include <deque>
#include <string>

#define PAGE_SIZE 4096
#define THREAD_NUM 4
#define TUPLE_NUM 10000000
#define MAX_OPE 16
#define SLEEP_POS 15
#define RW_RATE 50
#define EX_TIME 3
#define PRE_NUM 120000
#define SLEEP_TIME 100
#define SKEW_PAR 0.9

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

    void r_unlock()
    {
        counter--;
    }
    void w_unlock()
    {
        counter++;
    }
};

RWLock giant_lock;

class Tuple
{
public:
    RWLock lock_;
    RWLock lock_for_lock_;
    std::vector<uint64_t> wait_list_;
    uint64_t value_;

    void wait_add(uint64_t tx_id)
    {
    ADD_RETRY:
        if (!lock_for_lock_.w_try_lock())
        {
            goto ADD_RETRY;
        }
        else
        {
            if (wait_list_.size() > 1)
            {
                if (wait_list_[wait_list_.size() - 1] == tx_id)
                    std::cout << "alert" << tx_id << std::endl;
            }

            wait_list_.emplace_back(tx_id);
            lock_for_lock_.w_unlock();
        }
    }
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
    std::vector<Task> future_lock_;
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

    int dup_check(int key, int upper)
    {
        for (int i = 0; i < upper; i++)
        {
            if (task_set_[i].key_ == key)
            {
                switch (task_set_[i].ope_)
                {
                case Ope::READ:
                    return 0;
                    break;
                case Ope::WRITE:
                    return 1;
                    break;
                default:
                    break;
                }
            }
        }
        return -1;
    }
};

void lockmanager_regist(Transaction &trans, uint64_t tx_id, int thread_id)
{                    // lock取りに行って、取れなかったらlock managerに登録するフェーズ
    int ope_pos = 0; //オペレーションがtx中で何番目かを示す dup_check用
    for (auto item = trans.task_set_.begin(); item != trans.task_set_.end();)
    {
        // search tuple
        Tuple *tuple = &Table[item->key_];

        switch (item->ope_)
        {
        case Ope::READ:
            if (trans.dup_check(item->key_, ope_pos) > -1)
            {
                //既に同一トランザクション内で同じデータアイテムに対して、Read/Writeを行なっている
                // std::cout << item->key_ << "read dup" << thread_id << std::endl;
            }
            else
            {
                tuple->wait_add(tx_id);
                trans.future_lock_.emplace_back(Ope::READ, item->key_);
            }

            item++;
            break;
        case Ope::WRITE:
            if (trans.dup_check(item->key_, ope_pos) == 1)
            {
                //既に同一トランザクション内で同じデータアイテムに対して、Writeを行なっている
                // std::cout << item->key_ << "write dup" << thread_id << std::endl;
            }
            else
            {
                // upgrade or normal write
                //  lock_wait に追加
                tuple->wait_add(tx_id);
                trans.future_lock_.emplace_back(Ope::WRITE, item->key_);
            }
            item++;
            break;

        case Ope::SLEEP:
            item++;
            break;
        default:
            std::cout << "lock logical error" << std::endl;
            exit(0);
        }
        ope_pos++;
    }
}

bool check_waitlist(Transaction &trans, uint64_t tx_id, int thread_id, const bool &quit)
{
    while (trans.future_lock_.size() > 0)
    {
        if (__atomic_load_n(&quit, __ATOMIC_SEQ_CST))
            return false;
        for (auto item = trans.future_lock_.begin(); item != trans.future_lock_.end();)
        {
            // std::cout << trans.future_lock_.size() << ":" << thread_id << std::endl;
            int count = 0; // upgrade read lock削除用
            bool dup_flag = false;
            Tuple *tuple = &Table[item->key_];
            if (tx_id == __atomic_load_n(&tuple->wait_list_[0], __ATOMIC_SEQ_CST))
            {
                if (!tuple->lock_for_lock_.w_try_lock())
                {
                    // retry
                    std::cout << item->key_ << ":" << tx_id << std::endl;
                    item++;
                }
                else
                {
                    switch (item->ope_)
                    {
                    case Ope::READ:
                        if (!tuple->lock_.r_try_lock())
                        {
                            //  retry
                            // std::cout << "read lock" << std::endl;
                            // std::cout << thread_id << "read" << item->key_ << std::endl;
                            item++;
                        }
                        else
                        {
                            // std::cout << item->key_ << ":" << thread_id << std::endl;
                            item = trans.future_lock_.erase(item);
                            tuple->wait_list_.erase(tuple->wait_list_.begin());
                            trans.r_lock_list_.emplace_back(&tuple->lock_);
                        }
                        break;
                    case Ope::WRITE:
                        // upgradeかどうか
                        for (auto r_lock : trans.r_lock_list_)
                        {
                            count++;
                            if (&tuple->lock_ == r_lock)
                            {
                                // std::cout << item->key_ << "upgrade" << thread_id << std::endl;
                                // delete from task_set and upgrade
                                if (tx_id == __atomic_load_n(&tuple->wait_list_[1], __ATOMIC_SEQ_CST))
                                {
                                    // read-write-write事案を避けたい
                                    item = trans.future_lock_.erase(item);
                                    tuple->wait_list_.erase(tuple->wait_list_.begin());
                                }
                                else
                                {
                                    if (!tuple->lock_.try_upgrade())
                                    {
                                        // retry
                                        // std::cout << "upgrade lock" << std::endl;
                                        // std::cout << thread_id << "upgrade" << item->key_ << std::endl;
                                        item++;
                                    }
                                    else
                                    {
                                        // std::cout << item->key_ << ":" << thread_id << std::endl;
                                        trans.w_lock_list_.emplace_back(&tuple->lock_);
                                        item = trans.future_lock_.erase(item);
                                        tuple->wait_list_.erase(tuple->wait_list_.begin());
                                        trans.r_lock_list_.erase(trans.r_lock_list_.begin() + count - 1);
                                    }
                                }
                                dup_flag = true;
                                break;
                            }
                        }
                        if (dup_flag)
                        {
                            break;
                        }

                        if (tx_id == __atomic_load_n(&tuple->wait_list_[1], __ATOMIC_SEQ_CST))
                        {
                            // read-writeのupgradeでwriteを先にロック取るのを防ぐ
                            // std::cout << item->key_ << ":" << thread_id << ":" << tuple->wait_list_[0] << ":" << tuple->wait_list_[1] << std::endl;
                            item++;
                            break;
                        }

                        if (!tuple->lock_.w_try_lock())
                        {
                            // retry
                            // std::cout << "write lock" << std::endl;
                            // std::cout << thread_id << "write" << item->key_ << std::endl;
                            item++;
                        }
                        else
                        {
                            // std::cout << item->key_ << ":" << thread_id << std::endl;
                            item = trans.future_lock_.erase(item);
                            tuple->wait_list_.erase(tuple->wait_list_.begin());
                            trans.w_lock_list_.emplace_back(&tuple->lock_);
                        }
                        break;
                    default:
                        std::cout << "invalid operation" << std::endl;
                        break;
                    }
                    tuple->lock_for_lock_.w_unlock();
                }
            }
            else
            {
                item++;
            }
        }
    }
    return true;
}

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
        else
        {

            if ((rnd.next() % 100) < RW_RATE)
            {
                // std::cout << "read" << std::endl;
                tasks.emplace_back(Ope::READ, random_gen_key + 1);
            }
            else
            {
                // std::cout << "write" << std::endl;
                tasks.emplace_back(Ope::WRITE, random_gen_key + 1);
            }
        }
    }
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
    uint64_t tx_id;

    __atomic_store_n(&ready, 1, __ATOMIC_SEQ_CST);

    while (!__atomic_load_n(&start, __ATOMIC_SEQ_CST))
    {
    }

GIANT_RETRY:

    while (!__atomic_load_n(&quit, __ATOMIC_SEQ_CST))
    {
        // aquire giant lock
        if (!giant_lock.w_try_lock())
        {
            goto GIANT_RETRY;
        }
        // std::cout << "1" << thread_id << std::endl;

        //取得すべきtxの現在地　ロック必要か
        tx_id = __atomic_load_n(&tx_counter, __ATOMIC_SEQ_CST);
        if (tx_id >= PRE_NUM)
        {
            return;
        }
        __atomic_store_n(&tx_counter, tx_id + 1, __ATOMIC_SEQ_CST);

        // pre_tx_setからコピー
        Pre &work_tx = std::ref(Pre_tx_set[tx_id]);
        trans.task_set_ = work_tx.task_set_;

        trans.begin();

        // std::cout << trans.task_set_.size() << "g" << std::endl;
        // std::cout << "1.5" << thread_id << std::endl;
        // lock_managerに登録
        lockmanager_regist(trans, tx_id, thread_id);
        // std::cout << "2" << thread_id << std::endl;

        // for (auto &item : trans.future_lock_)
        // {
        //     switch (item.ope_)
        //     {
        //     case Ope::READ:
        //         std::cout << item.key_ << "read" << thread_id << std::endl;
        //         break;
        //     case Ope::WRITE:
        //         std::cout << item.key_ << "write" << thread_id << std::endl;
        //         break;
        //     default:
        //         std::cout << "fail" << std::endl;
        //         break;
        //     }
        // }

        // release giant lock
        giant_lock.w_unlock();

        // std::cout << trans.future_lock_.size() << "ga" << std::endl;

        bool end = check_waitlist(trans, tx_id, thread_id, quit);

        // std::cout << "3" << thread_id << std::endl;

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
        }
        if (end)
        {
            trans.commit();
            myres.commit_cnt_++;
        }
        // std::cout << "4" << thread_id << std::endl;
    }

GIANT_RE:

    // aquire giant lock
    if (!giant_lock.w_try_lock())
    {
        goto GIANT_RE;
    }
    std::cout << thread_id << ":" << trans.future_lock_.size() << ":" << myres.commit_cnt_ << std::endl;
    for (int i = 0; i < trans.future_lock_.size(); i++)
    {
        switch (trans.future_lock_[i].ope_)
        {
        case Ope::READ:
            std::cout << Table[trans.future_lock_[i].key_].wait_list_[0] << "read" << trans.future_lock_[i].key_ << ":" << tx_id << ":" << Table[trans.future_lock_[i].key_].lock_.counter << ":" << Table[trans.future_lock_[i].key_].lock_for_lock_.counter << std::endl;
            break;
        case Ope::WRITE:
            std::cout << Table[trans.future_lock_[i].key_].wait_list_[0] << "write" << trans.future_lock_[i].key_ << ":" << tx_id << ":" << Table[trans.future_lock_[i].key_].lock_.counter << ":" << Table[trans.future_lock_[i].key_].lock_for_lock_.counter << std::endl;
            break;
        case Ope::SLEEP:
            std::cout << "sleep" << trans.future_lock_[i].key_ << std::endl;
            break;
        default:
            std::cout << "fail" << std::endl;
            break;
        }
    }
    std::cout << "already have" << std::endl;
    for (int i = 0; i < trans.task_set_.size(); i++)
    {
        switch (trans.task_set_[i].ope_)
        {
        case Ope::READ:
            std::cout << "read" << trans.task_set_[i].key_ << std::endl;
            break;
        case Ope::WRITE:
            std::cout << "write" << trans.task_set_[i].key_ << std::endl;
            break;
        case Ope::SLEEP:
            std::cout << "sleep" << trans.task_set_[i].key_ << std::endl;
            break;
        default:
            std::cout << "fail" << std::endl;
            break;
        }
    }
    giant_lock.w_unlock();
}

int main(int argc, char *argv[])
{

    // initilize rnd and zipf
    Xoroshiro128Plus rnd;
    FastZipf zipf(&rnd, SKEW_PAR, TUPLE_NUM - 1);

    makeDB();

    tx_counter = 1;

    // Tuple *tuple = &Table[5];

    // std::cout << tuple->wait_list_.size() << "gaho" << std::endl;

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