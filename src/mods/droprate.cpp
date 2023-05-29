#include "moddef.h"

static uintptr_t scanAddr = 0;
static uintptr_t patchAddr = 0;
static uint8_t oldBytes[7];

MOD_LOAD(DropRate) {
    if (patchAddr == 0) {
        uint16_t pattern[] = {
            0x41, 0x0f, 0x28, 0xf8, 0x48, 0x85, 0xd2
        };
        uint8_t patchCodes[] = {
            // movss xmm7,[rip+0x8]
            0xf3, 0x0f, 0x10, 0x3d, 0x08, 0x00, 0x00, 0x00,
            // test rdx,rdx
            0x48, 0x85, 0xd2,
            // jmp addr+7
            0xe9, 0x00, 0x00, 0x00, 0x00,
            // drop rate (as float = 1000.0f)
            0x00, 0x00, 0x7A, 0x44
        };
        auto addr = ModUtils::sigScan(pattern, countof(pattern));
        if (addr == 0) return;
        auto mult = configByFloat("multiplier");
        if (mult != 0.0f) *(float *)&patchCodes[16] = mult;
        scanAddr = addr;
        patchAddr = ModUtils::allocMemoryNear(scanAddr, sizeof(patchCodes));
        *(uint32_t *)&patchCodes[0x0C] = (uint32_t)(scanAddr + 0x07 - (patchAddr + 0x0B + 5));
        memcpy((void *)patchAddr, patchCodes, sizeof(patchCodes));
    }
    ModUtils::hookAsmManually(scanAddr, 7, patchAddr, oldBytes);
}

MOD_UNLOAD(DropRate) {
    if (scanAddr == 0) return;
    ModUtils::memCopySafe(scanAddr, (uintptr_t)oldBytes, 7);
    scanAddr = 0;
    ModUtils::freeMemory(patchAddr);
    patchAddr = 0;
}
