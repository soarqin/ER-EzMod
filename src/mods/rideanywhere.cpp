#include "moddef.h"

MOD_BEGIN(RideAnywhere)
    uint16_t pattern[] = {
        0x80, MASKED, MASKED, MASKED, MASKED, MASKED, MASKED, 0x48,
        MASKED, MASKED, MASKED, MASKED, 0x48, MASKED, MASKED, MASKED,
        MASKED, MASKED, MASKED, MASKED, MASKED, 0x40, MASKED, MASKED,
        MASKED, MASKED, MASKED, 0x44, MASKED, MASKED, MASKED, MASKED,
        MASKED, MASKED, MASKED, 0x48, MASKED, MASKED, MASKED, MASKED,
        MASKED, MASKED, MASKED, MASKED, 0xE8, MASKED, MASKED, MASKED,
        MASKED, 0x48, PATTERN_END};
    uint8_t newBytes[] = {
        // mov byte ptr [rcx+36],00
        // mov al,0
        // nop
        0xC6, 0x41, 0x36, 0x00, 0xB0, 0x00, 0x90
    };
    ModUtils::scanAndPatch(pattern, 0, newBytes, sizeof(newBytes));
MOD_END(RideAnywhere)
