// tests/optimizer_test.cc
#include <cassert>
#include <iostream>
#include <chrono>
#include "../include/log_manager.h"
#include "../include/log_optimizer.h"

void TestMassiveDataSetOptimization() {
    std::cout << "\n=== Testing Massive Dataset Optimization (1M records) ===" << std::endl;
    
    // 大規模テストデータの設定
    const size_t NUM_RECORDS = 1000000;  // 100万レコード
    const size_t NUM_KEYS = 10000;       // 1万種類のキー
    
    std::cout << "Generating " << NUM_RECORDS << " records..." << std::endl;
    
    // データ生成開始時間を記録
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<LogRecord> massive_logs;
    massive_logs.reserve(NUM_RECORDS);  // メモリを事前確保

    // テストデータの生成
    for(size_t i = 0; i < NUM_RECORDS; i++) {
        uint64_t key = i % NUM_KEYS;  // キーを循環させる
        uint64_t value = i * 100;
        Ope op = (i % 3 == 0) ? Ope::READ : Ope::WRITE;  // 33%がREAD操作
        
        massive_logs.push_back({
            static_cast<uint64_t>(i),  // tx_id
            key,
            value,
            op
        });

        // 進捗表示（10万レコードごと）
        if (i % 100000 == 0) {
            std::cout << "Generated " << i << " records..." << std::endl;
        }
    }

    auto generation_end = std::chrono::high_resolution_clock::now();
    auto generation_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        generation_end - start_time).count();
    
    std::cout << "Data generation completed in " << generation_time << "ms" << std::endl;
    std::cout << "Total size: " << massive_logs.size() << " records" << std::endl;

    // 最適化の実行と時間計測
    std::cout << "\nStarting optimization..." << std::endl;
    LogOptimizer optimizer;
    auto optimize_start = std::chrono::high_resolution_clock::now();
    
    auto optimized = optimizer.OptimizeLogs(massive_logs);
    
    auto optimize_end = std::chrono::high_resolution_clock::now();
    auto optimization_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        optimize_end - optimize_start).count();

    // 結果の表示
    std::cout << "\nOptimization Results:" << std::endl;
    std::cout << "Original log size: " << massive_logs.size() << std::endl;
    std::cout << "Optimized log size: " << optimized.size() << std::endl;
    std::cout << "Optimization rate: " 
              << (1.0 - static_cast<double>(optimized.size()) / massive_logs.size()) * 100 
              << "%" << std::endl;
    std::cout << "Optimization time: " << optimization_time << "ms" << std::endl;
    
    // キーの分布の確認（最初の10件だけ表示）
    std::cout << "\nFirst 10 optimized records:" << std::endl;
    for(size_t i = 0; i < std::min(size_t(10), optimized.size()); i++) {
        optimized[i].Print();
    }
}

int main() {
    TestMassiveDataSetOptimization();
    return 0;
}