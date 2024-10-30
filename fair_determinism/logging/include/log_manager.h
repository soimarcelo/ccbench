// include/log_manager.h
#pragma once
#include <vector>
#include <fstream>
#include <iostream>
#include <chrono>

// ss2pl.ccからOpeだけを抽出
enum class Ope {
    READ,
    WRITE,
    SLEEP
};

struct LogRecord {
    uint64_t tx_id;
    uint64_t key;
    uint64_t value;
    Ope operation_type;

    void Print() const {
        std::cout << "TX: " << tx_id 
                  << ", Key: " << key 
                  << ", Value: " << value 
                  << ", Op: " << (operation_type == Ope::READ ? "READ" : "WRITE")
                  << std::endl;
    }
};

class LogManager {

public:
    LogManager(const std::string& file_path = "storage.log") 
        : file_path_(file_path) {
        storage_file_.open(file_path_, std::ios::app);
    }

    void AppendLog(const LogRecord& record) {
        log_buffer_.push_back(record);
    }

    void FlushToStorage() {
        for (const auto& record : log_buffer_) {
            storage_file_ << record.tx_id << ","
                         << record.key << ","
                         << record.value << ","
                         << static_cast<int>(record.operation_type) << "\n";
        }
        storage_file_.flush();
        log_buffer_.clear();
    }

    size_t GetLogSize() const {
        return log_buffer_.size();
    }

    const std::vector<LogRecord>& GetLogs() const {
        return log_buffer_;
    }

private:
    std::vector<LogRecord> log_buffer_;
    std::string file_path_;
    std::ofstream storage_file_;
};