#include <windows.h>
#include <shellapi.h>
#include <stdio.h>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001

HHOOK hHook = NULL;
NOTIFYICONDATA nid = {0};
int ctrlPressed = 0, shiftPressed = 0, altPressed = 0, hotkeyArmed = 0;

#define MAX_LANGS 10

typedef struct {
    int vkCode;            // VK_1, VK_2, ...
    char layoutStr[10];    // e.g., "0000043f"
} LangHotkey;

LangHotkey altHotkeys[MAX_LANGS];
int altHotkeyCount = 0;

char ctrlShiftLayouts[2][10] = {"00000419", "00000409"}; // RU <-> EN
int currentCtrlShiftLayout = 0;

void LoadConfig() {
    FILE *f = fopen("langswitcher.cfg", "r");
    if (!f) return;

    char line[64];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "alt", 3) == 0) {
            int number;
            char layout[10];
            if (sscanf(line, "alt+%d=%8s", &number, layout) == 2 && number >= 1 && number <= 9 && altHotkeyCount < MAX_LANGS) {
                altHotkeys[altHotkeyCount].vkCode = 0x30 + number; // 0x31 — это '1', 0x39 — это '9'
                strcpy(altHotkeys[altHotkeyCount].layoutStr, layout);
                altHotkeyCount++;
            }
        } else if (strncmp(line, "ctrlshift", 9) == 0) {
            char layout1[10], layout2[10];
            if (sscanf(line, "ctrlshift=%8s,%8s", layout1, layout2) == 2) {
                strcpy(ctrlShiftLayouts[0], layout1);
                strcpy(ctrlShiftLayouts[1], layout2);
            }
        }
    }

    fclose(f);
}

void ActivateLanguage(const char* layoutStr) {
    HWND hForegroundWnd = GetForegroundWindow();
    if (!hForegroundWnd) return;

    HKL hkl = LoadKeyboardLayoutA(layoutStr, KLF_ACTIVATE);
    if (hkl) {
        PostMessage(hForegroundWnd, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)hkl);
    }
}

void SwitchLanguage() {
    currentCtrlShiftLayout = 1 - currentCtrlShiftLayout;
    ActivateLanguage(ctrlShiftLayouts[currentCtrlShiftLayout]);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            if (p->vkCode == VK_LCONTROL || p->vkCode == VK_RCONTROL) ctrlPressed = 1;
            if (p->vkCode == VK_LSHIFT || p->vkCode == VK_RSHIFT) shiftPressed = 1;
            if (p->vkCode == VK_LMENU || p->vkCode == VK_RMENU) altPressed = 1;

            if (ctrlPressed && shiftPressed && !hotkeyArmed) {
                hotkeyArmed = 1;
                SwitchLanguage();
            }

            if (altPressed) {
                for (int i = 0; i < altHotkeyCount; i++) {
                    if (p->vkCode == altHotkeys[i].vkCode) {
                        ActivateLanguage(altHotkeys[i].layoutStr);
                        break;
                    }
                }
            }
        } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            if (p->vkCode == VK_LCONTROL || p->vkCode == VK_RCONTROL) ctrlPressed = 0;
            if (p->vkCode == VK_LSHIFT || p->vkCode == VK_RSHIFT) shiftPressed = 0;
            if (p->vkCode == VK_LMENU || p->vkCode == VK_RMENU) altPressed = 0;

            if (!ctrlPressed || !shiftPressed)
                hotkeyArmed = 0;
        }
    }

    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_TRAYICON) {
        if (lParam == WM_RBUTTONUP) {
            HMENU menu = CreatePopupMenu();
            AppendMenu(menu, MF_STRING, ID_TRAY_EXIT, "Exit");
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            TrackPopupMenu(menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(menu);
        }
    }
    else if (msg == WM_COMMAND) {
        if (LOWORD(wParam) == ID_TRAY_EXIT) {
            PostQuitMessage(0);
        }
    }
    else if (msg == WM_DESTROY) {
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    LoadConfig();

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "LangSwitcherClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindow("LangSwitcherClass", "LangSwitcher", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (!hHook) return 1;

    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    lstrcpy(nid.szTip, "LangSwitcher (Ctrl+Shift, Alt+1-9)");

    Shell_NotifyIcon(NIM_ADD, &nid);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hHook);
    return 0;
}