#include "report_view_window.h"
#include "transaction_manager.h"
#include "common.h"

LRESULT CALLBACK ReportViewWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hReportContentList = NULL;

    switch (uMsg) {
    case WM_CREATE:
    {
        CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
        ReportData* reportData = (ReportData*)createStruct->lpCreateParams;

        if (!reportData) {
            DestroyWindow(hWnd);
            break;
        }

        std::wstring title = L"Отчет по кошельку: " + reportData->wallet.name +
            L" за период " + reportData->startDate + L" - " + reportData->endDate;

        CreateWindow(L"STATIC", title.c_str(),
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 20, 500, 30, hWnd, NULL, NULL, NULL);

        hReportContentList = CreateWindow(L"LISTBOX", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL,
            50, 60, 500, 400, hWnd, NULL, NULL, NULL);

        CreateWindow(L"BUTTON", L"Закрыть",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            250, 470, 100, 30, hWnd,
            (HMENU)800, NULL, NULL);

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

        delete reportData;
    }
    break;

    case WM_COMMAND:
    {
        int buttonId = LOWORD(wParam);
        if (buttonId == 800) {
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

void FillReportContent(HWND hListBox, const ReportData& reportData) {
    if (!hListBox) return;

    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

    ReportStats stats = TransactionManager::CalculateReportStats(reportData.transactions);

    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"════════════════ ОТЧЕТ ════════════════");

    std::wstring periodInfo = L"Период: " + reportData.startDate + L" - " + reportData.endDate;
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)periodInfo.c_str());

    std::wstring walletInfo = L"Кошелек: " + reportData.wallet.name;
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)walletInfo.c_str());

    std::wstring countInfo = L"Количество транзакций: " + std::to_wstring(stats.transactionCount);
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)countInfo.c_str());

    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"");
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"════════════ СТАТИСТИКА ════════════");

    std::wstring soldInfo = L"Всего продано: " + std::to_wstring(stats.totalSold);
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)soldInfo.c_str());

    std::wstring boughtInfo = L"Всего куплено: " + std::to_wstring(stats.totalBought);
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)boughtInfo.c_str());

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