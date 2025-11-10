#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <map>

#pragma comment(lib, "winhttp.lib")

// Структура для валюты в кошельке
struct WalletCurrency {
    std::wstring currency;
    double amount;
};

// Структура для кошелька
struct Wallet {
    std::wstring name;
    std::vector<WalletCurrency> currencies;
};

// Функция для расчета статистики по отчету
struct ReportStats {
    double totalSold = 0.0;
    double totalBought = 0.0;
    int transactionCount = 0;
    std::map<std::wstring, double> currencyChanges;
};

// Глобальные переменные
HINSTANCE g_hInstance;
std::wstring g_currentUser;
std::vector<std::wstring> g_currencyPairs;
std::vector<Wallet> g_wallets;
HWND hMain;

// Прототипы функций
LRESULT CALLBACK LoginWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MainMenuWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK CurrencyPairsWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK NotificationsWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WalletsWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL RegisterWindowClass(LPCWSTR className, WNDPROC windowProc);

// Структура для уведомления
struct Notification {
    std::wstring currency1;
    std::wstring currency2;
    double value1;
    double value2;
    std::wstring condition; // ">", "<", "="
    bool active;
};

// Список поддерживаемых валют Frankfurter API
std::vector<std::wstring> supportedCurrencies = {
    L"AUD", L"BGN", L"BRL", L"CAD", L"CHF", L"CNY", L"CZK", L"DKK", L"EUR",
    L"GBP", L"HKD", L"HUF", L"IDR", L"ILS", L"INR", L"ISK", L"JPY", L"KRW",
    L"MXN", L"MYR", L"NOK", L"NZD", L"PHP", L"PLN", L"RON", L"SEK", L"SGD",
    L"THB", L"TRY", L"USD", L"ZAR"
};


std::vector<Notification> g_notifications;
bool g_notificationThreadRunning = false;

// Структура для транзакции
struct Transaction {
    std::wstring date;
    std::wstring soldCurrency;
    double soldAmount;
    std::wstring boughtCurrency;
    double boughtAmount;
    double exchangeRate;
    std::vector<WalletCurrency> walletState; // Состояние кошелька после транзакции
};

// Глобальные переменные
int g_currentWalletIndex = -1; // Индекс открытого кошелька

// Функции для работы с валютными парами
std::wstring GetUserCurrencyFile() {
    if (g_currentUser == L"Гость") return L"";

    std::string userStr(g_currentUser.begin(), g_currentUser.end());
    return std::wstring(userStr.begin(), userStr.end()) + L"_pairs.txt";
}

// Структура для передачи данных в окно отчета
struct ReportData {
    Wallet wallet;
    std::vector<Transaction> transactions;
    std::wstring startDate;
    std::wstring endDate;
};

// Функция для расчета статистики по отчету
ReportStats CalculateReportStats(const std::vector<Transaction>& transactions) {
    ReportStats stats;
    stats.transactionCount = static_cast<int>(transactions.size());

    for (const auto& transaction : transactions) {
        stats.totalSold += transaction.soldAmount;
        stats.totalBought += transaction.boughtAmount;

        // Считаем изменения по валютам
        stats.currencyChanges[transaction.soldCurrency] -= transaction.soldAmount;
        stats.currencyChanges[transaction.boughtCurrency] += transaction.boughtAmount;
    }

    return stats;
}

// Функция создания окна отчета
void CreateReportWindow(HWND parent, const Wallet& wallet,
    const std::vector<Transaction>& transactions,
    const std::wstring& startDate, const std::wstring& endDate) {
    // Создаем данные для отчета
    ReportData* reportData = new ReportData;
    reportData->wallet = wallet;
    reportData->transactions = transactions;
    reportData->startDate = startDate;
    reportData->endDate = endDate;

    // Создаем окно отчета
    HWND hReportWindow = CreateWindow(
        L"ReportViewWindowClass",
        L"Отчет по кошельку",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 600,
        parent, NULL, g_hInstance, reportData
    );

    if (hReportWindow) {
        ShowWindow(hReportWindow, SW_SHOW);
        UpdateWindow(hReportWindow);
    }
    else {
        delete reportData; // Если окно не создалось, очищаем память
    }
}

// Функция заполнения содержимого отчета
void FillReportContent(HWND hListBox, const ReportData& reportData) {
    if (!hListBox) return;

    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

    // Статистика
    ReportStats stats = CalculateReportStats(reportData.transactions);

    // Заголовок
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"════════════════ ОТЧЕТ ════════════════");

    // Общая информация
    std::wstring periodInfo = L"Период: " + reportData.startDate + L" - " + reportData.endDate;
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)periodInfo.c_str());

    std::wstring walletInfo = L"Кошелек: " + reportData.wallet.name;
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)walletInfo.c_str());

    std::wstring countInfo = L"Количество транзакций: " + std::to_wstring(stats.transactionCount);
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)countInfo.c_str());

    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"");
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"════════════ СТАТИСТИКА ════════════");

    // Статистика
    std::wstring soldInfo = L"Всего продано: " + std::to_wstring(stats.totalSold);
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)soldInfo.c_str());

    std::wstring boughtInfo = L"Всего куплено: " + std::to_wstring(stats.totalBought);
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)boughtInfo.c_str());

    // Изменения по валютам
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"");
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"════════ ИЗМЕНЕНИЯ ПО ВАЛЮТАМ ════════");

    for (const auto& currencyChange : stats.currencyChanges) {
        if (currencyChange.second != 0) {
            std::wstring changeInfo = currencyChange.first + L": " +
                (currencyChange.second > 0 ? L"+" : L"") +
                std::to_wstring(currencyChange.second);
            SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)changeInfo.c_str());
        }
    }

    // Детали транзакций
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"");
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"═══════════ ТРАНЗАКЦИИ ═══════════");

    for (const auto& transaction : reportData.transactions) {
        std::wstring transactionInfo = transaction.date + L": " +
            std::to_wstring(transaction.soldAmount) + L" " +
            transaction.soldCurrency + L" → " +
            std::to_wstring(transaction.boughtAmount) + L" " +
            transaction.boughtCurrency +
            L" (курс: " + std::to_wstring(transaction.exchangeRate) + L")";
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)transactionInfo.c_str());
    }
}

// Функция обновления списка кошельков в окне отчетов
void UpdateReportsWalletsList(HWND hListBox) {
    if (!hListBox) return;

    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

    if (g_wallets.empty()) {
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"Нет кошельков");
        return;
    }

    for (size_t i = 0; i < g_wallets.size(); i++) {
        const auto& wallet = g_wallets[i];
        std::wstring displayText = wallet.name + L" (" +
            std::to_wstring(wallet.currencies.size()) +
            L" валют)";
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)displayText.c_str());
    }
}

// Функции для работы с отчетами
std::vector<Transaction> LoadTransactions(const Wallet& wallet) {
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
            currentTransaction.date = line.substr(5); // "DATE:".length()
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
            // Парсим состояние кошелька
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

    // Добавляем последнюю транзакцию если есть
    if (readingTransaction) {
        transactions.push_back(currentTransaction);
    }

    file.close();
    return transactions;
}

// Функция для фильтрации транзакций по дате
std::vector<Transaction> FilterTransactionsByDate(const std::vector<Transaction>& transactions,
    const std::wstring& startDate,
    const std::wstring& endDate) {
    std::vector<Transaction> filtered;

    for (const auto& transaction : transactions) {
        // Простая проверка - если дата транзакции между startDate и endDate
        if (transaction.date >= startDate && transaction.date <= endDate) {
            filtered.push_back(transaction);
        }
    }

    return filtered;
}


// Функция обновления содержимого кошелька в списке
void UpdateWalletContentList(HWND hListBox, const Wallet& wallet) {
    if (!hListBox) return;

    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

    if (wallet.currencies.empty()) {
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"Кошелек пуст");
        return;
    }

    for (const auto& currency : wallet.currencies) {
        std::wstring displayText = currency.currency + L": " +
            std::to_wstring(currency.amount);
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)displayText.c_str());
    }
}

// Функции для работы с транзакциями
void SaveTransaction(const Wallet& wallet, const Transaction& transaction) {
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

std::wstring GetCurrentDateTime() {
    time_t now = time(0);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);

    wchar_t buffer[80];
    wcsftime(buffer, sizeof(buffer), L"%Y-%m-%d %H:%M:%S", &timeinfo);
    return std::wstring(buffer);
}

// Функция для получения состояния кошелька (копии)
std::vector<WalletCurrency> GetWalletState(const Wallet& wallet) {
    return wallet.currencies;
}
// Функция обновления списка кошельков
void UpdateWalletsList(HWND hListBox) {
    if (!hListBox) return;

    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

    if (g_wallets.empty()) {
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"Нет кошельков");
        return;
    }

    for (size_t i = 0; i < g_wallets.size(); i++) {
        const auto& wallet = g_wallets[i];

        // Создаем информативную строку с количеством валют
        std::wstring displayText = wallet.name + L" (" +
            std::to_wstring(wallet.currencies.size()) +
            L" валют)";

        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)displayText.c_str());
    }
}
// Функция обновления списка валют в создаваемом кошельке
void UpdateCurrentCurrenciesList(HWND hListBox, const std::vector<WalletCurrency>& currencies) {
    if (!hListBox) return;

    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

    if (currencies.empty()) {
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"Валюты не добавлены");
        return;
    }

    for (const auto& currency : currencies) {
        std::wstring displayText = currency.currency + L": " + std::to_wstring(currency.amount);
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)displayText.c_str());
    }
}

