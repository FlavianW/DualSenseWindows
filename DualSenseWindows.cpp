#include <Windows.h>
#include <ds5w.h>
#include <tchar.h>
#include <Commdlg.h>
#include <gdiplus.h>

#pragma comment (lib,"Gdiplus.lib")



#define IDT_TIMER1 1
#define BTN_GAME 101 // Identifiant pour le bouton de jeu
#define BTN_CHANGE_COLOR 1001
#define BTN_GAME1 1002
#define BTN_GAME2 1003
#define BTN_GAME3 1004
#define BTN_GAME4 1005

ULONG_PTR gdiplusToken;
HINSTANCE g_hInstance;
HWND hButtonChangeColor, hButtonGame1, hButtonGame2, hButtonGame3, hButtonGame4;
COLORREF chosenColor = RGB(255, 255, 255);

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
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    g_hInstance = hInstance;

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
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        // Créer un timer pour vérifier l'état de la manette
        SetTimer(hwnd, IDT_TIMER1, 500, NULL);

        // Créer le bouton qui contiendra l'image
        HWND hButtonGame = CreateWindow(
            L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_BITMAP,
            0, 0, 100, 100, // Taille et position initiale (seront ajustées dans WM_SIZE)
            hwnd, (HMENU)BTN_GAME, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        // Supposons que 'chosenColor' est la couleur sélectionnée par l'utilisateur.

        Gdiplus::Bitmap* originalBitmap = Gdiplus::Bitmap::FromFile(L"../../Assets/rl.jpg");
        if (originalBitmap) {
            // Créez une nouvelle image bitmap avec la taille souhaitée pour le bouton
            Gdiplus::Bitmap* resizedBitmap = new Gdiplus::Bitmap(100, 100, originalBitmap->GetPixelFormat());

            // Obtenez un contexte graphique pour dessiner dans le bitmap redimensionné
            Gdiplus::Graphics graphics(resizedBitmap);
            graphics.Clear(Gdiplus::Color(GetRValue(chosenColor), GetGValue(chosenColor), GetBValue(chosenColor)));
            graphics.DrawImage(originalBitmap, 0, 0, 100, 100); // Dessinez l'image originale redimensionnée

            // Convertissez le bitmap redimensionné en HBITMAP
            HBITMAP hBitmap;
            resizedBitmap->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hBitmap); // Transparence si nécessaire

            // Utilisez hBitmap pour le bouton
            SendMessage(hButtonGame, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

            // Nettoyage
            delete originalBitmap;
            delete resizedBitmap;
        }
        else {
            MessageBox(hwnd, L"Échec du chargement de l'image", L"Erreur", MB_OK | MB_ICONERROR);
        }


        return 0;
    }

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
    case WM_COMMAND:
        if (LOWORD(wParam) == BTN_GAME) {
            CHOOSECOLOR cc = { sizeof(CHOOSECOLOR) };
            COLORREF acrCustClr[16]; // Tableau de couleurs personnalisées
            ZeroMemory(&cc, sizeof(cc));
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = hwnd;
            cc.lpCustColors = (LPDWORD)acrCustClr;
            cc.rgbResult = chosenColor;
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;

            if (ChooseColor(&cc) == TRUE) {
                chosenColor = cc.rgbResult;

                // Appliquer la couleur à la manette si elle est connectée
                if (isControllerConnected) {
                    DS5W::DS5OutputState outState = {};
                    outState.lightbar = { GetRValue(chosenColor), GetGValue(chosenColor), GetBValue(chosenColor) };
                    DS5W::setDeviceOutputState(&con, &outState);
                }
            }
        }
        break; // Ajouter break ici pour terminer correctement le cas

    return 0;

    case WM_SIZE:
    {
        int cxClient = LOWORD(lParam);
        int cyClient = HIWORD(lParam);

        // Positionner le bouton au centre
        HWND hButtonGame = GetDlgItem(hwnd, BTN_GAME);
        if (hButtonGame) {
            SetWindowPos(hButtonGame, NULL, (cxClient - 100) / 2, (cyClient - 100) / 2, 100, 100, SWP_NOZORDER);
        }
    }
    return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
