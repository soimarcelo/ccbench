#include <random>
#include <stdlib.h>
#include <atomic>
#include "include/tuple.hpp"
#include "include/debug.hpp"
#include "include/common.hpp"

using namespace std;

void
makeDB()
{
	Tuple *tmp;
	Version *verTmp;
	random_device rnd;

	try {
		if (posix_memalign((void**)&Table, 64, (TUPLE_NUM) * sizeof(Tuple)) != 0) ERR;
		for (unsigned int i = 0; i < TUPLE_NUM; i++) {
			if (posix_memalign((void**)&Table[i].latest, 64, sizeof(Version)) != 0) ERR;
		}
	} catch (bad_alloc) {
		ERR;
	}

	for (unsigned int i = 1; i <= TUPLE_NUM; i++) {
		tmp = &Table[i % TUPLE_NUM];
		tmp->key = i;
		verTmp = tmp->latest.load(std::memory_order_acquire);
		verTmp->cstamp = 0;
		verTmp->pstamp = 0;
		verTmp->sstamp = UINT64_MAX & ~(1);
		// cstamp, sstamp の最下位ビットは TID フラグ
		// 1の時はTID, 0の時はstamp
		verTmp->val.store(rnd() % (TUPLE_NUM * 10), memory_order_release);
		verTmp->prev = nullptr;
		verTmp->committed_prev = nullptr;
		verTmp->status.store(VersionStatus::committed, std::memory_order_release);
		verTmp->readers.store(0, memory_order_release);
	}
}
