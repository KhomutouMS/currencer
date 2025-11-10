#include "notification_manager.h"
#include "api_client.h"
#include "common.h"

void NotificationManager::LoadNotifications() {
    g_notifications.clear();

    if (g_currentUser == L"Гость") return;

    std::wstring filename = GetUserCurrencyFile() + L"_notifications.txt";
    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            std::stringstream ss(line);
            std::string token;
            std::vector<std::string> tokens;

            while (std::getline(ss, token, ',')) {
                tokens.push_back(token);
            }

            if (tokens.size() >= 6) {
                Notification notification;
                notification.currency1 = std::wstring(tokens[0].begin(), tokens[0].end());
                notification.currency2 = std::wstring(tokens[1].begin(), tokens[1].end());
                notification.value1 = std::stod(tokens[2]);
                notification.value2 = std::stod(tokens[3]);
                notification.condition = std::wstring(tokens[4].begin(), tokens[4].end());
                notification.active = (tokens[5] == "1");

                g_notifications.push_back(notification);
            }
        }
    }
    file.close();
}

void NotificationManager::SaveNotifications() {
    if (g_currentUser == L"Гость") return;

    std::wstring filename = GetUserCurrencyFile() + L"_notifications.txt";
    std::ofstream file(filename);
    if (!file.is_open()) return;

    for (const auto& notification : g_notifications) {
        std::string curr1(notification.currency1.begin(), notification.currency1.end());
        std::string curr2(notification.currency2.begin(), notification.currency2.end());
        std::string cond(notification.condition.begin(), notification.condition.end());

        file << curr1 << "," << curr2 << "," << notification.value1
            << "," << notification.value2 << "," << cond
            << "," << notification.active << std::endl;
    }
    file.close();
}

void NotificationManager::AddNotification(const Notification& notification) {
    g_notifications.push_back(notification);
    SaveNotifications();
}

bool NotificationManager::CheckNotificationCondition(const Notification& notification, double rate1, double rate2) {
    double actualRatio = (notification.value1 * rate1) / (notification.value2 * rate2);
    double targetRatio = 1.0;

    if (notification.condition == L">") {
        return actualRatio > targetRatio;
    }
    else if (notification.condition == L"<") {
        return actualRatio < targetRatio;
    }
    else if (notification.condition == L"=") {
        return std::abs(actualRatio - targetRatio) < 0.01;
    }

    return false;
}

void NotificationManager::NotificationCheckerThread() {
    while (g_notificationThreadRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(5));

        for (const auto& notification : g_notifications) {
            if (notification.active) {
                double rate1 = APIClient::GetCurrencyRate(L"USD", notification.currency1);
                double rate2 = APIClient::GetCurrencyRate(L"USD", notification.currency2);

                if (rate1 > 0.0 && rate2 > 0.0) {
                    if (CheckNotificationCondition(notification, rate1, rate2)) {
                        std::wstring message = L"Уведомление: " + notification.currency1 +
                            L" и " + notification.currency2 +
                            L" достигли заданного соотношения!";
                        MessageBox(NULL, message.c_str(), L"Валютное уведомление", MB_OK | MB_ICONINFORMATION);
                    }
                }
            }
        }
    }
}

void NotificationManager::StartNotificationChecker() {
    if (!g_notificationThreadRunning) {
        g_notificationThreadRunning = true;
        std::thread(NotificationCheckerThread).detach();
    }
}

void NotificationManager::StopNotificationChecker() {
    g_notificationThreadRunning = false;
}

void NotificationManager::UpdateNotificationsList(HWND hListBox) {
    if (!hListBox) return;

    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

    for (const auto& notification : g_notifications) {
        std::wstring displayText = notification.currency1 + L" " +
            std::to_wstring((int)notification.value1) +
            notification.condition +
            std::to_wstring((int)notification.value2) + L" " +
            notification.currency2 +
            (notification.active ? L" (активно)" : L" (неактивно)");

        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)displayText.c_str());
    }
}