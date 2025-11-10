#include "main_window.h"
#include "currency_pairs_window.h"
#include "notifications_window.h"
#include "wallets_window.h"
#include "reports_window.h"
#include "api_client.h"
#include "common.h"

LRESULT CALLBACK MainWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        OnCreate(hWnd);
        break;
    case WM_COMMAND:
        OnCommand(hWnd, wParam);
        break;
    case WM_DESTROY:
        OnDestroy(hWnd);
        break;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

void MainWindow::OnCreate(HWND hWnd) {
    hMain = hWnd;
    CreateChildWindows(hWnd);

    // Устанавливаем шрифты
    HFONT hTitleFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH, L"Microsoft Sans Serif");
    HFONT hNormalFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH, L"Microsoft Sans Serif");

    if (hTitleFont && hNormalFont) {
        HWND hChild = GetWindow(hWnd, GW_CHILD);
        while (hChild) {
            wchar_t className[100];
            GetClassName(hChild, className, 100);
            if (wcscmp(className, L"Static") == 0) {
                SendMessage(hChild, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
            }
            else if (wcscmp(className, L"ListBox") == 0) {
                SendMessage(hChild, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
            }
            else {
                SendMessage(hChild, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
            }
            hChild = GetNextWindow(hChild, GW_HWNDNEXT);
        }
    }

    // Загружаем и отображаем курсы валют
    UpdateCurrencyRates(GetDlgItem(hWnd, 2000));
}

void MainWindow::CreateChildWindows(HWND hWnd) {
    // Приветствие
    std::wstring welcomeText = L"Добро пожаловать, " + g_currentUser + L"!";
    CreateWindow(L"STATIC", welcomeText.c_str(),
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        50, 20, 400, 30, hWnd, NULL, NULL, NULL);

    // Заголовок курсов валют
    CreateWindow(L"STATIC", L"Актуальные курсы валют:",
        WS_VISIBLE | WS_CHILD,
        50, 60, 200, 20, hWnd, NULL, NULL, NULL);

    // ListBox для отображения курсов
    CreateWindow(L"LISTBOX", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
        50, 85, 400, 200, hWnd, (HMENU)2000, NULL, NULL);

    // Кнопки меню
    CreateWindow(L"BUTTON", L"Добавить/удалить валютную пару",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        50, 300, 200, 40, hWnd,
        (HMENU)200, NULL, NULL);

    CreateWindow(L"BUTTON", L"Настройки уведомлений",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        260, 300, 200, 40, hWnd,
        (HMENU)201, NULL, NULL);

    CreateWindow(L"BUTTON", L"Кошельки",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        50, 350, 200, 40, hWnd,
        (HMENU)202, NULL, NULL);

    CreateWindow(L"BUTTON", L"Создать отчет",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        260, 350, 200, 40, hWnd,
        (HMENU)203, NULL, NULL);

    CreateWindow(L"BUTTON", L"Обновить курсы",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        50, 400, 200, 40, hWnd,
        (HMENU)204, NULL, NULL);

    CreateWindow(L"BUTTON", L"Выйти",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        260, 400, 200, 40, hWnd,
        (HMENU)205, NULL, NULL);
}

void MainWindow::OnCommand(HWND hWnd, WPARAM wParam) {
    int buttonId = LOWORD(wParam);
    switch (buttonId) {
    case 200: // Добавить/удалить валютную пару
    {
        HWND hPairsWindow = CreateWindow(
            L"CurrencyPairsClass",
            L"Управление валютными парами",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, 400, 500,
            hWnd, NULL, g_hInstance, NULL
        );
        ShowWindow(hPairsWindow, SW_SHOW);
    }
    break;

    case 201: // Настройки уведомлений
    {
        HWND hNotificationsWindow = CreateWindow(
            L"NotificationsWindowClass",
            L"Настройка уведомлений",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, 400, 450,
            hWnd, NULL, g_hInstance, NULL
        );
        ShowWindow(hNotificationsWindow, SW_SHOW);
    }
    break;

    case 202: // Кошельки
    {
        HWND hWalletsWindow = CreateWindow(
            L"WalletsWindowClass",
            L"Список Кошельков",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, 400, 550,
            hWnd, NULL, g_hInstance, NULL
        );
        ShowWindow(hWalletsWindow, SW_SHOW);
    }
    break;

    case 203: // Создать отчет
    {
        HWND hReportsWindow = CreateWindow(
            L"ReportsWindowClass",
            L"Окно отчетов",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, 400, 450,
            hWnd, NULL, g_hInstance, NULL
        );
        ShowWindow(hReportsWindow, SW_SHOW);
    }
    break;

    case 204: // Обновить курсы
        UpdateCurrencyRates(GetDlgItem(hWnd, 2000));
        MessageBox(hWnd, L"Курсы валют обновлены!", L"Обновление", MB_OK);
        break;

    case 205: // Выйти
        DestroyWindow(hWnd);
        break;
    }
}

void MainWindow::OnDestroy(HWND hWnd) {
    PostQuitMessage(0);
}

void MainWindow::UpdateCurrencyRates(HWND hListBox) {
    if (!hListBox) return;

    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

    for (const auto& pair : g_currencyPairs) {
        size_t dashPos = pair.find(L'-');
        if (dashPos != std::wstring::npos) {
            std::wstring from = pair.substr(0, dashPos);
            std::wstring to = pair.substr(dashPos + 1);

            double rate = APIClient::GetCurrencyRate(from, to);

            std::wstringstream ws;
            ws << std::fixed << std::setprecision(4);
            if (rate > 0.0) {
                ws << from << L" → " << to << L": " << rate;
            }
            else {
                ws << from << L" → " << to << L": Ошибка получения курса";
            }
            SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)ws.str().c_str());
        }
    }
}