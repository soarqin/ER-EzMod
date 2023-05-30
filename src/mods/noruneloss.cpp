#include "moddef.h"

static uintptr_t scanAddr = 0;
static uint8_t oldBytes[5];

MOD_LOAD(NoRuneLoss) {
    if (scanAddr == 0) {
        uint16_t pattern[] = {
            0xb0, 0x01, MASKED, 0x8b, MASKED, 0xe8, MASKED, MASKED,
            MASKED, MASKED, MASKED, 0x8b, MASKED, MASKED, MASKED, 0x32,
            0xc0, MASKED, 0x83, MASKED, 0x28, 0xc3};
        auto addr = ModUtils::sigScan(pattern, countof(pattern));
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
