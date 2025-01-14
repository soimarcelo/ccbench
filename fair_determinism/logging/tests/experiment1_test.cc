// tests/experiment1_test.cc
#include <cassert>
#include <iostream>
#include <chrono>
#include <vector>
#include "../include/logging_system.h"
#include "../include/logging_observer.h"
#include "../include/util/zipf.hh"
#include "../include/util/random.hh"

// 実験パラメータ
constexpr size_t TUPLE_NUM = 1000000;  // 100万レコード
constexpr int RW_RATE = 80;            // 80% READ
constexpr int DURATION_SEC = 3;        // 3秒実行

void runBenchmark(double skew, bool use_optimizer) {
    // 初期化
    Xoroshiro128Plus rnd;
    FastZipf zipf(&rnd, skew, TUPLE_NUM);
    LoggingSystem system;  

    uint64_t write_count = 0;  // WRITEのみカウント
    auto start = std::chrono::high_resolution_clock::now();
    auto end_time = start + std::chrono::seconds(DURATION_SEC);

    while (std::chrono::high_resolution_clock::now() < end_time) {
        uint64_t key = zipf();
        if ((rnd.next() % 100) >= RW_RATE) {  // WRITE操作の場合のみ
            LogRecord record{write_count, key, write_count * 100, Ope::WRITE};
            system.ProcessTransaction(record);
            write_count++;

            // 1000書き込みごとに最適化（use_optimizerがtrueの場合）
            if (use_optimizer && (write_count % 1000 == 0)) {
                system.OptimizeLogs();
            }
        }
    }

    double throughput = static_cast<double>(write_count) / DURATION_SEC;
    std::cout << std::fixed << std::setprecision(1) 
              << skew << ", " << throughput << std::endl;
}

int main() {
    std::cout << "skew, throughput(writes/sec)" << std::endl;

    std::cout << "\n# Without Optimization" << std::endl;
    for (double skew = 0.0; skew <= 1.0; skew += 0.1) {
        runBenchmark(skew, false);
    }

    std::cout << "\n# With Optimization" << std::endl;
    for (double skew = 0.0; skew <= 1.0; skew += 0.1) {
        runBenchmark(skew, true);
    }

    return 0;
}