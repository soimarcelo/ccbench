#pragma once

#include <atomic>

#include "../../include/result.hh"

class CicadaResult : public Result {
public:
  using Result::Result;

#if ADD_ANALYSIS
  uint64_t total_gc_counts_ = 0;
  uint64_t local_gc_counts_ = 0;
  uint64_t total_gc_versions_ = 0;
  uint64_t local_gc_versions_ = 0;
  uint64_t local_gc_tics_ = 0;
  uint64_t total_gc_tics_ = 0;

  void displayTotalGCCounts();
  void displayTotalGCVersions();
  void displayTotalGCTics();
  void displayGCPhaseRate();
  void addLocalGCCounts(uint64_t gcount);
  void addLocalGCVersions(uint64_t vcount);
  void addLocalGCTics(uint64_t tics);
#endif
  void displayAllCicadaResult();
  void addLocalAllCicadaResult(CicadaResult &other);
};

