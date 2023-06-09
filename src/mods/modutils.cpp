#include "modutils.h"

#include <share.h>
#include <xinput.h>
#include <cstdarg>
#include <cctype>

namespace ModUtils {

static wchar_t muModulePath[MAX_PATH] = {0};
static wchar_t muModuleName[MAX_PATH] = {0};
static HWND muWindow = nullptr;
static FILE *muLogFile = nullptr;
static bool muLogOpened = false;
static HMODULE mainModuleHandle = nullptr;
static struct {
    uint8_t *addr;
    size_t size;
} virtMemBlocks[256];
static size_t virtMemBlockCount = 0;

void init() {
    static wchar_t lpFilename[MAX_PATH];
    static wchar_t dummy = L'x';
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       &dummy,
                       &module);
    GetModuleFileNameW(module, lpFilename, sizeof(lpFilename));
    auto *moduleName = (wchar_t*)wcsrchr(lpFilename, L'\\');
    *moduleName = 0;
    moduleName++;

    auto *pos = (wchar_t*)wcsstr(moduleName, L".dll");
    if (pos != nullptr) *pos = 0;
    lstrcpyW(muModulePath, lpFilename);
    lstrcpyW(muModuleName, moduleName);

    int retry = 3;
    do {
        mainModuleHandle = GetModuleHandleW(L"eldenring.exe");
        if (mainModuleHandle) break;
        Sleep(500);
    } while (retry-- > 0);

    auto *regionStart = (uint8_t*)mainModuleHandle;
    MEMORY_BASIC_INFORMATION memInfo;
    if (VirtualQuery((void*)regionStart, &memInfo, sizeof(memInfo)) == 0) return;

