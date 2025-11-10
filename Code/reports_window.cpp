#include "reports_window.h"
#include "report_view_window.h"
#include "wallet_manager.h"
#include "transaction_manager.h"
#include "common.h"

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

        hStartDateEdit = CreateWindow(L"EDIT", L"2025-01-01",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            210, 250, 120, 20, hWnd, (HMENU)7000, NULL, NULL);

        CreateWindow(L"STATIC", L"По дату (гггг-мм-дд):",
            WS_VISIBLE | WS_CHILD,
            50, 280, 150, 20, hWnd, NULL, NULL, NULL);

        hEndDateEdit = CreateWindow(L"EDIT", L"2025-12-31",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            210, 280, 120, 20, hWnd, (HMENU)7001, NULL, NULL);

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
            50, 370, 300, 20, hWnd, (HMENU)7002, NULL, NULL);

        // Заполняем список кошельков
        UpdateReportsWalletsList(hWalletsList);

        // Устанавливаем шрифт
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

            if (startDateStr.length() != 10 || endDateStr.length() != 10 ||
                startDateStr[4] != '-' || startDateStr[7] != '-' ||
                endDateStr[4] != '-' || endDateStr[7] != '-') {
                SetWindowText(hStatusLabel, L"Неверный формат даты! Используйте гггг-мм-дд");
                break;
            }

            const Wallet& wallet = g_wallets[walletIndex];

            std::vector<Transaction> allTransactions = TransactionManager::LoadTransactions(wallet);
            if (allTransactions.empty()) {
                SetWindowText(hStatusLabel, L"Нет транзакций для выбранного кошелька!");
                break;
            }

            std::vector<Transaction> filteredTransactions =
                TransactionManager::FilterTransactionsByDate(allTransactions, startDateStr, endDateStr);

            if (filteredTransactions.empty()) {
                SetWindowText(hStatusLabel, L"Нет транзакций за выбранный период!");
                break;
            }

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

void CreateReportWindow(HWND parent, const Wallet& wallet,
    const std::vector<Transaction>& transactions,
    const std::wstring& startDate, const std::wstring& endDate) {

    ReportData* reportData = new ReportData;
    reportData->wallet = wallet;
    reportData->transactions = transactions;
    reportData->startDate = startDate;
    reportData->endDate = endDate;

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
        delete reportData;
    }
}