// Функции для работы с кошельками
// Функции для работы с кошельками
void SaveWallets() {
    if (g_currentUser == L"Гость") return;

    std::wstring filename = GetUserCurrencyFile() + L"_wallets.txt";
    std::wofstream file(filename); // Используем wofstream для широких символов
    if (!file.is_open()) return;

    // Устанавливаем локаль для правильной работы с Unicode
    file.imbue(std::locale(""));

    for (const auto& wallet : g_wallets) {
        file << L"WALLET:" << wallet.name << std::endl;
        for (const auto& currency : wallet.currencies) {
            file << L"CURRENCY:" << currency.currency << L":" << currency.amount << std::endl;
        }
        file << L"ENDWALLET" << std::endl;
    }
    file.close();
}

void LoadWallets() {
    g_wallets.clear();

    if (g_currentUser == L"Гость") return;

    std::wstring filename = GetUserCurrencyFile() + L"_wallets.txt";
    std::wifstream file(filename); // Используем wifstream для широких символов
    if (!file.is_open()) {
        // Если файла нет - это нормально для нового пользователя
        return;
    }

    // Устанавливаем локаль для правильной работы с Unicode
    file.imbue(std::locale(""));

    std::wstring line;
    Wallet currentWallet;
    bool readingWallet = false;

    while (std::getline(file, line)) {
        if (line.find(L"WALLET:") == 0) {
            if (readingWallet && !currentWallet.name.empty()) {
                g_wallets.push_back(currentWallet);
            }
            currentWallet = Wallet();
            currentWallet.name = line.substr(7); // "WALLET:".length()
            readingWallet = true;
        }
        else if (line.find(L"CURRENCY:") == 0) {
            size_t firstColon = 9; // "CURRENCY:".length()
            size_t secondColon = line.find(L':', firstColon);
            if (secondColon != std::wstring::npos) {
                std::wstring currencyStr = line.substr(firstColon, secondColon - firstColon);
                std::wstring amountStr = line.substr(secondColon + 1);

                WalletCurrency wc;
                wc.currency = currencyStr;
                try {
                    wc.amount = std::stod(amountStr);
                    currentWallet.currencies.push_back(wc);
                }
                catch (...) {
                    // Игнорируем ошибки парсинга чисел
                }
            }
        }
        else if (line == L"ENDWALLET") {
            if (readingWallet && !currentWallet.name.empty()) {
                g_wallets.push_back(currentWallet);
            }
            readingWallet = false;
            currentWallet = Wallet();
        }
    }

    // Добавляем последний кошелек если он есть
    if (readingWallet && !currentWallet.name.empty()) {
        g_wallets.push_back(currentWallet);
    }

    file.close();

    // Отладочный вывод для проверки загрузки
    std::wstring debugInfo = L"Загружено кошельков: " + std::to_wstring(g_wallets.size());
    OutputDebugString(debugInfo.c_str());
}

void AddWallet(const Wallet& wallet) {
    g_wallets.push_back(wallet);
    SaveWallets();
}

void RemoveWallet(int index) {
    if (index >= 0 && index < g_wallets.size()) {
        g_wallets.erase(g_wallets.begin() + index);
        SaveWallets();
    }
}

std::wstring GenerateWalletName() {
    int walletNumber = g_wallets.size() + 1;
    return L"Кошелек" + std::to_wstring(walletNumber);
}

bool SaveUserToFile(const std::wstring& username, const std::wstring& password) {
    std::ofstream file("users.txt", std::ios::app);
    if (!file.is_open()) return false;

    std::string userStr(username.begin(), username.end());
    std::string passStr(password.begin(), password.end());
    file << userStr << ":" << passStr << std::endl;
    file.close();

    return true;
}


void SaveCurrencyPairs() {
    if (g_currentUser == L"Гость") return;

    std::wstring filename = GetUserCurrencyFile();
    std::ofstream file(filename);
    if (!file.is_open()) return;

    for (const auto& pair : g_currencyPairs) {
        std::string pairStr(pair.begin(), pair.end());
        file << pairStr << std::endl;
    }
    file.close();
}

// Проверка поддержки валюты
bool IsCurrencySupported(const std::wstring& currency) {
    std::wstring upperCurrency = currency;
    std::transform(upperCurrency.begin(), upperCurrency.end(), upperCurrency.begin(), ::towupper);

    return std::find(supportedCurrencies.begin(), supportedCurrencies.end(), upperCurrency) != supportedCurrencies.end();
}

// Функция для получения курса валюты с улучшенной обработкой ошибок
double GetCurrencyRate(const std::wstring& from, const std::wstring& to) {
    if (from == to) return 1.0;

    // Проверяем поддержку валют
    if (!IsCurrencySupported(from) || !IsCurrencySupported(to)) {
        return 0.0;
    }

    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    std::string response;

    // Конвертируем в верхний регистр для API
    std::wstring fromUpper = from;
    std::wstring toUpper = to;
    std::transform(fromUpper.begin(), fromUpper.end(), fromUpper.begin(), ::towupper);
    std::transform(toUpper.begin(), toUpper.end(), toUpper.begin(), ::towupper);

    hSession = WinHttpOpen(L"Currency App",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return 0.0;

    hConnect = WinHttpConnect(hSession, L"api.frankfurter.app",
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return 0.0;
    }

    std::wstring path = L"/latest?from=" + fromUpper;
    hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
        NULL, NULL, NULL, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 0.0;
    }

    // Устанавливаем таймаут
    DWORD timeout = 10000; // 10 секунд
    WinHttpSetTimeouts(hRequest, timeout, timeout, timeout, timeout);

    BOOL requestSent = WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0);
    if (!requestSent) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 0.0;
    }

    BOOL responseReceived = WinHttpReceiveResponse(hRequest, NULL);
    if (!responseReceived) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 0.0;
    }

    // Читаем ответ
    char buffer[4096];
    DWORD bytesRead = 0;

    while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        response += buffer;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    // Проверяем, что ответ не пустой
    if (response.empty()) {
        return 0.0;
    }

    // Парсим JSON для получения нужной валюты
    std::string toStr(toUpper.begin(), toUpper.end());
    std::string searchStr = "\"" + toStr + "\":";

    size_t pos = response.find(searchStr);
    if (pos != std::string::npos) {
        size_t valueStart = response.find(":", pos) + 1;
        size_t valueEnd = response.find_first_of(",}", valueStart);
        if (valueEnd == std::string::npos) {
            valueEnd = response.length();
        }

        std::string valueStr = response.substr(valueStart, valueEnd - valueStart);

        // Удаляем пробелы и кавычки
        valueStr.erase(std::remove(valueStr.begin(), valueStr.end(), ' '), valueStr.end());
        valueStr.erase(std::remove(valueStr.begin(), valueStr.end(), '\"'), valueStr.end());

        try {
            double rate = std::stod(valueStr);
            return (rate > 0.0) ? rate : 0.0;
        }
        catch (...) {
            return 0.0;
        }
    }

    return 0.0;
}

// Функция обновления курсов валют в ListBox
void UpdateCurrencyRates(HWND hListBox) {
    if (!hListBox) return;

    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

    for (const auto& pair : g_currencyPairs) {
        // Разделяем пару на две валюты
        size_t dashPos = pair.find(L'-');
        if (dashPos != std::wstring::npos) {
            std::wstring from = pair.substr(0, dashPos);
            std::wstring to = pair.substr(dashPos + 1);

            double rate = GetCurrencyRate(from, to);

            std::wstringstream ws;
            ws << std::fixed << std::setprecision(4);
            if (rate > 0.0) {
                ws << from << L" → " << to << L": " << rate;
            }
            else {
                ws << from << L" → " << to << L": Ошибка получения курса";
            }
            SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)ws.str().c_str());
        }
    }
}

// Функция обновления списка уведомлений
void UpdateNotificationsList(HWND hListBox) {
    if (!hListBox) return;

    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

    for (const auto& notification : g_notifications) {
        std::wstring displayText = notification.currency1 + L" " +
            std::to_wstring((int)notification.value1) +
            notification.condition +
            std::to_wstring((int)notification.value2) + L" " +
            notification.currency2 +
            (notification.active ? L" (активно)" : L" (неактивно)");

        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)displayText.c_str());
    }
}

bool CheckUserCredentials(const std::wstring& username, const std::wstring& password) {
    std::ifstream file("users.txt");
    if (!file.is_open()) return false;

    std::string line;
    std::string userStr(username.begin(), username.end());
    std::string passStr(password.begin(), password.end());
    std::string target = userStr + ":" + passStr;

    while (std::getline(file, line)) {
        if (line == target) {
            file.close();
            return true;
        }
    }

    file.close();
    return false;
}

bool UserExists(const std::wstring& username) {
    std::ifstream file("users.txt");
    if (!file.is_open()) return false;

    std::string line;
    std::string userStr(username.begin(), username.end());
    std::string target = userStr + ":";

    while (std::getline(file, line)) {
        if (line.find(target) == 0) {
            file.close();
            return true;
        }
    }

    file.close();
    return false;
}

void LoadCurrencyPairs() {
    g_currencyPairs.clear();

    if (g_currentUser == L"Гость") {
        // Для гостя - стандартный набор из поддерживаемых валют
        g_currencyPairs = { L"USD-EUR", L"USD-GBP", L"USD-JPY" };
        return;
    }

    std::wstring filename = GetUserCurrencyFile();
    std::ifstream file(filename);
    if (!file.is_open()) {
        // Если файла нет, создаем стандартный набор
        g_currencyPairs = { L"USD-EUR", L"USD-GBP", L"USD-JPY" };
        SaveCurrencyPairs();
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            g_currencyPairs.push_back(std::wstring(line.begin(), line.end()));
        }
    }
    file.close();
}

