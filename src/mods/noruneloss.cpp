#include "moddef.h"

MOD_BEGIN(NoRuneLoss)
    uint16_t pattern[] = {
        0xb0, 0x01, MASKED, 0x8b, MASKED, 0xe8, MASKED, MASKED,
        MASKED, MASKED, MASKED, 0x8b, MASKED, MASKED, MASKED, 0x32,
        0xc0, MASKED, 0x83, MASKED, 0x28, 0xc3, PATTERN_END};
    uint8_t newBytes[] = {0x90, 0x90, 0x90, 0x90, 0x90};
    ModUtils::scanAndPatch(pattern, 5, newBytes, sizeof(newBytes));
MOD_END(NoRuneLoss)
