// include/log_optimizer.h
#pragma once
#include <map>
#include "log_manager.h"  // LogRecordとOpeの定義を使用

class LogOptimizer {
public:
    // 最適化関数
    std::vector<LogRecord> OptimizeLogs(const std::vector<LogRecord>& logs) {
        std::map<uint64_t, LogRecord> last_writes;  // キーごとの最後の書き込み
        
        // 全ログをスキャンして各キーの最後の書き込みを記録
        for (const auto& record : logs) {
            if (record.operation_type == Ope::WRITE) {
                last_writes[record.key] = record;
            }
        }
        
        // 最適化されたログの生成
        std::vector<LogRecord> optimized_logs;
        for (const auto& [key, record] : last_writes) {
            optimized_logs.push_back(record);
        }
        
        return optimized_logs;
    }

    // デバッグ用の出力関数
    void PrintOptimizationResult(const std::vector<LogRecord>& original,
                                const std::vector<LogRecord>& optimized) {
        std::cout << "\n=== Optimization Results ===" << std::endl;
        std::cout << "Original logs: " << original.size() << " records" << std::endl;
        std::cout << "Optimized logs: " << optimized.size() << " records" << std::endl;
        
        std::cout << "\nOriginal logs:" << std::endl;
        for (const auto& record : original) {
            record.Print();
        }
        
        std::cout << "\nOptimized logs:" << std::endl;
        for (const auto& record : optimized) {
            record.Print();
        }
    }
};