#include <windows.h>
#include <shellapi.h>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001

HHOOK hHook = NULL;
NOTIFYICONDATA nid = {0};
int ctrlPressed = 0, shiftPressed = 0, hotkeyArmed = 0;

void SwitchLanguage() {
    HWND hWnd = GetForegroundWindow();
    if (!hWnd) return;

    DWORD tid = GetWindowThreadProcessId(hWnd, NULL);
    HKL layout = GetKeyboardLayout(tid);
    LANGID langId = LOWORD(layout);

    const char* nextLayoutStr = (langId == 0x0419) ? "00000409" : "00000419";
    HKL nextLayout = LoadKeyboardLayoutA(nextLayoutStr, KLF_ACTIVATE);
    if (nextLayout)
        PostMessage(hWnd, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)nextLayout);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            if (p->vkCode == VK_CONTROL || p->vkCode == VK_LCONTROL || p->vkCode == VK_RCONTROL)
                ctrlPressed = 1;
            if (p->vkCode == VK_SHIFT || p->vkCode == VK_LSHIFT || p->vkCode == VK_RSHIFT)
                shiftPressed = 1;

            if (ctrlPressed && shiftPressed && !hotkeyArmed) {
                hotkeyArmed = 1;
                SwitchLanguage();
            }
        }
        else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            if (p->vkCode == VK_CONTROL || p->vkCode == VK_LCONTROL || p->vkCode == VK_RCONTROL)
                ctrlPressed = 0;
            if (p->vkCode == VK_SHIFT || p->vkCode == VK_LSHIFT || p->vkCode == VK_RSHIFT)
                shiftPressed = 0;

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
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "LangSwitcherClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindow("LangSwitcherClass", "Lang_switcher", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (!hHook) return 1;

    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101)); // 101 — ID иконки
    lstrcpy(nid.szTip, "Lang_switcher (Ctrl+Shift)");

    Shell_NotifyIcon(NIM_ADD, &nid);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hHook);
    return 0;
}
