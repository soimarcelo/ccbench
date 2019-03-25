
#include <ctype.h>  //isdigit, 
#include <pthread.h>
#include <string.h> //strlen,
#include <sys/syscall.h>  //syscall(SYS_gettid),  
#include <sys/types.h>  //syscall(SYS_gettid),
#include <time.h>
#include <unistd.h> //syscall(SYS_gettid), 

#include <iostream>
#include <string> //string

#define GLOBAL_VALUE_DEFINE
#include "../include/cpu.hpp"
#include "../include/debug.hpp"
#include "../include/int64byte.hpp"
#include "../include/random.hpp"
#include "../include/tsc.hpp"
#include "../include/zipf.hpp"

#include "include/common.hpp"
#include "include/garbageCollection.hpp"
#include "include/result.hpp"
#include "include/transaction.hpp"

using namespace std;

extern void chkArg(const int argc, const char *argv[]);
extern bool chkClkSpan(uint64_t &start, uint64_t &stop, uint64_t threshold);
extern void makeDB();
extern void makeProcedure(Procedure *pro, Xoroshiro128Plus &rnd);
extern void makeProcedure(Procedure *pro, Xoroshiro128Plus &rnd, FastZipf &zipf);
extern void naiveGarbageCollection();
extern void waitForReadyOfAllThread();

static void *
manager_worker(void *arg)
{
  // start, inital work
  int *myid = (int *)arg;
  GarbageCollection gcobject;
  Result rsobject;
  uint64_t bgn, end;
 
#ifdef Linux 
  setThreadAffinity(*myid);
#endif // Linux

  gcobject.decideFirstRange();
  waitForReadyOfAllThread();
  // end, initial work
  
  
  bgn = rdtsc();
  for (;;) {
    usleep(1);
    end = rdtsc();
    if (chkClkSpan(bgn, end, EXTIME * 1000 * 1000 * CLOCK_PER_US)) {
      Finish.store(true, std::memory_order_release);
      rsobject.Bgn = bgn;
      rsobject.End = end;
      return nullptr;
    }

    if (gcobject.chkSecondRange()) {
      gcobject.decideGcThreshold();
      gcobject.mvSecondRangeToFirstRange();
    }
  }

  return nullptr;
}


static void *
worker(void *arg)
{
  int *myid = (int *)arg;
  TxExecutor trans(*myid, MAX_OPE);
  Procedure pro[MAX_OPE];
  Xoroshiro128Plus rnd;
  rnd.init();
  FastZipf zipf(&rnd, ZIPF_SKEW, TUPLE_NUM);
  
#ifdef Linux
  setThreadAffinity(*myid);
  //printf("Thread #%d: on CPU %d\n", *myid, sched_getcpu());
  //printf("sysconf(_SC_NPROCESSORS_CONF) %ld\n", sysconf(_SC_NPROCESSORS_CONF));
#endif // Linux

  waitForReadyOfAllThread();
  
  //start work (transaction)
  try {
    //printf("Thread #%d: on CPU %d\n", *myid, sched_getcpu());
    for(;;) {
      if (YCSB) makeProcedure(pro, rnd, zipf);
      else makeProcedure(pro, rnd);

      asm volatile ("" ::: "memory");
RETRY:

      if (Finish.load(std::memory_order_acquire)) {
        trans.rsobject.sumUpAbortCounts();
        trans.rsobject.sumUpCommitCounts();
        trans.rsobject.sumUpGCVersionCounts();
#ifdef CCTR_ON
        trans.rsobject.sumUpGCTMTElementsCounts();
#endif // CCTR_ON
        return nullptr;
      }

      //-----
      //transaction begin
      trans.tbegin();
      for (unsigned int i = 0; i < MAX_OPE; ++i) {
        if (pro[i].ope == Ope::READ) {
          trans.tread(pro[i].key);
        } else {
          if (RMW) {
            trans.tread(pro[i].key);
            trans.twrite(pro[i].key);
          } else
            trans.twrite(pro[i].key);
        }

        if (trans.status == TransactionStatus::aborted) {
          trans.abort();
          goto RETRY;
        }
      }

      trans.commit();

      // maintenance phase
      // garbage collection
      uint32_t loadThreshold = trans.gcobject.getGcThreshold();
      if (trans.preGcThreshold != loadThreshold) {
        trans.gcobject.gcVersion(trans.rsobject);
        trans.preGcThreshold = loadThreshold;
#ifdef CCTR_ON
        trans.gcobject.gcTMTelement(trans.rsobject);
#endif // CCTR_ON
      }
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

  if (*myid == 0) {
    if (pthread_create(&t, nullptr, manager_worker, (void *)myid)) ERR;
  } else {
    if (pthread_create(&t, nullptr, worker, (void *)myid)) ERR;
  }

  return t;
}

int
main(const int argc, const char *argv[])
{
  Result rsobject;
  chkArg(argc, argv);
  makeDB();

  //displayDB();
  //displayPRO();

  pthread_t thread[THREAD_NUM];

  for (unsigned int i = 0; i < THREAD_NUM; ++i) {
    thread[i] = threadCreate(i);
  }

  for (unsigned int i = 0; i < THREAD_NUM; ++i) {
    pthread_join(thread[i], nullptr);
  }

  //displayDB();

  rsobject.displayTPS();
  rsobject.displayAbortRate();
  //rsobject.displayAbortCounts();
  //rsobject.displayGCVersionCountsPS();
  //rsobject.displayGCTMTElementsCountsPS();

  return 0;
}
