#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "common.h"

class MainWindow {
public:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void UpdateCurrencyRates(HWND hListBox);

private:
    static void OnCreate(HWND hWnd);
    static void OnCommand(HWND hWnd, WPARAM wParam);
    static void OnDestroy(HWND hWnd);
    static void CreateChildWindows(HWND hWnd);
};

#endif