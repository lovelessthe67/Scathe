#pragma once

#include <iostream>
#include <windows.h>
#include <psapi.h>
#include <thread>
#include <TlHelp32.h>
#include <cstdint>
#include <chrono>
#include <string>
#include <vector>

namespace mem
{
    std::string fetchstring(std::uint64_t address);

    inline INT32 PID;
    inline HANDLE handleProcess;
    inline uintptr_t base;

    inline INT32 FindProcess(LPCTSTR processName) {
        PROCESSENTRY32 processEntry{};
        HANDLE handleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        processEntry.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(handleSnap, &processEntry)) {
            do {
                if (!lstrcmpi(processEntry.szExeFile, processName)) {
                    CloseHandle(handleSnap);
                    return processEntry.th32ProcessID;
                }
            } while (Process32Next(handleSnap, &processEntry));
        }
        CloseHandle(handleSnap);
        return 0;
    }

    inline uintptr_t GetProcessBase() {
        HANDLE handleProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);
        if (handleProcess == NULL)
        {
            throw std::runtime_error("Unable to open process");
        }

        HMODULE handleModules[1024];
        DWORD BytesNeeded;

        if (EnumProcessModules(handleProcess, handleModules, sizeof(handleModules), &BytesNeeded))
        {

            uintptr_t baseAddy = (uintptr_t)handleModules[0];
            CloseHandle(handleProcess);
            return baseAddy;
        }
        else
        {
            CloseHandle(handleProcess);
            throw std::runtime_error("Unable to enumerate process modules");
        }
    }

    template <typename T>
    T read(uint64_t addy) {
        static const auto NtReadVirtualMemory = reinterpret_cast<NTSTATUS(NTAPI*)(
            HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T
            )>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtReadVirtualMemory"));

        T buffer{};
        SIZE_T bytesRead;
        NtReadVirtualMemory(mem::handleProcess, reinterpret_cast<PVOID>(addy), &buffer, sizeof(T), &bytesRead);
        return buffer;
    }

    inline NTSTATUS(NTAPI* GetNtWriteVirtualMemory())(
        HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T) {

        static NTSTATUS(NTAPI * fn)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T) =
            reinterpret_cast<NTSTATUS(NTAPI*)(
                HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T)>(
                    ::GetProcAddress(::GetModuleHandleA("ntdll.dll"), "NtWriteVirtualMemory"));
        return fn;
    }

    template <typename T>
    __forceinline bool write(uint64_t address, const T& buffer) {
        auto NtWriteVirtualMemory = GetNtWriteVirtualMemory();
        if (!NtWriteVirtualMemory)
            return false;

        SIZE_T bytesWritten;
        NTSTATUS status = NtWriteVirtualMemory(
            mem::handleProcess,
            reinterpret_cast<void*>(address),
            const_cast<void*>(static_cast<const void*>(&buffer)),
            sizeof(T),
            &bytesWritten
        );

        return status >= 0 && bytesWritten == sizeof(T);
    }

    inline LPVOID allocate(SIZE_T size) {
        return VirtualAllocEx(mem::handleProcess, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    }
}

inline bool is_valid_address(uintptr_t address) {
    return address != 0 && address >= 0x10000 && address < 0x7FFFFFFFFFFF;
}


