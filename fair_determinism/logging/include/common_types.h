// include/common_types.h
#pragma once
#include <cstdint>
#include <iostream>

// 操作の種類を定義
// Note: SS2PLと統合する際には、SS2PLのOpe enumを使用する予定
enum class Ope {
    READ,
    WRITE,
    SLEEP
};

// ログレコードの構造体
// トランザクションIDとキー、値、操作種別を保持
struct LogRecord {
    uint64_t tx_id;  // トランザクションの識別子
    uint64_t key;    // 操作対象のキー
    uint64_t value;  // 操作の値
    Ope operation_type;  // READ/WRITE/SLEEP

    // デバッグ用の出力メソッド
    void Print() const {
        std::cout << "TX: " << tx_id 
                  << ", Key: " << key 
                  << ", Value: " << value 
                  << ", Op: " << (operation_type == Ope::READ ? "READ" : "WRITE")
                  << std::endl;
    }
};