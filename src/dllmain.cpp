#include "mods/moddef.h"
#include <Windows.h>

DWORD WINAPI MainThread(LPVOID) {
    Mods::modList.loadAll();
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        CreateThread(nullptr, 0, &MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
