#ifndef TRANSACTION_MANAGER_H
#define TRANSACTION_MANAGER_H

#include "common.h"

class TransactionManager {
public:
    static void SaveTransaction(const Wallet& wallet, const Transaction& transaction);
    static std::vector<Transaction> LoadTransactions(const Wallet& wallet);
    static std::vector<Transaction> FilterTransactionsByDate(const std::vector<Transaction>& transactions,
        const std::wstring& startDate, const std::wstring& endDate);
    static ReportStats CalculateReportStats(const std::vector<Transaction>& transactions);
};

#endif