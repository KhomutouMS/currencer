#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "common.h"

class FileManager {
public:
    static bool SaveUserToFile(const std::wstring& username, const std::wstring& password);
    static bool CheckUserCredentials(const std::wstring& username, const std::wstring& password);
    static bool UserExists(const std::wstring& username);
    static void LoadCurrencyPairs();
    static void SaveCurrencyPairs();
    static void AddCurrencyPair(const std::wstring& pair);
    static void RemoveCurrencyPair(const std::wstring& pair);
};

#endif