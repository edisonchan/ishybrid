#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sched.h>
#endif

// Function to execute the CPUID instruction
void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
#ifdef _WIN32
    int cpuInfo[4];
    __cpuidex(cpuInfo, leaf, subleaf);
    *eax = cpuInfo[0];
    *ebx = cpuInfo[1];
    *ecx = cpuInfo[2];
    *edx = cpuInfo[3];
#else
    __asm__ __volatile__(
        "cpuid"
        : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
        : "a" (leaf), "c" (subleaf)
    );
#endif
}

int main(int argc, char* argv[]) {
    uint32_t eax, ebx, ecx, edx;
    int cpu = -1;
    int num_cpus = 1;
    int pcore_start = -1, pcore_end = -1, ecore_start = -1, ecore_end = -1;
    int isHybrid_bit = 0;

#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    num_cpus = sysinfo.dwNumberOfProcessors;
#else
    num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
#endif

    // Check CPU vendor
    cpuid(0x00000000, 0x00, &eax, &ebx, &ecx, &edx);
    char vendor[13];
    memcpy(vendor, &ebx, 4);
    memcpy(vendor + 4, &edx, 4);
    memcpy(vendor + 8, &ecx, 4);
    vendor[12] = '\0';

    if (strcmp(vendor, "GenuineIntel") == 0) {
        // Check if the CPU is hybrid
        cpuid(0x00000007, 0x00, &eax, &ebx, &ecx, &edx);
        isHybrid_bit = (edx >> 15) & 1;

        printf("Hybrid CPU: %s\n", isHybrid_bit ? "True" : "False");

        if (isHybrid_bit) {
            // Enumerate all cores to determine core types
            for (int i = 0; i < num_cpus; i++) {
#ifdef _WIN32
                SetThreadAffinityMask(GetCurrentThread(), 1 << i);
#else
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(i, &cpuset);
                if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
                    perror("sched_setaffinity");
                    return 1;
                }
#endif
                // Read core type from CPUID leaf 1A, subleaf 0
                cpuid(0x0000001A, 0x00, &eax, &ebx, &ecx, &edx);
                uint32_t core_type = (eax >> 24) & 0xFF;
                if (core_type == 0x20) {
                    if (ecore_start == -1) ecore_start = i;
                    ecore_end = i;
                }
                else if (core_type == 0x40) {
                    if (pcore_start == -1) pcore_start = i;
                    pcore_end = i;
                }
            }

            if (pcore_start != -1 && pcore_end != -1) {
                printf("P-Core: %d-%d\n", pcore_start, pcore_end);
            }
            if (ecore_start != -1 && ecore_end != -1) {
                printf("E-Core: %d-%d\n", ecore_start, ecore_end);
            }
        }
    }
    else if (strcmp(vendor, "AuthenticAMD") == 0) {
        printf("This is an AMD CPU.\n");
    }
    else {
        printf("Unknown CPU vendor: %s\n", vendor);
    }

    return 0;
}

