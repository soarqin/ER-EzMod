#include "moddef.h"

MOD_BEGIN(RemoveVignette)
    uint16_t pattern[] = {
        0xf3, 0x0f, 0x10, MASKED, 0x50, 0xf3, 0x0f, 0x59,
        MASKED, MASKED, MASKED, MASKED, MASKED, 0xe8, MASKED, MASKED,
        MASKED, MASKED, 0xf3, MASKED, 0x0f, 0x5c, MASKED, 0xf3,
        MASKED, 0x0f, 0x59, MASKED, MASKED, 0x8d, MASKED, MASKED,
        0xa0, 0x00, 0x00, 0x00, PATTERN_END};
    uint8_t newBytes[] = {0xf3, 0x0f, 0x5c, 0xc0, 0x90};
    ModUtils::scanAndPatch(pattern, 0x17, newBytes, sizeof(newBytes));
MOD_END(RemoveVignette)
