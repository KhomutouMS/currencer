#include "file_manager.h"
#include "common.h"

bool FileManager::SaveUserToFile(const std::wstring& username, const std::wstring& password) {
    std::ofstream file("users.txt", std::ios::app);
    if (!file.is_open()) return false;

    std::string userStr(username.begin(), username.end());
    std::string passStr(password.begin(), password.end());
    file << userStr << ":" << passStr << std::endl;
    file.close();

    return true;
}

bool FileManager::CheckUserCredentials(const std::wstring& username, const std::wstring& password) {
    std::ifstream file("users.txt");
    if (!file.is_open()) return false;

    std::string line;
    std::string userStr(username.begin(), username.end());
    std::string passStr(password.begin(), password.end());
    std::string target = userStr + ":" + passStr;

    while (std::getline(file, line)) {
        if (line == target) {
            file.close();
            return true;
        }
    }

    file.close();
    return false;
}

bool FileManager::UserExists(const std::wstring& username) {
    std::ifstream file("users.txt");
    if (!file.is_open()) return false;

    std::string line;
    std::string userStr(username.begin(), username.end());
    std::string target = userStr + ":";

    while (std::getline(file, line)) {
        if (line.find(target) == 0) {
            file.close();
            return true;
        }
    }

    file.close();
    return false;
}

void FileManager::LoadCurrencyPairs() {
    g_currencyPairs.clear();

    if (g_currentUser == L"Гость") {
        g_currencyPairs = { L"USD-EUR", L"USD-GBP", L"USD-JPY" };
        return;
    }

    std::wstring filename = GetUserCurrencyFile();
    std::ifstream file(filename);
    if (!file.is_open()) {
        g_currencyPairs = { L"USD-EUR", L"USD-GBP", L"USD-JPY" };
        SaveCurrencyPairs();
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            g_currencyPairs.push_back(std::wstring(line.begin(), line.end()));
        }
    }
    file.close();
}

void FileManager::SaveCurrencyPairs() {
    if (g_currentUser == L"Гость") return;

    std::wstring filename = GetUserCurrencyFile();
    std::ofstream file(filename);
    if (!file.is_open()) return;

    for (const auto& pair : g_currencyPairs) {
        std::string pairStr(pair.begin(), pair.end());
        file << pairStr << std::endl;
    }
    file.close();
}

void FileManager::AddCurrencyPair(const std::wstring& pair) {
    if (std::find(g_currencyPairs.begin(), g_currencyPairs.end(), pair) == g_currencyPairs.end()) {
        g_currencyPairs.push_back(pair);
        SaveCurrencyPairs();
    }
}

void FileManager::RemoveCurrencyPair(const std::wstring& pair) {
    auto it = std::find(g_currencyPairs.begin(), g_currencyPairs.end(), pair);
    if (it != g_currencyPairs.end()) {
        g_currencyPairs.erase(it);
        SaveCurrencyPairs();
    }
}