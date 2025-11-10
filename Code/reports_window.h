#ifndef REPORTS_WINDOW_H
#define REPORTS_WINDOW_H

#include "common.h"

LRESULT CALLBACK ReportsWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void UpdateReportsWalletsList(HWND hListBox);
void CreateReportWindow(HWND parent, const Wallet& wallet,
    const std::vector<Transaction>& transactions,
    const std::wstring& startDate, const std::wstring& endDate);

#endif