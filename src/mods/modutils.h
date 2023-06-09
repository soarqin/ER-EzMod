#pragma once

#include <Windows.h>
#include <cstdio>
#include <cstdint>

namespace ModUtils {

extern void init();

// Gets the path of the .dll which the mod code is running in
extern const wchar_t *getModulePath();

// Gets the name of the .dll which the mod code is running in
extern const wchar_t *getModuleName();

// Logs both to std::out and to a log file simultaneously
extern void log(const wchar_t *msg, ...);

#if defined(NDEBUG)
#define logDebug
#else
#define logDebug(...) log("[DEBUG] " __VA_ARGS__)
#endif

// The log should preferably be closed when code execution is finished.
extern void closeLog();

// Scans the whole memory of the main process module for the given signature.
extern uintptr_t sigScan(const char *pattern);
extern uintptr_t sigScan(const uint8_t *pattern, const uint8_t *mask, size_t size);

// Attempts different methods to get the main window handle.
extern bool getWindowHandle(uint32_t *threadId = nullptr);

// Copies memory after changing the permissions at destination, to ensure write success.
extern void memCopySafe(uintptr_t destination, uintptr_t source, size_t numBytes);

// Allocate memory block for hook use
extern uintptr_t allocMemoryNear(uintptr_t address, size_t size);

// Free memory block
extern void freeMemory(uintptr_t address);

// Replaces the memory at a given address with newBytes.
extern void patch(uintptr_t address, const uint8_t *newBytes, size_t newBytesSize, uint8_t *oldBytes = nullptr);

// Places a 5-byte absolutely rel-jump from A to B, run asm codes and return to A.
extern bool hookAsm(uintptr_t address, size_t skipBytes, uint8_t *patchCodes, size_t patchBytes, uint8_t *oldBytes = nullptr);

// Places a 5-byte absolutely rel-jump from A to B, run asm codes and return to A.
extern bool hookAsmManually(uintptr_t address, size_t skipBytes, uintptr_t patchAddress, uint8_t *oldBytes = nullptr);

extern uint32_t mapStringToVKey(const char *name, uint32_t &mods);

}
