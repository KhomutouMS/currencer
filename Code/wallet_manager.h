#ifndef WALLET_MANAGER_H
#define WALLET_MANAGER_H

#include "common.h"

class WalletManager {
public:
    static void LoadWallets();
    static void SaveWallets();
    static void AddWallet(const Wallet& wallet);
    static void RemoveWallet(int index);
    static std::wstring GenerateWalletName();
    static std::vector<WalletCurrency> GetWalletState(const Wallet& wallet);

    // UI helper functions
    static void UpdateWalletsList(HWND hListBox);
    static void UpdateWalletContentList(HWND hListBox, const Wallet& wallet);
    static void UpdateCurrentCurrenciesList(HWND hListBox, const std::vector<WalletCurrency>& currencies);
};

#endif