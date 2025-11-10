#ifndef COMMON_H
#define COMMON_H

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

// Структуры данных
struct WalletCurrency {
    std::wstring currency;
    double amount;
};

struct Wallet {
    std::wstring name;
    std::vector<WalletCurrency> currencies;
};

struct Notification {
    std::wstring currency1;
    std::wstring currency2;
    double value1;
    double value2;
    std::wstring condition;
    bool active;
};

struct Transaction {
    std::wstring date;
    std::wstring soldCurrency;
    double soldAmount;
    std::wstring boughtCurrency;
    double boughtAmount;
    double exchangeRate;
    std::vector<WalletCurrency> walletState;
};

struct ReportStats {
    double totalSold = 0.0;
    double totalBought = 0.0;
    int transactionCount = 0;
    std::map<std::wstring, double> currencyChanges;
};

struct ReportData {
    Wallet wallet;
    std::vector<Transaction> transactions;
    std::wstring startDate;
    std::wstring endDate;
};

// Глобальные переменные
extern HINSTANCE g_hInstance;
extern std::wstring g_currentUser;
extern std::vector<std::wstring> g_currencyPairs;
extern std::vector<Wallet> g_wallets;
extern std::vector<Notification> g_notifications;
extern HWND hMain;
extern int g_currentWalletIndex;
extern bool g_notificationThreadRunning;

// Поддерживаемые валюты
extern std::vector<std::wstring> supportedCurrencies;

// Прототипы общих функций
std::wstring GetUserCurrencyFile();
std::wstring GetCurrentDateTime();
bool IsCurrencySupported(const std::wstring& currency);
bool ParseNotificationCondition(const std::wstring& conditionStr,
    double& value1, double& value2, std::wstring& condition);

// Прототипы оконных процедур (добавлены недостающие)
LRESULT CALLBACK LoginWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MainMenuWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK CurrencyPairsWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK NotificationsWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WalletsWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WalletDetailWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ReportsWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ReportViewWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif