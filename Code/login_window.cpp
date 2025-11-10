#include "login_window.h"
#include "main_window.h"
#include "file_manager.h"
#include "wallet_manager.h"
#include "common.h"

LRESULT CALLBACK LoginWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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

void LoginWindow::OnCreate(HWND hWnd) {
    CreateWindow(L"STATIC", L"Система авторизации",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        50, 20, 200, 30, hWnd, NULL, NULL, NULL);

    CreateWindow(L"STATIC", L"Логин:",
        WS_VISIBLE | WS_CHILD,
        50, 70, 80, 20, hWnd, NULL, NULL, NULL);

    CreateWindow(L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        120, 70, 150, 20, hWnd, (HMENU)1000, NULL, NULL);

    CreateWindow(L"STATIC", L"Пароль:",
        WS_VISIBLE | WS_CHILD,
        50, 100, 80, 20, hWnd, NULL, NULL, NULL);

    CreateWindow(L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD,
        120, 100, 150, 20, hWnd, (HMENU)1001, NULL, NULL);

    CreateWindow(L"BUTTON", L"Войти",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        50, 140, 100, 30, hWnd,
        (HMENU)100, NULL, NULL);

    CreateWindow(L"BUTTON", L"Зарегистрироваться",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        160, 140, 120, 30, hWnd,
        (HMENU)101, NULL, NULL);

    CreateWindow(L"BUTTON", L"Войти как гость",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        50, 180, 230, 30, hWnd,
        (HMENU)102, NULL, NULL);

    CreateWindow(L"STATIC", L"Введите данные для входа",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        50, 220, 230, 20, hWnd, (HMENU)1002, NULL, NULL);

    // Установка шрифтов
    HFONT hTitleFont = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
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
                wchar_t text[100];
                GetWindowText(hChild, text, 100);
                if (wcscmp(text, L"Система авторизации") == 0) {
                    SendMessage(hChild, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
                }
                else {
                    SendMessage(hChild, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
                }
            }
            else {
                SendMessage(hChild, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
            }
            hChild = GetNextWindow(hChild, GW_HWNDNEXT);
        }
    }
}

void LoginWindow::OnCommand(HWND hWnd, WPARAM wParam) {
    int buttonId = LOWORD(wParam);

    wchar_t username[100];
    wchar_t password[100];
    GetWindowText(GetDlgItem(hWnd, 1000), username, 100);
    GetWindowText(GetDlgItem(hWnd, 1001), password, 100);

    std::wstring usernameStr = username;
    std::wstring passwordStr = password;

    switch (buttonId) {
    case 100: // Войти
        HandleLogin(hWnd, usernameStr, passwordStr);
        break;
    case 101: // Зарегистрироваться
        HandleRegister(hWnd, usernameStr, passwordStr);
        break;
    case 102: // Войти как гость
        HandleGuestLogin(hWnd);
        break;
    }
}

void LoginWindow::OnDestroy(HWND hWnd) {
    if (g_currentUser.empty()) {
        PostQuitMessage(0);
    }
}

void LoginWindow::HandleLogin(HWND hWnd, const std::wstring& username, const std::wstring& password) {
    if (username.empty() || password.empty()) {
        SetWindowText(GetDlgItem(hWnd, 1002), L"Заполните все поля!");
        return;
    }

    if (FileManager::CheckUserCredentials(username, password)) {
        g_currentUser = username;
        FileManager::LoadCurrencyPairs();
        WalletManager::LoadWallets();
        SetWindowText(GetDlgItem(hWnd, 1002), L"Успешный вход!");

        DestroyWindow(hWnd);
        HWND hMainWindow = CreateWindow(
            L"MainMenuClass",
            L"Главное меню",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 500, 500,
            NULL, NULL, g_hInstance, NULL
        );
        ShowWindow(hMainWindow, SW_SHOW);
    }
    else {
        SetWindowText(GetDlgItem(hWnd, 1002), L"Неверный логин или пароль!");
    }
}

void LoginWindow::HandleRegister(HWND hWnd, const std::wstring& username, const std::wstring& password) {
    if (username.empty() || password.empty()) {
        SetWindowText(GetDlgItem(hWnd, 1002), L"Заполните все поля!");
        return;
    }

    if (FileManager::UserExists(username)) {
        SetWindowText(GetDlgItem(hWnd, 1002), L"Пользователь уже существует!");
        return;
    }

    if (FileManager::SaveUserToFile(username, password)) {
        g_currentUser = username;
        FileManager::LoadCurrencyPairs();
        WalletManager::LoadWallets();
        SetWindowText(GetDlgItem(hWnd, 1002), L"Регистрация успешна!");

        DestroyWindow(hWnd);
        HWND hMainWindow = CreateWindow(
            L"MainMenuClass",
            L"Главное меню",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 500, 500,
            NULL, NULL, g_hInstance, NULL
        );
        ShowWindow(hMainWindow, SW_SHOW);
    }
    else {
        SetWindowText(GetDlgItem(hWnd, 1002), L"Ошибка регистрации!");
    }
}

void LoginWindow::HandleGuestLogin(HWND hWnd) {
    g_currentUser = L"Гость";
    FileManager::LoadCurrencyPairs();
    WalletManager::LoadWallets();
    SetWindowText(GetDlgItem(hWnd, 1002), L"Вход как гость!");

    DestroyWindow(hWnd);
    HWND hMainWindow = CreateWindow(
        L"MainMenuClass",
        L"Главное меню (Гость)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 500,
        NULL, NULL, g_hInstance, NULL
    );
    ShowWindow(hMainWindow, SW_SHOW);
}