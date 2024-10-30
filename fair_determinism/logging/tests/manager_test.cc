// tests/manager_test.cc
#include "../include/log_manager.h"
#include <iostream>

int main() {
    std::cout << "=== Starting Log Manager Test ===" << std::endl;

    // 1. LogManagerのインスタンス作成
    std::cout << "Creating LogManager..." << std::endl;
    LogManager log_manager("test.log");

    // 2. テスト用のログレコードを作成
    std::cout << "\nCreating test log records..." << std::endl;
    LogRecord record1{1, 100, 1000, Ope::WRITE};
    LogRecord record2{1, 200, 2000, Ope::READ};

    // 3. レコードの内容を確認
    std::cout << "\nPrinting record contents:" << std::endl;
    std::cout << "Record 1: ";
    record1.Print();
    std::cout << "Record 2: ";
    record2.Print();

    // 4. ログの追加
    std::cout << "\nAppending logs..." << std::endl;
    log_manager.AppendLog(record1);
    log_manager.AppendLog(record2);
    std::cout << "Current log size: " << log_manager.GetLogSize() << std::endl;

    // 5. ストレージへの書き込み
    std::cout << "\nFlushing to storage..." << std::endl;
    log_manager.FlushToStorage();
    std::cout << "After flush, log size: " << log_manager.GetLogSize() << std::endl;

    std::cout << "\n=== Test Completed ===" << std::endl;
    return 0;
} // ここ以外に } があれば削除