#include "moddef.h"

static uintptr_t scanAddr = 0;
static uint8_t oldBytes[5];

MOD_LOAD(RemoveVignette) {
    if (scanAddr == 0) {
        uint16_t pattern[] = {
            0xf3, 0x0f, 0x10, MASKED, 0x50, 0xf3, 0x0f, 0x59,
            MASKED, MASKED, MASKED, MASKED, MASKED, 0xe8, MASKED, MASKED,
            MASKED, MASKED, 0xf3, MASKED, 0x0f, 0x5c, MASKED, 0xf3,
            MASKED, 0x0f, 0x59, MASKED, MASKED, 0x8d, MASKED, MASKED,
            0xa0, 0x00, 0x00, 0x00};
        auto addr = ModUtils::sigScan(pattern, countof(pattern));
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
