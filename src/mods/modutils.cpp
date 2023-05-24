#include "modutils.h"

#include <fileapi.h>
#include <Psapi.h>
#include <xinput.h>
#include <cstdarg>
#include <share.h>
#include <cctype>

namespace ModUtils {

static wchar_t muModulePath[MAX_PATH] = {0};
static wchar_t muModuleName[MAX_PATH] = {0};
static HWND muWindow = nullptr;
static FILE *muLogFile = nullptr;
static bool muLogOpened = false;

// Gets the name of the .dll which the mod code is running in
void calcModuleName() {
    if (muModuleName[0] != 0) return;
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
}

const wchar_t *getModulePath() {
    calcModuleName();
    return muModulePath;
}

const wchar_t *getModuleName() {
    calcModuleName();
    return muModuleName;
}

// Gets the path to the current mod relative to the game root folder
inline const wchar_t *getModuleFolderPath() {
    calcModuleName();

    static wchar_t lpFullPath[MAX_PATH];
    _snwprintf(lpFullPath, MAX_PATH, L"mods\\%ls", muModuleName);
    return lpFullPath;
}

// Logs both to std::out and to a log file simultaneously
void log(const wchar_t *msg, ...) {
    calcModuleName();

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
inline void raiseError(const wchar_t *error) {
    log(L"Raised error: %ls", error);
    MessageBoxW(nullptr, error, muModuleName, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
}

// Gets the base address of the game's memory.
inline DWORD_PTR getProcessBaseAddress(DWORD processId) {
    DWORD_PTR baseAddress = 0;
    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    HMODULE *moduleArray = nullptr;
    LPBYTE moduleArrayBytes = nullptr;
    DWORD bytesRequired = 0;

    if (processHandle) {
        if (EnumProcessModules(processHandle, nullptr, 0, &bytesRequired)) {
            if (bytesRequired) {
                moduleArrayBytes = (LPBYTE)LocalAlloc(LPTR, bytesRequired);

                if (moduleArrayBytes) {
                    unsigned int moduleCount;

                    moduleCount = bytesRequired / sizeof(HMODULE);
                    moduleArray = (HMODULE *)moduleArrayBytes;

                    if (EnumProcessModules(processHandle, moduleArray, bytesRequired, &bytesRequired)) {
                        baseAddress = (DWORD_PTR)moduleArray[0];
                    }

                    LocalFree(moduleArrayBytes);
                }
            }
        }

        CloseHandle(processHandle);
    }

    return baseAddress;
}

// Scans the whole memory of the main process module for the given signature.
uintptr_t sigScan(const uint16_t *pattern, size_t size) {
    DWORD processId = GetCurrentProcessId();
    auto *regionStart = (uint8_t*)getProcessBaseAddress(processId);
    logDebug(L"Scan in exe:");
    logDebug(L"  Process ID: %i", processId);
    logDebug(L"  Process base address: 0x%llX", regionStart);

#if !defined(NDEBUG)
    char patternString[1024] = {0};
    size_t offset = 0;
    for (size_t i = 0; i < size && offset < 1024; ++i) {
        auto byte = pattern[i];
        if (byte == MASKED) {
            offset += snprintf(patternString + offset, 1024 - offset, " ?");
        } else {
            offset += snprintf(patternString + offset, 1024 - offset, " 0x%02X", byte);
        }
    }
    logDebug(L"Pattern: %hs", patternString);
#endif

    size_t numRegionsChecked = 0;
    uint8_t *currentAddress = nullptr;
    while (numRegionsChecked < 10000) {
        MEMORY_BASIC_INFORMATION memoryInfo = {nullptr};
        if (VirtualQuery(regionStart, &memoryInfo, sizeof(MEMORY_BASIC_INFORMATION)) == 0) {
            DWORD error = GetLastError();
            if (error == ERROR_INVALID_PARAMETER) {
                logDebug(L"Reached end of scannable memory.");
            } else {
                logDebug(L"VirtualQuery failed, error code: %i.", error);
            }
            break;
        }
        regionStart = (uint8_t*)memoryInfo.BaseAddress;
        auto regionSize = (size_t)memoryInfo.RegionSize;
        auto regionEnd = (uint8_t*)(regionStart + regionSize);
        auto protection = memoryInfo.Protect;
        auto state = memoryInfo.State;

        bool readableMemory = (
            protection == PAGE_EXECUTE_READWRITE ||
                protection == PAGE_READWRITE ||
                protection == PAGE_READONLY ||
                protection == PAGE_WRITECOPY ||
                protection == PAGE_EXECUTE_WRITECOPY)
            && state == MEM_COMMIT;

        if (readableMemory) {
            logDebug(L"Checking region: 0x%llX", regionStart);
            currentAddress = regionStart;
            regionEnd -= size;
            while (currentAddress < regionEnd) {
                bool match = true;
                for (size_t i = 0; i < size; i++) {
                    if (pattern[i] == MASKED ||
                        currentAddress[i] == (unsigned char)pattern[i]) {
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
        } else {
            logDebug(L"Skipped region: 0x%llX", regionStart);
        }

        numRegionsChecked++;
        regionStart += memoryInfo.RegionSize;
    }

    logDebug(L"Stopped at: 0x%llX, num regions checked: %i", currentAddress, numRegionsChecked);
    raiseError(L"Could not find signature!");
    return 0;
}

// Replaces the memory at a given address with newBytes.
void patch(uintptr_t address, const uint8_t *newBytes, size_t newBytesSize, uint8_t *oldBytes) {
    char newBytesString[1024];
    size_t offset = 0;
    size_t index = 0;
    for (const auto *bytes = newBytes; offset < 1024 && index < newBytesSize; index++) {
        offset += snprintf(newBytesString + offset, 1024 - offset, " 0x%02X", bytes[index]);
    }
    logDebug(L"New bytes: %hs", newBytesString);

    if (oldBytes) memcpy(oldBytes, (void *)address, newBytesSize);
    memcpy((void *)address, newBytes, newBytesSize);
    logDebug(L"Patch applied");
}

uintptr_t scanAndPatch(const uint16_t *pattern, size_t size, intptr_t offset, const uint8_t *newBytes, size_t newBytesSize, uint8_t *oldBytes) {
    auto addr = sigScan(pattern, size);
    if (addr == 0) return 0;
    patch(addr + offset, newBytes, newBytesSize, oldBytes);
    return addr + offset;
}

// Winapi callback that receives all active window handles one by one.
inline BOOL CALLBACK enumWindowHandles(HWND hwnd, LPARAM) {
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == GetCurrentProcessId()) {
        wchar_t buffer[100];
        GetWindowTextW(hwnd, buffer, 100);
        logDebug(L"Found window belonging to ER: %ls", buffer);
        if (wcsstr(buffer, L"ELDEN RING") != nullptr) {
            logDebug(L"%ls handle selected", buffer);
            muWindow = hwnd;
            return false;
        }
    }
    return true;
}

// Attempts different methods to get the main window handle.
bool getWindowHandle() {
    logDebug(L"Finding application window...");

    for (size_t i = 0; i < 10000; i++) {
        HWND hwnd = FindWindowExW(nullptr, nullptr, nullptr, L"ELDEN RING™");
        DWORD processId = 0;
        GetWindowThreadProcessId(hwnd, &processId);
        if (processId == GetCurrentProcessId()) {
            muWindow = hwnd;
            logDebug(L"FindWindowExA: found window handle");
            break;
        }
        Sleep(1);
    }

    // Backup method
    if (muWindow == nullptr) {
        logDebug(L"Enumerating windows...");
        for (size_t i = 0; i < 10000; i++) {
            EnumWindows(&enumWindowHandles, 0L);
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

// Read memory after changing the permission.
void memReadSafe(uintptr_t addr, void *data, size_t numBytes) {
    auto oldProt = toggleMemoryProtection(PAGE_EXECUTE_READWRITE, addr, numBytes);
    memcpy(data, (void *)addr, numBytes);
    toggleMemoryProtection(oldProt, addr, numBytes);
}

// Copies memory after changing the permissions at both the source and destination, so we don't get an access violation.
void memCopySafe(uintptr_t destination, uintptr_t source, size_t numBytes) {
    auto oldProt0 = toggleMemoryProtection(PAGE_EXECUTE_READWRITE, destination, numBytes);
    auto oldProt1 = toggleMemoryProtection(PAGE_EXECUTE_READWRITE, source, numBytes);
    memcpy((void *)destination, (void *)source, numBytes);
    toggleMemoryProtection(oldProt1, source, numBytes);
    toggleMemoryProtection(oldProt0, destination, numBytes);
}

// Simple wrapper around memset
void memSetSafe(uintptr_t address, unsigned char byte, size_t numBytes) {
    auto oldProt = toggleMemoryProtection(PAGE_EXECUTE_READWRITE, address, numBytes);
    memset((void *)address, byte, numBytes);
    toggleMemoryProtection(oldProt, address, numBytes);
}

// Takes a 4-byte relative address and converts it to an absolute 8-byte address.
uintptr_t relativeToAbsoluteAddress(uintptr_t relativeAddressLocation) {
    uintptr_t absoluteAddress = 0;
    intptr_t relativeAddress = 0;
    memCopySafe((uintptr_t)&relativeAddress, relativeAddressLocation, 4);
    absoluteAddress = relativeAddressLocation + 4 + relativeAddress;
    return absoluteAddress;
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

// Places a 5-byte absolutely rel-jump from A to B, run asm codes and return to A.
bool hookAsm(uintptr_t address, size_t skipBytes, uint8_t *patchCodes, size_t patchBytes) {
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
        {"SHIFT", MOD_SHIFT},
        {"CONTROL", MOD_CONTROL},
        {"CTRL", MOD_CONTROL},
        {"ALT", MOD_ALT},
        {"WIN", MOD_WIN},
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
        {"LSHIFT", VK_LSHIFT},
        {"RSHIFT", VK_RSHIFT},
        {"LCONTROL", VK_LCONTROL},
        {"RCONTROL", VK_RCONTROL},
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
        if (found) continue;
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
        if (spl == nullptr) break;
        cur = spl + 1;
    }
    return ret;
}

}
