#include "api_client.h"
#include <winhttp.h>

double APIClient::GetCurrencyRate(const std::wstring& from, const std::wstring& to) {
    if (from == to) return 1.0;

    if (!IsCurrencySupported(from) || !IsCurrencySupported(to)) {
        return 0.0;
    }

    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    std::string response;

    std::wstring fromUpper = from;
    std::wstring toUpper = to;
    std::transform(fromUpper.begin(), fromUpper.end(), fromUpper.begin(), ::towupper);
    std::transform(toUpper.begin(), toUpper.end(), toUpper.begin(), ::towupper);

    hSession = WinHttpOpen(L"Currency App",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return 0.0;

    hConnect = WinHttpConnect(hSession, L"api.frankfurter.app",
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return 0.0;
    }

    std::wstring path = L"/latest?from=" + fromUpper;
    hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
        NULL, NULL, NULL, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 0.0;
    }

    DWORD timeout = 10000;
    WinHttpSetTimeouts(hRequest, timeout, timeout, timeout, timeout);

    BOOL requestSent = WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0);
    if (!requestSent) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 0.0;
    }

    BOOL responseReceived = WinHttpReceiveResponse(hRequest, NULL);
    if (!responseReceived) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 0.0;
    }

    char buffer[4096];
    DWORD bytesRead = 0;

    while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        response += buffer;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (response.empty()) {
        return 0.0;
    }

    std::string toStr(toUpper.begin(), toUpper.end());
    std::string searchStr = "\"" + toStr + "\":";

    size_t pos = response.find(searchStr);
    if (pos != std::string::npos) {
        size_t valueStart = response.find(":", pos) + 1;
        size_t valueEnd = response.find_first_of(",}", valueStart);
        if (valueEnd == std::string::npos) {
            valueEnd = response.length();
        }

        std::string valueStr = response.substr(valueStart, valueEnd - valueStart);
        valueStr.erase(std::remove(valueStr.begin(), valueStr.end(), ' '), valueStr.end());
        valueStr.erase(std::remove(valueStr.begin(), valueStr.end(), '\"'), valueStr.end());

        try {
            double rate = std::stod(valueStr);
            return (rate > 0.0) ? rate : 0.0;
        }
        catch (...) {
            return 0.0;
        }
    }

    return 0.0;
}

bool APIClient::CheckCurrencyPairExists(const std::wstring& from, const std::wstring& to) {
    if (!IsCurrencySupported(from)) {
        return false;
    }
    if (!IsCurrencySupported(to)) {
        return false;
    }

    double rate = GetCurrencyRate(from, to);
    return rate > 0.0;
}