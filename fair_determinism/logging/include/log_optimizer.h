// include/log_optimizer.h
#pragma once
#include <vector>
#include <map>
#include "common_types.h"

/**
 * LogOptimizer: ログの最適化を行うクラス
 * - 各キーの最後の書き込み操作のみを保持
 * - 読み取り操作は最適化の対象外
 */
class LogOptimizer {
public:
    // ログの最適化を実行
    // @param logs 最適化対象のログレコード群
    // @return 最適化後のログレコード群
    std::vector<LogRecord> OptimizeLogs(const std::vector<LogRecord>& logs) {
        // キーごとに最後の書き込み操作を保持
        std::map<uint64_t, LogRecord> last_writes;
        
        // 全ログをスキャンし、書き込み操作のみを対象に最適化
        for (const auto& log : logs) {
            if (log.operation_type == Ope::WRITE) {
                last_writes[log.key] = log;  // 同じキーの場合は上書き
            }
        }
        
        // 最適化されたログの生成
        // mapから最終的な書き込み操作のみを取り出す
        std::vector<LogRecord> optimized;
        for (const auto& [key, record] : last_writes) {
            optimized.push_back(record);
        }
        
        return optimized;
    }
};