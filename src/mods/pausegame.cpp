#include "moddef.h"

static uintptr_t scanAddr = 0;
static uint8_t oldBytes[1];

MOD_LOAD(PauseGame) {
    if (scanAddr == 0) {
        auto addr = ModUtils::sigScan(
            "80 BB 28 01 00 00 00 0F"
            "84");
        if (addr == 0) return;
        scanAddr = addr;
    }
    uint8_t newBytes[] = {0x01};
    ModUtils::patch(scanAddr + 6, newBytes, sizeof(newBytes), oldBytes);
}

MOD_UNLOAD(PauseGame) {
    if (scanAddr == 0) return;
    ModUtils::memCopySafe(scanAddr, (uintptr_t)oldBytes, 5);
}
