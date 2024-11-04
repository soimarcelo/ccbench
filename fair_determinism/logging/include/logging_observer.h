// include/logging_observer.h
#pragma once
#include "logging_system.h"
#include <memory>
#include <atomic>
#include <iostream>

class LoggingObserver {
private:
    static std::unique_ptr<LoggingSystem> logging_system_;
    static std::atomic<uint64_t> log_counter_;

public:
    // 初期化
    static void initialize(bool use_optimizer = true) {
        logging_system_ = std::make_unique<LoggingSystem>();
        log_counter_ = 0;
    }

    // トランザクション操作の監視
    static void observeWrite(uint64_t tx_id, uint64_t key, uint64_t value) {
        if (!logging_system_) return;
        LogRecord record{tx_id, key, value, Ope::WRITE};
        logging_system_->ProcessTransaction(record);
        log_counter_++;
    }

    static void observeRead(uint64_t tx_id, uint64_t key, uint64_t value) {
        if (!logging_system_) return;
        LogRecord record{tx_id, key, value, Ope::READ};
        logging_system_->ProcessTransaction(record);
        log_counter_++;
    }

    // ログ取得用のメソッド（名前修正）
    static std::vector<LogRecord> getCurrentLogs() {
        if (!logging_system_) return {};
        return logging_system_->GetCurrentLogs();
    }

    // 最適化実行用のメソッド（名前修正）
    static void optimizeLogs() {
        if (!logging_system_) return;
        logging_system_->OptimizeLogs();
    }

    static uint64_t getThrouput() {
        return log_counter_.load();
    }
};

// static変数の定義
std::unique_ptr<LoggingSystem> LoggingObserver::logging_system_ = nullptr;
std::atomic<uint64_t> LoggingObserver::log_counter_(0);