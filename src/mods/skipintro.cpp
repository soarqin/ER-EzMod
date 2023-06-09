#include "moddef.h"

static uintptr_t scanAddr = 0;
static uint8_t oldBytes[2];

MOD_LOAD(SkipIntro) {
    if (scanAddr == 0) {
        auto addr = ModUtils::sigScan(
            "c6 ?? ?? ?? ?? ?? 01 ??"
            "03 00 00 00 ?? 8b ?? e8"
            "?? ?? ?? ?? e9 ?? ?? ??"
            "?? ?? 8d");
        if (addr == 0) return;
        scanAddr = addr;
    }
    uint8_t newBytes[] = {0x90, 0x90};
    ModUtils::patch(scanAddr - 60, newBytes, sizeof(newBytes), oldBytes);
}

MOD_UNLOAD(SkipIntro) {
    if (scanAddr == 0) return;
    ModUtils::memCopySafe(scanAddr, (uintptr_t)oldBytes, 5);
}
