// include/log_manager.h
#pragma once
#include <vector>
#include <fstream>
#include "common_types.h"

/**
 * LogManager: トランザクションログの記録と管理を行うクラス
 * - ログをメモリ上のバッファに保持
 * - ストレージへの書き込み機能を提供
 */
class LogManager {
private:
    std::vector<LogRecord> log_buffer_;     // メモリ上のログバッファ
    std::string file_path_;                 // ストレージファイルのパス
    std::ofstream storage_file_;            // ストレージファイルのハンドル

public:
    // コンストラクタ: ストレージファイルをオープン
    explicit LogManager(const std::string& file_path = "storage.log") 
        : file_path_(file_path) {
        storage_file_.open(file_path_, std::ios::app);
    }

    // ログレコードをバッファに追加
    void AppendLog(const LogRecord& record) {
        log_buffer_.push_back(record);
    }

    // 最適化済みログでバッファを更新
    void UpdateLogs(const std::vector<LogRecord>& new_logs) {
        log_buffer_ = new_logs;
    }

    // 現在のログ数を取得
    size_t GetLogSize() const {
        return log_buffer_.size();
    }

    // 現在のログバッファを取得
    const std::vector<LogRecord>& GetLogs() const {
        return log_buffer_;
    }

    // バッファの内容をストレージに書き込み
    void FlushToStorage() {
        for (const auto& record : log_buffer_) {
            storage_file_ << record.tx_id << ","
                         << record.key << ","
                         << record.value << ","
                         << static_cast<int>(record.operation_type) << "\n";
        }
        storage_file_.flush();
        log_buffer_.clear();  // バッファをクリア
    }
};