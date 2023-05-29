#include "moddef.h"

static uintptr_t scanAddr = 0;
static uintptr_t patchAddr = 0;
static uint8_t oldBytes[12];
static uintptr_t scanAddr2 = 0;
static uintptr_t patchAddr2 = 0;
static uint8_t oldBytes2[7];

MOD_LOAD(SummonAnywhere) {
    if (scanAddr == 0) {
        uint16_t pattern[] = {
            0x48, 0x8b, 0x47, MASKED, 0xf3, 0x0f, 0x10, 0x90,
            MASKED, MASKED, MASKED, MASKED, 0x0f, 0x2f, 0xd0,
        };
        uint8_t patchCodes[] = {
            // mov rax,[rdi+28]
            0x48, 0x8B, 0x47, 0x28,
            // cmp dword [rax],7d0
            0x81, 0x38, 0xD0, 0x07, 0x00, 0x00,
            // jne rip+0a
            0x0F, 0x85, 0x0A, 0x00, 0x00, 0x00,
            // movss xmm2,[rip+0f]
            0xF3, 0x0F, 0x10, 0x15, 0x0F, 0x00, 0x00, 0x00,
            // jmp rip+08
            0xEB, 0x08,
            // movss xmm2,[rax+00000084]
            0xF3, 0x0F, 0x10, 0x90, 0x84, 0x00, 0x00, 0x00,
            // jmp [return addr]
            0xE9, 0x00, 0x00, 0x00, 0x00,
            // db float 1000.0
            0x00, 0x00, 0x7A, 0x44,
        };
        auto addr = ModUtils::sigScan(pattern, countof(pattern));
        if (addr == 0) return;
        scanAddr = addr;
        patchAddr = ModUtils::allocMemoryNear(addr, sizeof(patchCodes));
        *(uint32_t *)&patchCodes[35] = (uint32_t)(addr + 12 - (patchAddr + 34 + 5));
        memcpy((void *)patchAddr, patchCodes, sizeof(patchCodes));
    }
    ModUtils::hookAsmManually(scanAddr, 12, patchAddr, oldBytes);
    if (scanAddr2 == 0) {
        uint16_t pattern[] = {
            0x48, 0x8B, 0x45, 0x98, 0x48, 0x85, 0xC0, 0x0F,
            0x84, MASKED, MASKED, MASKED, MASKED, 0x8B, 0x40, 0x20,
        };
        uint8_t patchCodes[] = {
            // mov rax,[rbp-68]
            0x48, 0x8B, 0x45, 0x98,
            // test rax,rax
            0x48, 0x85, 0xC0,
            // je +0d
            0x0F, 0x84, 0x0D, 0x00, 0x00, 0x00,
            // mov dword [rax+20],0
            0xC7, 0x40, 0x20, 0x00, 0x00, 0x00, 0x00,
            // mov word [rax+1c],ffff
            0x66, 0xC7, 0x40, 0x1C, 0xFF, 0xFF,
            // test rax,rax
            0x48, 0x85, 0xC0,
            // jmp [return addr]
            0xE9, 0x00, 0x00, 0x00, 0x00,
        };
        auto addr = ModUtils::sigScan(pattern, countof(pattern));
        if (addr == 0) return;
        scanAddr2 = addr;
        patchAddr2 = ModUtils::allocMemoryNear(addr, sizeof(patchCodes));
        *(uint32_t *)&patchCodes[30] = (uint32_t)(addr + 7 - (patchAddr2 + 29 + 5));
        memcpy((void *)patchAddr2, patchCodes, sizeof(patchCodes));
    }
    ModUtils::hookAsmManually(scanAddr2, 7, patchAddr2, oldBytes2);
}

MOD_UNLOAD(SummonAnywhere) {
    if (scanAddr != 0) {
        ModUtils::memCopySafe(scanAddr, (uintptr_t)oldBytes, 12);
        scanAddr = 0;
        ModUtils::freeMemory(patchAddr);
        patchAddr = 0;
    }
    if (scanAddr2 != 0) {
        ModUtils::memCopySafe(scanAddr2, (uintptr_t)oldBytes2, 7);
        scanAddr2 = 0;
        ModUtils::freeMemory(patchAddr2);
        patchAddr2 = 0;
    }
}
