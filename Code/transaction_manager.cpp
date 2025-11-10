#include "transaction_manager.h"
#include "common.h"

void TransactionManager::SaveTransaction(const Wallet& wallet, const Transaction& transaction) {
    if (g_currentUser == L"Гость") return;

    std::wstring filename = GetUserCurrencyFile() + L"_" + wallet.name + L"_transactions.txt";
    std::wofstream file(filename, std::ios::app);
    if (!file.is_open()) return;

    file.imbue(std::locale(""));

    file << L"DATE:" << transaction.date << std::endl;
    file << L"SOLD:" << transaction.soldCurrency << L":" << transaction.soldAmount << std::endl;
    file << L"BOUGHT:" << transaction.boughtCurrency << L":" << transaction.boughtAmount << std::endl;
    file << L"RATE:" << transaction.exchangeRate << std::endl;
    file << L"WALLET_STATE:" << std::endl;

    for (const auto& currency : transaction.walletState) {
        file << currency.currency << L":" << currency.amount << L";";
    }
    file << std::endl;
    file << L"ENDTRANSACTION" << std::endl;

    file.close();
}

std::vector<Transaction> TransactionManager::LoadTransactions(const Wallet& wallet) {
    std::vector<Transaction> transactions;

    if (g_currentUser == L"Гость") return transactions;

    std::wstring filename = GetUserCurrencyFile() + L"_" + wallet.name + L"_transactions.txt";
    std::wifstream file(filename);
    if (!file.is_open()) return transactions;

    file.imbue(std::locale(""));

    std::wstring line;
    Transaction currentTransaction;
    bool readingTransaction = false;
    bool readingWalletState = false;

    while (std::getline(file, line)) {
        if (line.find(L"DATE:") == 0) {
            if (readingTransaction) {
                transactions.push_back(currentTransaction);
            }
            currentTransaction = Transaction();
            currentTransaction.date = line.substr(5);
            readingTransaction = true;
        }
        else if (line.find(L"SOLD:") == 0) {
            size_t colonPos = line.find(L':', 5);
            if (colonPos != std::wstring::npos) {
                currentTransaction.soldCurrency = line.substr(5, colonPos - 5);
                std::wstring amountStr = line.substr(colonPos + 1);
                try {
                    currentTransaction.soldAmount = std::stod(amountStr);
                }
                catch (...) {}
            }
        }
        else if (line.find(L"BOUGHT:") == 0) {
            size_t colonPos = line.find(L':', 7);
            if (colonPos != std::wstring::npos) {
                currentTransaction.boughtCurrency = line.substr(7, colonPos - 7);
                std::wstring amountStr = line.substr(colonPos + 1);
                try {
                    currentTransaction.boughtAmount = std::stod(amountStr);
                }
                catch (...) {}
            }
        }
        else if (line.find(L"RATE:") == 0) {
            std::wstring rateStr = line.substr(5);
            try {
                currentTransaction.exchangeRate = std::stod(rateStr);
            }
            catch (...) {}
        }
        else if (line.find(L"WALLET_STATE:") == 0) {
            readingWalletState = true;
            currentTransaction.walletState.clear();
        }
        else if (readingWalletState && !line.empty()) {
            size_t start = 0;
            size_t end = line.find(L';');
            while (end != std::wstring::npos) {
                std::wstring currencyState = line.substr(start, end - start);
                size_t colonPos = currencyState.find(L':');
                if (colonPos != std::wstring::npos) {
                    WalletCurrency wc;
                    wc.currency = currencyState.substr(0, colonPos);
                    try {
                        wc.amount = std::stod(currencyState.substr(colonPos + 1));
                        currentTransaction.walletState.push_back(wc);
                    }
                    catch (...) {}
                }
                start = end + 1;
                end = line.find(L';', start);
            }
            readingWalletState = false;
        }
        else if (line == L"ENDTRANSACTION") {
            if (readingTransaction) {
                transactions.push_back(currentTransaction);
            }
            readingTransaction = false;
        }
    }

    if (readingTransaction) {
        transactions.push_back(currentTransaction);
    }

    file.close();
    return transactions;
}

std::vector<Transaction> TransactionManager::FilterTransactionsByDate(const std::vector<Transaction>& transactions,
    const std::wstring& startDate, const std::wstring& endDate) {
    std::vector<Transaction> filtered;

    for (const auto& transaction : transactions) {
        if (transaction.date >= startDate && transaction.date <= endDate) {
            filtered.push_back(transaction);
        }
    }

    return filtered;
}

ReportStats TransactionManager::CalculateReportStats(const std::vector<Transaction>& transactions) {
    ReportStats stats;
    stats.transactionCount = static_cast<int>(transactions.size());

    for (const auto& transaction : transactions) {
        stats.totalSold += transaction.soldAmount;
        stats.totalBought += transaction.boughtAmount;

        stats.currencyChanges[transaction.soldCurrency] -= transaction.soldAmount;
        stats.currencyChanges[transaction.boughtCurrency] += transaction.boughtAmount;
    }

    return stats;
}