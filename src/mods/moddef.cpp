#include "moddef.h"
#include "ini.h"

namespace Mods {

ModList modList;

ModBase::~ModBase() {
    free(configs_);
}

void ModBase::addConfig(const char *name, const char *value) {
    if (configsSize_ >= configsCapacity_) {
        configsCapacity_ = configsCapacity_ == 0 ? 8 : configsCapacity_ * 2;
        configs_ = (ModConfig*)realloc(configs_, configsCapacity_ * sizeof(ModConfig));
    }
    auto *cfg = &configs_[configsSize_++];
    strncpy(cfg->name, name, 64);
    strncpy(cfg->value, value, 64);
}

const char *ModBase::config(const char *name) const {
    for (size_t i = 0; i < configsSize_; i++) {
        if (!strcmp(configs_[i].name, name))
            return configs_[i].value;
    }
    return nullptr;
}

float ModBase::configByFloat(const char *name) const {
    const auto *value = config(name);
    if (!value) return 0;
    return strtof(value, nullptr);
}

long ModBase::configByInt(const char *name) const {
    const auto *value = config(name);
    if (!value) return 0;
    return strtol(value, nullptr, 0);
}

ModList::~ModList() {
    free(mods_);
}

void ModList::add(ModBase *mod) {
    if (modsSize_ >= modsCapacity_) {
        modsCapacity_ = modsCapacity_ == 0 ? 8 : modsCapacity_ * 2;
        mods_ = (ModBase **)realloc(mods_, modsCapacity_ * sizeof(ModBase *));
    }
    mods_[modsSize_++] = mod;
}

void ModList::loadAll() {
    struct ModLoadCarry {
        const char *lastSection;
        ModBase *lastMod;
    } modLoadCarry = { nullptr, nullptr };
    wchar_t path[MAX_PATH];
    _snwprintf(path, MAX_PATH, L"%s\\%s.ini", ModUtils::getModulePath(), ModUtils::getModuleName());
    ModUtils::log(L"Loading config file: %s", path);
    FILE *f = _wfopen(path, L"r");
    if (f == nullptr) return;
    ini_parse_file(f, [](void *userp, const char *section, const char *name, const char *value) {
        auto *modLoadCarry = (ModLoadCarry*)userp;
        if (modLoadCarry->lastSection == nullptr || !strcmp(modLoadCarry->lastSection, section)) {
            modLoadCarry->lastSection = section;
            bool found = false;
            for (size_t i = 0; i < modList.modsSize_; i++) {
                auto *mod = modList.mods_[i];
                if (!strcmp(mod->name(), section)) {
                    modLoadCarry->lastMod = mod;
                    found = true;
                    break;
                }
            }
            if (!found) {
                modLoadCarry->lastMod = nullptr;
                ModUtils::log(L"Mod %s not found, skipping...", section);
            }
        }
        if (modLoadCarry->lastMod) {
            if (!strcmp(name, "enabled")) {
                if (!strcmp(value, "0") || !strcmp(value, "false")) {
                    modLoadCarry->lastMod->disable();
                }
            } else if (!strcmp(name, "order")) {
                modLoadCarry->lastMod->setOrder(strtol(value, nullptr, 0));
            } else if (!strcmp(name, "delay")) {
                modLoadCarry->lastMod->setDelayed(strtol(value, nullptr, 0));
            } else {
                modLoadCarry->lastMod->addConfig(name, value);
            }
        }
        return 1;
    }, &modLoadCarry);
    fclose(f);
    qsort(mods_, modsSize_, sizeof(ModBase *), [](const void *a, const void *b) {
        auto orderA = (*(ModBase**)a)->order();
        auto orderB = (*(ModBase**)b)->order();
        if (orderA == orderB)
            return strcmp((*(ModBase**)a)->name(), (*(ModBase**)b)->name());
        return orderA - orderB;
    });
    int lastDelay = 0;
    for (size_t i = 0; i < modsSize_; i++) {
        auto *mod = mods_[i];
        if (!mod->enabled()) continue;
        ModUtils::log(L"Loading %S...", mod->name());
        if (mod->order() > 0x70000000) {
            auto delay = mod->order() - 0x70000000;
            Sleep(delay - lastDelay);
            lastDelay = delay;
        }
        mod->load();
    }
    ModUtils::closeLog();
}

}
