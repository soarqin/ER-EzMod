#include "moddef.h"

static uintptr_t scanAddr = 0;
static uintptr_t patchAddr = 0;
static uint8_t oldBytes[8];

MOD_LOAD(RuneRate) {
    if (scanAddr == 0) {
        auto addr = ModUtils::sigScan(
            "f3 0f 59 c1 f3 0f 2c f8"
            "48 8b 8b");
        if (addr == 0) return;
        scanAddr = addr;
    }
    if (patchAddr == 0) {
        uint8_t patchCodes[] = {
            // movss xmm0,[rip+0xD]
            0xf3, 0x0f, 0x10, 0x05, 0x0d, 0x00, 0x00, 0x00,
            // mulss xmm0,xmm1
            0xf3, 0x0f, 0x59, 0xc1,
            // cvttss2si edi,xmm0
            0xf3, 0x0f, 0x2c, 0xf8,
            // jmp addr+8
            0xe9, 0x00, 0x00, 0x00, 0x00,
            // rune rate (as float = 2.0f)
            0x00, 0x00, 0x00, 0x40
        };
        patchAddr = ModUtils::allocMemoryNear(scanAddr, sizeof(patchCodes));
        *(uint32_t *)&patchCodes[0x11] = (uint32_t)(scanAddr + 0x08 - (patchAddr + 0x10 + 5));
        auto mult = configByFloat("multiplier");
        if (mult != 0.0f) *(float *)&patchCodes[21] = mult;
        memcpy((void *)patchAddr, patchCodes, sizeof(patchCodes));
    }
    ModUtils::hookAsmManually(scanAddr, 8, patchAddr, oldBytes);
}

MOD_UNLOAD(RuneRate) {
    if (scanAddr == 0) return;
    ModUtils::memCopySafe(scanAddr, (uintptr_t)oldBytes, 7);
}
