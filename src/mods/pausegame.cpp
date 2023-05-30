#include "moddef.h"

static uintptr_t scanAddr = 0;
static uint8_t oldBytes[1];

MOD_LOAD(PauseGame) {
    if (scanAddr == 0) {
        uint16_t pattern[] = {
            0x80, 0xBB, 0x28, 0x01, 0x00, 0x00, 0x00, 0x0F,
            0x84,
        };
        auto addr = ModUtils::sigScan(pattern, countof(pattern));
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
