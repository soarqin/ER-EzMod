#include "moddef.h"

static uintptr_t scanAddr = 0;
static uint8_t oldBytes[7];

MOD_LOAD(RideAnywhere) {
    if (scanAddr != 0) return;
    uint16_t pattern[] = {
        0x80, MASKED, MASKED, MASKED, MASKED, MASKED, MASKED, 0x48,
        MASKED, MASKED, MASKED, MASKED, 0x48, MASKED, MASKED, MASKED,
        MASKED, MASKED, MASKED, MASKED, MASKED, 0x40, MASKED, MASKED,
        MASKED, MASKED, MASKED, 0x44, MASKED, MASKED, MASKED, MASKED,
        MASKED, MASKED, MASKED, 0x48, MASKED, MASKED, MASKED, MASKED,
        MASKED, MASKED, MASKED, MASKED, 0xE8, MASKED, MASKED, MASKED,
        MASKED, 0x48};
    uint8_t newBytes[] = {
        // mov byte ptr [rcx+36],00
        // mov al,0
        // nop
        0xC6, 0x41, 0x36, 0x00, 0xB0, 0x00, 0x90
    };
    scanAddr = ModUtils::scanAndPatch(pattern, countof(pattern), 0, newBytes, sizeof(newBytes), oldBytes);
}

MOD_UNLOAD(RideAnywhere) {
    if (scanAddr == 0) return;
    ModUtils::memCopySafe(scanAddr, (uintptr_t)oldBytes, 5);
    scanAddr = 0;
}
