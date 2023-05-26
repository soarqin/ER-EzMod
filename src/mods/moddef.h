#pragma once

#include "modutils.h"

namespace Mods {

#if !defined(countof)
#define countof(n) sizeof(n) / sizeof(n[0])
#endif

class ModBase {
public:
    virtual ~ModBase();
    virtual const char *name() = 0;
    virtual void load() = 0;
    virtual void unload() = 0;

    inline void disable() { enabled_ = false; }
    inline void enable() { enabled_ = true; }
    inline void setPriority(int o) { priority_ = o; }
    inline void setDelayed(int d) { priority_ = 0x70000000 + d; }
    inline void setShortcut(uint32_t vkey, uint32_t modkey) {
        vkey_ = vkey;
        modkey_ = modkey;
    }
    void addConfig(const char *name, const char *value);

    [[nodiscard]] bool enabled() const { return enabled_; }
    [[nodiscard]] inline int priority() const { return priority_; }
    [[nodiscard]] const char *config(const char *name) const;
    [[nodiscard]] float configByFloat(const char *name) const;
    [[nodiscard]] long configByInt(const char *name) const;
    [[nodiscard]] inline uint32_t vkey() const { return vkey_; }
    [[nodiscard]] inline uint32_t modkey() const { return modkey_; }

protected:
    struct ModConfig {
        char name[64];
        char value[64];
    };
    ModConfig *configs_ = nullptr;
    size_t configsSize_ = 0;
    size_t configsCapacity_ = 0;
    uint32_t vkey_ = 0;
    uint32_t modkey_ = 0;

private:
    bool enabled_ = true;
    int priority_ = 0;
};

class ModList final {
public:
    ~ModList();
    void add(ModBase *mod);
    void loadAll();
    void checkKeyPress(uint32_t vkey);

private:
    ModBase **mods_ = nullptr;
    size_t modsSize_ = 0;
    size_t modsCapacity_ = 0;
};

extern ModList modList;

}

#define MOD_LOAD(N)                             \
class N final: Mods::ModBase {                  \
public:                                         \
    N() { Mods::modList.add(this); }            \
    const char *name() override { return #N; }  \
    void load() override;                       \
    void unload() override;                     \
};                                              \
static N s##N##instance;                        \
void N::load()

#define MOD_UNLOAD(N) \
void N::unload()
