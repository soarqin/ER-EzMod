#include "moddef.h"

MOD_BEGIN(CharFlags)
    uint16_t pattern[] = {
        0x80, 0x3D, MASKED, MASKED, MASKED, MASKED, 0x00, 0x0F,
        0x85, MASKED, MASKED, MASKED, MASKED, 0x32, 0xC0, 0x48,
        PATTERN_END
    };
    auto addr = ModUtils::sigScan(pattern);
    if (addr == 0) return;
    addr = addr + *(uint32_t*)(addr + 2) + 7;
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
#pragma pack(pop)
    FlagArray newArray = {
        configByInt("noDead") != 0,
        configByInt("noHorseDead") != 0,
        configByInt("oneHitKill") != 0,
        configByInt("noConsumeItemCost") != 0,
        configByInt("noStaminaCost") != 0,
        configByInt("noFPCost") != 0,
        configByInt("noArrowCost") != 0
    };
    ModUtils::patch(addr + 2, (const uint8_t*)&newArray, 7);
MOD_END(CharFlags)
