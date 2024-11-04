// tests/optimizer_test.cc
#include "../include/log_optimizer.h"  // 最適化のテストなのでlog_optimizerをインクルード
#include <vector>
#include <iostream>
#include <iomanip>

void printLogResults(const std::vector<LogRecord>& logs, bool is_small_dataset) {
    std::cout << "\nTotal records: " << logs.size() << std::endl;

    if (is_small_dataset) {
        // 小規模データの場合は全レコード表示
        std::cout << "\n=== All Records ===" << std::endl;
        for (const auto& log : logs) {
            std::cout << "TX: " << std::setw(3) << log.tx_id 
                      << ", Key: " << std::setw(3) << log.key 
                      << ", Value: " << std::setw(5) << log.value
                      << ", Op: " << (log.operation_type == Ope::READ ? "READ " : "WRITE")
                      << std::endl;
        }
    }
    std::cout << "===================" << std::endl;
}

void runTest(size_t num_records, size_t num_keys, const std::string& test_name) {
    bool is_small_dataset = (num_records <= 20);
    std::cout << "\n=== " << test_name << " ===" << std::endl;
    
    // テストデータの作成
    std::vector<LogRecord> logs;
    for (size_t i = 0; i < num_records; i++) {
        uint64_t key = i % num_keys;  // キーの繰り返し
        logs.push_back({i, key, i*100, Ope::WRITE});
        if (i % 2 == 0) {  // 50%の確率で読み込み
            logs.push_back({i, key, i*100, Ope::READ});
        }
    }

    std::cout << "\nOriginal logs:" << std::endl;
    printLogResults(logs, is_small_dataset);

    // 最適化の実行
    LogOptimizer optimizer;
    auto optimized_logs = optimizer.OptimizeLogs(logs);

    std::cout << "\nAfter optimization:" << std::endl;
    printLogResults(optimized_logs, is_small_dataset);

    // 削減率を計算
    double reduction_rate = 100.0 * 
        (1.0 - static_cast<double>(optimized_logs.size()) / logs.size());
    std::cout << "Reduction rate: " << std::fixed << std::setprecision(2) 
              << reduction_rate << "%" << std::endl;
}

int main() {
    // 小規模テスト（全レコード表示）
    runTest(10, 2, "Small Test (10 records, 2 keys)");
    
    // 大規模テスト
    runTest(1000000, 1000, "Large Test (1M records, 1000 keys)");

    return 0;
}