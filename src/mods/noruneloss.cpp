#include "moddef.h"

MOD_LOAD(NoRuneLoss) {
    uint16_t pattern[] = {
        0xb0, 0x01, MASKED, 0x8b, MASKED, 0xe8, MASKED, MASKED,
        MASKED, MASKED, MASKED, 0x8b, MASKED, MASKED, MASKED, 0x32,
        0xc0, MASKED, 0x83, MASKED, 0x28, 0xc3};
    uint8_t newBytes[] = {0x90, 0x90, 0x90, 0x90, 0x90};
    uint8_t oldBytes[sizeof(newBytes)];
    ModUtils::scanAndPatch(pattern, countof(pattern), 5, newBytes, sizeof(newBytes), oldBytes);
}

MOD_UNLOAD(NoRuneLoss) {
}
