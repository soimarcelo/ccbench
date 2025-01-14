#include <iostream>
#include <vector>
#include <iomanip>
#include "../include/logging_system.h"
#include "../include/log_optimizer.h"
#include "../include/common_types.h"
// トランザクション生成関数
void generate_transactions(LoggingSystem& system, int num_transactions, double rw_ratio) {
    for (int i = 0; i < num_transactions; ++i) {
        uint64_t tx_id = i;
        uint64_t key = i % 100; // キーは0-99で循環
        uint64_t value = 123;   // ダミーの値
        Ope operation_type = (i < num_transactions * rw_ratio) ? Ope::WRITE : Ope::READ;

        // トランザクションログを作成
        LogRecord record{tx_id, key, value, operation_type};

        // ログシステムに追加
        system.ProcessTransaction(record);
    }
}

void run_rw_ratio_test(int num_transactions, double step = 0.1) {
    std::cout << std::setw(10) << "RW Ratio" << std::setw(20) << "Reduction Rate (%)" << "\n";
    std::cout << "------------------------------------------\n";

    for (double rw_ratio = 0.0; rw_ratio <= 1.0; rw_ratio += step) {
        // LoggingSystem と LogOptimizer の初期化
        LoggingSystem logging_system;
        LogOptimizer optimizer;

        // トランザクションを生成してシステムに追加
        generate_transactions(logging_system, num_transactions, rw_ratio);

        // ログの取得と最適化
        auto original_logs = logging_system.GetCurrentLogs();
        auto optimized_logs = optimizer.OptimizeLogs(original_logs);

        // 削減率の計算
        double original_size = original_logs.size();
        double optimized_size = optimized_logs.size();
        double reduction_rate = (1.0 - optimized_size / original_size) * 100.0;

        // 結果を出力
        std::cout << std::fixed << std::setprecision(2);
        std::cout << std::setw(10) << rw_ratio << std::setw(20) << reduction_rate << "\n";
    }
}

int main() {
    // テスト実行
    int num_transactions = 1000; // 生成するトランザクション数
    run_rw_ratio_test(num_transactions);

    return 0;
}
