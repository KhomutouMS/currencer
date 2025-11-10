#include "notifications_window.h"
#include "notification_manager.h"
#include "api_client.h"
#include "common.h"

LRESULT CALLBACK NotificationsWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hCurrency1Edit = NULL;
    static HWND hCurrency2Edit = NULL;
    static HWND hConditionEdit = NULL;
    static HWND hStatusLabel = NULL;
    static HWND hNotificationsList = NULL;

    switch (uMsg) {
    case WM_CREATE:
    {
        CreateWindow(L"STATIC", L"Настройка уведомлений:",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 20, 300, 30, hWnd, NULL, NULL, NULL);

        // Поле для валюты 1
        CreateWindow(L"STATIC", L"Валюта 1:",
            WS_VISIBLE | WS_CHILD,
            50, 60, 80, 20, hWnd, NULL, NULL, NULL);

        hCurrency1Edit = CreateWindow(L"EDIT", L"USD",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_UPPERCASE,
            140, 60, 100, 20, hWnd, (HMENU)4000, NULL, NULL);

        // Поле для валюты 2
        CreateWindow(L"STATIC", L"Валюта 2:",
            WS_VISIBLE | WS_CHILD,
            50, 90, 80, 20, hWnd, NULL, NULL, NULL);

        hCurrency2Edit = CreateWindow(L"EDIT", L"EUR",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_UPPERCASE,
            140, 90, 100, 20, hWnd, (HMENU)4001, NULL, NULL);

        // Поле для условия
        CreateWindow(L"STATIC", L"Условие (напр. 5>2):",
            WS_VISIBLE | WS_CHILD,
            50, 120, 120, 20, hWnd, NULL, NULL, NULL);

        hConditionEdit = CreateWindow(L"EDIT", L"1>1",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            180, 120, 100, 20, hWnd, (HMENU)4002, NULL, NULL);

        // Кнопки
        CreateWindow(L"BUTTON", L"Назад",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            50, 150, 80, 30, hWnd,
            (HMENU)400, NULL, NULL);

        CreateWindow(L"BUTTON", L"Создать",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            140, 150, 80, 30, hWnd,
            (HMENU)401, NULL, NULL);

        CreateWindow(L"BUTTON", L"Удалить",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            230, 150, 120, 30, hWnd,
            (HMENU)402, NULL, NULL);

        hStatusLabel = CreateWindow(L"STATIC", L"Введите условие в формате: 5>2",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            50, 230, 300, 20, hWnd, (HMENU)4003, NULL, NULL);

        // Список активных уведомлений
        CreateWindow(L"STATIC", L"Активные уведомления:",
            WS_VISIBLE | WS_CHILD,
            50, 260, 200, 20, hWnd, NULL, NULL, NULL);

        hNotificationsList = CreateWindow(L"LISTBOX", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
            50, 285, 300, 150, hWnd, (HMENU)403, NULL, NULL);

        // Загружаем уведомления
        NotificationManager::LoadNotifications();
        NotificationManager::UpdateNotificationsList(hNotificationsList);

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

        // Запускаем проверку уведомлений если есть активные
        bool hasActiveNotifications = false;
        for (const auto& notification : g_notifications) {
            if (notification.active) {
                hasActiveNotifications = true;
                break;
            }
        }

        if (hasActiveNotifications) {
            NotificationManager::StartNotificationChecker();
        }
    }
    break;

    case WM_COMMAND:
    {
        int buttonId = LOWORD(wParam);

        wchar_t currency1[10], currency2[10], condition[20];
        GetWindowText(hCurrency1Edit, currency1, 10);
        GetWindowText(hCurrency2Edit, currency2, 10);
        GetWindowText(hConditionEdit, condition, 20);

        std::wstring currency1Str = currency1;
        std::wstring currency2Str = currency2;
        std::wstring conditionStr = condition;

        switch (buttonId) {
        case 400: // Назад
            DestroyWindow(hWnd);
            break;

        case 401: // Создать
        {
            if (currency1Str.empty() || currency2Str.empty() || conditionStr.empty()) {
                SetWindowText(hStatusLabel, L"Заполните все поля!");
                break;
            }

            double value1 = 0.0;
            double value2 = 0.0;
            std::wstring conditionOp = L"";

            if (!ParseNotificationCondition(conditionStr, value1, value2, conditionOp)) {
                SetWindowText(hStatusLabel, L"Ошибка в формате условия! Используйте: 5>2");
                break;
            }

            if (!IsCurrencySupported(currency1Str)) {
                SetWindowText(hStatusLabel, L"Валюта 1 не поддерживается!");
                break;
            }
            if (!IsCurrencySupported(currency2Str)) {
                SetWindowText(hStatusLabel, L"Валюта 2 не поддерживается!");
                break;
            }

            Notification newNotification;
            newNotification.currency1 = currency1Str;
            newNotification.currency2 = currency2Str;
            newNotification.value1 = value1;
            newNotification.value2 = value2;
            newNotification.condition = conditionOp;
            newNotification.active = true;

            NotificationManager::AddNotification(newNotification);
            SetWindowText(hStatusLabel, L"Уведомление создано!");

            NotificationManager::UpdateNotificationsList(hNotificationsList);
            NotificationManager::StartNotificationChecker();
        }
        break;

        case 402: // Удалить
        {
            int index = SendMessage(hNotificationsList, LB_GETCURSEL, 0, 0);
            if (index != LB_ERR) {
                g_notifications.erase(g_notifications.begin() + index);
                NotificationManager::SaveNotifications();
                NotificationManager::UpdateNotificationsList(hNotificationsList);
                SetWindowText(hStatusLabel, L"Уведомление удалено!");
            }
            else {
                SetWindowText(hStatusLabel, L"Выберите уведомление для удаления!");
            }
        }
        break;

        case 403: // Выбор уведомления из списка
            if (HIWORD(wParam) == LBN_SELCHANGE) {
                int index = SendMessage(hNotificationsList, LB_GETCURSEL, 0, 0);
                if (index != LB_ERR && index < g_notifications.size()) {
                    const auto& notification = g_notifications[index];
                    SetWindowText(hCurrency1Edit, notification.currency1.c_str());
                    SetWindowText(hCurrency2Edit, notification.currency2.c_str());

                    std::wstring conditionDisplay = std::to_wstring((int)notification.value1) +
                        notification.condition +
                        std::to_wstring((int)notification.value2);
                    SetWindowText(hConditionEdit, conditionDisplay.c_str());
                }
            }
            break;
        }
    }
    break;

    case WM_DESTROY:
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}