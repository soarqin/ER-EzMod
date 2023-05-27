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
        configs_ = (ModConfig *)realloc(configs_, configsCapacity_ * sizeof(ModConfig));
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
        char lastSection[64];
        ModBase *lastMod;
        bool isCommon;
        uint32_t delayInMs;
    } modLoadCarry = {{0}, nullptr, false, 0};
    wchar_t path[MAX_PATH];
    _snwprintf(path, MAX_PATH, L"%ls\\%ls.ini", ModUtils::getModulePath(), ModUtils::getModuleName());
    ModUtils::log(L"Loading config file: %ls", path);
    FILE *f = _wfopen(path, L"r");
    if (f == nullptr) return;
    ini_parse_file(f, [](void *userp, const char *section, const char *name, const char *value) {
        auto *modLoadCarry = (ModLoadCarry *)userp;
        if (modLoadCarry->lastSection[0] == 0 || strcmp(modLoadCarry->lastSection, section) != 0) {
            strncpy(modLoadCarry->lastSection, section, 64);
            if (strcmp(section, "EzMod") == 0) {
                modLoadCarry->isCommon = true;
                modLoadCarry->lastMod = nullptr;
            } else {
                modLoadCarry->isCommon = false;
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
                    ModUtils::log(L"Mod %hs not found, skipping...", section);
                }
            }
        }
        if (modLoadCarry->isCommon) {
            if (!strcmp(name, "delay")) {
                modLoadCarry->delayInMs = strtoul(value, nullptr, 0);
            }
        } else if (modLoadCarry->lastMod) {
            if (!strcmp(name, "enabled")) {
                if (!strcmp(value, "0") || !strcmp(value, "false")) {
                    modLoadCarry->lastMod->disable();
                }
            } else if (!strcmp(name, "priority")) {
                modLoadCarry->lastMod->setPriority(strtol(value, nullptr, 0));
            } else if (!strcmp(name, "delay")) {
                modLoadCarry->lastMod->setDelayed(strtol(value, nullptr, 0));
            } else if (!strcmp(name, "shortcut")) {
                uint32_t vkey, modkey;
                vkey = ModUtils::mapStringToVKey(value, modkey);
                modLoadCarry->lastMod->setShortcut(vkey, modkey);
            } else {
                modLoadCarry->lastMod->addConfig(name, value);
            }
        }
        return 1;
    }, &modLoadCarry);
    fclose(f);
    qsort(mods_, modsSize_, sizeof(ModBase *), [](const void *a, const void *b) {
        auto priorityA = (*(ModBase **)a)->priority();
        auto priorityB = (*(ModBase **)b)->priority();
        if (priorityA == priorityB)
            return strcmp((*(ModBase **)a)->name(), (*(ModBase **)b)->name());
        return priorityA - priorityB;
    });
    if (modLoadCarry.delayInMs) {
        ModUtils::log(L"Delay %u ms for mod loading...", modLoadCarry.delayInMs);
        Sleep(modLoadCarry.delayInMs);
    }
    int lastDelay = 0;
    for (size_t i = 0; i < modsSize_; i++) {
        auto *mod = mods_[i];
        if (!mod->enabled()) continue;
        if (mod->priority() > 0x70000000) {
            auto delay = mod->priority() - 0x70000000;
            auto d = delay - lastDelay;
            if (d) {
                ModUtils::log(L"Delay %d ms for %hs...", d, mod->name());
                Sleep(d);
            }
            lastDelay = delay;
        }
        ModUtils::log(L"Loading %hs...", mod->name());
        mod->load();
    }
}

void ModList::checkKeyPress(uint32_t vkey) {
    ModBase *modToToggle[16];
    size_t modToToggleSize = 0;
    for (size_t i = 0; i < modsSize_ && modToToggleSize < 16; i++) {
        auto *mod = mods_[i];
        if (mod->vkey() == vkey) {
            auto modkey = mod->modkey();
            if ((modkey & 0x0001u) && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
                continue;
            }
            if ((modkey & 0x0002u) && !(GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
                continue;
            }
            if ((modkey & 0x0004u) && !(GetAsyncKeyState(VK_MENU) & 0x8000)) {
                continue;
            }
            if ((modkey & 0x0010u) && !(GetAsyncKeyState(VK_LSHIFT) & 0x8000)) {
                continue;
            }
            if ((modkey & 0x0020u) && !(GetAsyncKeyState(VK_LCONTROL) & 0x8000)) {
                continue;
            }
            if ((modkey & 0x0040u) && !(GetAsyncKeyState(VK_LMENU) & 0x8000)) {
                continue;
            }
            if ((modkey & 0x0100u) && !(GetAsyncKeyState(VK_RSHIFT) & 0x8000)) {
                continue;
            }
            if ((modkey & 0x0200u) && !(GetAsyncKeyState(VK_RCONTROL) & 0x8000)) {
                continue;
            }
            if ((modkey & 0x0400u) && !(GetAsyncKeyState(VK_RMENU) & 0x8000)) {
                continue;
            }
            if ((modkey & 0x1000u) && !(GetAsyncKeyState(VK_LWIN) & 0x8000)) {
                continue;
            }
            ModUtils::log(L"Shortcut for %hs pressed...", mod->name());
            modToToggle[modToToggleSize++] = mod;
        }
    }
    for (size_t i = 0; i < modToToggleSize; i++) {
        auto *mod = modToToggle[i];
        if (mod->enabled()) {
            ModUtils::log(L"Unloading %hs...", mod->name());
            mod->unload();
            mod->disable();
            Beep(350, 300);
        } else {
            ModUtils::log(L"Loading %hs...", mod->name());
            mod->load();
            mod->enable();
            Beep(550, 300);
        }
    }
}

}
