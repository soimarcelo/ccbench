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
#include <string>

#define PAGE_SIZE 4096
#define THREAD_NUM 4
#define TUPLE_NUM 1000000
#define MAX_OPE 100
#define SLEEP_POS 10
#define RW_RATE 50
#define EX_TIME 3
#define PRE_NUM 300000
#define SLEEP_TIME 100
#define SLEEP_TIME_INIT 1000 * 1000 * 3
#define SKEW_PAR 0.0

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
        // 同一tx内で既にreadしてたら0, writeしてたら1を返す
        // 初めてアクセスする場合は-1を返す
        for (int i = 0; i < upper; i++)
        {
            if (task_set_[i].key_ == key)
            {
                switch (task_set_[i].ope_)
                {
                case Ope::READ:
                    return 0;
                case Ope::WRITE:
                    return 1;
                default:
                    break;
                }
            }
        }
        return -1;
    }
};

void lockmanager_regist(Transaction &trans, uint64_t tx_id, int thread_id)
{                    // 全てlock managerに登録するフェーズ
    int ope_pos = 0; // オペレーションがtx中で何番目かを示す 関数dup_check用
    for (auto item = trans.task_set_.begin(); item != trans.task_set_.end();)
    {
        // search tuple
        Tuple *tuple = &Table[item->key_];
        int ret;

        switch (item->ope_)
        {
        case Ope::READ:
            if (trans.dup_check(item->key_, ope_pos) > -1)
            {
                // 既に同一トランザクション内で同じデータアイテムに対して、Read/Writeを行なっている
                //  read-read:0, read-write:1
            }
            else
            {
                // first access
                tuple->wait_add(tx_id);
                trans.future_lock_.emplace_back(Ope::READ, item->key_);
            }
            item++;
            break;
        case Ope::WRITE:
            ret = trans.dup_check(item->key_, ope_pos);
            if (ret == 1)
            {
                // 既に同一トランザクション内で同じデータアイテムに対して、Writeを行なっている
                //  write-write:1
            }
            else if (ret == 0)
            {
                for (int i = 0; i < trans.future_lock_.size(); i++)
                {
                    if (item->key_ == trans.future_lock_[i].key_)
                    {
                        trans.future_lock_[i].ope_ = Ope::WRITE;
                        break;
                    }
                }
            }
            else
            {
                // upgrade(read-write:0): or normal write(first access):-1
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

void check_waitlist(Transaction &trans, uint64_t tx_id, int thread_id, const bool &quit)
{
    while (trans.future_lock_.size() > 0)
    {
        // if (__atomic_load_n(&quit, __ATOMIC_SEQ_CST))
        //     break;
        for (auto item = trans.future_lock_.begin(); item != trans.future_lock_.end();)
        {
            int count = 0; // upgrade read lock削除用
            bool dup_flag = false;
            Tuple *tuple = &Table[item->key_];
            if (tx_id == __atomic_load_n(&tuple->wait_list_[0], __ATOMIC_SEQ_CST))
            {
                // lock managerのwait_list_の先頭に自身のtx_idがある
                if (!tuple->lock_for_lock_.w_try_lock())
                {
                    // tupleのロックが取れない
                    //  retry
                    item++;
                }
                else
                {
                    // tupleのロックが取れる
                    switch (item->ope_)
                    {
                    case Ope::READ:
                        if (!tuple->lock_.r_try_lock())
                        {
                            // レコードのreadロックが取れない
                            //   retry
                            item++;
                        }
                        else
                        {
                            // レコードのreadロックが取れる
                            item = trans.future_lock_.erase(item);
                            tuple->wait_list_.erase(tuple->wait_list_.begin());
                            trans.r_lock_list_.emplace_back(&tuple->lock_);
                        }
                        break;
                    case Ope::WRITE:
                        // 以下、txが初めてアクセスするレコードに対するwrite(normal write)
                        if (!tuple->lock_.w_try_lock())
                        {
                            // レコードのwriteロックが取れない
                            //  retry
                            item++;
                        }
                        else
                        {
                            // レコードのwriteロックが取れる
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
                // lock managerのwait_list_の先頭が自身のtx_idじゃない、先行して待っているtxがある
                item++;
            }
        }
    }
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
                tasks.emplace_back(Ope::READ, random_gen_key + 1);
            }
            else
            {
                tasks.emplace_back(Ope::WRITE, random_gen_key + 1);
            }
        }
    }
}

void makeTask_init(std::vector<Task> &tasks, Xoroshiro128Plus &rnd, FastZipf &zipf)
{
    tasks.clear();
    for (size_t i = 0; i < 10; ++i)
    {
        uint64_t random_gen_key = zipf() % 1;
        // std::cout << random_gen_key << std::endl;
        assert(random_gen_key < TUPLE_NUM);

        if (i == 9)
        {
            tasks.emplace_back(Ope::SLEEP, 0);
        }
        else
        {
            tasks.emplace_back(Ope::WRITE, random_gen_key + 1);
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

        // 取得すべきtxの現在地　ロック必要か
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

        // lock_managerに登録
        lockmanager_regist(trans, tx_id, thread_id);

        // release giant lock
        giant_lock.w_unlock();

        check_waitlist(trans, tx_id, thread_id, quit);

        for (auto &task : trans.task_set_)
        {
            switch (task.ope_)
            {
            case Ope::READ:
                trans.read(task.key_);
                break;
            case Ope::WRITE:
                trans.write(task.key_);
                break;
            case Ope::SLEEP:
                if (tx_id == 1 || tx_id == 2)
                {
                    std::cout << tx_id << std::endl;
                    std::this_thread::sleep_for(std::chrono::microseconds(SLEEP_TIME_INIT));
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::microseconds(SLEEP_TIME));
                }
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

    // initilize rnd and zipf
    Xoroshiro128Plus rnd;
    FastZipf zipf(&rnd, SKEW_PAR, TUPLE_NUM - 1);

    makeDB();

    tx_counter = 1;

    bool start = false;
    bool quit = false;

    // transaction generate
    int tx_make_count = 0;
    for (auto &pre : Pre_tx_set)
    {
        if (tx_make_count == 0 || tx_make_count == 1)
        {
            makeTask_init(pre.task_set_, rnd, zipf);
        }
        else
        {
            makeTask(pre.task_set_, rnd, zipf);
        }
        tx_make_count++;
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