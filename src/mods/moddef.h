#pragma once

#include "modutils.h"

namespace Mods {

class ModBase {
public:
    virtual ~ModBase();
    virtual const char *name() = 0;
    virtual void load() = 0;
    [[nodiscard]] bool enabled() const { return enabled_; }
    inline void disable() { enabled_ = false; }
    [[nodiscard]] inline int order() const { return order_; }
    inline void setOrder(int o) { order_ = o; }
    void addConfig(const char *name, const char *value);
    [[nodiscard]] const char *config(const char *name) const;
    [[nodiscard]] float configByFloat(const char *name) const;
    [[nodiscard]] long configByInt(const char *name) const;

protected:
    struct ModConfig {
        char name[64];
        char value[64];
    };
    ModConfig *configs_ = nullptr;
    size_t configsSize_ = 0;
    size_t configsCapacity_ = 0;

private:
    bool enabled_ = true;
    int order_ = 0;
};

class ModList final {
public:
    ~ModList();
    void add(ModBase *mod);
    void loadAll();

private:
    ModBase **mods_ = nullptr;
    size_t modsSize_ = 0;
    size_t modsCapacity_ = 0;
};

extern ModList modList;

}

#define MOD_BEGIN(N)                                        \
class N final: Mods::ModBase {                              \
public:                                                     \
    N() { Mods::modList.add(this); }                        \
    const char *name() override { return #N; }              \
    void load() override {        \
        constexpr auto MASKED = ModUtils::MASKED;           \
        constexpr auto PATTERN_END = ModUtils::PATTERN_END;

#define MOD_END(N)  \
    }               \
};                  \
static N s##N##instance;