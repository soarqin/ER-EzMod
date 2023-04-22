#include "moddef.h"

MOD_BEGIN(Insensible)
    uint16_t pattern[] = {
        0x74, MASKED, 0x44, 0x88, 0xb9, MASKED, MASKED, MASKED,
        MASKED, 0xe9, PATTERN_END};
    uint8_t newBytes[] = {0x90, 0x90};
    ModUtils::scanAndPatch(pattern, 0, newBytes, sizeof(newBytes));
MOD_END(Insensible)
