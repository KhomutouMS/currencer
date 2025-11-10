#ifndef REPORT_VIEW_WINDOW_H
#define REPORT_VIEW_WINDOW_H

#include "common.h"

LRESULT CALLBACK ReportViewWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void FillReportContent(HWND hListBox, const ReportData& reportData);

#endif#pragma once
