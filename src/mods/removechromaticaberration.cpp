#include "moddef.h"

MOD_LOAD(RemoveChromaticAberration) {
    uint16_t pattern[] = {
        0x0f, 0x11, MASKED, 0x60, MASKED, 0x8d, MASKED, 0x80,
        0x00, 0x00, 0x00, 0x0f, 0x10, MASKED, 0xa0, 0x00,
        0x00, 0x00, 0x0f, 0x11, MASKED, 0xf0, MASKED, 0x8d,
        MASKED, 0xb0, 0x00, 0x00, 0x00, 0x0f, 0x10, MASKED,
        0x0f, 0x11, MASKED, 0x0f, 0x10, MASKED, 0x10};
    uint8_t newBytes[] = {0x66, 0x0f, 0xef, 0xc9};
    uint8_t oldBytes[sizeof(newBytes)];
    ModUtils::scanAndPatch(pattern, countof(pattern), 0x2f, newBytes, sizeof(newBytes), oldBytes);
}

MOD_UNLOAD(RemoveChromaticAberration) {
}