    auto* dos = (IMAGE_DOS_HEADER*)regionStart;
    auto* pe = (IMAGE_NT_HEADERS*)((ULONG64)memInfo.AllocationBase + (ULONG64)dos->e_lfanew);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || pe->Signature != IMAGE_NT_SIGNATURE) return;
    auto *baseAddress = memInfo.AllocationBase;
    IMAGE_SECTION_HEADER* sections = IMAGE_FIRST_SECTION(pe);
    for (int i = 0; i < pe->FileHeader.NumberOfSections; ++i) {
        if (!(sections[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        virtMemBlocks[virtMemBlockCount] = { (uint8_t*)baseAddress + sections[i].VirtualAddress, sections[i].Misc.VirtualSize };
        if (++virtMemBlockCount >= 256) {
            break;
        }
    }
}

const wchar_t *getModulePath() {
    return muModulePath;
}

const wchar_t *getModuleName() {
    return muModuleName;
}

// Logs both to std::out and to a log file simultaneously
void log(const wchar_t *msg, ...) {
    if (muLogFile == nullptr && !muLogOpened) {
        wchar_t path[MAX_PATH];
        _snwprintf(path, MAX_PATH, L"%ls\\%ls.log", muModulePath, muModuleName);
        muLogFile = _wfsopen(path, L"w", _SH_DENYWR);
        muLogOpened = true;
    }

    va_list args;
    va_start(args, msg);
    wprintf(L"%ls > ", muModuleName);
    vwprintf(msg, args);
    wprintf(L"\n");
    if (muLogFile != nullptr) {
        fwprintf(muLogFile, L"%ls > ", muModuleName);
        vfwprintf(muLogFile, msg, args);
        fwprintf(muLogFile, L"\n");
        fflush(muLogFile);
    }
    va_end(args);
}

// The log should preferably be closed when code execution is finished.
void closeLog() {
    if (muLogFile != nullptr) {
        fclose(muLogFile);
        muLogFile = nullptr;
    }
}

// Shows a popup with a warning and logs that same warning.
inline void raiseError(const wchar_t *error, ...) {
    va_list args;
    va_start(args, error);
    wchar_t msg[1024];
    _vsnwprintf(msg, 1024, error, args);
    va_end(args);
    log(L"Raised error: %ls", msg);
    MessageBoxW(nullptr, msg, muModuleName, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
}

bool patternToBytes(const char *pattern, size_t &size, uint8_t *bytes, uint8_t *mask) {
    const auto *ptr = pattern;
#define SKIP_SPACES() while (*ptr == ' ' || *ptr == '\t' || *ptr == '\v' || *ptr == '\r' || *ptr == '\n') ++ptr
    SKIP_SPACES();
    size_t sz = 0;
    if (size == 0) size = (uint32_t)-1;
    if (mask) {
        while (*ptr != 0 && sz < size) {
            if (*ptr == '?' && *(ptr + 1) == '?') {
                if (mask) *mask++ = 0;
                *bytes++ = 0;
                ptr += 2;
            } else if (isxdigit(*ptr)) {
                *mask++ = 0xFF;
                char src[3] = {*ptr, *(ptr + 1), 0};
                *bytes++ = (uint8_t)strtoul(src, nullptr, 16);
                ptr += 2;
            } else {
                raiseError(L"Invalid pattern: %hs", pattern);
                return false;
            }
            sz++;
            SKIP_SPACES();
        }
    } else {
        while (*ptr != 0 && sz < size) {
            if (isxdigit(*ptr)) {
                char src[3] = {*ptr, *(ptr + 1), 0};
                *bytes++ = (uint8_t)strtoul(src, nullptr, 16);
                ptr += 2;
            } else {
                raiseError(L"Invalid pattern: %hs", pattern);
                return false;
            }
            sz++;
            SKIP_SPACES();
        }
    }
#undef SKIP_SPACES
    size = sz;
    return true;
}

// Scans the whole memory of the main process module for the given signature.
uintptr_t sigScan(const char *pattern) {
    size_t size = strlen(pattern);
    auto *bytes = (uint8_t*)malloc(size);
    auto *mask = (uint8_t*)malloc(size);
    patternToBytes(pattern, size, bytes, mask);
    auto result = sigScan(bytes, mask, size);
    free(bytes);
    free(mask);
    return result;
}

uintptr_t sigScan(const uint8_t *pattern, const uint8_t *mask, size_t size) {
#if !defined(NDEBUG)
    char patternString[1024] = {0};
    size_t offset = 0;
    for (size_t i = 0; i < size && offset < 1024; ++i) {
        if (mask[i] == 0) {
            offset += snprintf(patternString + offset, 1024 - offset, " ??");
        } else {
            offset += snprintf(patternString + offset, 1024 - offset, " %02x", pattern[i]);
        }
    }
    log(L"Scanning pattern: %hs", patternString);
#endif

    for (size_t i = 0; i < virtMemBlockCount; ++i) {
        logDebug(L"Checking region: 0x%llX", virtMemBlocks[i]);
        auto *currentAddress = virtMemBlocks[i].addr;
        auto *regionEnd = currentAddress + virtMemBlocks[i].size - size;
        while (currentAddress < regionEnd) {
            bool match = true;
            for (size_t j = 0; j < size; j++) {
                if (!((currentAddress[j] & mask[j]) ^ pattern[j])) {
                    continue;
                }
                match = false;
                break;
            }
            if (match) {
                logDebug(L"Found signature at 0x%llX", currentAddress);
                return (uintptr_t)currentAddress;
            }
            currentAddress++;
        }
    }
    raiseError(L"Could not find signature!");
    return 0;
}

// Winapi callback that receives all active window handles one by one.
inline BOOL CALLBACK enumWindowHandles(HWND hwnd, LPARAM lParam) {
    DWORD processId = 0;
    auto thId = GetWindowThreadProcessId(hwnd, &processId);
    if (processId == GetCurrentProcessId()) {
        wchar_t buffer[100];
        GetWindowTextW(hwnd, buffer, 100);
        logDebug(L"Found window belonging to ER: %ls", buffer);
        if (wcsstr(buffer, L"ELDEN RING™") != nullptr) {
            logDebug(L"%ls handle selected", buffer);
            if (lParam) *(uint32_t*)lParam = thId;
            muWindow = hwnd;
            return false;
        }
    }
    return true;
}

// Attempts different methods to get the main window handle.
bool getWindowHandle(uint32_t *threadId) {
    logDebug(L"Finding application window...");

    for (size_t i = 0; i < 10000; i++) {
        HWND hwnd = FindWindowExW(nullptr, nullptr, nullptr, L"ELDEN RING™");
        DWORD processId = 0;
        auto thId = GetWindowThreadProcessId(hwnd, &processId);
        if (processId == GetCurrentProcessId()) {
            muWindow = hwnd;
            if (threadId) *threadId = thId;
            logDebug(L"FindWindowExW: found window handle");
            break;
        }
        Sleep(1);
    }

    // Backup method
    if (muWindow == nullptr) {
        logDebug(L"Enumerating windows...");
        for (size_t i = 0; i < 10000; i++) {
            EnumWindows(&enumWindowHandles, (LPARAM)threadId);
            if (muWindow != nullptr) {
                break;
            }
            Sleep(1);
        }
    }

    return muWindow != nullptr;
}

// Disables or enables the memory protection in a given region.
// Remembers and restores the original memory protection type of the given addresses.
inline DWORD toggleMemoryProtection(DWORD protection, uintptr_t address, size_t size) {
    DWORD oldProtection = 0;
    VirtualProtect((void *)address, size, protection, &oldProtection);
    return oldProtection;
}

// Copies memory after changing the permissions at destination, to ensure write success.
void memCopySafe(uintptr_t destination, uintptr_t source, size_t numBytes) {
    auto oldProt0 = toggleMemoryProtection(PAGE_EXECUTE_READWRITE, destination, numBytes);
    memcpy((void *)destination, (void *)source, numBytes);
    toggleMemoryProtection(oldProt0, destination, numBytes);
}

// Allocate memory block for hook use
uintptr_t allocMemoryNear(uintptr_t address, size_t size) {
    uintptr_t newAddr = (address - 0x40000000ULL) & ~0xFFFULL;
    uintptr_t endAddr = newAddr + 0x80000000ULL;
    LPVOID allocAddr;
    size_t step = size > 0x1000 ? (size + 0xFFFU) & ~0xFFFU : 0x1000U;
    while (newAddr < endAddr) {
        logDebug(L"Trying to alloc 0x%llX", newAddr);
        allocAddr = VirtualAlloc((LPVOID)newAddr,
                                 size,
                                 MEM_RESERVE | MEM_COMMIT,
                                 PAGE_EXECUTE_READWRITE);
        if (allocAddr) break;
        newAddr += step;
    }
    return (uintptr_t)allocAddr;
}

// Free memory block
void freeMemory(uintptr_t address) {
    VirtualFree((LPVOID)address, 0, MEM_RELEASE);
}

// Replaces the memory at a given address with newBytes.
void patch(uintptr_t address, const uint8_t *newBytes, size_t newBytesSize, uint8_t *oldBytes) {
#if !defined(NDEBUG)
    char newBytesString[1024];
    size_t offset = 0;
    size_t index = 0;
    for (const auto *bytes = newBytes; offset < 1024 && index < newBytesSize; index++) {
        offset += snprintf(newBytesString + offset, 1024 - offset, " %02x", bytes[index]);
    }
    log(L"New bytes: %hs", newBytesString);
#endif

    if (oldBytes) memcpy(oldBytes, (void *)address, newBytesSize);
    memCopySafe(address, (uintptr_t)newBytes, newBytesSize);
    logDebug(L"Patch applied");
}

// Places a 5-byte absolutely rel-jump from A to B, run asm codes and return to A.
bool hookAsm(uintptr_t address, size_t skipBytes, uint8_t *patchCodes, size_t patchBytes, uint8_t *oldBytes) {
    if (skipBytes < 5) return false;
    uintptr_t allocAddr = allocMemoryNear(address, patchBytes + 5);
    if (!allocAddr) {
        logDebug(L"Unable to allocate memory for hook");
        return false;
    }
    logDebug(L"Hook JMP from 0x%llX to 0x%llX", address, allocAddr);
    uint8_t jmp[32];
    jmp[0] = 0xE9;
    *(uint32_t*)&jmp[1] = (uint32_t)((uintptr_t)allocAddr - address - 5);
    if (skipBytes > 5) memset(jmp + 5, 0x90, skipBytes - 5);
    if (oldBytes) memcpy(oldBytes, (void *)address, skipBytes);
    memCopySafe(address, (uintptr_t)jmp, skipBytes);
    memcpy((void*)allocAddr, patchCodes, patchBytes);
    auto *jmp2 = (uint8_t*)allocAddr + patchBytes;
    jmp2[0] = 0xE9;
    *(uint32_t*)&jmp2[1] = (uint32_t)(address + skipBytes - ((uintptr_t)allocAddr + patchBytes + 5));
    return true;
}

bool hookAsmManually(uintptr_t address, size_t skipBytes, uintptr_t patchAddress, uint8_t *oldBytes) {
    if (skipBytes < 5) return false;
    logDebug(L"Hook JMP from 0x%llX to 0x%llX manually", address, patchAddress);
    uint8_t jmp[32] = {0xE9};
    *(uint32_t*)&jmp[1] = (uint32_t)((uintptr_t)patchAddress - address - 5);
    if (skipBytes > 5) memset(jmp + 5, 0x90, skipBytes - 5);
    if (oldBytes) memcpy(oldBytes, (void*)address, skipBytes);
    memCopySafe(address, (uintptr_t)jmp, skipBytes);
    return true;
}


uint32_t mapStringToVKey(const char *name, uint32_t &mods) {
    struct KeyMap {
        const char *name;
        uint32_t keyId;
    };
    static const KeyMap sModMap[] = {
        {"SHIFT", 0x01},
        {"CONTROL", 0x02},
        {"CTRL", 0x02},
        {"ALT", 0x04},
        {"LSHIFT", 0x10},
        {"LCONTROL", 0x20},
        {"LCTRL", 0x20},
        {"LALT", 0x40},
        {"RSHIFT", 0x100},
        {"RCONTROL", 0x200},
        {"RCTRL", 0x200},
        {"RALT", 0x400},
        {"WIN", 0x1000},
    };

    static const KeyMap sVKeyMap[] = {
        {"LBUTTON", VK_LBUTTON},
        {"RBUTTON", VK_RBUTTON},
        {"CANCEL", VK_CANCEL},
        {"MBUTTON", VK_MBUTTON},
        {"XBUTTON1", VK_XBUTTON1},
        {"XBUTTON2", VK_XBUTTON2},
        {"BACK", VK_BACK},
        {"BACKSPACE", VK_BACK},
        {"TAB", VK_TAB},
        {"CLEAR", VK_CLEAR},
        {"RETURN", VK_RETURN},
        {"ENTER", VK_RETURN},
        {"PAUSE", VK_PAUSE},
        {"CAPITAL", VK_CAPITAL},
        {"CAPSLOCK", VK_CAPITAL},
        {"KANA", VK_KANA},
        {"HANGUL", VK_HANGUL},
        {"JUNJA", VK_JUNJA},
        {"FINAL", VK_FINAL},
        {"HANJA", VK_HANJA},
        {"KANJI", VK_KANJI},
        {"ESCAPE", VK_ESCAPE},
        {"ESC", VK_ESCAPE},
        {"CONVERT", VK_CONVERT},
        {"NONCONVERT", VK_NONCONVERT},
        {"ACCEPT", VK_ACCEPT},
        {"MODECHANGE", VK_MODECHANGE},
        {"SPACE", VK_SPACE},
        {"PRIOR", VK_PRIOR},
        {"NEXT", VK_NEXT},
        {"END", VK_END},
        {"HOME", VK_HOME},
        {"LEFT", VK_LEFT},
        {"UP", VK_UP},
        {"RIGHT", VK_RIGHT},
        {"DOWN", VK_DOWN},
        {"SELECT", VK_SELECT},
        {"PRINT", VK_PRINT},
        {"EXECUTE", VK_EXECUTE},
        {"SNAPSHOT", VK_SNAPSHOT},
        {"INSERT", VK_INSERT},
        {"DELETE", VK_DELETE},
        {"HELP", VK_HELP},
        {"0", 0x30},
        {"1", 0x31},
        {"2", 0x32},
        {"3", 0x33},
        {"4", 0x34},
        {"5", 0x35},
        {"6", 0x36},
        {"7", 0x37},
        {"8", 0x38},
        {"9", 0x39},
        {"A", 0x41},
        {"B", 0x42},
        {"C", 0x43},
        {"D", 0x44},
        {"E", 0x45},
        {"F", 0x46},
        {"G", 0x47},
        {"H", 0x48},
        {"I", 0x49},
        {"J", 0x4A},
        {"K", 0x4B},
        {"L", 0x4C},
        {"M", 0x4D},
        {"N", 0x4E},
        {"O", 0x4F},
        {"P", 0x50},
        {"Q", 0x51},
        {"R", 0x52},
        {"S", 0x53},
        {"T", 0x54},
        {"U", 0x55},
        {"V", 0x56},
        {"W", 0x57},
        {"X", 0x58},
        {"Y", 0x59},
        {"Z", 0x5A},
        {"APPS", VK_APPS},
        {"SLEEP", VK_SLEEP},
        {"NUMPAD0", VK_NUMPAD0},
        {"NUMPAD1", VK_NUMPAD1},
        {"NUMPAD2", VK_NUMPAD2},
        {"NUMPAD3", VK_NUMPAD3},
        {"NUMPAD4", VK_NUMPAD4},
        {"NUMPAD5", VK_NUMPAD5},
        {"NUMPAD6", VK_NUMPAD6},
        {"NUMPAD7", VK_NUMPAD7},
        {"NUMPAD8", VK_NUMPAD8},
        {"NUMPAD9", VK_NUMPAD9},
        {"NUM0", VK_NUMPAD0},
        {"NUM1", VK_NUMPAD1},
        {"NUM2", VK_NUMPAD2},
        {"NUM3", VK_NUMPAD3},
        {"NUM4", VK_NUMPAD4},
        {"NUM5", VK_NUMPAD5},
        {"NUM6", VK_NUMPAD6},
        {"NUM7", VK_NUMPAD7},
        {"NUM8", VK_NUMPAD8},
        {"NUM9", VK_NUMPAD9},
        {"MULTIPLY", VK_MULTIPLY},
        {"ADD", VK_ADD},
        {"SUBTRACT", VK_SUBTRACT},
        {"MINUS", VK_SUBTRACT},
        {"DECIMAL", VK_DECIMAL},
        {"DIVIDE", VK_DIVIDE},
        {"F1", VK_F1},
        {"F2", VK_F2},
        {"F3", VK_F3},
        {"F4", VK_F4},
        {"F5", VK_F5},
        {"F6", VK_F6},
        {"F7", VK_F7},
        {"F8", VK_F8},
        {"F9", VK_F9},
        {"F10", VK_F10},
        {"F11", VK_F11},
        {"F12", VK_F12},
        {"F13", VK_F13},
        {"F14", VK_F14},
        {"F15", VK_F15},
        {"F16", VK_F16},
        {"F17", VK_F17},
        {"F18", VK_F18},
        {"F19", VK_F19},
        {"F20", VK_F20},
        {"F21", VK_F21},
        {"F22", VK_F22},
        {"F23", VK_F23},
        {"F24", VK_F24},
        {"NUMLOCK", VK_NUMLOCK},
        {"SCROLL", VK_SCROLL},
        {"LMENU", VK_LMENU},
        {"RMENU", VK_RMENU},
        {"BROWSER_BACK", VK_BROWSER_BACK},
        {"BROWSER_FORWARD", VK_BROWSER_FORWARD},
        {"BROWSER_REFRESH", VK_BROWSER_REFRESH},
        {"BROWSER_STOP", VK_BROWSER_STOP},
        {"BROWSER_SEARCH", VK_BROWSER_SEARCH},
        {"BROWSER_FAVORITES", VK_BROWSER_FAVORITES},
        {"BROWSER_HOME", VK_BROWSER_HOME},
        {"VOLUME_MUTE", VK_VOLUME_MUTE},
        {"VOLUME_DOWN", VK_VOLUME_DOWN},
        {"VOLUME_UP", VK_VOLUME_UP},
        {"MEDIA_NEXT_TRACK", VK_MEDIA_NEXT_TRACK},
        {"MEDIA_PREV_TRACK", VK_MEDIA_PREV_TRACK},
        {"MEDIA_STOP", VK_MEDIA_STOP},
        {"MEDIA_PLAY_PAUSE", VK_MEDIA_PLAY_PAUSE},
        {"LAUNCH_MAIL", VK_LAUNCH_MAIL},
        {"LAUNCH_MEDIA_SELECT", VK_LAUNCH_MEDIA_SELECT},
        {"LAUNCH_APP1", VK_LAUNCH_APP1},
        {"LAUNCH_APP2", VK_LAUNCH_APP2},
        {"OEM_PLUS", VK_OEM_PLUS},
        {"OEM_COMMA", VK_OEM_COMMA},
        {"OEM_MINUS", VK_OEM_MINUS},
        {"OEM_PERIOD", VK_OEM_PERIOD},
        {";", VK_OEM_1},
        {"/", VK_OEM_2},
        {"~", VK_OEM_3},
        {"[", VK_OEM_4},
        {"\\", VK_OEM_5},
        {"]", VK_OEM_6},
        {"'", VK_OEM_7},
        {"PROCESSKEY", VK_PROCESSKEY},
        {"ATTN", VK_ATTN},
        {"CRSEL", VK_CRSEL},
        {"EXSEL", VK_EXSEL},
        {"EREOF", VK_EREOF},
        {"PLAY", VK_PLAY},
        {"ZOOM", VK_ZOOM},
        {"PA1", VK_PA1},
        {"OEM_CLEAR", VK_OEM_CLEAR},
    };

    mods = 0;
    char str[256];
    strcpy(str, name);
    for (auto *c = str; *c != 0; c++) { *c = (char)std::toupper(*c); }
    auto *cur = str;
    uint32_t ret = 0;
    for (;;) {
        auto *spl = strchr(cur, '+');
        if (spl != nullptr) {
            *spl = 0;
        }
        bool found = false;
        for (const auto &m: sModMap) {
            if (strcmp(cur, m.name) == 0) {
                mods |= m.keyId;
                found = true;
                break;
            }
        }
        if (!found) {
            for (const auto &v: sVKeyMap) {
                if (strcmp(cur, v.name) == 0) {
                    ret = v.keyId;
                    found = true;
                    break;
                }
            }
            if (!found) {
                log(L"Unknown key name: %hs\n", cur);
            }
        }
        if (spl == nullptr) break;
        cur = spl + 1;
    }
    return ret;
}

}
