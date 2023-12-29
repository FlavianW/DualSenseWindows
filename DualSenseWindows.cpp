#include <Windows.h>
#include <ds5w.h>
#include <tchar.h>

#define IDT_TIMER1 1

// Variables globales pour le contexte de la manette et l'état de connexion
DS5W::DeviceContext con;
bool isControllerConnected = false;

// Prototype de la procédure de fenêtre
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Nom de la classe de la fenêtre
const TCHAR CLASS_NAME[] = _T("DualSense Status Class");

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    // Enregistrer la classe de fenêtre
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    // Créer la fenêtre
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        _T("DualSense Status"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Boucle de messages
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Libération du contexte de la manette
    if (isControllerConnected) {
        DS5W::freeDeviceContext(&con);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        // Créer un timer pour vérifier l'état de la manette toutes les 500 ms
        SetTimer(hwnd, IDT_TIMER1, 500, NULL);
        return 0;

    case WM_TIMER:
        switch (wParam) {
        case IDT_TIMER1:
        {
            DS5W::DeviceEnumInfo infos[16];
            unsigned int controllersCount = 0;
            DS5W::enumDevices(infos, 16, &controllersCount);

            if (controllersCount > 0 && !isControllerConnected) {
                isControllerConnected = true;
                DS5W::initDeviceContext(&infos[0], &con);
                DS5W::DS5OutputState outState = {};
                outState.lightbar = { 255, 255, 255};
                DS5W::setDeviceOutputState(&con, &outState);
            }
            else if (controllersCount == 0 && isControllerConnected) {
                isControllerConnected = false;
                DS5W::freeDeviceContext(&con);
            }

            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, IDT_TIMER1);
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);
        HBRUSH backgroundBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
        FillRect(hdc, &rect, backgroundBrush);
        DeleteObject(backgroundBrush);

        const TCHAR* message = isControllerConnected ? _T("Manette connectée.") : _T("Manette déconnectée.");

        // Définir une nouvelle police si la manette est déconnectée
        if (!isControllerConnected) {
            HFONT hFont = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                DEFAULT_PITCH | FF_DONTCARE, _T("Arial"));
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
            SetRect(&rect, 0, 0, rect.right, rect.bottom);
            DrawText(hdc, message, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);
        }
        else {
            SetRect(&rect, 10, rect.bottom - 30, rect.right, rect.bottom);
            DrawText(hdc, message, -1, &rect, DT_LEFT | DT_BOTTOM | DT_SINGLELINE);
        }

        EndPaint(hwnd, &ps);
    }
    return 0;

    return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