void AddCurrencyPair(const std::wstring& pair) {
    // Проверяем, нет ли уже такой пары
    if (std::find(g_currencyPairs.begin(), g_currencyPairs.end(), pair) == g_currencyPairs.end()) {
        g_currencyPairs.push_back(pair);
        SaveCurrencyPairs();
    }
}

void RemoveCurrencyPair(const std::wstring& pair) {
    auto it = std::find(g_currencyPairs.begin(), g_currencyPairs.end(), pair);
    if (it != g_currencyPairs.end()) {
        g_currencyPairs.erase(it);
        SaveCurrencyPairs();
    }
}


// Функции для работы с уведомлениями
void SaveNotifications() {
    if (g_currentUser == L"Гость") return;

    std::wstring filename = GetUserCurrencyFile() + L"_notifications.txt";
    std::ofstream file(filename);
    if (!file.is_open()) return;

    for (const auto& notification : g_notifications) {
        std::string curr1(notification.currency1.begin(), notification.currency1.end());
        std::string curr2(notification.currency2.begin(), notification.currency2.end());
        std::string cond(notification.condition.begin(), notification.condition.end());

        file << curr1 << "," << curr2 << "," << notification.value1
            << "," << notification.value2 << "," << cond
            << "," << notification.active << std::endl;
    }
    file.close();
}

void LoadNotifications() {
    g_notifications.clear();

    if (g_currentUser == L"Гость") return;

    std::wstring filename = GetUserCurrencyFile() + L"_notifications.txt";
    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            std::stringstream ss(line);
            std::string token;
            std::vector<std::string> tokens;

            while (std::getline(ss, token, ',')) {
                tokens.push_back(token);
            }

            if (tokens.size() >= 6) {
                Notification notification;
                notification.currency1 = std::wstring(tokens[0].begin(), tokens[0].end());
                notification.currency2 = std::wstring(tokens[1].begin(), tokens[1].end());
                notification.value1 = std::stod(tokens[2]);
                notification.value2 = std::stod(tokens[3]);
                notification.condition = std::wstring(tokens[4].begin(), tokens[4].end());
                notification.active = (tokens[5] == "1");

                g_notifications.push_back(notification);
            }
        }
    }
    file.close();
}

void AddNotification(const Notification& notification) {
    g_notifications.push_back(notification);
    SaveNotifications();
}

// Функция проверки условия уведомления
bool CheckNotificationCondition(const Notification& notification, double rate1, double rate2) {
    // Рассчитываем фактическое соотношение
    double actualRatio = (notification.value1 * rate1) / (notification.value2 * rate2);
    double targetRatio = 1.0; // 1:1 соотношение

    if (notification.condition == L">") {
        return actualRatio > targetRatio;
    }
    else if (notification.condition == L"<") {
        return actualRatio < targetRatio;
    }
    else if (notification.condition == L"=") {
        return std::abs(actualRatio - targetRatio) < 0.01; // Погрешность 1%
    }

    return false;
}

// Поток для проверки уведомлений
void NotificationCheckerThread() {
    while (g_notificationThreadRunning) {
        // Ждем 5 минут
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // Проверяем активные уведомления
        for (const auto& notification : g_notifications) {
            if (notification.active) {
                // Получаем текущие курсы
                double rate1 = GetCurrencyRate(L"USD", notification.currency1);
                double rate2 = GetCurrencyRate(L"USD", notification.currency2);

                if (rate1 > 0.0 && rate2 > 0.0) {
                    if (CheckNotificationCondition(notification, rate1, rate2)) {
                        // Показываем уведомление
                        std::wstring message = L"Уведомление: " + notification.currency1 +
                            L" и " + notification.currency2 +
                            L" достигли заданного соотношения!";

                        // Можно использовать MessageBox или более красивый способ
                        MessageBox(NULL, message.c_str(), L"Валютное уведомление", MB_OK | MB_ICONINFORMATION);
                    }
                }
            }
        }
    }
}

void StartNotificationChecker() {
    if (!g_notificationThreadRunning) {
        g_notificationThreadRunning = true;
        std::thread(NotificationCheckerThread).detach();
    }
}

void StopNotificationChecker() {
    g_notificationThreadRunning = false;
}

bool ParseNotificationCondition(const std::wstring& conditionStr,
    double& value1, double& value2, std::wstring& condition) {
    // Инициализируем выходные параметры
    value1 = 0.0;
    value2 = 0.0;
    condition = L"";

    // Ищем оператор сравнения
    size_t opPos = conditionStr.find(L'>');
    if (opPos != std::wstring::npos) {
        condition = L">";
    }
    else {
        opPos = conditionStr.find(L'<');
        if (opPos != std::wstring::npos) {
            condition = L"<";
        }
        else {
            opPos = conditionStr.find(L'=');
            if (opPos != std::wstring::npos) {
                condition = L"=";
            }
            else {
                return false; // Не найден оператор
            }
        }
    }

    // Парсим значения
    try {
        std::wstring val1Str = conditionStr.substr(0, opPos);
        std::wstring val2Str = conditionStr.substr(opPos + 1);

        value1 = std::stod(val1Str);
        value2 = std::stod(val2Str);

        return true;
    }
    catch (...) {
        return false;
    }
}

// Оконная процедура окна НАСТРОЙКИ УВЕДОМЛЕНИЙ
LRESULT CALLBACK NotificationsWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hCurrency1Edit = NULL;
    static HWND hCurrency2Edit = NULL;
    static HWND hConditionEdit = NULL;
    static HWND hStatusLabel = NULL;
    static HWND hNotificationsList = NULL;

    switch (uMsg) {
        // Обновим WM_CREATE в окне уведомлений
    case WM_CREATE:
    {
        CreateWindow(L"STATIC", L"Настройка уведомлений:",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 20, 300, 30, hWnd, NULL, NULL, NULL);

        // Поле для валюты 1
        CreateWindow(L"STATIC", L"Валюта 1:",
            WS_VISIBLE | WS_CHILD,
            50, 60, 80, 20, hWnd, NULL, NULL, NULL);

        hCurrency1Edit = CreateWindow(L"EDIT", L"USD",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_UPPERCASE,
            140, 60, 100, 20, hWnd, NULL, NULL, NULL);

        // Поле для валюты 2
        CreateWindow(L"STATIC", L"Валюта 2:",
            WS_VISIBLE | WS_CHILD,
            50, 90, 80, 20, hWnd, NULL, NULL, NULL);

        hCurrency2Edit = CreateWindow(L"EDIT", L"EUR",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_UPPERCASE,
            140, 90, 100, 20, hWnd, NULL, NULL, NULL);

        // Поле для условия
        CreateWindow(L"STATIC", L"Условие (напр. 5>2):",
            WS_VISIBLE | WS_CHILD,
            50, 120, 120, 20, hWnd, NULL, NULL, NULL);

        hConditionEdit = CreateWindow(L"EDIT", L"1>1",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            180, 120, 100, 20, hWnd, NULL, NULL, NULL);

        // Кнопки
        CreateWindow(L"BUTTON", L"Назад",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            50, 150, 80, 30, hWnd,
            (HMENU)400, NULL, NULL);

        CreateWindow(L"BUTTON", L"Создать",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            140, 150, 80, 30, hWnd,
            (HMENU)401, NULL, NULL);

        CreateWindow(L"BUTTON", L"Удалить",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            230, 150, 120, 30, hWnd,
            (HMENU)402, NULL, NULL);

        hStatusLabel = CreateWindow(L"STATIC", L"Введите условие в формате: 5>2",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 230, 300, 20, hWnd, NULL, NULL, NULL);

        // Список активных уведомлений
        CreateWindow(L"STATIC", L"Активные уведомления:",
            WS_VISIBLE | WS_CHILD,
            50, 260, 200, 20, hWnd, NULL, NULL, NULL);

        hNotificationsList = CreateWindow(L"LISTBOX", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
            50, 285, 300, 150, hWnd, (HMENU)403, NULL, NULL);

        // ЗАГРУЖАЕМ УВЕДОМЛЕНИЯ СРАЗУ ПРИ ОТКРЫТИИ ОКНА
        LoadNotifications();
        UpdateNotificationsList(hNotificationsList);

        HFONT hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, L"Microsoft Sans Serif");
        if (hFont) {
            HWND hChild = GetWindow(hWnd, GW_CHILD);
            while (hChild) {
                SendMessage(hChild, WM_SETFONT, (WPARAM)hFont, TRUE);
                hChild = GetNextWindow(hChild, GW_HWNDNEXT);
            }
        }

        // Запускаем проверку уведомлений если есть активные
        bool hasActiveNotifications = false;
        for (const auto& notification : g_notifications) {
            if (notification.active) {
                hasActiveNotifications = true;
                break;
            }
        }

        if (hasActiveNotifications) {
            StartNotificationChecker();
        }
    }
    break;

    case WM_COMMAND:
    {
        int buttonId = LOWORD(wParam);

        wchar_t currency1[10], currency2[10], condition[20];
        GetWindowText(hCurrency1Edit, currency1, 10);
        GetWindowText(hCurrency2Edit, currency2, 10);
        GetWindowText(hConditionEdit, condition, 20);

        std::wstring currency1Str = currency1;
        std::wstring currency2Str = currency2;
        std::wstring conditionStr = condition;

        switch (buttonId) {
        case 400: // Назад
            DestroyWindow(hWnd);
            break;

        case 401: // Создать
        {
            if (currency1Str.empty() || currency2Str.empty() || conditionStr.empty()) {
                SetWindowText(hStatusLabel, L"Заполните все поля!");
                break;
            }

            // Инициализируем переменные ДО парсинга
            double value1 = 0.0;
            double value2 = 0.0;
            std::wstring conditionOp = L"";

            // Парсим условие
            if (!ParseNotificationCondition(conditionStr, value1, value2, conditionOp)) {
                SetWindowText(hStatusLabel, L"Ошибка в формате условия! Используйте: 5>2");
                break;
            }

            // Проверяем поддержку валют
            if (!IsCurrencySupported(currency1Str)) {
                SetWindowText(hStatusLabel, L"Валюта 1 не поддерживается!");
                break;
            }
            if (!IsCurrencySupported(currency2Str)) {
                SetWindowText(hStatusLabel, L"Валюта 2 не поддерживается!");
                break;
            }

            // Создаем и инициализируем уведомление
            Notification newNotification;
            newNotification.currency1 = currency1Str;
            newNotification.currency2 = currency2Str;
            newNotification.value1 = value1;
            newNotification.value2 = value2;
            newNotification.condition = conditionOp;
            newNotification.active = true;

            AddNotification(newNotification);
            SetWindowText(hStatusLabel, L"Уведомление создано!");

            // Обновляем список
            UpdateNotificationsList(hNotificationsList);

            // Запускаем проверку уведомлений
            StartNotificationChecker();
        }
        break;

        case 402: // Удалить
        {
            int index = SendMessage(hNotificationsList, LB_GETCURSEL, 0, 0);
            if (index != LB_ERR) {
                g_notifications.erase(g_notifications.begin() + index);
                SaveNotifications();
                UpdateNotificationsList(hNotificationsList);
                SetWindowText(hStatusLabel, L"Уведомление удалено!");
            }
            else {
                SetWindowText(hStatusLabel, L"Выберите уведомление для удаления!");
            }
        }
        break;

        case 403: // Выбор уведомления из списка
            if (HIWORD(wParam) == LBN_SELCHANGE) {
                int index = SendMessage(hNotificationsList, LB_GETCURSEL, 0, 0);
                if (index != LB_ERR && index < g_notifications.size()) {
                    const auto& notification = g_notifications[index];
                    SetWindowText(hCurrency1Edit, notification.currency1.c_str());
                    SetWindowText(hCurrency2Edit, notification.currency2.c_str());

                    std::wstring conditionDisplay = std::to_wstring((int)notification.value1) +
                        notification.condition +
                        std::to_wstring((int)notification.value2);
                    SetWindowText(hConditionEdit, conditionDisplay.c_str());
                }
            }
            break;
        }
    }
    break;

    case WM_DESTROY:
        // Не останавливаем поток, чтобы уведомления работали даже когда окно закрыто
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Прототипы функций
LRESULT CALLBACK LoginWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MainMenuWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK CurrencyPairsWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL RegisterWindowClass(LPCWSTR className, WNDPROC windowProc);


