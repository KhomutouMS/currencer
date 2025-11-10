#ifndef LOGIN_WINDOW_H
#define LOGIN_WINDOW_H

#include "common.h"

class LoginWindow {
public:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void CreateLoginWindow(HINSTANCE hInstance);

private:
    static void OnCreate(HWND hWnd);
    static void OnCommand(HWND hWnd, WPARAM wParam);
    static void OnDestroy(HWND hWnd);
    static void HandleLogin(HWND hWnd, const std::wstring& username, const std::wstring& password);
    static void HandleRegister(HWND hWnd, const std::wstring& username, const std::wstring& password);
    static void HandleGuestLogin(HWND hWnd);
};

#endif