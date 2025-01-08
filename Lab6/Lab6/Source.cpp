#include <windows.h>
#include <iostream>
#include <tchar.h>
#include <string>
#include <intrin.h>
#include <vector>
#include <iphlpapi.h>
#include <tlhelp32.h>
#include <cstdint>
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wlanapi.lib")
typedef LONG(WINAPI* RtlGetVersionFunc)(OSVERSIONINFOEXW*);

void PrintSystemInfo() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    std::cout << "Hardware Information:" << std::endl;
    std::cout << "  Processor Architecture: ";
    switch (si.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
        std::cout << "x64 (AMD or Intel)" << std::endl;
        break;
    case PROCESSOR_ARCHITECTURE_ARM:
        std::cout << "ARM" << std::endl;
        break;
    case PROCESSOR_ARCHITECTURE_ARM64:
        std::cout << "ARM64" << std::endl;
        break;
    case PROCESSOR_ARCHITECTURE_IA64:
        std::cout << "Intel Itanium-based" << std::endl;
        break;
    case PROCESSOR_ARCHITECTURE_INTEL:
        std::cout << "x86" << std::endl;
        break;
    case PROCESSOR_ARCHITECTURE_UNKNOWN:
        std::cout << "Unknown architecture" << std::endl;
        break;
    default:
        std::cout << "Undefined architecture" << std::endl;
        break;
    }
    std::cout << "  Number of Processors: " << si.dwNumberOfProcessors << std::endl;
    std::cout << "  Page Size: " << si.dwPageSize << " bytes" << std::endl;
}


void PrintMemoryInfo() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        std::cout << "\nMemory Information:" << std::endl;
        std::cout << "  Total Physical Memory: " << memInfo.ullTotalPhys / (1024 * 1024) << " MB" << std::endl;
        std::cout << "  Available Physical Memory: " << memInfo.ullAvailPhys / (1024 * 1024) << " MB" << std::endl;
        std::cout << "  Total Virtual Memory: " << memInfo.ullTotalVirtual / (1024 * 1024) << " MB" << std::endl;
        std::cout << "  Available Virtual Memory: " << memInfo.ullAvailVirtual / (1024 * 1024) << " MB" << std::endl;
    }
    else {
        std::cerr << "Failed to retrieve memory information." << std::endl;
    }
}

void PrintUptime() {
    DWORD uptime = GetTickCount() / 1000;
    DWORD seconds = uptime % 60;
    DWORD minutes = (uptime / 60) % 60;
    DWORD hours = (uptime / 3600) % 24;
    DWORD days = uptime / 86400;
    std::cout << "\nSystem Uptime: ";
    std::cout << days << " days, " << hours << " hours, " << minutes << " minutes, " << seconds << " seconds" << std::endl;
}

void PrintOSVersion() {
    OSVERSIONINFOEXW osInfo;
    ZeroMemory(&osInfo, sizeof(OSVERSIONINFOEXW));
    osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll) {
        auto RtlGetVersion = (RtlGetVersionFunc)GetProcAddress(hNtdll, "RtlGetVersion");
        if (RtlGetVersion) {
            if (RtlGetVersion(&osInfo) == 0) { 
                std::wcout << L"\nOS Information:" << std::endl;
                std::wcout << L"  Version: " << osInfo.dwMajorVersion + 1 << L"." << osInfo.dwMinorVersion << std::endl;
                std::wcout << L"  Build Number: " << osInfo.dwBuildNumber << std::endl;
                std::wcout << L"  System Type: " << (osInfo.wProductType == VER_NT_WORKSTATION ? L"Workstation" : L"Server") << std::endl;
                return;
            }
        }
    }

    std::cerr << "Failed to retrieve OS information." << std::endl;
}

