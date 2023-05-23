#include "moddef.h"

static float oldRespawnTime = 0.0f;
static float oldRespawnTime2 = 0.0f;
static uintptr_t patchAddr = 0;

MOD_LOAD(FasterRespawn) {
    if (patchAddr == 0) {
        uint16_t pattern[] = {
            0x48, 0x89, 0x5C, 0x24, 0x48, 0x8B, 0xFA, 0x48,
            0x8B, 0xD9, 0xC7, 0x44, 0x24, 0x20, 0x00, 0x00,
            0x00, 0x00, 0x48
        };
        auto addr = ModUtils::sigScan(pattern, countof(pattern));
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
        patchAddr = addr;
        oldRespawnTime = *(float *)patchAddr;
        oldRespawnTime2 = *(float *)(patchAddr + 0x04);
    }
    *(float *)patchAddr = 0.0f;
    *(float *)(patchAddr + 0x04) = 0.0f;
    ModUtils::log(L"Respawn time patched: %g->%g %g->%g",
                  oldRespawnTime, *(float *)patchAddr,
                  oldRespawnTime2, *(float *)(patchAddr + 0x04));
}

MOD_UNLOAD(FasterRespawn) {
    if (patchAddr == 0) return;
    *(float *)patchAddr = oldRespawnTime;
    *(float *)(patchAddr + 0x04) = oldRespawnTime2;
    patchAddr = 0;
    oldRespawnTime = 0.0f;
    oldRespawnTime2 = 0.0f;
}
