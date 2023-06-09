#include "moddef.h"

static uintptr_t scanAddr = 0;
static uint8_t oldBytes[4];

MOD_LOAD(RemoveChromaticAberration) {
    if (scanAddr == 0) {
        auto addr = ModUtils::sigScan(
            "0f 11 ?? 60 ?? 8d ?? 80"
            "00 00 00 0f 10 ?? a0 00"
            "00 00 0f 11 ?? f0 ?? 8d"
            "?? b0 00 00 00 0f 10 ??"
            "0f 11 ?? 0f 10 ?? 10");
        if (addr == 0) return;
        scanAddr = addr;
    }
    uint8_t newBytes[] = {0x66, 0x0f, 0xef, 0xc9};
    ModUtils::patch(scanAddr + 0x2f, newBytes, sizeof(newBytes), oldBytes);
}

MOD_UNLOAD(RemoveChromaticAberration) {
    if (scanAddr == 0) return;
    ModUtils::memCopySafe(scanAddr, (uintptr_t)oldBytes, 5);
}
