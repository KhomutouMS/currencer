#include "wallets_window.h"
#include "wallet_detail_window.h"
#include "wallet_manager.h"
#include "common.h"

LRESULT CALLBACK WalletsWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hWalletsList = NULL;
    static HWND hCurrencyEdit = NULL;
    static HWND hAmountEdit = NULL;
    static HWND hStatusLabel = NULL;
    static HWND hCurrentCurrenciesList = NULL;

    static std::vector<WalletCurrency> tempCurrencies;

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
            140, 250, 100, 20, hWnd, (HMENU)5000, NULL, NULL);

        CreateWindow(L"STATIC", L"Количество:",
            WS_VISIBLE | WS_CHILD,
            50, 280, 80, 20, hWnd, NULL, NULL, NULL);

        hAmountEdit = CreateWindow(L"EDIT", L"100",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            140, 280, 100, 20, hWnd, (HMENU)5001, NULL, NULL);

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
            50, 450, 300, 20, hWnd, (HMENU)5002, NULL, NULL);

        // Загружаем кошельки
        WalletManager::LoadWallets();
        WalletManager::UpdateWalletsList(hWalletsList);

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

                WalletCurrency wc;
                wc.currency = currencyStr;
                wc.amount = amountValue;
                tempCurrencies.push_back(wc);

                WalletManager::UpdateCurrentCurrenciesList(hCurrentCurrenciesList, tempCurrencies);
                SetWindowText(hStatusLabel, L"Валюта добавлена!");

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

            Wallet newWallet;
            newWallet.name = WalletManager::GenerateWalletName();
            newWallet.currencies = tempCurrencies;

            WalletManager::AddWallet(newWallet);
            SetWindowText(hStatusLabel, (L"Кошелек создан: " + newWallet.name).c_str());

            tempCurrencies.clear();
            WalletManager::UpdateCurrentCurrenciesList(hCurrentCurrenciesList, tempCurrencies);
            WalletManager::UpdateWalletsList(hWalletsList);
        }
        break;

        case 505: // Открыть кошелек
        {
            int index = SendMessage(hWalletsList, LB_GETCURSEL, 0, 0);
            if (index != LB_ERR && index < g_wallets.size()) {
                HWND hWalletDetailWindow = CreateWindow(
                    L"WalletDetailWindowClass",
                    L"Детали кошелька",
                    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                    CW_USEDEFAULT, CW_USEDEFAULT, 450, 550,
                    hWnd, NULL, g_hInstance, (LPVOID)index
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

        case 506: // Удалить кошелек
        {
            int index = SendMessage(hWalletsList, LB_GETCURSEL, 0, 0);
            if (index != LB_ERR && index < g_wallets.size()) {
                const auto& wallet = g_wallets[index];
                WalletManager::RemoveWallet(index);

                WalletManager::UpdateWalletsList(hWalletsList);
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