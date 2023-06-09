#include "moddef.h"

static uintptr_t scanAddr = 0;
static uint8_t oldBytes[5];

MOD_LOAD(RemoveVignette) {
    if (scanAddr == 0) {
        auto addr = ModUtils::sigScan(
            "f3 0f 10 ?? 50 f3 0f 59"
            "?? ?? ?? ?? ?? e8 ?? ??"
            "?? ?? f3 ?? 0f 5c ?? f3"
            "?? 0f 59 ?? ?? 8d ?? ??"
            "a0 00 00 00");
        if (addr == 0) return;
        scanAddr = addr;
    }
    uint8_t newBytes[] = {0xf3, 0x0f, 0x5c, 0xc0, 0x90};
    ModUtils::patch(scanAddr + 0x17, newBytes, sizeof(newBytes), oldBytes);
}

MOD_UNLOAD(RemoveVignette) {
    if (scanAddr == 0) return;
    ModUtils::memCopySafe(scanAddr, (uintptr_t)oldBytes, 5);
}
