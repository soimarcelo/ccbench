// tests/logging_system_test.cc
#include <cassert>
#include <iostream>
#include "../include/logging_system.h"

void PrintTestHeader(const std::string& test_name) {
    std::cout << "\n=== Starting Test: " << test_name << " ===" << std::endl;
}

void PrintTestFooter(const std::string& test_name) {
    std::cout << "=== Test Completed: " << test_name << " ===" << std::endl;
}

// 基本的な操作のテスト
void TestBasicOperations() {
    PrintTestHeader("Basic Operations");
    
    LoggingSystem system;
    
    // 基本的なログ記録のテスト
    LogRecord record1{1, 100, 1000, Ope::WRITE};
    LogRecord record2{1, 200, 2000, Ope::READ};
    
    std::cout << "Adding logs..." << std::endl;
    system.ProcessTransaction(record1);
    system.ProcessTransaction(record2);
    
    auto logs = system.GetCurrentLogs();
    std::cout << "Current log count: " << logs.size() << std::endl;
    
    system.DumpStatus();
    
    PrintTestFooter("Basic Operations");
}

// バッチ処理のテスト
void TestBatchProcessing() {
    PrintTestHeader("Batch Processing");
    
    LoggingSystem system;
    
    // バッチサイズまでログを追加
    std::cout << "Adding logs up to batch size..." << std::endl;
    for(uint64_t i = 0; i < 1000; i++) {
        LogRecord record{i, i % 100, i * 100, Ope::WRITE};
        system.ProcessTransaction(record);
    }
    
    auto logs = system.GetCurrentLogs();
    std::cout << "Final log count: " << logs.size() << std::endl;
    
    system.DumpStatus();
    
    PrintTestFooter("Batch Processing");
}

int main() {
    std::cout << "Starting LoggingSystem tests..." << std::endl;
    
    TestBasicOperations();
    TestBatchProcessing();
    
    std::cout << "\nAll tests completed successfully!" << std::endl;
    return 0;
}