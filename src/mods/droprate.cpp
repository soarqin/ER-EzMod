#include "moddef.h"

MOD_BEGIN(DropRate)
    uint16_t pattern[] = {
        0x41, 0x0f, 0x28, 0xf8, 0x48, 0x85, 0xd2, PATTERN_END
    };
    auto addr = ModUtils::sigScan(pattern);
    if (addr == 0) return;
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
    auto mult = configByFloat("multiplier");
    if (mult != 0.0f) *(float*)&patchCodes[16] = mult;
    auto patchAddr = ModUtils::allocMemoryNear(addr, sizeof(patchCodes));
    *(uint32_t*)&patchCodes[0x0C] = addr + 0x07 - (patchAddr + 0x0B + 5);
    memcpy((void*)patchAddr, patchCodes, sizeof(patchCodes));
    ModUtils::hookAsmManually(addr, 7, patchAddr);
MOD_END(DropRate)
