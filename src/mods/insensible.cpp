#include "moddef.h"

MOD_DEF(Insensible) {
    uint16_t pattern[] = {
        0x74, MASKED, 0x44, 0x88, 0xb9, MASKED, MASKED, MASKED,
        MASKED, 0xe9};
    uint8_t newBytes[] = {0x90, 0x90};
    uint8_t oldBytes[sizeof(newBytes)];
    ModUtils::scanAndPatch(pattern, countof(pattern), 0, newBytes, sizeof(newBytes), oldBytes);
}