void PrintProcessorFrequency() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    HKEY hKey;
    DWORD data, dataSize = sizeof(data);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, TEXT("~MHz"), NULL, NULL, (LPBYTE)&data, &dataSize) == ERROR_SUCCESS) {
            std::cout << "Processor Frequency: " << data << " MHz" << std::endl;
        }
        RegCloseKey(hKey);
    }
    else {
        std::cerr << "Failed to retrieve processor frequency." << std::endl;
    }
}

void PrintNetworkInfo() {
    ULONG bufferSize = 0;
    GetAdaptersInfo(NULL, &bufferSize);
    PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO*)malloc(bufferSize);
    if (GetAdaptersInfo(pAdapterInfo, &bufferSize) == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
        std::cout << "\nNetwork Information:" << std::endl;
        while (pAdapter) {
            std::cout << "  Adapter Name: " << pAdapter->AdapterName << std::endl;
            std::cout << "  MAC Address: ";
            for (UINT i = 0; i < pAdapter->AddressLength; ++i) {
                if (i != 0) std::cout << "-";
                printf("%02X", pAdapter->Address[i]);
            }
            std::cout << std::endl;
            std::cout << "  IP Address: " << pAdapter->IpAddressList.IpAddress.String << std::endl;
            std::cout << "  Description: " << pAdapter->Description << std::endl;
            pAdapter = pAdapter->Next;
        }
    }
    free(pAdapterInfo);
}

void PrintProcessesInfo() {
    PROCESSENTRY32 pe32;
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to take snapshot of processes." << std::endl;
        return;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hProcessSnap, &pe32)) {
        std::cerr << "Failed to get first process." << std::endl;
        CloseHandle(hProcessSnap);
        return;
    }

    std::cout << "\nRunning Processes:" << std::endl;
    do {
        std::wcout << "  Process: " << pe32.szExeFile << std::endl;
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
}

void PrintCacheInfo() {
    DWORD bufferSize = 0;
    GetLogicalProcessorInformation(nullptr, &bufferSize);
    std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
    if (!GetLogicalProcessorInformation(buffer.data(), &bufferSize)) {
        std::cerr << "Failed to get processor information.\n";
        return;
    }

    for (const auto& info : buffer) {
        if (info.Relationship == RelationCache) {
            auto& cache = info.Cache;
            std::string cacheType;
            switch (cache.Type) {
            case CacheData: cacheType = "Data Cache"; break;
            case CacheInstruction: cacheType = "Instruction Cache"; break;
            case CacheUnified: cacheType = "Unified Cache"; break;
            default: cacheType = "Unknown Cache"; break;
            }
            std::cout << "Cache Level: L" << static_cast<int>(cache.Level)
                << "\nCache Type: " << cacheType
                << "\nCache Size: " << cache.Size / 1024 << " KB\n\n";

        }
    }
}
void cache_sum() {
    DWORD bufferSize = 0;
    GetLogicalProcessorInformation(nullptr, &bufferSize);

    std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));

    if (!GetLogicalProcessorInformation(buffer.data(), &bufferSize)) {
        std::cerr << "Failed to retrieve processor information.\n";
        return;
    }
    size_t l1Cache = 0, l2Cache = 0, l3Cache = 0;

    for (const auto& info : buffer) {
        if (info.Relationship == RelationCache) {
            switch (info.Cache.Level) {
            case 1: l1Cache += info.Cache.Size; break;
            case 2: l2Cache += info.Cache.Size; break;
            case 3: l3Cache += info.Cache.Size; break;
            default: break;
            }
        }
    }

    std::cout << "L1 Cache Size: " << l1Cache / 1024 << " KB\n"
        << "L2 Cache Size: " << l2Cache / 1024 << " KB\n"
        << "L3 Cache Size: " << l3Cache / 1024 << " KB\n";
}

int main() {
    std::cout << "System Information Utility" << std::endl;
    PrintSystemInfo();
    PrintMemoryInfo();
    PrintOSVersion();
    PrintUptime();
    PrintProcessorFrequency();
    PrintCacheInfo();
    cache_sum();
    PrintNetworkInfo();
    PrintProcessesInfo();
    return 0;
}






