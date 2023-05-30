#include "moddef.h"

static uintptr_t scanAddr = 0;
static uint8_t oldBytes[7];

static uintptr_t scanAddr2 = 0;
static uintptr_t patchAddr2 = 0;
static uint8_t oldBytes2[8];

MOD_LOAD(RideAnywhere) {
    if (scanAddr == 0) {
        uint16_t pattern[] = {
            0x80, MASKED, MASKED, MASKED, MASKED, MASKED, MASKED, 0x48,
            MASKED, MASKED, MASKED, MASKED, 0x48, MASKED, MASKED, MASKED,
            MASKED, MASKED, MASKED, MASKED, MASKED, 0x40, MASKED, MASKED,
            MASKED, MASKED, MASKED, 0x44, MASKED, MASKED, MASKED, MASKED,
            MASKED, MASKED, MASKED, 0x48, MASKED, MASKED, MASKED, MASKED,
            MASKED, MASKED, MASKED, MASKED, 0xE8, MASKED, MASKED, MASKED,
            MASKED, 0x48};
        auto addr = ModUtils::sigScan(pattern, countof(pattern));
        if (addr == 0) return;
        scanAddr = addr;
    }
    uint8_t newBytes[] = {
        // mov byte ptr [rcx+36],00
        0xC6, 0x41, 0x36, 0x00,
        // mov al,0
        0xB0, 0x00,
        // nop
        0x90
    };
    ModUtils::patch(scanAddr, newBytes, sizeof(newBytes), oldBytes);
    if (scanAddr2 == 0) {
        uint16_t pattern2[] = {
            0x48, 0x8B, 0x40, 0x68, 0x80, 0x78, 0x36, 0x00,
        };
        auto addr = ModUtils::sigScan(pattern2, countof(pattern2));
        if (addr == 0) return;
        scanAddr2 = addr;
    }
    if (patchAddr2 == 0) {
        uint8_t newBytes2[] = {
            // mov rax,[rax+68]
            0x48, 0x8B, 0x40, 0x68,
            // mov byte ptr [rax+36],00
            0xC6, 0x40, 0x36, 0x00,
            // cmp byte ptr [rax+36],00
            0x80, 0x78, 0x36, 0x00,
            // jmp [return addr]
            0xE9, 0x00, 0x00, 0x00, 0x00,
        };
        patchAddr2 = ModUtils::allocMemoryNear(scanAddr2, sizeof(newBytes2));
        *(uint32_t *)&newBytes2[13] = (uint32_t)(scanAddr2 + 0x08 - (patchAddr2 + 12 + 5));
        memcpy((void *)patchAddr2, newBytes2, sizeof(newBytes2));
    }
    ModUtils::hookAsmManually(scanAddr2, 8, (uintptr_t)patchAddr2, oldBytes2);
}

MOD_UNLOAD(RideAnywhere) {
    if (scanAddr != 0) {
        ModUtils::memCopySafe(scanAddr, (uintptr_t)oldBytes, 5);
    }
    if (scanAddr2 != 0) {
        ModUtils::memCopySafe(scanAddr2, (uintptr_t)oldBytes2, 8);
    }
}
