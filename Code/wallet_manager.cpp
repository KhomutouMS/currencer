#include "wallet_manager.h"
#include "common.h"

void WalletManager::LoadWallets() {
    g_wallets.clear();

    if (g_currentUser == L"Гость") return;

    std::wstring filename = GetUserCurrencyFile() + L"_wallets.txt";
    std::wifstream file(filename);
    if (!file.is_open()) {
        return;
    }

    file.imbue(std::locale(""));

    std::wstring line;
    Wallet currentWallet;
    bool readingWallet = false;

    while (std::getline(file, line)) {
        if (line.find(L"WALLET:") == 0) {
            if (readingWallet && !currentWallet.name.empty()) {
                g_wallets.push_back(currentWallet);
            }
            currentWallet = Wallet();
            currentWallet.name = line.substr(7);
            readingWallet = true;
        }
        else if (line.find(L"CURRENCY:") == 0) {
            size_t firstColon = 9;
            size_t secondColon = line.find(L':', firstColon);
            if (secondColon != std::wstring::npos) {
                std::wstring currencyStr = line.substr(firstColon, secondColon - firstColon);
                std::wstring amountStr = line.substr(secondColon + 1);

                WalletCurrency wc;
                wc.currency = currencyStr;
                try {
                    wc.amount = std::stod(amountStr);
                    currentWallet.currencies.push_back(wc);
                }
                catch (...) {
                    // Игнорируем ошибки парсинга чисел
                }
            }
        }
        else if (line == L"ENDWALLET") {
            if (readingWallet && !currentWallet.name.empty()) {
                g_wallets.push_back(currentWallet);
            }
            readingWallet = false;
            currentWallet = Wallet();
        }
    }

    if (readingWallet && !currentWallet.name.empty()) {
        g_wallets.push_back(currentWallet);
    }

    file.close();
}

void WalletManager::SaveWallets() {
    if (g_currentUser == L"Гость") return;

    std::wstring filename = GetUserCurrencyFile() + L"_wallets.txt";
    std::wofstream file(filename);
    if (!file.is_open()) return;

    file.imbue(std::locale(""));

    for (const auto& wallet : g_wallets) {
        file << L"WALLET:" << wallet.name << std::endl;
        for (const auto& currency : wallet.currencies) {
            file << L"CURRENCY:" << currency.currency << L":" << currency.amount << std::endl;
        }
        file << L"ENDWALLET" << std::endl;
    }
    file.close();
}

void WalletManager::AddWallet(const Wallet& wallet) {
    g_wallets.push_back(wallet);
    SaveWallets();
}

void WalletManager::RemoveWallet(int index) {
    if (index >= 0 && index < g_wallets.size()) {
        g_wallets.erase(g_wallets.begin() + index);
        SaveWallets();
    }
}

std::wstring WalletManager::GenerateWalletName() {
    int walletNumber = g_wallets.size() + 1;
    return L"Кошелек" + std::to_wstring(walletNumber);
}

std::vector<WalletCurrency> WalletManager::GetWalletState(const Wallet& wallet) {
    return wallet.currencies;
}

void WalletManager::UpdateWalletsList(HWND hListBox) {
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

void WalletManager::UpdateWalletContentList(HWND hListBox, const Wallet& wallet) {
    if (!hListBox) return;

    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

    if (wallet.currencies.empty()) {
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"Кошелек пуст");
        return;
    }

    for (const auto& currency : wallet.currencies) {
        std::wstring displayText = currency.currency + L": " +
            std::to_wstring(currency.amount);
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)displayText.c_str());
    }
}

void WalletManager::UpdateCurrentCurrenciesList(HWND hListBox, const std::vector<WalletCurrency>& currencies) {
    if (!hListBox) return;

    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

    if (currencies.empty()) {
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"Валюты не добавлены");
        return;
    }

    for (const auto& currency : currencies) {
        std::wstring displayText = currency.currency + L": " + std::to_wstring(currency.amount);
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)displayText.c_str());
    }
}