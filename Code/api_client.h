#ifndef API_CLIENT_H
#define API_CLIENT_H

#include "common.h"

class APIClient {
public:
    static double GetCurrencyRate(const std::wstring& from, const std::wstring& to);
    static bool CheckCurrencyPairExists(const std::wstring& from, const std::wstring& to);

private:
    static std::string MakeHTTPRequest(const std::wstring& url);
    static double ParseExchangeRate(const std::string& response, const std::wstring& toCurrency);
};

#endif