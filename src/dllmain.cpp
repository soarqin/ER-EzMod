#include "mods/moddef.h"
#include <Windows.h>

LRESULT wndHookproc(int code, WPARAM wParam, LPARAM lParam) {
    if (code != HC_ACTION) return CallNextHookEx(nullptr, code, wParam, lParam);
    if (HIWORD(lParam) & KF_UP) {
        Mods::modList.checkKeyPress((uint32_t)wParam);
    }
    return 0;
}

DWORD WINAPI MainThread(LPVOID) {
    Mods::modList.loadAll();
    uint32_t thId = 0;
    ModUtils::getWindowHandle(&thId);
    auto hook = SetWindowsHookEx(WH_KEYBOARD, &wndHookproc, nullptr, thId);
    if (hook == nullptr) {
        ModUtils::log(L"Failed to set hook: %08X", GetLastError());
    } else {
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            if (msg.message == WM_QUIT) break;
        }
        UnhookWindowsHookEx(hook);
    }
    ModUtils::closeLog();
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        CreateThread(nullptr, 0, &MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
