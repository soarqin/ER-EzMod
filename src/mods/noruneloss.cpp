#include "moddef.h"

static uintptr_t scanAddr = 0;
static uint8_t oldBytes[5];

MOD_LOAD(NoRuneLoss) {
    if (scanAddr == 0) {
        auto addr = ModUtils::sigScan(
            "b0 01 ?? 8b ?? e8 ?? ??"
            "?? ?? ?? 8b ?? ?? ?? 32"
            "c0 ?? 83 ?? 28 c3");
        if (addr == 0) return;
        scanAddr = addr;
    }
    uint8_t newBytes[] = {0x90, 0x90, 0x90, 0x90, 0x90};
    ModUtils::patch(scanAddr + 5, newBytes, sizeof(newBytes), oldBytes);
}

MOD_UNLOAD(NoRuneLoss) {
    if (scanAddr == 0) return;
    ModUtils::memCopySafe(scanAddr, (uintptr_t)oldBytes, 5);
}
