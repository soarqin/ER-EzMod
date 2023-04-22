#include "moddef.h"

MOD_BEGIN(SkipIntro)
    uint16_t pattern[] = {
        0xc6, MASKED, MASKED, MASKED, MASKED, MASKED, 0x01, MASKED,
        0x03, 0x00, 0x00, 0x00, MASKED, 0x8b, MASKED, 0xe8,
        MASKED, MASKED, MASKED, MASKED, 0xe9, MASKED, MASKED, MASKED,
        MASKED, MASKED, 0x8d, PATTERN_END};
    uint8_t newBytes[] = {0x90, 0x90};
    ModUtils::scanAndPatch(pattern, -60, newBytes, sizeof(newBytes));
MOD_END(SkipIntro)
