#include "moddef.h"

static uintptr_t scanAddr = 0;
static uintptr_t patchAddr = 0;
static uint8_t oldBytes[7];

MOD_LOAD(NoHitbox) {
    if (scanAddr == 0) {
        auto addr = ModUtils::sigScan(
            "45 0f b6 ce 4c 8b c3 48"
            "8b d6 48 8b cf e8");
        if (addr == 0) return;
        scanAddr = addr;
    }
    if (patchAddr == 0) {
        uint8_t patchCodes[] = {
            // mov rdx, [worldChrManFinder+0x0A]
            0x48, 0xba, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            // mov eax, [rdx]
            0x8b, 0x02,
            // cdqe
            0x48, 0x98,
            // mov rdx,[rax+rdx+4]
            0x48, 0x8b, 0x54, 0x10, 0x04,
            // cmp rdx,0x10000
            0x48, 0x81, 0xfa, 0x00, 0x00, 0x01, 0x00,
            // jl +0x12
            0x7c, 0x12,
            // mov rdx,[rdx+0x1E508]
            0x48, 0x8b, 0x92, 0x08, 0xe5, 0x01, 0x00,
            // cmp rdx,[rdi+0x08]
            0x48, 0x3b, 0x57, 0x08,
            // jne +0x05
            0x75, 0x05,
            // jmp addr+0x12
            0xe9, 0x00, 0x00, 0x00, 0x00,
            // movzx r9d,r14b
            0x45, 0x0f, 0xb6, 0xce,
            // mov r8,rbx
            0x4c, 0x8b, 0xc3,
            // jmp addr+0x07
            0xe9, 0x00, 0x00, 0x00, 0x00,
        };
        auto worldChrManFinder = ModUtils::sigScan(
            "48 8b fa 0f 11 41 70 48"
            "8b 05");
        if (worldChrManFinder == 0) return;
        patchAddr = ModUtils::allocMemoryNear(scanAddr, sizeof(patchCodes));
        *(uintptr_t *)&patchCodes[2] = worldChrManFinder + 0x0A;
        *(uint32_t *)&patchCodes[0x2A] = (uint32_t)(scanAddr + 0x12 - (patchAddr + 0x29 + 5));
        *(uint32_t *)&patchCodes[0x36] = (uint32_t)(scanAddr + 0x07 - (patchAddr + 0x35 + 5));
        memcpy((void *)patchAddr, patchCodes, sizeof(patchCodes));
    }
    ModUtils::hookAsmManually(scanAddr, 7, patchAddr, oldBytes);
}

MOD_UNLOAD(NoHitbox) {
    if (scanAddr == 0) return;
    ModUtils::memCopySafe(scanAddr, (uintptr_t)oldBytes, 7);
}
