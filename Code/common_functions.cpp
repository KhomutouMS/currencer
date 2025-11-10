#include "common.h"

std::wstring GetUserCurrencyFile() {
    if (g_currentUser == L"Гость") return L"";

    std::string userStr(g_currentUser.begin(), g_currentUser.end());
    return std::wstring(userStr.begin(), userStr.end()) + L"_pairs.txt";
}

std::wstring GetCurrentDateTime() {
    time_t now = time(0);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);

    wchar_t buffer[80];
    wcsftime(buffer, sizeof(buffer), L"%Y-%m-%d %H:%M:%S", &timeinfo);
    return std::wstring(buffer);
}

bool IsCurrencySupported(const std::wstring& currency) {
    std::wstring upperCurrency = currency;
    std::transform(upperCurrency.begin(), upperCurrency.end(), upperCurrency.begin(), ::towupper);

    return std::find(supportedCurrencies.begin(), supportedCurrencies.end(), upperCurrency) != supportedCurrencies.end();
}

bool ParseNotificationCondition(const std::wstring& conditionStr,
    double& value1, double& value2, std::wstring& condition) {

    value1 = 0.0;
    value2 = 0.0;
    condition = L"";

    size_t opPos = conditionStr.find(L'>');
    if (opPos != std::wstring::npos) {
        condition = L">";
    }
    else {
        opPos = conditionStr.find(L'<');
        if (opPos != std::wstring::npos) {
            condition = L"<";
        }
        else {
            opPos = conditionStr.find(L'=');
            if (opPos != std::wstring::npos) {
                condition = L"=";
            }
            else {
                return false;
            }
        }
    }

    try {
        std::wstring val1Str = conditionStr.substr(0, opPos);
        std::wstring val2Str = conditionStr.substr(opPos + 1);

        value1 = std::stod(val1Str);
        value2 = std::stod(val2Str);

        return true;
    }
    catch (...) {
        return false;
    }
}