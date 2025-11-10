#ifndef CURRENCY_PAIRS_WINDOW_H
#define CURRENCY_PAIRS_WINDOW_H

#include "common.h"

class CurrencyPairsWindow {
public:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    static void OnCreate(HWND hWnd);
    static void OnCommand(HWND hWnd, WPARAM wParam);
    static void AddCurrencyPair(HWND hWnd, HWND hFromEdit, HWND hToEdit, HWND hStatusLabel);
    static void RemoveCurrencyPair(HWND hWnd, HWND hFromEdit, HWND hToEdit, HWND hStatusLabel);
};

#endif