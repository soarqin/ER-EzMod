#include "moddef.h"

MOD_BEGIN(FasterRespawn)
    Sleep(10000);
    uint16_t pattern[] = {
        0x48, 0x89, 0x5C, 0x24, 0x48, 0x8B, 0xFA, 0x48,
        0x8B, 0xD9, 0xC7, 0x44, 0x24, 0x20, 0x00, 0x00,
        0x00, 0x00, 0x48, ModUtils::PATTERN_END
    };
    auto addr = ModUtils::sigScan(pattern);
    addr = *(uintptr_t *)(addr + 0x19 + *(uint32_t *)(addr + 0x15));
    for(;;) {
        uintptr_t checkAddr = *(uintptr_t*)(addr + 0x28C0);
        if (checkAddr != 0) { addr = checkAddr; break; }
        Sleep(1000);
    }
    addr = *(uintptr_t*)(addr + 0x80);
    addr = *(uintptr_t*)(addr + 0x80);
    addr = addr + 0x58;
    float oldRespawnTime = *(float*)addr;
    float oldRespawnTime2 = *(float*)(addr + 0x04);
    *(float*)addr = 0.0f;
    *(float*)(addr + 0x04) = 0.0f;
    ModUtils::log("Respawn time patched: %g->%g %g->%g",
                  oldRespawnTime, *(float*)addr,
                  oldRespawnTime2, *(float*)(addr + 0x04));
MOD_END(FasterRespawn)
