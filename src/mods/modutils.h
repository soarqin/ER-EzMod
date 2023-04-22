#pragma once

#include <Windows.h>
#include <cstdio>
#include <cstdint>

namespace ModUtils {

static constexpr int MASKED = 0xffff;
static constexpr int PATTERN_END = 0xfffe;

// Gets the name of the .dll which the mod code is running in
const char *getModuleName(bool thisModule = true);

// Logs both to std::out and to a log file simultaneously
void log(const char *msg, ...);

#if defined(NDEBUG)
#define logDebug(msg, ...)
#else
#define logDebug(msg, ...) log("[DEBUG] " msg, __VA_ARGS__)
#endif

// The log should preferably be closed when code execution is finished.
void closeLog();

// Scans the whole memory of the main process module for the given signature.
uintptr_t sigScan(const uint16_t *pattern);

// Replaces the memory at a given address with newBytes.
void patch(uintptr_t address, const uint8_t *newBytes, size_t newBytesSize);

// Scans the whole memory of the main process module and replace
bool scanAndPatch(const uint16_t *pattern, intptr_t offset, const uint8_t *newBytes, size_t newBytesSize);

// Attempts different methods to get the main window handle.
bool getWindowHandle();

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
bool hookAsmManually(uintptr_t address, size_t skipBytes, uintptr_t patchAddress);

}