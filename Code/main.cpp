#include "common.h"
#include "login_window.h"
#include "main_window.h"
#include "currency_pairs_window.h"
#include "notifications_window.h"
#include "wallets_window.h"
#include "wallet_detail_window.h"
#include "reports_window.h"
#include "report_view_window.h"

// Инициализация глобальных переменных
HINSTANCE g_hInstance;
std::wstring g_currentUser;
std::vector<std::wstring> g_currencyPairs;
std::vector<Wallet> g_wallets;
std::vector<Notification> g_notifications;
HWND hMain;
int g_currentWalletIndex = -1;
bool g_notificationThreadRunning = false;

std::vector<std::wstring> supportedCurrencies = {
    L"AUD", L"BGN", L"BRL", L"CAD", L"CHF", L"CNY", L"CZK", L"DKK", L"EUR",
    L"GBP", L"HKD", L"HUF", L"IDR", L"ILS", L"INR", L"ISK", L"JPY", L"KRW",
    L"MXN", L"MYR", L"NOK", L"NZD", L"PHP", L"PLN", L"RON", L"SEK", L"SGD",
    L"THB", L"TRY", L"USD", L"ZAR"
};

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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInstance = hInstance;

    // Регистрируем классы окон (используем статические методы классов)
    if (!RegisterWindowClass(L"LoginWindowClass", LoginWindow::WindowProc) ||
        !RegisterWindowClass(L"MainMenuClass", MainWindow::WindowProc) ||
        !RegisterWindowClass(L"CurrencyPairsClass", CurrencyPairsWindow::WindowProc) ||
        !RegisterWindowClass(L"NotificationsWindowClass", NotificationsWindowProc) ||
        !RegisterWindowClass(L"WalletsWindowClass", WalletsWindowProc) ||
        !RegisterWindowClass(L"WalletDetailWindowClass", WalletDetailWindowProc) ||
        !RegisterWindowClass(L"ReportViewWindowClass", ReportViewWindowProc) ||
        !RegisterWindowClass(L"ReportsWindowClass", ReportsWindowProc)) {
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