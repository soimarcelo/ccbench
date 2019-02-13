#include <algorithm>
#include <cctype>
#include <cstdint>
#include <ctype.h>
#include <pthread.h>
#include <random>
#include <string.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdlib.h>
#include <thread>
#include <unistd.h>

#define GLOBAL_VALUE_DEFINE
#include "include/common.hpp"
#include "include/procedure.hpp"
#include "include/result.hpp"
#include "include/transaction.hpp"

#include "../include/debug.hpp"
#include "../include/int64byte.hpp"
#include "../include/random.hpp"
#include "../include/zipf.hpp"

using namespace std;

extern bool chkClkSpan(uint64_t &start, uint64_t &stop, uint64_t threshold);
extern bool chkSpan(struct timeval &start, struct timeval &stop, long threshold);
extern void makeDB(uint64_t *initial_wts);
extern void makeProcedure(Procedure *pro, Xoroshiro128Plus &rnd);
extern void makeProcedure(Procedure *pro, Xoroshiro128Plus &rnd, FastZipf &zipf);
extern void setThreadAffinity(int myid);
extern void waitForReadyOfAllThread();

static bool
chkInt(char *arg)
{
    for (uint i=0; i<strlen(arg); i++) {
        if (!isdigit(arg[i])) {
      cout << std::string(arg) << " is not a number." << endl;
      exit(0);
        }
    }
  return true;
}

static void 
chkArg(const int argc, char *argv[])
{
  if (argc != 15) {
    cout << "usage: ./cicada.exe TUPLE_NUM MAX_OPE THREAD_NUM RRATIO ZIPF_SKEW YCSB WAL GROUP_COMMIT CPU_MHZ IO_TIME_NS GROUP_COMMIT_TIMEOUT_US LOCK_RELEASE_METHOD GC_INTER_US EXTIME" << endl << endl;
    cout << "example:./main 200 10 24 5 0 ON OFF OFF 2400 5 2 E 10 3" << endl << endl;
    cout << "TUPLE_NUM(int): total numbers of sets of key-value (1, 100), (2, 100)" << endl;
    cout << "MAX_OPE(int):    total numbers of operations" << endl;
    cout << "THREAD_NUM(int): total numbers of worker thread." << endl;
    cout << "RRATIO: read ratio [%%]" << endl;
    cout << "ZIPF_SKEW : zipf skew. 0 ~ 0.999..." << endl;
    cout << "YCSB : ON or OFF. switch makeProcedure function." << endl;
    cout << "WAL: P or S or OFF." << endl;
    cout << "GROUP_COMMIT:  unsigned integer or OFF, i reccomend OFF or 3" << endl;
    cout << "CPU_MHZ(float):  your cpuMHz. used by calculate time of yours 1clock." << endl;
    cout << "IO_TIME_NS: instead of exporting to disk, delay is inserted. the time(nano seconds)." << endl;
    cout << "GROUP_COMMIT_TIMEOUT_US: Invocation condition of group commit by timeout(micro seconds)." << endl;
    cout << "LOCK_RELEASE_METHOD: E or NE or N. Early lock release(tanabe original) or Normal Early Lock release or Normal lock release." << endl;
    cout << "GC_INTER_US: garbage collection interval [usec]" << endl;
    cout << "EXTIME: execution time [sec]" << endl << endl;

    cout << "Tuple " << sizeof(Tuple) << endl;
    cout << "Version " << sizeof(Version) << endl;
    cout << "TimeStamp " << sizeof(TimeStamp) << endl;
    cout << "Procedure" << sizeof(Procedure) << endl;
    exit(0);
  }

  chkInt(argv[1]);
  chkInt(argv[2]);
  chkInt(argv[3]);
  chkInt(argv[4]);
  chkInt(argv[9]);
  chkInt(argv[10]);
  chkInt(argv[11]);
  chkInt(argv[13]);
  chkInt(argv[14]);

  TUPLE_NUM = atoi(argv[1]);
  MAX_OPE = atoi(argv[2]);
  THREAD_NUM = atoi(argv[3]);
  RRATIO = atoi(argv[4]);
  ZIPF_SKEW = atof(argv[5]);
  string argycsb = argv[6];
  string argwal = argv[7];
  string arggrpc = argv[8];
  CLOCK_PER_US = atof(argv[9]);
  IO_TIME_NS = atof(argv[10]);
  GROUP_COMMIT_TIMEOUT_US = atoi(argv[11]);
  string arglr = argv[12];
  GC_INTER_US = atoi(argv[13]);
  EXTIME = atoi(argv[14]);

  if (RRATIO > 100) {
    cout << "rratio [%%] must be 0 ~ 100)" << endl;
    ERR;
  }

  if (ZIPF_SKEW >= 1) {
    cout << "ZIPF_SKEW must be 0 ~ 0.999..." << endl;
    ERR;
  }

  if (argycsb == "ON") {
    YCSB = true;
  }
  else if (argycsb == "OFF") {
    YCSB = false;
  }
  else ERR;

  if (argwal == "P")  {
    P_WAL = true;
    S_WAL = false;
  } else if (argwal == "S") {
    P_WAL = false;
    S_WAL = true;
  } else if (argwal == "OFF") {
    P_WAL = false;
    S_WAL = false;

    if (arggrpc != "OFF") {
      printf("i don't implement below.\n\
P_WAL OFF, S_WAL OFF, GROUP_COMMIT number.\n\
usage: P_WAL or S_WAL is selected. \n\
P_WAL and S_WAL isn't selected, GROUP_COMMIT must be OFF. this isn't logging. performance is concurrency control only.\n\n");
      exit(0);
    }
  }
  else {
    printf("WAL(argv[7]) must be P or S or OFF\n");
    exit(0);
  }

  if (arggrpc == "OFF") GROUP_COMMIT = 0;
  else if (chkInt(argv[8])) {
      GROUP_COMMIT = atoi(argv[8]);
  }
  else {
    printf("GROUP_COMMIT(argv[8]) must be unsigned integer or OFF\n");
    exit(0);
  }

  if (CLOCK_PER_US < 100) {
    printf("CPU_MHZ is less than 100. are you really?\n");
    exit(0);
  }

  if (arglr == "N") {
    NLR = true;
    ELR = false;
  }
  else if (arglr == "E") {
    NLR = false;
    ELR = true;
  }
  else {
    printf("LockRelease(argv[12]) must be E or N\n");
    exit(0);
  }

  try {
    if (posix_memalign((void**)&ThreadRtsArrayForGroup, 64, THREAD_NUM * sizeof(uint64_t_64byte)) != 0) ERR;
    if (posix_memalign((void**)&ThreadWtsArray, 64, THREAD_NUM * sizeof(uint64_t_64byte)) != 0) ERR;
    if (posix_memalign((void**)&ThreadRtsArray, 64, THREAD_NUM * sizeof(uint64_t_64byte)) != 0) ERR;
    if (posix_memalign((void**)&GROUP_COMMIT_INDEX, 64, THREAD_NUM * sizeof(uint64_t_64byte)) != 0) ERR;
    if (posix_memalign((void**)&GROUP_COMMIT_COUNTER, 64, THREAD_NUM * sizeof(uint64_t_64byte)) != 0) ERR;
    if (posix_memalign((void**)&GCFlag, 64, THREAD_NUM * sizeof(uint64_t_64byte)) != 0) ERR;
    if (posix_memalign((void**)&GCExecuteFlag, 64, THREAD_NUM * sizeof(uint64_t_64byte)) != 0) ERR;
    
    SLogSet = new Version*[(MAX_OPE) * (GROUP_COMMIT)]; 
    PLogSet = new Version**[THREAD_NUM];

    for (unsigned int i = 0; i < THREAD_NUM; ++i) {
      PLogSet[i] = new Version*[(MAX_OPE) * (GROUP_COMMIT)];
    }
  } catch (bad_alloc) {
    ERR;
  }
  //init
  for (unsigned int i = 0; i < THREAD_NUM; ++i) {
    GCFlag[i].obj = 0;
    GCExecuteFlag[i].obj = 0;
    GROUP_COMMIT_INDEX[i].obj = 0;
    GROUP_COMMIT_COUNTER[i].obj = 0;
    ThreadRtsArray[i].obj = 0;
    ThreadWtsArray[i].obj = 0;
    ThreadRtsArrayForGroup[i].obj = 0;
  }
}

