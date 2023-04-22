#include "modutils.h"

#include <fileapi.h>
#include <Psapi.h>
#include <xinput.h>
#include <cstdarg>

namespace ModUtils {

static char muModuleName[MAX_PATH] = {0};
static HWND muWindow = nullptr;
static FILE *muLogFile = nullptr;
static bool muLogOpened = false;

// Gets the name of the .dll which the mod code is running in
const char *getModuleName(bool thisModule) {
    static char lpFilename[MAX_PATH];
    static char dummy = 'x';
    HMODULE module = nullptr;

    if (thisModule) {
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           &dummy,
                           &module);
    }

    GetModuleFileNameA(module, lpFilename, sizeof(lpFilename));
    const char *moduleName = strrchr(lpFilename, '\\') + 1;

    if (thisModule) {
        auto *pos = strstr(moduleName, ".dll");
        if (pos != nullptr) *pos = 0;
    }

    return moduleName;
}

// Gets the path to the current mod relative to the game root folder
inline const char *getModuleFolderPath() {
    static char lpFullPath[MAX_PATH];
    snprintf(lpFullPath, MAX_PATH, "mods\\%s", getModuleName(true));
    return lpFullPath;
}

// Logs both to std::out and to a log file simultaneously
void log(const char *msg, ...) {
    if (muModuleName[0] == 0) {
        lstrcpyA(muModuleName, getModuleName(true));
    }

    if (muLogFile == nullptr && !muLogOpened) {
        CreateDirectoryA("mods\\log", nullptr);
        char path[MAX_PATH];
        snprintf(path, MAX_PATH, "mods\\log\\%s.log", muModuleName);
        fopen_s(&muLogFile, path, "w");
        muLogOpened = true;
    }

    va_list args;
    va_start(args, msg);
    printf("%s > ", muModuleName);
    vprintf(msg, args);
    printf("\n");
    if (muLogFile != nullptr) {
        fprintf(muLogFile, "%s > ", muModuleName);
        vfprintf(muLogFile, msg, args);
        fprintf(muLogFile, "\n");
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
inline void raiseError(const char *error) {
    if (muModuleName[0] == 0) {
        lstrcpyA(muModuleName, getModuleName(true));
    }
    logDebug("Raised error: %s", error);
    MessageBox(nullptr, error, muModuleName, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
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
uintptr_t sigScan(const uint16_t *pattern) {
    DWORD processId = GetCurrentProcessId();
    uintptr_t regionStart = getProcessBaseAddress(processId);
    logDebug("Scan in: %s", getModuleName(false));
    logDebug("  Process ID: %i", processId);
    logDebug("  Process base address: 0x%llX", regionStart);

    char patternString[1024] = {0};
    size_t offset = 0;
    size_t patternSize = 0;
    for (const auto *bytes = pattern; offset < 1024 && *bytes != PATTERN_END; bytes++) {
        if (*bytes == MASKED) {
            offset += snprintf(patternString + offset, 1024 - offset, " ?");
        } else {
            offset += snprintf(patternString + offset, 1024 - offset, " 0x%02X", *bytes);
        }
        ++patternSize;
    }
    logDebug("Pattern: %s", patternString);

    size_t numRegionsChecked = 0;
    uintptr_t currentAddress = 0;
    while (numRegionsChecked < 10000) {
        MEMORY_BASIC_INFORMATION memoryInfo = {nullptr};
        if (VirtualQuery((void *)regionStart, &memoryInfo, sizeof(MEMORY_BASIC_INFORMATION)) == 0) {
            DWORD error = GetLastError();
            if (error == ERROR_INVALID_PARAMETER) {
                logDebug("Reached end of scannable memory.");
            } else {
                logDebug("VirtualQuery failed, error code: %i.", error);
            }
            break;
        }
        regionStart = (uintptr_t)memoryInfo.BaseAddress;
        auto regionSize = (uintptr_t)memoryInfo.RegionSize;
        auto regionEnd = (uintptr_t)(regionStart + regionSize);
        auto protection = (uintptr_t)memoryInfo.Protect;
        auto state = (uintptr_t)memoryInfo.State;

        bool readableMemory = (
            protection == PAGE_EXECUTE_READWRITE ||
                protection == PAGE_READWRITE ||
                protection == PAGE_READONLY ||
                protection == PAGE_WRITECOPY ||
                protection == PAGE_EXECUTE_WRITECOPY)
            && state == MEM_COMMIT;

        if (readableMemory) {
            logDebug("Checking region: 0x%llX", regionStart);
            currentAddress = regionStart;
            regionEnd -= patternSize;
            while (currentAddress < regionEnd) {
                bool match = true;
                for (size_t i = 0; i < patternSize; i++) {
                    if (pattern[i] == MASKED ||
                        *(unsigned char *)(currentAddress + i) == (unsigned char)pattern[i]) {
                        continue;
                    }
                    match = false;
                    break;
                }
                if (match) {
                    logDebug("Found signature at 0x%llX", currentAddress);
                    return currentAddress;
                }
                currentAddress++;
            }
        } else {
            logDebug("Skipped region: 0x%llX", regionStart);
        }

        numRegionsChecked++;
        regionStart += memoryInfo.RegionSize;
    }

    logDebug("Stopped at: 0x%llX, num regions checked: %i", currentAddress, numRegionsChecked);
    raiseError("Could not find signature!");
    return 0;
}

// Replaces the memory at a given address with newBytes.
void patch(uintptr_t address, const uint8_t *newBytes, size_t newBytesSize) {
    char newBytesString[1024];
    size_t offset = 0;
    size_t index = 0;
    for (const auto *bytes = newBytes; offset < 1024 && index < newBytesSize; index++) {
        offset += snprintf(newBytesString + offset, 1024 - offset, " 0x%02X", bytes[index]);
    }
    logDebug("New bytes: %s", newBytesString);

    memcpy((void *)address, newBytes, newBytesSize);
    logDebug("Patch applied");
}

bool scanAndPatch(const uint16_t *pattern, intptr_t offset, const uint8_t *newBytes, size_t newBytesSize) {
    auto addr = sigScan(pattern);
    if (addr == 0) return false;
    patch(addr + offset, newBytes, newBytesSize);
    return true;
}

// Winapi callback that receives all active window handles one by one.
inline BOOL CALLBACK enumWindowHandles(HWND hwnd, LPARAM) {
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == GetCurrentProcessId()) {
        char buffer[100];
        GetWindowTextA(hwnd, buffer, 100);
        logDebug("Found window belonging to ER: %s", buffer);
        if (strstr(buffer, "ELDEN RING") != nullptr) {
            logDebug("%s handle selected", buffer);
            muWindow = hwnd;
            return false;
        }
    }
    return true;
}

// Attempts different methods to get the main window handle.
bool getWindowHandle() {
    logDebug("Finding application window...");

    for (size_t i = 0; i < 10000; i++) {
        HWND hwnd = FindWindowExW(nullptr, nullptr, nullptr, L"ELDEN RING™");
        DWORD processId = 0;
        GetWindowThreadProcessId(hwnd, &processId);
        if (processId == GetCurrentProcessId()) {
            muWindow = hwnd;
            logDebug("FindWindowExA: found window handle");
            break;
        }
        Sleep(1);
    }

    // Backup method
    if (muWindow == nullptr) {
        logDebug("Enumerating windows...");
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
        logDebug("Trying to alloc 0x%llX", newAddr);
        allocAddr = VirtualAlloc((LPVOID)newAddr,
                                 size,
                                 MEM_RESERVE | MEM_COMMIT,
                                 PAGE_EXECUTE_READWRITE);
        if (allocAddr) break;
        newAddr += step;
    }
    return (uintptr_t)allocAddr;
}

// Places a 5-byte absolutely rel-jump from A to B, run asm codes and return to A.
bool hookAsm(uintptr_t address, size_t skipBytes, uint8_t *patchCodes, size_t patchBytes) {
    if (skipBytes < 5) return false;
    uintptr_t allocAddr = allocMemoryNear(address, patchBytes + 5);
    if (!allocAddr) {
        logDebug("Unable to allocate memory for hook");
        return false;
    }
    logDebug("Hook JMP from 0x%llX to 0x%llX", address, allocAddr);
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

bool hookAsmManually(uintptr_t address, size_t skipBytes, uintptr_t patchAddress) {
    if (skipBytes < 5) return true;
    logDebug("Hook JMP from 0x%llX to 0x%llX manually", address, patchAddress);
    uint8_t jmp[32];
    jmp[0] = 0xE9;
    *(uint32_t*)&jmp[1] = (uint32_t)((uintptr_t)patchAddress - address - 5);
    if (skipBytes > 5) memset(jmp + 5, 0x90, skipBytes - 5);
    memCopySafe(address, (uintptr_t)jmp, skipBytes);
    return true;
}

/*
// Checks if a keyboard or controller key is pressed.
inline bool IsKeyPressed(const std::vector<unsigned short> &keys,
                         bool falseWhileHolding = true,
                         bool checkController = false) {
    static std::vector<std::vector<unsigned short>> notReleasedKeys;
    static bool retrievedWindowHandle = false;

    if (!retrievedWindowHandle) {
        if (GetWindowHandle()) {
            char buffer[100];
            GetWindowTextA(muWindow, buffer, 100);
            logDebug("Found application window: %s", buffer);
        } else {
            logDebug("Failed to get window handle, inputs will be detected globally");
        }
        retrievedWindowHandle = true;
    }

    if (muWindow != nullptr && muWindow != GetForegroundWindow()) {
        return false;
    }

    size_t numKeys = keys.size();
    size_t numKeysBeingPressed = 0;

    if (checkController) {
        for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++) {
            XINPUT_STATE state = {0};
            DWORD result = XInputGetState(controllerIndex, &state);
            if (result == ERROR_SUCCESS) {
                for (auto key: keys) {
                    if ((key & state.Gamepad.wButtons) == key) {
                        numKeysBeingPressed++;
                    }
                }
            }
        }
    } else {
        for (auto key: keys) {
            if (GetAsyncKeyState(key)) {
                numKeysBeingPressed++;
            }
        }
    }

    auto iterator = std::find(notReleasedKeys.begin(), notReleasedKeys.end(), keys);
    bool keysBeingHeld = iterator != notReleasedKeys.end();
    if (numKeysBeingPressed == numKeys) {
        if (keysBeingHeld) {
            if (falseWhileHolding) {
                return false;
            }
        } else {
            notReleasedKeys.push_back(keys);
        }
    } else {
        if (keysBeingHeld) {
            notReleasedKeys.erase(iterator);
        }
        return false;
    }

    return true;
}

inline bool IsKeyPressed(unsigned short key, bool falseWhileHolding = true, bool checkController = false) {
    return IsKeyPressed({key}, falseWhileHolding, checkController);
}
*/

}
