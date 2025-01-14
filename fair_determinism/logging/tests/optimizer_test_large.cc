// tests/optimizer_test_large.cc
#include <cassert>
#include <iostream>
#include <iomanip>
#include "../include/log_optimizer.h"
#include "../include/util/zipf.hh"
#include "../include/util/random.hh"

constexpr size_t NUM_RECORDS = 1000000;
constexpr size_t NUM_KEYS = NUM_RECORDS;
constexpr int RW_RATE = 80;

void runLargeTest(double skew) {
   Xoroshiro128Plus rnd;
   FastZipf zipf(&rnd, skew, NUM_KEYS);
   std::vector<LogRecord> logs;

   for (size_t i = 0; i < NUM_RECORDS; i++) {
       uint64_t key = zipf();
       if ((rnd.next() % 100) >= RW_RATE) {
           logs.push_back({i, key, i*100, Ope::WRITE});
       }
   }

   LogOptimizer optimizer;
   auto optimized = optimizer.OptimizeLogs(logs);
   
   double reduction = 100.0 * (1.0 - static_cast<double>(optimized.size()) / logs.size());
   std::cout << std::fixed << std::setprecision(4) << skew << ", " << reduction << std::endl;
}

int main() {
   std::cout << "skew, reduction_rate(%)" << std::endl;
   for (double skew = 0.0; skew <= 1.0; skew += 0.1) {
       runLargeTest(skew);
   }
   return 0;
}