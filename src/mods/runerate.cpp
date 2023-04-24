#include "moddef.h"

MOD_BEGIN(RuneRate)
    uint16_t pattern[] = {
        0xf3, 0x0f, 0x59, 0xc1, 0xf3, 0x0f, 0x2c, 0xf8,
        0x48, 0x8b, 0x8b, PATTERN_END
    };
    auto addr = ModUtils::sigScan(pattern);
    if (addr == 0) return;
    uint8_t patchCodes[] = {
        // movss xmm0,[rip+0xD]
        0xf3, 0x0f, 0x10, 0x05, 0x0d, 0x00, 0x00, 0x00,
        // mulss xmm0,xmm1
        0xf3, 0x0f, 0x59, 0xc1,
        // cvttss2si edi,xmm0
        0xf3, 0x0f, 0x2c, 0xf8,
        // jmp addr+8
        0xe9, 0x00, 0x00, 0x00, 0x00,
        // run rate (as float = 2.0f)
        0x00, 0x00, 0x00, 0x40
    };
    auto mult = configByFloat("multiplier");
    if (mult != 0.0f) *(float*)&patchCodes[21] = mult;
    auto patchAddr = ModUtils::allocMemoryNear(addr, sizeof(patchCodes));
    *(uint32_t*)&patchCodes[0x11] = (uint32_t)(addr + 0x08 - (patchAddr + 0x10 + 5));
    memcpy((void*)patchAddr, patchCodes, sizeof(patchCodes));
    ModUtils::hookAsmManually(addr, 8, patchAddr);
MOD_END(RuneRate)
