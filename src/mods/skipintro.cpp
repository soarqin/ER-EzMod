#include "moddef.h"

MOD_DEF(SkipIntro) {
    uint16_t pattern[] = {
        0xc6, MASKED, MASKED, MASKED, MASKED, MASKED, 0x01, MASKED,
        0x03, 0x00, 0x00, 0x00, MASKED, 0x8b, MASKED, 0xe8,
        MASKED, MASKED, MASKED, MASKED, 0xe9, MASKED, MASKED, MASKED,
        MASKED, MASKED, 0x8d};
    uint8_t newBytes[] = {0x90, 0x90};
    uint8_t oldBytes[sizeof(newBytes)];
    ModUtils::scanAndPatch(pattern, countof(pattern), -60, newBytes, sizeof(newBytes), oldBytes);
}
