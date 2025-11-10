#include "currency_pairs_window.h"
#include "api_client.h"
#include "file_manager.h"
#include "common.h"
#include "main_window.h"

LRESULT CALLBACK CurrencyPairsWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        OnCreate(hWnd);
        break;
    case WM_COMMAND:
        OnCommand(hWnd, wParam);
        break;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

void CurrencyPairsWindow::OnCreate(HWND hWnd) {
    CreateWindow(L"STATIC", L"Введите валютную пару:",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        50, 20, 300, 30, hWnd, NULL, NULL, NULL);

    CreateWindow(L"STATIC", L"Из валюты:",
        WS_VISIBLE | WS_CHILD,
        50, 60, 80, 20, hWnd, NULL, NULL, NULL);

    CreateWindow(L"EDIT", L"USD",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_UPPERCASE,
        140, 60, 100, 20, hWnd, (HMENU)3000, NULL, NULL);

    CreateWindow(L"STATIC", L"В валюту:",
        WS_VISIBLE | WS_CHILD,
        50, 90, 80, 20, hWnd, NULL, NULL, NULL);

    CreateWindow(L"EDIT", L"EUR",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_UPPERCASE,
        140, 90, 100, 20, hWnd, (HMENU)3001, NULL, NULL);

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

    CreateWindow(L"STATIC", L"Введите пару в формате USD-EUR",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        50, 170, 300, 20, hWnd, (HMENU)3002, NULL, NULL);

    // Список текущих пар
    CreateWindow(L"STATIC", L"Текущие валютные пары:",
        WS_VISIBLE | WS_CHILD,
        50, 200, 200, 20, hWnd, NULL, NULL, NULL);

    CreateWindow(L"LISTBOX", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
        50, 225, 300, 150, hWnd, (HMENU)303, NULL, NULL);

    // Заполняем список текущих пар
    HWND hListBox = GetDlgItem(hWnd, 303);
    for (const auto& pair : g_currencyPairs) {
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)pair.c_str());
    }

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

void CurrencyPairsWindow::OnCommand(HWND hWnd, WPARAM wParam) {
    int buttonId = LOWORD(wParam);

    HWND hFromEdit = GetDlgItem(hWnd, 3000);
    HWND hToEdit = GetDlgItem(hWnd, 3001);
    HWND hStatusLabel = GetDlgItem(hWnd, 3002);
    HWND hCurrentPairsList = GetDlgItem(hWnd, 303);

    wchar_t from[10], to[10];
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
        RemoveCurrencyPair(hWnd, hFromEdit, hToEdit, hStatusLabel);
        break;

    case 302: // Добавить
        AddCurrencyPair(hWnd, hFromEdit, hToEdit, hStatusLabel);
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

void CurrencyPairsWindow::AddCurrencyPair(HWND hWnd, HWND hFromEdit, HWND hToEdit, HWND hStatusLabel) {
    wchar_t from[10], to[10];
    GetWindowText(hFromEdit, from, 10);
    GetWindowText(hToEdit, to, 10);

    std::wstring fromStr = from;
    std::wstring toStr = to;

    if (fromStr.empty() || toStr.empty()) {
        SetWindowText(hStatusLabel, L"Заполните оба поля!");
        return;
    }

    // Проверяем поддержку валют
    if (!IsCurrencySupported(fromStr)) {
        SetWindowText(hStatusLabel, L"Ошибка: валюта 'from' не поддерживается!");
        return;
    }
    if (!IsCurrencySupported(toStr)) {
        SetWindowText(hStatusLabel, L"Ошибка: валюта 'to' не поддерживается!");
        return;
    }

    if (APIClient::CheckCurrencyPairExists(fromStr, toStr)) {
        std::wstring pair = fromStr + L"-" + toStr;
        FileManager::AddCurrencyPair(pair);
        SetWindowText(hStatusLabel, L"Пара добавлена!");

        // Обновляем список текущих пар
        HWND hCurrentPairsList = GetDlgItem(hWnd, 303);
        SendMessage(hCurrentPairsList, LB_RESETCONTENT, 0, 0);
        for (const auto& p : g_currencyPairs) {
            SendMessage(hCurrentPairsList, LB_ADDSTRING, 0, (LPARAM)p.c_str());
        }

        // Обновляем список в главном окне
        hMain = FindWindow(L"MainMenuClass", NULL);
        if (hMain) {
            HWND hList = GetDlgItem(hMain, 2000);
            if (hList) {
                MainWindow::UpdateCurrencyRates(hList);
            }
        }
    }
    else {
        SetWindowText(hStatusLabel, L"Ошибка: неверная валютная пара или проблема с API!");
    }
}

void CurrencyPairsWindow::RemoveCurrencyPair(HWND hWnd, HWND hFromEdit, HWND hToEdit, HWND hStatusLabel) {
    wchar_t from[10], to[10];
    GetWindowText(hFromEdit, from, 10);
    GetWindowText(hToEdit, to, 10);

    std::wstring fromStr = from;
    std::wstring toStr = to;

    if (fromStr.empty() || toStr.empty()) {
        SetWindowText(hStatusLabel, L"Заполните оба поля!");
        return;
    }

    std::wstring pair = fromStr + L"-" + toStr;
    FileManager::RemoveCurrencyPair(pair);
    SetWindowText(hStatusLabel, L"Пара удалена!");

    // Обновляем список текущих пар
    HWND hCurrentPairsList = GetDlgItem(hWnd, 303);
    SendMessage(hCurrentPairsList, LB_RESETCONTENT, 0, 0);
    for (const auto& p : g_currencyPairs) {
        SendMessage(hCurrentPairsList, LB_ADDSTRING, 0, (LPARAM)p.c_str());
    }

    // Обновляем список в главном окне
    hMain = FindWindow(L"MainMenuClass", NULL);
    if (hMain) {
        HWND hList = GetDlgItem(hMain, 2000);
        if (hList) {
            MainWindow::UpdateCurrencyRates(hList);
        }
    }
}