#include "moddef.h"

static float oldRespawnTime = 0.0f;
static float oldRespawnTime2 = 0.0f;
static uintptr_t scanAddr = 0;

MOD_LOAD(FasterRespawn) {
    if (scanAddr == 0) {
        auto addr = ModUtils::sigScan(
            "48 89 5C 24 48 8B FA 48"
            "8B D9 C7 44 24 20 00 00"
            "00 00 48");
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
