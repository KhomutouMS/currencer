#ifndef NOTIFICATION_MANAGER_H
#define NOTIFICATION_MANAGER_H

#include "common.h"

class NotificationManager {
public:
    static void LoadNotifications();
    static void SaveNotifications();
    static void AddNotification(const Notification& notification);
    static void StartNotificationChecker();
    static void StopNotificationChecker();
    static void NotificationCheckerThread();
    static bool CheckNotificationCondition(const Notification& notification, double rate1, double rate2);
    static void UpdateNotificationsList(HWND hListBox);
};

#endif