static void *
manager_worker(void *arg)
{
  int *myid = (int *)arg;
  TimeStamp tmp;

  uint64_t initial_wts;
  makeDB(&initial_wts);
  MinWts.store(initial_wts + 2, memory_order_release);
  Result rsobject;

  setThreadAffinity(*myid);
  //printf("Thread #%d: on CPU %d\n", *myid, sched_getcpu());
  waitForReadyOfAllThread();
  while (FirstAllocateTimestamp.load(memory_order_acquire) != THREAD_NUM - 1) {}

  rsobject.Bgn = rdtsc();
  // leader work
  for(;;) {
    rsobject.End = rdtsc();
    if (chkClkSpan(rsobject.Bgn, rsobject.End, EXTIME * 1000 * 1000 * CLOCK_PER_US)) {
      rsobject.Finish.store(true, std::memory_order_release);
      return nullptr;
    }

    bool gc_update = true;
    for (unsigned int i = 1; i < THREAD_NUM; ++i) {
    //check all thread's flag raising
      if (__atomic_load_n(&(GCFlag[i].obj), __ATOMIC_ACQUIRE) == 0) {
        usleep(1);
        gc_update = false;
        break;
      }
    }
    if (gc_update) {
      uint64_t minw = __atomic_load_n(&(ThreadWtsArray[1].obj), __ATOMIC_ACQUIRE);
      uint64_t minr;
      if (GROUP_COMMIT == 0) {
        minr = __atomic_load_n(&(ThreadRtsArray[1].obj), __ATOMIC_ACQUIRE);
      }
      else {
        minr = __atomic_load_n(&(ThreadRtsArrayForGroup[1].obj), __ATOMIC_ACQUIRE);
      }

      for (unsigned int i = 1; i < THREAD_NUM; ++i) {
        uint64_t tmp = __atomic_load_n(&(ThreadWtsArray[i].obj), __ATOMIC_ACQUIRE);
        if (minw > tmp) minw = tmp;
        if (GROUP_COMMIT == 0) {
          tmp = __atomic_load_n(&(ThreadRtsArray[i].obj), __ATOMIC_ACQUIRE);
          if (minr > tmp) minr = tmp;
        }
        else {
          tmp = __atomic_load_n(&(ThreadRtsArrayForGroup[i].obj), __ATOMIC_ACQUIRE);
          if (minr > tmp) minr = tmp;
        }
      }
      MinWts.store(minw, memory_order_release);
      MinRts.store(minr, memory_order_release);

      // downgrade gc flag
      for (unsigned int i = 1; i < THREAD_NUM; ++i) {
        __atomic_store_n(&(GCFlag[i].obj), 0, __ATOMIC_RELEASE);
        __atomic_store_n(&(GCExecuteFlag[i].obj), 1, __ATOMIC_RELEASE);
      }
    }
  }

  return nullptr;
}

static void *
worker(void *arg)
{
  int *myid = (int *)arg;
  Xoroshiro128Plus rnd;
  rnd.init();
  Procedure pro[MAX_OPE];
  Transaction trans(*myid);
  FastZipf zipf(&rnd, ZIPF_SKEW, TUPLE_NUM);

  setThreadAffinity(*myid);
  //printf("Thread #%d: on CPU %d\n", *myid, sched_getcpu());
  //printf("sysconf(_SC_NPROCESSORS_CONF) %d\n", sysconf(_SC_NPROCESSORS_CONF));
  waitForReadyOfAllThread();

  try {
    //start work(transaction)
    for (;;) {
      if (YCSB) 
        makeProcedure(pro, rnd, zipf);
      else 
        makeProcedure(pro, rnd);

      asm volatile ("" ::: "memory");
RETRY:
      if (trans.rsobject.Finish.load(std::memory_order_acquire)) {
        trans.rsobject.sumUpAbortCounts();
        trans.rsobject.sumUpCommitCounts();
        trans.rsobject.sumUpGCCounts();
        //trans.rsobject.displayLocalCommitCounts();
        return nullptr;
      }

      trans.tbegin(pro[0].ronly);

      //Read phase
      //Search versions
      for (unsigned int i = 0; i < MAX_OPE; ++i) {
        if (pro[i].ope == Ope::READ) {
          trans.tread(pro[i].key);
        } else {
          trans.twrite(pro[i].key, pro[i].val);
        }

        if (trans.status == TransactionStatus::abort) {
          trans.earlyAbort();
          goto RETRY;
        }
      }

      //read onlyトランザクションはread setを集めず、validationもしない。
      //write phaseはログを取り仮バージョンのコミットを行う．これをスキップできる．
      if (trans.ronly) {
        ++trans.rsobject.localCommitCounts;
        ++trans.continuingCommit;
        continue;
      }

      //Validation phase
      if (!trans.validation()) {
        trans.abort();
        goto RETRY;
      }

      //Write phase
      trans.writePhase();

      //Maintenance
      //Schedule garbage collection
      //Declare quiescent state
      //Collect garbage created by prior transactions
      trans.mainte();
    }
  } catch (bad_alloc) {
    ERR;
  }

  return nullptr;
}

static pthread_t
threadCreate(int id)
{
  pthread_t t;
  int *myid;

  try {
    myid = new int;
  } catch (bad_alloc) {
    ERR;
  }
  *myid = id;

  if (id == 0) {
    if (pthread_create(&t, nullptr, manager_worker, (void *)myid)) ERR;
    return t;
  }
  else {
    if (pthread_create(&t, nullptr, worker, (void *)myid)) ERR;
    return t;
  }
}

int 
main(int argc, char *argv[]) 
{
  Result rsobject;
  chkArg(argc, argv);

  pthread_t thread[THREAD_NUM];

  for (unsigned int i = 0; i < THREAD_NUM; i++) {
    thread[i] = threadCreate(i);
  }

  for (unsigned int i = 0; i < THREAD_NUM; i++) {
    pthread_join(thread[i], nullptr);
  }

  rsobject.displayTPS();
  //rsobject.displayCommitCounts();
  //rsobject.displayAbortCounts();
  //rsobject.displayAbortRate();
  //rsobject.displayGCCounts();

  return 0;
}