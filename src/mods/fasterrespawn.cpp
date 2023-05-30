#include "moddef.h"

static float oldRespawnTime = 0.0f;
static float oldRespawnTime2 = 0.0f;
static uintptr_t scanAddr = 0;

MOD_LOAD(FasterRespawn) {
    if (scanAddr == 0) {
        uint16_t pattern[] = {
            0x48, 0x89, 0x5C, 0x24, 0x48, 0x8B, 0xFA, 0x48,
            0x8B, 0xD9, 0xC7, 0x44, 0x24, 0x20, 0x00, 0x00,
            0x00, 0x00, 0x48
        };
        auto addr = ModUtils::sigScan(pattern, countof(pattern));
        if (addr == 0) return;
        addr = *(uintptr_t *)(addr + 0x19 + *(uint32_t *)(addr + 0x15));
        for (;;) {
            uintptr_t checkAddr = *(uintptr_t *)(addr + 0x28C0);
            if (checkAddr != 0) {
                addr = checkAddr;
                break;
            }
            Sleep(1000);
        }
        addr = *(uintptr_t *)(addr + 0x80);
        addr = *(uintptr_t *)(addr + 0x80);
        addr = addr + 0x58;
        scanAddr = addr;
    }
    oldRespawnTime = *(float *)scanAddr;
    oldRespawnTime2 = *(float *)(scanAddr + 0x04);
    *(float *)scanAddr = 0.0f;
    *(float *)(scanAddr + 0x04) = 0.0f;
    ModUtils::log(L"Respawn time patched: %g->%g %g->%g",
                  oldRespawnTime, *(float *)scanAddr,
                  oldRespawnTime2, *(float *)(scanAddr + 0x04));
}

MOD_UNLOAD(FasterRespawn) {
    if (scanAddr == 0) return;
    *(float *)scanAddr = oldRespawnTime;
    *(float *)(scanAddr + 0x04) = oldRespawnTime2;
    oldRespawnTime = 0.0f;
    oldRespawnTime2 = 0.0f;
}