// Функция проверки существования валютной пары через API
bool CheckCurrencyPairExists(const std::wstring& from, const std::wstring& to) {
    // Сначала проверяем поддержку валют
    if (!IsCurrencySupported(from)) {
        return false;
    }
    if (!IsCurrencySupported(to)) {
        return false;
    }

    double rate = GetCurrencyRate(from, to);
    return rate > 0.0;
}

// Функция регистрации класса окна
BOOL RegisterWindowClass(LPCWSTR className, WNDPROC windowProc) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = windowProc;
    wc.hInstance = g_hInstance;
    wc.lpszClassName = className;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    return RegisterClass(&wc);
}

// Оконная процедура окна ОТЧЕТОВ
LRESULT CALLBACK ReportsWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hWalletsList = NULL;
    static HWND hStartDateEdit = NULL;
    static HWND hEndDateEdit = NULL;
    static HWND hStatusLabel = NULL;

    switch (uMsg) {
    case WM_CREATE:
    {
        CreateWindow(L"STATIC", L"Создание отчета:",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 20, 300, 30, hWnd, NULL, NULL, NULL);

        // Список кошельков
        CreateWindow(L"STATIC", L"Выберите кошелек:",
            WS_VISIBLE | WS_CHILD,
            50, 60, 150, 20, hWnd, NULL, NULL, NULL);

        hWalletsList = CreateWindow(L"LISTBOX", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
            50, 85, 300, 120, hWnd, (HMENU)700, NULL, NULL);

        // Поля для дат
        CreateWindow(L"STATIC", L"Период отчета:",
            WS_VISIBLE | WS_CHILD,
            50, 220, 150, 20, hWnd, NULL, NULL, NULL);

        CreateWindow(L"STATIC", L"С даты (гггг-мм-дд):",
            WS_VISIBLE | WS_CHILD,
            50, 250, 150, 20, hWnd, NULL, NULL, NULL);

        hStartDateEdit = CreateWindow(L"EDIT", L"2024-01-01",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            210, 250, 120, 20, hWnd, NULL, NULL, NULL);

        CreateWindow(L"STATIC", L"По дату (гггг-мм-дд):",
            WS_VISIBLE | WS_CHILD,
            50, 280, 150, 20, hWnd, NULL, NULL, NULL);

        hEndDateEdit = CreateWindow(L"EDIT", L"2024-12-31",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            210, 280, 120, 20, hWnd, NULL, NULL, NULL);

        // Кнопки
        CreateWindow(L"BUTTON", L"Назад",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            50, 320, 100, 30, hWnd,
            (HMENU)701, NULL, NULL);

        CreateWindow(L"BUTTON", L"Создать отчет",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            160, 320, 120, 30, hWnd,
            (HMENU)702, NULL, NULL);

        hStatusLabel = CreateWindow(L"STATIC", L"Выберите кошелек и период для отчета",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 370, 300, 20, hWnd, NULL, NULL, NULL);

        // Заполняем список кошельков
        UpdateReportsWalletsList(hWalletsList);

        HFONT hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, L"Microsoft Sans Serif");
        if (hFont) {
            HWND hChild = GetWindow(hWnd, GW_CHILD);
            while (hChild) {
                SendMessage(hChild, WM_SETFONT, (WPARAM)hFont, TRUE);
                hChild = GetNextWindow(hChild, GW_HWNDNEXT);
            }
        }
    }
    break;

    case WM_COMMAND:
    {
        int buttonId = LOWORD(wParam);

        switch (buttonId) {
        case 701: // Назад
            DestroyWindow(hWnd);
            break;

        case 702: // Создать отчет
        {
            int walletIndex = SendMessage(hWalletsList, LB_GETCURSEL, 0, 0);
            if (walletIndex == LB_ERR || walletIndex >= g_wallets.size()) {
                SetWindowText(hStatusLabel, L"Выберите кошелек!");
                break;
            }

            wchar_t startDate[20], endDate[20];
            GetWindowText(hStartDateEdit, startDate, 20);
            GetWindowText(hEndDateEdit, endDate, 20);

            std::wstring startDateStr = startDate;
            std::wstring endDateStr = endDate;

            if (startDateStr.empty() || endDateStr.empty()) {
                SetWindowText(hStatusLabel, L"Заполните обе даты!");
                break;
            }

            // Проверяем формат дат (простая проверка)
            if (startDateStr.length() != 10 || endDateStr.length() != 10 ||
                startDateStr[4] != '-' || startDateStr[7] != '-' ||
                endDateStr[4] != '-' || endDateStr[7] != '-') {
                SetWindowText(hStatusLabel, L"Неверный формат даты! Используйте гггг-мм-дд");
                break;
            }

            const Wallet& wallet = g_wallets[walletIndex];

            // Загружаем транзакции
            std::vector<Transaction> allTransactions = LoadTransactions(wallet);
            if (allTransactions.empty()) {
                SetWindowText(hStatusLabel, L"Нет транзакций для выбранного кошелька!");
                break;
            }

            // Фильтруем по дате
            std::vector<Transaction> filteredTransactions =
                FilterTransactionsByDate(allTransactions, startDateStr, endDateStr);

            if (filteredTransactions.empty()) {
                SetWindowText(hStatusLabel, L"Нет транзакций за выбранный период!");
                break;
            }

            // Создаем окно отчета
            CreateReportWindow(hWnd, wallet, filteredTransactions, startDateStr, endDateStr);
            SetWindowText(hStatusLabel, L"Отчет создан!");
        }
        break;
        }
    }
    break;

    case WM_DESTROY:
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Оконная процедура окна ОТЧЕТА (результат)
LRESULT CALLBACK ReportViewWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hReportContentList = NULL;

    switch (uMsg) {
    case WM_CREATE:
    {
        // Получаем данные отчета из параметра создания
        CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
        ReportData* reportData = (ReportData*)createStruct->lpCreateParams;

        if (!reportData) {
            DestroyWindow(hWnd);
            break;
        }

        // Заголовок отчета
        std::wstring title = L"Отчет по кошельку: " + reportData->wallet.name +
            L" за период " + reportData->startDate + L" - " + reportData->endDate;

        CreateWindow(L"STATIC", title.c_str(),
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 20, 500, 30, hWnd, NULL, NULL, NULL);

        // Содержимое отчета
        hReportContentList = CreateWindow(L"LISTBOX", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL,
            50, 60, 500, 400, hWnd, NULL, NULL, NULL);

        // Кнопка закрытия
        CreateWindow(L"BUTTON", L"Закрыть",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            250, 470, 100, 30, hWnd,
            (HMENU)800, NULL, NULL);

        // Заполняем отчет
        FillReportContent(hReportContentList, *reportData);

        HFONT hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, L"Microsoft Sans Serif");
        if (hFont) {
            HWND hChild = GetWindow(hWnd, GW_CHILD);
            while (hChild) {
                SendMessage(hChild, WM_SETFONT, (WPARAM)hFont, TRUE);
                hChild = GetNextWindow(hChild, GW_HWNDNEXT);
            }
        }

        // Очищаем данные отчета после использования
        delete reportData;
    }
    break;

    case WM_COMMAND:
    {
        int buttonId = LOWORD(wParam);
        if (buttonId == 800) { // Закрыть
            DestroyWindow(hWnd);
        }
    }
    break;

    case WM_DESTROY:
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Оконная процедура окна УПРАВЛЕНИЯ ВАЛЮТНЫМИ ПАРАМИ (исправленная)
LRESULT CALLBACK CurrencyPairsWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hFromEdit = NULL;
    static HWND hToEdit = NULL;
    static HWND hStatusLabel = NULL;
    static HWND hCurrentPairsList = NULL;

    switch (uMsg) {
    case WM_CREATE:
    {
        CreateWindow(L"STATIC", L"Введите валютную пару:",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 20, 300, 30, hWnd, NULL, NULL, NULL);

        CreateWindow(L"STATIC", L"Из валюты:",
            WS_VISIBLE | WS_CHILD,
            50, 60, 80, 20, hWnd, NULL, NULL, NULL);

        hFromEdit = CreateWindow(L"EDIT", L"USD",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_UPPERCASE,
            140, 60, 100, 20, hWnd, NULL, NULL, NULL);

        CreateWindow(L"STATIC", L"В валюту:",
            WS_VISIBLE | WS_CHILD,
            50, 90, 80, 20, hWnd, NULL, NULL, NULL);

        hToEdit = CreateWindow(L"EDIT", L"EUR",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_UPPERCASE,
            140, 90, 100, 20, hWnd, NULL, NULL, NULL);

        CreateWindow(L"BUTTON", L"Назад",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            50, 130, 80, 30, hWnd,
            (HMENU)300, NULL, NULL);

        CreateWindow(L"BUTTON", L"Удалить",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            140, 130, 80, 30, hWnd,
            (HMENU)301, NULL, NULL);

        CreateWindow(L"BUTTON", L"Добавить",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            230, 130, 80, 30, hWnd,
            (HMENU)302, NULL, NULL);

        hStatusLabel = CreateWindow(L"STATIC", L"Введите пару в формате USD-EUR",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 170, 300, 20, hWnd, NULL, NULL, NULL);

        // Список текущих пар
        CreateWindow(L"STATIC", L"Текущие валютные пары:",
            WS_VISIBLE | WS_CHILD,
            50, 200, 200, 20, hWnd, NULL, NULL, NULL);

        hCurrentPairsList = CreateWindow(L"LISTBOX", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
            50, 225, 300, 150, hWnd, (HMENU)303, NULL, NULL); // Добавляем ID для списка

        // Заполняем список текущих пар
        for (const auto& pair : g_currencyPairs) {
            SendMessage(hCurrentPairsList, LB_ADDSTRING, 0, (LPARAM)pair.c_str());
        }

        HFONT hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, L"Microsoft Sans Serif");
        if (hFont) {
            HWND hChild = GetWindow(hWnd, GW_CHILD);
            while (hChild) {
                SendMessage(hChild, WM_SETFONT, (WPARAM)hFont, TRUE);
                hChild = GetNextWindow(hChild, GW_HWNDNEXT);
            }
        }
    }
    break;

    case WM_COMMAND:
    {
        int buttonId = LOWORD(wParam);

        wchar_t from[10];
        wchar_t to[10];
        GetWindowText(hFromEdit, from, 10);
        GetWindowText(hToEdit, to, 10);

        std::wstring fromStr = from;
        std::wstring toStr = to;
        std::wstring pair = fromStr + L"-" + toStr;

        switch (buttonId) {
        case 300: // Назад
            DestroyWindow(hWnd);
            break;

        case 301: // Удалить
            if (fromStr.empty() || toStr.empty()) {
                SetWindowText(hStatusLabel, L"Заполните оба поля!");
                break;
            }

            RemoveCurrencyPair(pair);
            SetWindowText(hStatusLabel, L"Пара удалена!");

            // Обновляем список текущих пар
            SendMessage(hCurrentPairsList, LB_RESETCONTENT, 0, 0);
            for (const auto& p : g_currencyPairs) {
                SendMessage(hCurrentPairsList, LB_ADDSTRING, 0, (LPARAM)p.c_str());
            }

            // Обновляем список в главном окне
            hMain = FindWindow(L"MainMenuClass", NULL);
            if (hMain) {
                HWND hList = GetDlgItem(hMain, 0);
                if (hList) {
                    UpdateCurrencyRates(hList);
                }
            }
            break;

        case 302: // Добавить
            if (fromStr.empty() || toStr.empty()) {
                SetWindowText(hStatusLabel, L"Заполните оба поля!");
                break;
            }

            // Проверяем поддержку валют
            if (!IsCurrencySupported(fromStr)) {
                SetWindowText(hStatusLabel, L"Ошибка: валюта 'from' не поддерживается!");
                break;
            }
            if (!IsCurrencySupported(toStr)) {
                SetWindowText(hStatusLabel, L"Ошибка: валюта 'to' не поддерживается!");
                break;
            }

            if (CheckCurrencyPairExists(fromStr, toStr)) {
                AddCurrencyPair(pair);
                SetWindowText(hStatusLabel, L"Пара добавлена!");

                // Обновляем список текущих пар
                SendMessage(hCurrentPairsList, LB_RESETCONTENT, 0, 0);
                for (const auto& p : g_currencyPairs) {
                    SendMessage(hCurrentPairsList, LB_ADDSTRING, 0, (LPARAM)p.c_str());
                }

                // Обновляем список в главном окне
                hMain = FindWindow(L"MainMenuClass", NULL);
                if (hMain) {
                    HWND hList = GetDlgItem(hMain, 0);
                    if (hList) {
                        UpdateCurrencyRates(hList);
                    }
                }
            }
            else {
                SetWindowText(hStatusLabel, L"Ошибка: неверная валютная пара или проблема с API!");
            }
            break;

        case 303: // Выбор из списка
            if (HIWORD(wParam) == LBN_SELCHANGE) {
                int index = SendMessage(hCurrentPairsList, LB_GETCURSEL, 0, 0);
                if (index != LB_ERR) {
                    wchar_t selectedPair[50];
                    SendMessage(hCurrentPairsList, LB_GETTEXT, index, (LPARAM)selectedPair);

                    std::wstring pairStr = selectedPair;
                    size_t dashPos = pairStr.find(L'-');
                    if (dashPos != std::wstring::npos) {
                        std::wstring from = pairStr.substr(0, dashPos);
                        std::wstring to = pairStr.substr(dashPos + 1);

                        SetWindowText(hFromEdit, from.c_str());
                        SetWindowText(hToEdit, to.c_str());
                    }
                }
            }
            break;
        }
    }
    break;

    case WM_DESTROY:
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}
// Оконная процедура окна КОШЕЛЬКА (детальный вид)
LRESULT CALLBACK WalletDetailWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hWalletContentList = NULL;
    static HWND hSellCurrencyEdit = NULL;
    static HWND hBuyCurrencyEdit = NULL;
    static HWND hAmountEdit = NULL;
    static HWND hResultLabel = NULL;
    static HWND hStatusLabel = NULL;
    static HWND hExchangeRateLabel = NULL;

    switch (uMsg) {
    case WM_CREATE:
    {
        // Получаем индекс кошелька из параметра создания
        CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
        g_currentWalletIndex = (int)createStruct->lpCreateParams;

        if (g_currentWalletIndex < 0 || g_currentWalletIndex >= g_wallets.size()) {
            MessageBox(hWnd, L"Ошибка: кошелек не найден", L"Ошибка", MB_OK);
            DestroyWindow(hWnd);
            break;
        }

        Wallet& wallet = g_wallets[g_currentWalletIndex];

        // Заголовок с именем кошелька
        CreateWindow(L"STATIC", (L"Кошелек: " + wallet.name).c_str(),
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 20, 300, 30, hWnd, NULL, NULL, NULL);

        // Содержимое кошелька
        CreateWindow(L"STATIC", L"Содержимое кошелька:",
            WS_VISIBLE | WS_CHILD,
            50, 60, 200, 20, hWnd, NULL, NULL, NULL);

        hWalletContentList = CreateWindow(L"LISTBOX", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
            50, 85, 300, 150, hWnd, NULL, NULL, NULL);

        // Поля для обмена валют
        CreateWindow(L"STATIC", L"Обмен валют:",
            WS_VISIBLE | WS_CHILD,
            50, 250, 200, 20, hWnd, NULL, NULL, NULL);

        CreateWindow(L"STATIC", L"Продать валюту:",
            WS_VISIBLE | WS_CHILD,
            50, 280, 100, 20, hWnd, NULL, NULL, NULL);

        hSellCurrencyEdit = CreateWindow(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_UPPERCASE,
            160, 280, 80, 20, hWnd, NULL, NULL, NULL);

        CreateWindow(L"STATIC", L"Купить валюту:",
            WS_VISIBLE | WS_CHILD,
            50, 310, 100, 20, hWnd, NULL, NULL, NULL);

        hBuyCurrencyEdit = CreateWindow(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_UPPERCASE,
            160, 310, 80, 20, hWnd, NULL, NULL, NULL);

        CreateWindow(L"STATIC", L"Количество для продажи:",
            WS_VISIBLE | WS_CHILD,
            50, 340, 150, 20, hWnd, NULL, NULL, NULL);

        hAmountEdit = CreateWindow(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            210, 340, 80, 20, hWnd, NULL, NULL, NULL);

        // Поле для отображения курса и результата
        hExchangeRateLabel = CreateWindow(L"STATIC", L"Курс: -",
            WS_VISIBLE | WS_CHILD,
            50, 370, 300, 20, hWnd, NULL, NULL, NULL);

        hResultLabel = CreateWindow(L"STATIC", L"Вы получите: -",
            WS_VISIBLE | WS_CHILD,
            50, 390, 300, 20, hWnd, NULL, NULL, NULL);

        // Кнопки
        CreateWindow(L"BUTTON", L"Назад",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            50, 420, 100, 30, hWnd,
            (HMENU)600, NULL, NULL);

        CreateWindow(L"BUTTON", L"Рассчитать",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            160, 420, 100, 30, hWnd,
            (HMENU)601, NULL, NULL);

        CreateWindow(L"BUTTON", L"Купить",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            270, 420, 100, 30, hWnd,
            (HMENU)602, NULL, NULL);

        hStatusLabel = CreateWindow(L"STATIC", L"Введите данные для обмена",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 460, 300, 20, hWnd, NULL, NULL, NULL);

        // Обновляем содержимое кошелька
        UpdateWalletContentList(hWalletContentList, wallet);

        HFONT hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, L"Microsoft Sans Serif");
        if (hFont) {
            HWND hChild = GetWindow(hWnd, GW_CHILD);
            while (hChild) {
                SendMessage(hChild, WM_SETFONT, (WPARAM)hFont, TRUE);
                hChild = GetNextWindow(hChild, GW_HWNDNEXT);
            }
        }
    }
    break;

    case WM_COMMAND:
    {
        int buttonId = LOWORD(wParam);

        if (g_currentWalletIndex < 0 || g_currentWalletIndex >= g_wallets.size()) {
            SetWindowText(hStatusLabel, L"Ошибка: кошелек не найден");
            break;
        }

        Wallet& wallet = g_wallets[g_currentWalletIndex];

        switch (buttonId) {
        case 600: // Назад
            DestroyWindow(hWnd);
            break;

        case 601: // Рассчитать
        {
            wchar_t sellCurrency[10], buyCurrency[10], amount[20];
            GetWindowText(hSellCurrencyEdit, sellCurrency, 10);
            GetWindowText(hBuyCurrencyEdit, buyCurrency, 10);
            GetWindowText(hAmountEdit, amount, 20);

            std::wstring sellStr = sellCurrency;
            std::wstring buyStr = buyCurrency;
            std::wstring amountStr = amount;

            if (sellStr.empty() || buyStr.empty() || amountStr.empty()) {
                SetWindowText(hStatusLabel, L"Заполните все поля!");
                break;
            }

            // Проверяем поддержку валют
            if (!IsCurrencySupported(sellStr) || !IsCurrencySupported(buyStr)) {
                SetWindowText(hStatusLabel, L"Одна из валют не поддерживается!");
                break;
            }

            // Проверяем, есть ли такая валюта в кошельке
            bool hasCurrency = false;
            for (const auto& currency : wallet.currencies) {
                if (currency.currency == sellStr) {
                    hasCurrency = true;
                    break;
                }
            }

            if (!hasCurrency) {
                SetWindowText(hStatusLabel, L"У вас нет такой валюты в кошельке!");
                break;
            }

            try {
                double amountValue = std::stod(amountStr);
                if (amountValue <= 0) {
                    SetWindowText(hStatusLabel, L"Количество должно быть больше 0!");
                    break;
                }

                // Получаем курс обмена
                double exchangeRate = GetCurrencyRate(sellStr, buyStr);
                if (exchangeRate <= 0.0) {
                    SetWindowText(hStatusLabel, L"Ошибка получения курса!");
                    break;
                }

                // Рассчитываем результат
                double result = amountValue * exchangeRate;

                // Обновляем поля отображения
                std::wstring rateText = L"Курс: 1 " + sellStr + L" = " +
                    std::to_wstring(exchangeRate) + L" " + buyStr;
                SetWindowText(hExchangeRateLabel, rateText.c_str());

                std::wstring resultText = L"Вы получите: " + std::to_wstring(result) + L" " + buyStr;
                SetWindowText(hResultLabel, resultText.c_str());

                SetWindowText(hStatusLabel, L"Расчет завершен!");

            }
            catch (...) {
                SetWindowText(hStatusLabel, L"Ошибка в количестве!");
            }
        }
        break;

        case 602: // Купить (совершить транзакцию)
        {
            wchar_t sellCurrency[10], buyCurrency[10], amount[20];
            GetWindowText(hSellCurrencyEdit, sellCurrency, 10);
            GetWindowText(hBuyCurrencyEdit, buyCurrency, 10);
            GetWindowText(hAmountEdit, amount, 20);

            std::wstring sellStr = sellCurrency;
            std::wstring buyStr = buyCurrency;
            std::wstring amountStr = amount;

            if (sellStr.empty() || buyStr.empty() || amountStr.empty()) {
                SetWindowText(hStatusLabel, L"Заполните все поля!");
                break;
            }

            try {
                double amountValue = std::stod(amountStr);
                if (amountValue <= 0) {
                    SetWindowText(hStatusLabel, L"Количество должно быть больше 0!");
                    break;
                }

                // Проверяем, достаточно ли валюты для продажи
                double availableAmount = 0.0;
                int sellCurrencyIndex = -1;

                for (size_t i = 0; i < wallet.currencies.size(); i++) {
                    if (wallet.currencies[i].currency == sellStr) {
                        availableAmount = wallet.currencies[i].amount;
                        sellCurrencyIndex = i;
                        break;
                    }
                }

                if (sellCurrencyIndex == -1) {
                    SetWindowText(hStatusLabel, L"У вас нет такой валюты в кошельке!");
                    break;
                }

                if (availableAmount < amountValue) {
                    SetWindowText(hStatusLabel, L"Недостаточно валюты для продажи!");
                    break;
                }

                // Получаем курс обмена
                double exchangeRate = GetCurrencyRate(sellStr, buyStr);
                if (exchangeRate <= 0.0) {
                    SetWindowText(hStatusLabel, L"Ошибка получения курса!");
                    break;
                }

                // Рассчитываем количество покупаемой валюты
                double boughtAmount = amountValue * exchangeRate;

                // Сохраняем состояние кошелька до транзакции для истории
                std::vector<WalletCurrency> walletStateBefore = GetWalletState(wallet);

                // Выполняем транзакцию
                // Уменьшаем количество продаваемой валюты
                wallet.currencies[sellCurrencyIndex].amount -= amountValue;

                // Если количество стало 0, удаляем валюту из кошелька
                if (wallet.currencies[sellCurrencyIndex].amount <= 0) {
                    wallet.currencies.erase(wallet.currencies.begin() + sellCurrencyIndex);
                }

                // Увеличиваем количество покупаемой валюты
                bool currencyExists = false;
                for (auto& currency : wallet.currencies) {
                    if (currency.currency == buyStr) {
                        currency.amount += boughtAmount;
                        currencyExists = true;
                        break;
                    }
                }

                // Если валюты не было, добавляем новую
                if (!currencyExists) {
                    WalletCurrency newCurrency;
                    newCurrency.currency = buyStr;
                    newCurrency.amount = boughtAmount;
                    wallet.currencies.push_back(newCurrency);
                }

                // Сохраняем кошелек
                SaveWallets();

                // Создаем запись о транзакции
                Transaction transaction;
                transaction.date = GetCurrentDateTime();
                transaction.soldCurrency = sellStr;
                transaction.soldAmount = amountValue;
                transaction.boughtCurrency = buyStr;
                transaction.boughtAmount = boughtAmount;
                transaction.exchangeRate = exchangeRate;
                transaction.walletState = GetWalletState(wallet); // Состояние после транзакции

                SaveTransaction(wallet, transaction);

                // Обновляем интерфейс
                UpdateWalletContentList(hWalletContentList, wallet);

                std::wstring successMsg = L"Транзакция завершена! Куплено: " +
                    std::to_wstring(boughtAmount) + L" " + buyStr;
                SetWindowText(hStatusLabel, successMsg.c_str());

                // Очищаем поля ввода
                SetWindowText(hSellCurrencyEdit, L"");
                SetWindowText(hBuyCurrencyEdit, L"");
                SetWindowText(hAmountEdit, L"");
                SetWindowText(hExchangeRateLabel, L"Курс: -");
                SetWindowText(hResultLabel, L"Вы получите: -");

            }
            catch (...) {
                SetWindowText(hStatusLabel, L"Ошибка при выполнении транзакции!");
            }
        }
        break;
        }
    }
    break;

    case WM_DESTROY:
        g_currentWalletIndex = -1; // Сбрасываем индекс при закрытии окна
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}
// Оконная процедура окна КОШЕЛЬКОВ
LRESULT CALLBACK WalletsWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hWalletsList = NULL;
    static HWND hCurrencyEdit = NULL;
    static HWND hAmountEdit = NULL;
    static HWND hStatusLabel = NULL;
    static HWND hCurrentCurrenciesList = NULL;

    static std::vector<WalletCurrency> tempCurrencies; // Временное хранилище для создаваемого кошелька

    switch (uMsg) {
    case WM_CREATE:
    {
        CreateWindow(L"STATIC", L"Список Кошельков:",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 20, 300, 30, hWnd, NULL, NULL, NULL);

        // Список существующих кошельков
        CreateWindow(L"STATIC", L"Ваши кошельки:",
            WS_VISIBLE | WS_CHILD,
            50, 60, 150, 20, hWnd, NULL, NULL, NULL);

        hWalletsList = CreateWindow(L"LISTBOX", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
            50, 85, 200, 120, hWnd, (HMENU)500, NULL, NULL);

        // Поля для добавления валюты в новый кошелек
        CreateWindow(L"STATIC", L"Добавить количество валюты:",
            WS_VISIBLE | WS_CHILD,
            50, 220, 200, 20, hWnd, NULL, NULL, NULL);

        CreateWindow(L"STATIC", L"Валюта:",
            WS_VISIBLE | WS_CHILD,
            50, 250, 80, 20, hWnd, NULL, NULL, NULL);

        hCurrencyEdit = CreateWindow(L"EDIT", L"USD",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_UPPERCASE,
            140, 250, 100, 20, hWnd, NULL, NULL, NULL);

        CreateWindow(L"STATIC", L"Количество:",
            WS_VISIBLE | WS_CHILD,
            50, 280, 80, 20, hWnd, NULL, NULL, NULL);

        hAmountEdit = CreateWindow(L"EDIT", L"100",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            140, 280, 100, 20, hWnd, NULL, NULL, NULL);

        // Список валют в создаваемом кошельке
        CreateWindow(L"STATIC", L"Валюты в новом кошельке:",
            WS_VISIBLE | WS_CHILD,
            50, 310, 200, 20, hWnd, NULL, NULL, NULL);

        hCurrentCurrenciesList = CreateWindow(L"LISTBOX", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
            50, 335, 200, 100, hWnd, (HMENU)501, NULL, NULL);

        // Кнопки
        CreateWindow(L"BUTTON", L"Назад",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            270, 85, 100, 30, hWnd,
            (HMENU)502, NULL, NULL);

        CreateWindow(L"BUTTON", L"Добавить",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            270, 250, 100, 30, hWnd,
            (HMENU)503, NULL, NULL);

        CreateWindow(L"BUTTON", L"Создать",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            270, 290, 100, 30, hWnd,
            (HMENU)504, NULL, NULL);

        CreateWindow(L"BUTTON", L"Открыть",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            270, 125, 100, 30, hWnd,
            (HMENU)505, NULL, NULL);

        CreateWindow(L"BUTTON", L"Удалить",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            270, 165, 100, 30, hWnd,
            (HMENU)506, NULL, NULL);

        hStatusLabel = CreateWindow(L"STATIC", L"Добавьте валюты и создайте кошелек",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 450, 300, 20, hWnd, NULL, NULL, NULL);

        // ЗАГРУЖАЕМ КОШЕЛЬКИ СРАЗУ ПРИ ОТКРЫТИИ ОКНА
        LoadWallets();
        UpdateWalletsList(hWalletsList);

        HFONT hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, L"Microsoft Sans Serif");
        if (hFont) {
            HWND hChild = GetWindow(hWnd, GW_CHILD);
            while (hChild) {
                SendMessage(hChild, WM_SETFONT, (WPARAM)hFont, TRUE);
                hChild = GetNextWindow(hChild, GW_HWNDNEXT);
            }
        }
    }
    break;

    case WM_COMMAND:
    {
        int buttonId = LOWORD(wParam);

        switch (buttonId) {
        case 502: // Назад
            DestroyWindow(hWnd);
            break;

        case 503: // Добавить валюту
        {
            wchar_t currency[10], amount[20];
            GetWindowText(hCurrencyEdit, currency, 10);
            GetWindowText(hAmountEdit, amount, 20);

            std::wstring currencyStr = currency;
            std::wstring amountStr = amount;

            if (currencyStr.empty() || amountStr.empty()) {
                SetWindowText(hStatusLabel, L"Заполните все поля!");
                break;
            }

            // Проверяем поддержку валюты
            if (!IsCurrencySupported(currencyStr)) {
                SetWindowText(hStatusLabel, L"Валюта не поддерживается!");
                break;
            }

            try {
                double amountValue = std::stod(amountStr);
                if (amountValue <= 0) {
                    SetWindowText(hStatusLabel, L"Количество должно быть больше 0!");
                    break;
                }

                // Добавляем валюту во временный список
                WalletCurrency wc;
                wc.currency = currencyStr;
                wc.amount = amountValue;
                tempCurrencies.push_back(wc);

                // Обновляем список валют
                UpdateCurrentCurrenciesList(hCurrentCurrenciesList, tempCurrencies);
                SetWindowText(hStatusLabel, L"Валюта добавлена!");

                // Очищаем поля
                SetWindowText(hCurrencyEdit, L"");
                SetWindowText(hAmountEdit, L"");
            }
            catch (...) {
                SetWindowText(hStatusLabel, L"Ошибка в количестве!");
            }
        }
        break;

        case 504: // Создать кошелек
        {
            if (tempCurrencies.empty()) {
                SetWindowText(hStatusLabel, L"Добавьте хотя бы одну валюту!");
                break;
            }

            // Создаем новый кошелек
            Wallet newWallet;
            newWallet.name = GenerateWalletName();
            newWallet.currencies = tempCurrencies;

            AddWallet(newWallet);
            SetWindowText(hStatusLabel, (L"Кошелек создан: " + newWallet.name).c_str());

            // Очищаем временные данные
            tempCurrencies.clear();
            UpdateCurrentCurrenciesList(hCurrentCurrenciesList, tempCurrencies);

            // ОБНОВЛЯЕМ СПИСОК КОШЕЛЬКОВ (теперь там будет новый кошелек)
            UpdateWalletsList(hWalletsList);
        }
        break;

        // В WalletsWindowProc в обработке кнопки "Открыть":
        case 505: // Открыть кошелек
        {
            int index = SendMessage(hWalletsList, LB_GETCURSEL, 0, 0);
            if (index != LB_ERR && index < g_wallets.size()) {
                // Создаем окно детального просмотра кошелька
                HWND hWalletDetailWindow = CreateWindow(
                    L"WalletDetailWindowClass",
                    L"Детали кошелька",
                    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                    CW_USEDEFAULT, CW_USEDEFAULT, 450, 550,
                    hWnd, NULL, g_hInstance, (LPVOID)index // Передаем индекс кошелька
                );

                if (hWalletDetailWindow) {
                    ShowWindow(hWalletDetailWindow, SW_SHOW);
                    UpdateWindow(hWalletDetailWindow);
                }
            }
            else {
                SetWindowText(hStatusLabel, L"Выберите кошелек для открытия!");
            }
        }
        break;
        break;

        case 506: // Удалить кошелек
        {
            int index = SendMessage(hWalletsList, LB_GETCURSEL, 0, 0);
            if (index != LB_ERR && index < g_wallets.size()) {
                const auto& wallet = g_wallets[index];
                RemoveWallet(index);

                // ОБНОВЛЯЕМ СПИСОК КОШЕЛЬКОВ (удаленный кошелек исчезнет)
                UpdateWalletsList(hWalletsList);
                SetWindowText(hStatusLabel, (L"Кошелек удален: " + wallet.name).c_str());
            }
            else {
                SetWindowText(hStatusLabel, L"Выберите кошелек для удаления!");
            }
        }
        break;
        }
    }
    break;

    case WM_DESTROY:
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK LoginWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hUsernameEdit = NULL;
    static HWND hPasswordEdit = NULL;
    static HWND hStatusLabel = NULL;

    switch (uMsg) {
    case WM_CREATE:
    {
        CreateWindow(L"STATIC", L"Система авторизации",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 20, 200, 30, hWnd, NULL, NULL, NULL);

        CreateWindow(L"STATIC", L"Логин:",
            WS_VISIBLE | WS_CHILD,
            50, 70, 80, 20, hWnd, NULL, NULL, NULL);

        hUsernameEdit = CreateWindow(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            120, 70, 150, 20, hWnd, NULL, NULL, NULL);

        CreateWindow(L"STATIC", L"Пароль:",
            WS_VISIBLE | WS_CHILD,
            50, 100, 80, 20, hWnd, NULL, NULL, NULL);

        hPasswordEdit = CreateWindow(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD,
            120, 100, 150, 20, hWnd, NULL, NULL, NULL);

        CreateWindow(L"BUTTON", L"Войти",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            50, 140, 100, 30, hWnd,
            (HMENU)100, NULL, NULL);

        CreateWindow(L"BUTTON", L"Зарегистрироваться",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            160, 140, 120, 30, hWnd,
            (HMENU)101, NULL, NULL);

        CreateWindow(L"BUTTON", L"Войти как гость",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            50, 180, 230, 30, hWnd,
            (HMENU)102, NULL, NULL);

        hStatusLabel = CreateWindow(L"STATIC", L"Введите данные для входа",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 220, 230, 20, hWnd, NULL, NULL, NULL);

        HFONT hTitleFont = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, L"Microsoft Sans Serif");
        HFONT hNormalFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, L"Microsoft Sans Serif");

        if (hTitleFont && hNormalFont) {
            HWND hChild = GetWindow(hWnd, GW_CHILD);
            while (hChild) {
                wchar_t className[100];
                GetClassName(hChild, className, 100);
                if (wcscmp(className, L"Static") == 0) {
                    wchar_t text[100];
                    GetWindowText(hChild, text, 100);
                    if (wcscmp(text, L"Система авторизации") == 0) {
                        SendMessage(hChild, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
                    }
                    else {
                        SendMessage(hChild, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
                    }
                }
                else {
                    SendMessage(hChild, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
                }
                hChild = GetNextWindow(hChild, GW_HWNDNEXT);
            }
        }
    }
    break;

    case WM_COMMAND:
    {
        int buttonId = LOWORD(wParam);

        wchar_t username[100];
        wchar_t password[100];
        GetWindowText(hUsernameEdit, username, 100);
        GetWindowText(hPasswordEdit, password, 100);

        std::wstring usernameStr = username;
        std::wstring passwordStr = password;

        switch (buttonId) {
        case 100: // Войти
            if (usernameStr.empty() || passwordStr.empty()) {
                SetWindowText(hStatusLabel, L"Заполните все поля!");
                break;
            }

            if (CheckUserCredentials(usernameStr, passwordStr)) {
                g_currentUser = usernameStr;
                LoadCurrencyPairs(); // Загружаем пары пользователя
                LoadWallets();       // ← ДОБАВИТЬ: Загружаем кошельки пользователя
                SetWindowText(hStatusLabel, L"Успешный вход!");

                DestroyWindow(hWnd);
                HWND hMainWindow = CreateWindow(
                    L"MainMenuClass",
                    L"Главное меню",
                    WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT, CW_USEDEFAULT, 500, 500,
                    NULL, NULL, g_hInstance, NULL
                );
                ShowWindow(hMainWindow, SW_SHOW);
            }
            else {
                SetWindowText(hStatusLabel, L"Неверный логин или пароль!");
            }
            break;

        case 101: // Зарегистрироваться
            if (usernameStr.empty() || passwordStr.empty()) {
                SetWindowText(hStatusLabel, L"Заполните все поля!");
                break;
            }

            if (UserExists(usernameStr)) {
                SetWindowText(hStatusLabel, L"Пользователь уже существует!");
                break;
            }

            if (SaveUserToFile(usernameStr, passwordStr)) {
                g_currentUser = usernameStr;
                LoadCurrencyPairs(); // Создаем стандартные пары для нового пользователя
                LoadWallets();       // ← ДОБАВИТЬ: Загружаем кошельки (будут пустые)
                SetWindowText(hStatusLabel, L"Регистрация успешна!");

                DestroyWindow(hWnd);
                HWND hMainWindow = CreateWindow(
                    L"MainMenuClass",
                    L"Главное меню",
                    WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT, CW_USEDEFAULT, 500, 500,
                    NULL, NULL, g_hInstance, NULL
                );
                ShowWindow(hMainWindow, SW_SHOW);
            }
            else {
                SetWindowText(hStatusLabel, L"Ошибка регистрации!");
            }
            break;

        case 102: // Войти как гость
            g_currentUser = L"Гость";
            LoadCurrencyPairs(); // Загружаем стандартные пары для гостя
            LoadWallets();       // ← ДОБАВИТЬ: Загружаем кошельки (для гостя будут пустые)
            SetWindowText(hStatusLabel, L"Вход как гость!");

            DestroyWindow(hWnd);
            HWND hMainWindow = CreateWindow(
                L"MainMenuClass",
                L"Главное меню (Гость)",
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, 500, 500,
                NULL, NULL, g_hInstance, NULL
            );
            ShowWindow(hMainWindow, SW_SHOW);
            break;
        }
    }
    break;

    case WM_DESTROY:
        if (g_currentUser.empty()) {
            PostQuitMessage(0);
        }
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Оконная процедура ГЛАВНОГО МЕНЮ
LRESULT CALLBACK MainMenuWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hCurrencyList = NULL;

    switch (uMsg) {
    case WM_CREATE:
    {
        // Приветствие
        std::wstring welcomeText = L"Добро пожаловать, " + g_currentUser + L"!";
        CreateWindow(L"STATIC", welcomeText.c_str(),
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 20, 400, 30, hWnd, NULL, NULL, NULL);

        // Заголовок курсов валют
        CreateWindow(L"STATIC", L"Актуальные курсы валют:",
            WS_VISIBLE | WS_CHILD,
            50, 60, 200, 20, hWnd, NULL, NULL, NULL);

        // ListBox для отображения курсов
        hCurrencyList = CreateWindow(L"LISTBOX", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
            50, 85, 400, 200, hWnd, NULL, NULL, NULL);

        // Кнопки меню
        CreateWindow(L"BUTTON", L"Добавить/удалить валютную пару",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            50, 300, 200, 40, hWnd,
            (HMENU)200, NULL, NULL);

        CreateWindow(L"BUTTON", L"Настройки уведомлений",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            260, 300, 200, 40, hWnd,
            (HMENU)201, NULL, NULL);

        CreateWindow(L"BUTTON", L"Кошельки",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            50, 350, 200, 40, hWnd,
            (HMENU)202, NULL, NULL);

        CreateWindow(L"BUTTON", L"Создать отчет",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            260, 350, 200, 40, hWnd,
            (HMENU)203, NULL, NULL);

        CreateWindow(L"BUTTON", L"Обновить курсы",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            50, 400, 200, 40, hWnd,
            (HMENU)204, NULL, NULL);

        CreateWindow(L"BUTTON", L"Выйти",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            260, 400, 200, 40, hWnd,
            (HMENU)205, NULL, NULL);

        // Устанавливаем шрифты
        HFONT hTitleFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, L"Microsoft Sans Serif");
        HFONT hNormalFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, L"Microsoft Sans Serif");

        if (hTitleFont && hNormalFont) {
            HWND hChild = GetWindow(hWnd, GW_CHILD);
            while (hChild) {
                wchar_t className[100];
                GetClassName(hChild, className, 100);
                if (wcscmp(className, L"Static") == 0) {
                    SendMessage(hChild, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
                }
                else if (wcscmp(className, L"ListBox") == 0) {
                    SendMessage(hChild, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
                }
                else {
                    SendMessage(hChild, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
                }
                hChild = GetNextWindow(hChild, GW_HWNDNEXT);
            }
        }

        // Загружаем и отображаем курсы валют
        UpdateCurrencyRates(hCurrencyList);
    }
    break;

    case WM_COMMAND:
    {
        int buttonId = LOWORD(wParam);
        switch (buttonId) {
        case 200: // Добавить/удалить валютную пару
        {
            HWND hPairsWindow = CreateWindow(
                L"CurrencyPairsClass",
                L"Управление валютными парами",
                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
                hWnd, NULL, g_hInstance, NULL
            );
            ShowWindow(hPairsWindow, SW_SHOW);
        }
        break;

        case 201: // Настройки уведомлений
        {
            HWND hNotificationsWindow = CreateWindow(
                L"NotificationsWindowClass",
                L"Настройка уведомлений",
                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                CW_USEDEFAULT, CW_USEDEFAULT, 400, 450,
                hWnd, NULL, g_hInstance, NULL
            );
            ShowWindow(hNotificationsWindow, SW_SHOW);

            // Загружаем уведомления при открытии окна
            LoadNotifications();
        }
        break;

        case 202: // Кошельки
        {
            HWND hWalletsWindow = CreateWindow(
                L"WalletsWindowClass",
                L"Список Кошельков",
                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                CW_USEDEFAULT, CW_USEDEFAULT, 400, 550,
                hWnd, NULL, g_hInstance, NULL
            );
            ShowWindow(hWalletsWindow, SW_SHOW);
        }
        break;

        case 203: // Создать отчет
        {
            HWND hReportsWindow = CreateWindow(
                L"ReportsWindowClass",
                L"Окно отчетов",
                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                CW_USEDEFAULT, CW_USEDEFAULT, 400, 450,
                hWnd, NULL, g_hInstance, NULL
            );
            ShowWindow(hReportsWindow, SW_SHOW);
        }
        break;

        case 204: // Обновить курсы
            UpdateCurrencyRates(hCurrencyList);
            MessageBox(hWnd, L"Курсы валют обновлены!", L"Обновление", MB_OK);
            break;

        case 205: // Выйти
            DestroyWindow(hWnd);
            break;
        }
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}


// Точка входа
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInstance = hInstance;

    // Регистрируем классы окон
    if (!RegisterWindowClass(L"LoginWindowClass", LoginWindowProc) ||
        !RegisterWindowClass(L"MainMenuClass", MainMenuWindowProc) ||
        !RegisterWindowClass(L"CurrencyPairsClass", CurrencyPairsWindowProc) ||
        !RegisterWindowClass(L"NotificationsWindowClass", NotificationsWindowProc) ||
        !RegisterWindowClass(L"WalletsWindowClass", WalletsWindowProc) ||
        !RegisterWindowClass(L"WalletDetailWindowClass", WalletDetailWindowProc) ||
        !RegisterWindowClass(L"ReportViewWindowClass", ReportViewWindowProc)
        ) {
        MessageBox(NULL, L"Ошибка регистрации классов окон", L"Ошибка", MB_ICONERROR);
        return 0;
    }

    // Создаем окно авторизации
    HWND hLoginWindow = CreateWindow(
        L"LoginWindowClass",
        L"Авторизация",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 350, 320,
        NULL, NULL, hInstance, NULL
    );

    if (!hLoginWindow) {
        MessageBox(NULL, L"Ошибка создания окна авторизации", L"Ошибка", MB_ICONERROR);
        return 0;
    }

    // Центрируем окно
    RECT rc;
    GetWindowRect(hLoginWindow, &rc);
    int x = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
    SetWindowPos(hLoginWindow, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    ShowWindow(hLoginWindow, nCmdShow);
    UpdateWindow(hLoginWindow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}