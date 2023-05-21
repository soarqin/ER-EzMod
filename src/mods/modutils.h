#pragma once

#include <Windows.h>
#include <cstdio>
#include <cstdint>

enum :uint16_t {
    MASKED = 0xffff,
};

namespace ModUtils {

// Gets the path of the .dll which the mod code is running in
const wchar_t *getModulePath();

// Gets the name of the .dll which the mod code is running in
const wchar_t *getModuleName();

// Logs both to std::out and to a log file simultaneously
void log(const wchar_t *msg, ...);

#if defined(NDEBUG)
#define logDebug
#else
#define logDebug(...) log("[DEBUG] " __VA_ARGS__)
#endif

// The log should preferably be closed when code execution is finished.
void closeLog();

// Scans the whole memory of the main process module for the given signature.
uintptr_t sigScan(const uint16_t *pattern, size_t size);

// Replaces the memory at a given address with newBytes.
void patch(uintptr_t address, const uint8_t *newBytes, size_t newBytesSize, uint8_t *oldBytes = nullptr);

// Scans the whole memory of the main process module and replace
bool scanAndPatch(const uint16_t *pattern, size_t size, intptr_t offset, const uint8_t *newBytes, size_t newBytesSize, uint8_t *oldBytes = nullptr);

// Attempts different methods to get the main window handle.
bool getWindowHandle();

// Read memory after changing the permission.
void memReadSafe(uintptr_t addr, void *data, size_t numBytes);

// Copies memory after changing the permissions at both the source and destination, so we don't get an access violation.
void memCopySafe(uintptr_t destination, uintptr_t source, size_t numBytes);

// Simple wrapper around memset
void memSetSafe(uintptr_t address, unsigned char byte, size_t numBytes);

// Takes a 4-byte relative address and converts it to an absolute 8-byte address.
uintptr_t relativeToAbsoluteAddress(uintptr_t relativeAddressLocation);

// Allocate memory block for hook use
uintptr_t allocMemoryNear(uintptr_t address, size_t size);

// Places a 5-byte absolutely rel-jump from A to B, run asm codes and return to A.
bool hookAsm(uintptr_t address, size_t skipBytes, uint8_t *patchCodes, size_t patchBytes);

// Places a 5-byte absolutely rel-jump from A to B, run asm codes and return to A.
bool hookAsmManually(uintptr_t address, size_t skipBytes, uintptr_t patchAddress, uint8_t *oldBytes = nullptr);

}
