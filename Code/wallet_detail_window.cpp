#include "wallet_detail_window.h"
#include "wallet_manager.h"
#include "transaction_manager.h"
#include "api_client.h"
#include "common.h"

LRESULT CALLBACK WalletDetailWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hWalletContentList = NULL;
    static HWND hSellCurrencyEdit = NULL;
    static HWND hBuyCurrencyEdit = NULL;
    static HWND hAmountEdit = NULL;
    static HWND hResultLabel = NULL;
    static HWND hStatusLabel = NULL;
    static HWND hExchangeRateLabel = NULL;

    switch (uMsg) {
    case WM_CREATE:
    {
        CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
        g_currentWalletIndex = (int)createStruct->lpCreateParams;

        if (g_currentWalletIndex < 0 || g_currentWalletIndex >= g_wallets.size()) {
            MessageBox(hWnd, L"Ошибка: кошелек не найден", L"Ошибка", MB_OK);
            DestroyWindow(hWnd);
            break;
        }

        Wallet& wallet = g_wallets[g_currentWalletIndex];

        // Заголовок с именем кошелька
        CreateWindow(L"STATIC", (L"Кошелек: " + wallet.name).c_str(),
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 20, 300, 30, hWnd, NULL, NULL, NULL);

        // Содержимое кошелька
        CreateWindow(L"STATIC", L"Содержимое кошелька:",
            WS_VISIBLE | WS_CHILD,
            50, 60, 200, 20, hWnd, NULL, NULL, NULL);

        hWalletContentList = CreateWindow(L"LISTBOX", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
            50, 85, 300, 150, hWnd, NULL, NULL, NULL);

        // Поля для обмена валют
        CreateWindow(L"STATIC", L"Обмен валют:",
            WS_VISIBLE | WS_CHILD,
            50, 250, 200, 20, hWnd, NULL, NULL, NULL);

        CreateWindow(L"STATIC", L"Продать валюту:",
            WS_VISIBLE | WS_CHILD,
            50, 280, 100, 20, hWnd, NULL, NULL, NULL);

        hSellCurrencyEdit = CreateWindow(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_UPPERCASE,
            160, 280, 80, 20, hWnd, (HMENU)6000, NULL, NULL);

        CreateWindow(L"STATIC", L"Купить валюту:",
            WS_VISIBLE | WS_CHILD,
            50, 310, 100, 20, hWnd, NULL, NULL, NULL);

        hBuyCurrencyEdit = CreateWindow(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_UPPERCASE,
            160, 310, 80, 20, hWnd, (HMENU)6001, NULL, NULL);

        CreateWindow(L"STATIC", L"Количество для продажи:",
            WS_VISIBLE | WS_CHILD,
            50, 340, 150, 20, hWnd, NULL, NULL, NULL);

        hAmountEdit = CreateWindow(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            210, 340, 80, 20, hWnd, (HMENU)6002, NULL, NULL);

        // Поле для отображения курса и результата
        hExchangeRateLabel = CreateWindow(L"STATIC", L"Курс: -",
            WS_VISIBLE | WS_CHILD,
            50, 370, 300, 20, hWnd, (HMENU)6003, NULL, NULL);

        hResultLabel = CreateWindow(L"STATIC", L"Вы получите: -",
            WS_VISIBLE | WS_CHILD,
            50, 390, 300, 20, hWnd, (HMENU)6004, NULL, NULL);

        // Кнопки
        CreateWindow(L"BUTTON", L"Назад",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            50, 420, 100, 30, hWnd,
            (HMENU)600, NULL, NULL);

        CreateWindow(L"BUTTON", L"Рассчитать",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            160, 420, 100, 30, hWnd,
            (HMENU)601, NULL, NULL);

        CreateWindow(L"BUTTON", L"Купить",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            270, 420, 100, 30, hWnd,
            (HMENU)602, NULL, NULL);

        hStatusLabel = CreateWindow(L"STATIC", L"Введите данные для обмена",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 460, 300, 20, hWnd, (HMENU)6005, NULL, NULL);

        // Обновляем содержимое кошелька
        WalletManager::UpdateWalletContentList(hWalletContentList, wallet);

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

        if (g_currentWalletIndex < 0 || g_currentWalletIndex >= g_wallets.size()) {
            SetWindowText(hStatusLabel, L"Ошибка: кошелек не найден");
            break;
        }

        Wallet& wallet = g_wallets[g_currentWalletIndex];

        switch (buttonId) {
        case 600: // Назад
            DestroyWindow(hWnd);
            break;

        case 601: // Рассчитать
        {
            wchar_t sellCurrency[10], buyCurrency[10], amount[20];
            GetWindowText(hSellCurrencyEdit, sellCurrency, 10);
            GetWindowText(hBuyCurrencyEdit, buyCurrency, 10);
            GetWindowText(hAmountEdit, amount, 20);

            std::wstring sellStr = sellCurrency;
            std::wstring buyStr = buyCurrency;
            std::wstring amountStr = amount;

            if (sellStr.empty() || buyStr.empty() || amountStr.empty()) {
                SetWindowText(hStatusLabel, L"Заполните все поля!");
                break;
            }

            if (!IsCurrencySupported(sellStr) || !IsCurrencySupported(buyStr)) {
                SetWindowText(hStatusLabel, L"Одна из валют не поддерживается!");
                break;
            }

            bool hasCurrency = false;
            for (const auto& currency : wallet.currencies) {
                if (currency.currency == sellStr) {
                    hasCurrency = true;
                    break;
                }
            }

            if (!hasCurrency) {
                SetWindowText(hStatusLabel, L"У вас нет такой валюты в кошельке!");
                break;
            }

            try {
                double amountValue = std::stod(amountStr);
                if (amountValue <= 0) {
                    SetWindowText(hStatusLabel, L"Количество должно быть больше 0!");
                    break;
                }

                double exchangeRate = APIClient::GetCurrencyRate(sellStr, buyStr);
                if (exchangeRate <= 0.0) {
                    SetWindowText(hStatusLabel, L"Ошибка получения курса!");
                    break;
                }

                double result = amountValue * exchangeRate;

                std::wstring rateText = L"Курс: 1 " + sellStr + L" = " +
                    std::to_wstring(exchangeRate) + L" " + buyStr;
                SetWindowText(hExchangeRateLabel, rateText.c_str());

                std::wstring resultText = L"Вы получите: " + std::to_wstring(result) + L" " + buyStr;
                SetWindowText(hResultLabel, resultText.c_str());

                SetWindowText(hStatusLabel, L"Расчет завершен!");

            }
            catch (...) {
                SetWindowText(hStatusLabel, L"Ошибка в количестве!");
            }
        }
        break;

        case 602: // Купить (совершить транзакцию)
        {
            wchar_t sellCurrency[10], buyCurrency[10], amount[20];
            GetWindowText(hSellCurrencyEdit, sellCurrency, 10);
            GetWindowText(hBuyCurrencyEdit, buyCurrency, 10);
            GetWindowText(hAmountEdit, amount, 20);

            std::wstring sellStr = sellCurrency;
            std::wstring buyStr = buyCurrency;
            std::wstring amountStr = amount;

            if (sellStr.empty() || buyStr.empty() || amountStr.empty()) {
                SetWindowText(hStatusLabel, L"Заполните все поля!");
                break;
            }

            try {
                double amountValue = std::stod(amountStr);
                if (amountValue <= 0) {
                    SetWindowText(hStatusLabel, L"Количество должно быть больше 0!");
                    break;
                }

                double availableAmount = 0.0;
                int sellCurrencyIndex = -1;

                for (size_t i = 0; i < wallet.currencies.size(); i++) {
                    if (wallet.currencies[i].currency == sellStr) {
                        availableAmount = wallet.currencies[i].amount;
                        sellCurrencyIndex = i;
                        break;
                    }
                }

                if (sellCurrencyIndex == -1) {
                    SetWindowText(hStatusLabel, L"У вас нет такой валюты в кошельке!");
                    break;
                }

                if (availableAmount < amountValue) {
                    SetWindowText(hStatusLabel, L"Недостаточно валюты для продажи!");
                    break;
                }

                double exchangeRate = APIClient::GetCurrencyRate(sellStr, buyStr);
                if (exchangeRate <= 0.0) {
                    SetWindowText(hStatusLabel, L"Ошибка получения курса!");
                    break;
                }

                double boughtAmount = amountValue * exchangeRate;

                std::vector<WalletCurrency> walletStateBefore = WalletManager::GetWalletState(wallet);

                wallet.currencies[sellCurrencyIndex].amount -= amountValue;

                if (wallet.currencies[sellCurrencyIndex].amount <= 0) {
                    wallet.currencies.erase(wallet.currencies.begin() + sellCurrencyIndex);
                }

                bool currencyExists = false;
                for (auto& currency : wallet.currencies) {
                    if (currency.currency == buyStr) {
                        currency.amount += boughtAmount;
                        currencyExists = true;
                        break;
                    }
                }

                if (!currencyExists) {
                    WalletCurrency newCurrency;
                    newCurrency.currency = buyStr;
                    newCurrency.amount = boughtAmount;
                    wallet.currencies.push_back(newCurrency);
                }

                WalletManager::SaveWallets();

                Transaction transaction;
                transaction.date = GetCurrentDateTime();
                transaction.soldCurrency = sellStr;
                transaction.soldAmount = amountValue;
                transaction.boughtCurrency = buyStr;
                transaction.boughtAmount = boughtAmount;
                transaction.exchangeRate = exchangeRate;
                transaction.walletState = WalletManager::GetWalletState(wallet);

                TransactionManager::SaveTransaction(wallet, transaction);

                WalletManager::UpdateWalletContentList(hWalletContentList, wallet);

                std::wstring successMsg = L"Транзакция завершена! Куплено: " +
                    std::to_wstring(boughtAmount) + L" " + buyStr;
                SetWindowText(hStatusLabel, successMsg.c_str());

                SetWindowText(hSellCurrencyEdit, L"");
                SetWindowText(hBuyCurrencyEdit, L"");
                SetWindowText(hAmountEdit, L"");
                SetWindowText(hExchangeRateLabel, L"Курс: -");
                SetWindowText(hResultLabel, L"Вы получите: -");

            }
            catch (...) {
                SetWindowText(hStatusLabel, L"Ошибка при выполнении транзакции!");
            }
        }
        break;
        }
    }
    break;

    case WM_DESTROY:
        g_currentWalletIndex = -1;
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}