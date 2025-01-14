// tests/rw_ratio_test.cc
#include <cassert>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include "../include/logging_system.h"
#include "../include/logging_observer.h"
#include "../include/util/zipf.hh"
#include "../include/util/random.hh"

// 実験パラメータ
constexpr size_t DATABASE_SIZE = 1000000;   // 100万レコード
constexpr size_t DURATION_MS = 3000;        // 3秒実行
constexpr size_t OPERATIONS_PER_TX = 100;   // 1トランザクションあたりの操作数を増やす
constexpr size_t BATCH_SIZE = 1000;         // バッチサイズを明示的に設定

void writeToCSV(const std::string& filename, double skew, double rw_ratio, 
                double reduction_rate, double throughput) {
    static bool first_write = true;
    std::ofstream file;
    
    if (first_write) {
        file.open(filename);
        file << "skew,rw_ratio,reduction_rate,throughput" << std::endl;
        first_write = false;
    } else {
        file.open(filename, std::ios::app);
    }
    
    file << std::fixed 
         << std::setprecision(2) << skew << "," 
         << std::setprecision(2) << rw_ratio << ","
         << std::setprecision(2) << reduction_rate << ","
         << std::setprecision(2) << throughput << std::endl;
}

void runExperiment(double skew, double read_ratio) {
    // Zipf分布の初期化
    Xoroshiro128Plus rnd;
    FastZipf zipf(&rnd, skew, DATABASE_SIZE);

    // システム初期化
    LoggingObserver::initialize(true);
    
    auto start = std::chrono::high_resolution_clock::now();
    size_t operation_count = 0;
    size_t original_log_size = 0;
    size_t optimized_log_size = 0;

    while (true) {
        // 実行時間チェック
        auto current = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            current - start).count();
        if (elapsed >= DURATION_MS) break;

        // バッチ単位でトランザクションを生成
        for (size_t batch = 0; batch < BATCH_SIZE; batch++) {
            uint64_t key = zipf();  // より強いskewを反映
            bool is_read = (rnd.next() % 100) < (read_ratio * 100);

            LogRecord record{
                operation_count,
                key,
                operation_count * 100,
                is_read ? Ope::READ : Ope::WRITE
            };

            if (is_read) {
                LoggingObserver::observeRead(record.tx_id, record.key, record.value);
            } else {
                LoggingObserver::observeWrite(record.tx_id, record.key, record.value);
                original_log_size++;
            }
            operation_count++;
        }

        // バッチごとに最適化を実行
        LoggingObserver::optimizeLogs();
    }

    // 最終的な最適化サイズを取得
    auto optimized_logs = LoggingObserver::getCurrentLogs();
    optimized_log_size = optimized_logs.size();

    // 結果計算
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    double throughput = (operation_count * 1000.0) / duration.count();
    double reduction_rate = original_log_size > 0 ? 
        (1.0 - static_cast<double>(optimized_log_size) / original_log_size) * 100.0 : 0.0;

    std::cout << "Skew: " << std::fixed << std::setprecision(2) << skew 
              << " R/W Ratio: " << read_ratio
              << " Reduction Rate: " << reduction_rate << "%"
              << " Throughput: " << throughput << " ops/sec" << std::endl;
    
    writeToCSV("rw_ratio_results.csv", skew, read_ratio, reduction_rate, throughput);
}

int main() {
    std::cout << "=== R/W Ratio Impact Test ===" << std::endl;
    
    // skewとread_ratioを0.1刻みで実験
    for (double skew = 0.0; skew <= 0.9; skew += 0.1) {
        for (double read_ratio = 0.0; read_ratio <= 0.9; read_ratio += 0.1) {
            runExperiment(skew, read_ratio);
        }
    }
    
    return 0;
}