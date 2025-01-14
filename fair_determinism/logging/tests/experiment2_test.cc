// tests/experiment2_test.cc
#include <cassert>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include "../include/logging_system.h"
#include "../include/logging_observer.h"
#include "../include/util/zipf.hh"
#include "../include/util/random.hh"

// 定数定義
constexpr size_t DATABASE_SIZE = 1000000;  // 100万レコード
constexpr size_t DURATION_MS = 3000;       // 3秒実行
constexpr double FIXED_SKEW = 0.8;         // 固定skew値

// 結果をCSVに書き込むためのヘルパー関数
void writeToCSV(const std::string& filename, double write_ratio, double reduction_rate) {
    static bool first_write = true;
    std::ofstream file;
    
    if (first_write) {
        file.open(filename);
        file << "write_ratio,reduction_rate" << std::endl;
        first_write = false;
    } else {
        file.open(filename, std::ios::app);
    }
    
    file << std::fixed << std::setprecision(2) 
         << write_ratio << "," 
         << reduction_rate << std::endl;
}

// ベンチマーク実行関数
void runWriteRatioTest(double write_ratio) {
    // Zipf分布の初期化（固定skew）
    Xoroshiro128Plus rnd;
    FastZipf zipf(&rnd, FIXED_SKEW, DATABASE_SIZE);

    // システムの初期化
    LoggingObserver::initialize(true);
    
    // スループットの計測
    auto start = std::chrono::high_resolution_clock::now();
    uint64_t operation_count = 0;
    uint64_t write_count = 0;
    uint64_t original_log_size = 0;
    uint64_t optimized_log_size = 0;

    while (true) {
        // 一定時間経過したら終了
        auto current = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            current - start).count();
        if (elapsed >= DURATION_MS) break;

        // Zipf分布に従ってキーを生成
        uint64_t key = zipf();

        // write_ratioに基づいて操作を決定
        bool is_write = (rnd.next() % 100) < (write_ratio * 100);
        
        // トランザクション実行とログサイズ計測
        if (is_write) {
            LoggingObserver::observeWrite(operation_count, key, operation_count * 100);
            write_count++;
            original_log_size += sizeof(LogRecord);
        } else {
            LoggingObserver::observeRead(operation_count, key, 0);
            original_log_size += sizeof(LogRecord);
        }
        
        operation_count++;
    }

    // 最適化後のログサイズを取得
    auto optimized_logs = LoggingObserver::getCurrentLogs();
    optimized_log_size = optimized_logs.size() * sizeof(LogRecord);

    // Reduction rate計算
    double reduction_rate = 100.0 * (1.0 - static_cast<double>(optimized_log_size) / original_log_size);

    // 結果出力とCSV保存
    std::cout << "Write Ratio: " << std::fixed << std::setprecision(2) << write_ratio 
              << " Reduction Rate: " << reduction_rate << "%" << std::endl;
    
    writeToCSV("experiment2_results.csv", write_ratio, reduction_rate);
}

int main() {
    std::cout << "=== Write/Read Ratio Impact Test ===" << std::endl;
    std::cout << "Fixed Skew: " << FIXED_SKEW << std::endl;
    
    // write_ratioを0.0から1.0まで0.1刻みで実験
    for (double write_ratio = 0.0; write_ratio <= 1.0; write_ratio += 0.1) {
        runWriteRatioTest(write_ratio);
    }
    
    return 0;
}