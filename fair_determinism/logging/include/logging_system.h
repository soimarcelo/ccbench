// include/logging_system.h
#pragma once
#include <vector>
#include <memory>
#include <iostream>
#include "common_types.h"
#include "log_manager.h"
#include "log_optimizer.h"

/**
 * LoggingSystem: ログ管理と最適化を統合するシステム
 * - LogManagerとLogOptimizerを統合
 * - バッチ単位でログを処理
 * - 将来的にSS2PLと統合予定
 */
class LoggingSystem {
private:
    LogManager manager_;
    LogOptimizer optimizer_;
    size_t batch_count_;
    static constexpr size_t BATCH_SIZE = 1000;  // 最適化実行の閾値

    // デバッグ用の出力レベル
    void PrintDebug(const std::string& message) const {
        // std::cout << "[DEBUG] LoggingSystem: " << message << std::endl;
    }

    void PrintInfo(const std::string& message) const {
        // std::cout << "[INFO] LoggingSystem: " << message << std::endl;
    }

    void PrintStatus() const {
        std::cout << "\n=== LoggingSystem Status ===" << std::endl;
        std::cout << "Current batch count: " << batch_count_ << std::endl;
        std::cout << "Batch size limit: " << BATCH_SIZE << std::endl;
        std::cout << "Current log count: " << manager_.GetLogSize() << std::endl;
        std::cout << "=========================" << std::endl;
    }

public:
    LoggingSystem() : batch_count_(0) {
        PrintInfo("Initialized");
    }

    // トランザクションログの処理
    void ProcessTransaction(const LogRecord& record) {
        // PrintDebug("Processing transaction " + std::to_string(record.tx_id));
        
        // ログの追加
        manager_.AppendLog(record);
        batch_count_++;

        // PrintDebug("Current batch count: " + std::to_string(batch_count_));
        
        // バッチサイズに達したら最適化を実行
        if (batch_count_ >= BATCH_SIZE) {
            PrintInfo("Batch size reached. Starting optimization...");
            OptimizeLogs();
            batch_count_ = 0;
            PrintInfo("Optimization completed");
        }
    }

    // ログの最適化を実行
    void OptimizeLogs() {
        PrintDebug("Starting log optimization");
        auto logs = manager_.GetLogs();
        
        PrintInfo("Pre-optimization log count: " + std::to_string(logs.size()));
        
        auto optimized = optimizer_.OptimizeLogs(logs);
        manager_.UpdateLogs(optimized);
        
        PrintInfo("Post-optimization log count: " + std::to_string(optimized.size()));
    }

    // 現在のログを取得（テスト用）
    std::vector<LogRecord> GetCurrentLogs() const {
        return manager_.GetLogs();
    }

    // システムの状態を出力
    void DumpStatus() const {
        PrintStatus();
    }
};