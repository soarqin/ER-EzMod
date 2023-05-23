#include "moddef.h"

static uintptr_t scanAddr = 0;
static uint8_t oldBytes[2];

MOD_LOAD(SkipIntro) {
    uint16_t pattern[] = {
        0xc6, MASKED, MASKED, MASKED, MASKED, MASKED, 0x01, MASKED,
        0x03, 0x00, 0x00, 0x00, MASKED, 0x8b, MASKED, 0xe8,
        MASKED, MASKED, MASKED, MASKED, 0xe9, MASKED, MASKED, MASKED,
        MASKED, MASKED, 0x8d};
    uint8_t newBytes[] = {0x90, 0x90};
    scanAddr = ModUtils::scanAndPatch(pattern, countof(pattern), -60, newBytes, sizeof(newBytes), oldBytes);
}

MOD_UNLOAD(SkipIntro) {
    if (scanAddr == 0) return;
    ModUtils::memCopySafe(scanAddr, (uintptr_t)oldBytes, 5);
    scanAddr = 0;
}
