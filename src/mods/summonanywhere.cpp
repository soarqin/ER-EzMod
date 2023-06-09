#include "moddef.h"

static uintptr_t scanAddr = 0;
static uintptr_t patchAddr = 0;
static uint8_t oldBytes[12];
static uintptr_t scanAddr2 = 0;
static uintptr_t patchAddr2 = 0;
static uint8_t oldBytes2[7];

MOD_LOAD(SummonAnywhere) {
    if (scanAddr == 0) {
        auto addr = ModUtils::sigScan(
            "48 8b 47 ?? f3 0f 10 90"
            "?? ?? ?? ?? 0f 2f d0");
        if (addr == 0) return;
        scanAddr = addr;
    }
    if (patchAddr == 0)
    {
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
        patchAddr = ModUtils::allocMemoryNear(scanAddr, sizeof(patchCodes));
        *(uint32_t *)&patchCodes[35] = (uint32_t)(scanAddr + 12 - (patchAddr + 34 + 5));
        memcpy((void *)patchAddr, patchCodes, sizeof(patchCodes));
    }
    ModUtils::hookAsmManually(scanAddr, 12, patchAddr, oldBytes);
    if (scanAddr2 == 0) {
        auto addr = ModUtils::sigScan(
            "48 8B 45 98 48 85 C0 0F"
            "84 ?? ?? ?? ?? 8B 40 20");
        if (addr == 0) return;
        scanAddr2 = addr;
    }
    if (patchAddr2 == 0) {
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
        patchAddr2 = ModUtils::allocMemoryNear(scanAddr2, sizeof(patchCodes));
        *(uint32_t *)&patchCodes[30] = (uint32_t)(scanAddr2 + 7 - (patchAddr2 + 29 + 5));
        memcpy((void *)patchAddr2, patchCodes, sizeof(patchCodes));
    }
    ModUtils::hookAsmManually(scanAddr2, 7, patchAddr2, oldBytes2);
}

MOD_UNLOAD(SummonAnywhere) {
    if (scanAddr != 0) {
        ModUtils::memCopySafe(scanAddr, (uintptr_t)oldBytes, 12);
    }
    if (scanAddr2 != 0) {
        ModUtils::memCopySafe(scanAddr2, (uintptr_t)oldBytes2, 7);
    }
}
