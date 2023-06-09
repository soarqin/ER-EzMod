#include "moddef.h"

static uintptr_t scanAddr = 0;

MOD_LOAD(UnlockFPS) {
    if (scanAddr == 0) {
        auto addr = ModUtils::sigScan(
            "48 8b 0d ?? ?? ?? ?? 80"
            "bb d7 00 00 00 00 0f 84"
            "ce 00 00 00 48 85 c9 75"
            "2e");
        if (addr == 0) return;
        addr = addr + 0x07 + *(uint32_t *)(addr + 0x03);
        for (;;) {
            uintptr_t checkAddr = *(uintptr_t *)addr;
            if (checkAddr != 0) {
                addr = checkAddr;
                break;
            }
            Sleep(1000);
        }
        scanAddr = addr;
    }
    auto fps = configByFloat("fps");
    ModUtils::log(L"%p", scanAddr);
    *(float*)(scanAddr + 0x2CC) = fps;
    *(uint8_t*)(scanAddr + 0x2D0) = 1;
    ModUtils::log(L"FPS limit unlocked to %g", fps);
}

MOD_UNLOAD(UnlockFPS) {
    if (scanAddr == 0) return;
    *(uint8_t*)(scanAddr + 0x2D0) = 0;
    *(float*)(scanAddr + 0x2CC) = 30.0f;
}
