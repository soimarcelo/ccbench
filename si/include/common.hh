#pragma once

#include <mutex>
#include <vector>

#include "../../include/cache_line_size.hh"
#include "../../include/int64byte.hh"
#include "../../include/masstree_wrapper.hh"
#include "transaction_table.hh"
#include "tuple.hh"

#ifdef GLOBAL_VALUE_DEFINE
#define GLOBAL
alignas(CACHE_LINE_SIZE) GLOBAL std::atomic<uint64_t> CCtr(0);
#if MASSTREE_USE
alignas(CACHE_LINE_SIZE) GLOBAL MasstreeWrapper<Tuple> MT;
#endif
#else
#define GLOBAL extern
alignas(CACHE_LINE_SIZE) GLOBAL std::atomic<uint64_t> CCtr;
#if MASSTREE_USE
alignas(CACHE_LINE_SIZE) GLOBAL MasstreeWrapper<Tuple> MT;
#endif
#endif

// run-time args
GLOBAL size_t TUPLE_NUM;
GLOBAL size_t MAX_OPE;
GLOBAL size_t THREAD_NUM;
GLOBAL size_t RRATIO;
GLOBAL bool RMW;
GLOBAL double ZIPF_SKEW;
GLOBAL bool YCSB;
GLOBAL size_t CLOCKS_PER_US;
GLOBAL size_t GC_INTER_US;  // garbage collection interval
GLOBAL size_t PRE_RESERVE_VERSION;
GLOBAL size_t EXTIME;
// -----

#include "transaction.hh"

alignas(CACHE_LINE_SIZE) GLOBAL Tuple *Table;
alignas(CACHE_LINE_SIZE) GLOBAL
    TransactionTable **TMT;  // Transaction Mapping Table
