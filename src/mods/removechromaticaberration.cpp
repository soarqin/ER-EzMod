#include "moddef.h"

static uintptr_t scanAddr = 0;
static uint8_t oldBytes[4];

MOD_LOAD(RemoveChromaticAberration) {
    if (scanAddr == 0) {
        uint16_t pattern[] = {
            0x0f, 0x11, MASKED, 0x60, MASKED, 0x8d, MASKED, 0x80,
            0x00, 0x00, 0x00, 0x0f, 0x10, MASKED, 0xa0, 0x00,
            0x00, 0x00, 0x0f, 0x11, MASKED, 0xf0, MASKED, 0x8d,
            MASKED, 0xb0, 0x00, 0x00, 0x00, 0x0f, 0x10, MASKED,
            0x0f, 0x11, MASKED, 0x0f, 0x10, MASKED, 0x10};
        auto addr = ModUtils::sigScan(pattern, countof(pattern));
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
