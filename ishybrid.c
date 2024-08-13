#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/*
 * This program detects whether the CPU is a hybrid design and identifies the core types (P-core or E-core).
 * It uses the CPUID instruction to gather information about the CPU.
 * 
 * CPUID leaf 7, EDX bit 15 (isHybrid_bit) indicates a hybrid design.
 * CPUID leaf 1A, EAX bits 24-31 indicate the core type:
 *   0x20: Atom (E-core)
 *   0x40: Core (P-core)
 * 
 * Sources:
 * - https://stackoverflow.com/questions/69955410/how-to-detect-p-e-core-in-intel-alder-lake-cpu
 * - https://www.intel.com/content/www/us/en/developer/articles/guide/12th-gen-intel-core-processor-gamedev-guide.html
 */

// Function to execute the CPUID instruction
void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ __volatile__ (
        "cpuid"
        : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
        : "a" (leaf), "c" (subleaf)
    );
}

int main(int argc, char *argv[]) {
    uint32_t eax, ebx, ecx, edx;
    int cpu = -1;
    cpu_set_t cpuset;
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    int pcore_start = -1, pcore_end = -1, ecore_start = -1, ecore_end = -1;
    int isHybrid_bit = 0;

    // Check if the CPU is hybrid
    cpuid(0x00000007, 0x00, &eax, &ebx, &ecx, &edx);
    isHybrid_bit = (edx >> 15) & 1;

    if (argc == 3 && strcmp(argv[1], "-p") == 0) {
        cpu = atoi(argv[2]);

        // Set CPU affinity to the specified core
        CPU_ZERO(&cpuset);
        CPU_SET(cpu, &cpuset);
        if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
            perror("sched_setaffinity");
            return 1;
        }

        printf("CPU %d:\n", cpu);
        printf("Hybrid CPU: %s\n", isHybrid_bit ? "True" : "False");

        if (isHybrid_bit) {
            // Read core type from CPUID leaf 1A, subleaf 0
            cpuid(0x0000001A, 0x00, &eax, &ebx, &ecx, &edx);
            uint32_t core_type = (eax >> 24) & 0xFF;
            printf("Core type: 0x%02x\n", core_type);
            switch (core_type) {
                case 0x20:
                    printf("Core type: Atom (E-core)\n");
                    break;
                case 0x40:
                    printf("Core type: Core (P-core)\n");
                    break;
                default:
                    printf("Core type: Reserved or unknown\n");
                    break;
            }
        } else {
            // Read core type from CPUID leaf 1A, subleaf 0
            cpuid(0x0000001A, 0x00, &eax, &ebx, &ecx, &edx);
            uint32_t core_type = (eax >> 24) & 0xFF;
            printf("Core type: 0x%02x\n", core_type);
            switch (core_type) {
                case 0x20:
                    printf("Core type: Atom\n");
                    break;
                case 0x40:
                    printf("Core type: Core\n");
                    break;
                default:
                    printf("Core type: Reserved or unknown\n");
                    break;
            }
        }
    } else {
        printf("Hybrid CPU: %s\n", isHybrid_bit ? "True" : "False");

        if (isHybrid_bit) {
            // Enumerate all cores to determine core types
            for (int i = 0; i < num_cpus; i++) {
                CPU_ZERO(&cpuset);
                CPU_SET(i, &cpuset);
                if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
                    perror("sched_setaffinity");
                    return 1;
                }

                // Read core type from CPUID leaf 1A, subleaf 0
                cpuid(0x0000001A, 0x00, &eax, &ebx, &ecx, &edx);
                uint32_t core_type = (eax >> 24) & 0xFF;
                if (core_type == 0x20) {
                    if (ecore_start == -1) ecore_start = i;
                    ecore_end = i;
                } else if (core_type == 0x40) {
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
        } else {
            // Enumerate all cores to determine core types
            for (int i = 0; i < num_cpus; i++) {
                CPU_ZERO(&cpuset);
                CPU_SET(i, &cpuset);
                if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
                    perror("sched_setaffinity");
                    return 1;
                }

                // Read core type from CPUID leaf 1A, subleaf 0
                cpuid(0x0000001A, 0x00, &eax, &ebx, &ecx, &edx);
                uint32_t core_type = (eax >> 24) & 0xFF;
                if (core_type == 0x20) {
                    if (ecore_start == -1) ecore_start = i;
                    ecore_end = i;
                } else if (core_type == 0x40) {
                    if (pcore_start == -1) pcore_start = i;
                    pcore_end = i;
                }
            }

            if (pcore_start != -1 && pcore_end != -1) {
                printf("Core: %d-%d\n", pcore_start, pcore_end);
            }
            if (ecore_start != -1 && ecore_end != -1) {
                printf("Atom: %d-%d\n", ecore_start, ecore_end);
            }
        }
    }

    return 0;
}
