#include "moddef.h"

#pragma pack(push, 1)
struct FlagArray {
    uint8_t noDead;
    uint8_t noHorseDead;
    uint8_t oneHitKill;
    uint8_t noConsumeItemCost;
    uint8_t noStaminaCost;
    uint8_t noFPCost;
    uint8_t noArrowCost;
};
struct FlagArray2 {
    uint8_t playerHide;
    uint8_t playerSilence;
};
#pragma pack(pop)

static uintptr_t patchAddr = 0;

MOD_LOAD(CharFlags) {
    if (patchAddr == 0) {
        uint16_t pattern[] = {
            0x80, 0x3D, MASKED, MASKED, MASKED, MASKED, 0x00, 0x0F,
            0x85, MASKED, MASKED, MASKED, MASKED, 0x32, 0xC0, 0x48
        };
        auto addr = ModUtils::sigScan(pattern, countof(pattern));
        if (addr == 0) return;
        addr = addr + *(uint32_t *)(addr + 2) + 7;
        patchAddr = addr;
    }
    FlagArray newArray = {
        configByInt("noDead") != 0,
        configByInt("noHorseDead") != 0,
        configByInt("oneHitKill") != 0,
        configByInt("noConsumeItemCost") != 0,
        configByInt("noStaminaCost") != 0,
        configByInt("noFPCost") != 0,
        configByInt("noArrowCost") != 0
    };
    ModUtils::patch(patchAddr, (const uint8_t *)&newArray, 7);
    FlagArray2 newArray2 = {
        configByInt("playerHide") != 0,
        configByInt("playerSilence") != 0
    };
    ModUtils::patch(patchAddr + 8, (const uint8_t *)&newArray2, 2);
}

MOD_UNLOAD(CharFlags) {
    if (patchAddr == 0) return;
    FlagArray newArray = {0};
    ModUtils::patch(patchAddr, (const uint8_t *)&newArray, 7);
    FlagArray2 newArray2 = {0};
    ModUtils::patch(patchAddr + 8, (const uint8_t *)&newArray2, 2);
    patchAddr = 0;
}
