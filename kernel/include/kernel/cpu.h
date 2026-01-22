/* SYNAPSE SO - CPU Detection and Features */
/* Licensed under GPLv3 */

#ifndef KERNEL_CPU_H
#define KERNEL_CPU_H

#include <stdint.h>

/* CPU vendor IDs */
#define CPU_VENDOR_UNKNOWN  0
#define CPU_VENDOR_INTEL    1
#define CPU_VENDOR_AMD      2

/* CPU features (from CPUID) */
#define CPU_FEATURE_FPU     (1 << 0)   /* x87 FPU */
#define CPU_FEATURE_PSE     (1 << 3)   /* Page Size Extension */
#define CPU_FEATURE_TSC     (1 << 4)   /* Time Stamp Counter */
#define CPU_FEATURE_MSR     (1 << 5)   /* Model Specific Registers */
#define CPU_FEATURE_PAE     (1 << 6)   /* Physical Address Extension */
#define CPU_FEATURE_APIC    (1 << 9)   /* APIC on chip */
#define CPU_FEATURE_SEP     (1 << 11)  /* SYSENTER/SYSEXIT */
#define CPU_FEATURE_PGE     (1 << 13)  /* Page Global Enable */
#define CPU_FEATURE_CMOV    (1 << 15)  /* CMOV instruction */
#define CPU_FEATURE_PAT     (1 << 16)  /* Page Attribute Table */
#define CPU_FEATURE_PSE36   (1 << 17)  /* 36-bit PSE */
#define CPU_FEATURE_MMX     (1 << 23)  /* MMX Technology */
#define CPU_FEATURE_FXSR    (1 << 24)  /* FXSAVE/FXRSTOR */
#define CPU_FEATURE_SSE     (1 << 25)  /* SSE */
#define CPU_FEATURE_SSE2    (1 << 26)  /* SSE2 */

/* Extended features */
#define CPU_FEATURE_SSE3    (1 << 0)   /* SSE3 */
#define CPU_FEATURE_SSSE3   (1 << 9)   /* SSSE3 */
#define CPU_FEATURE_SSE4_1  (1 << 19)  /* SSE4.1 */
#define CPU_FEATURE_SSE4_2  (1 << 20)  /* SSE4.2 */
#define CPU_FEATURE_X2APIC  (1 << 21)  /* x2APIC */

/* CPU information structure */
typedef struct {
    /* Vendor info */
    uint32_t vendor_id;
    char vendor_string[13];
    
    /* CPU features */
    uint32_t features_edx;
    uint32_t features_ecx;
    
    /* CPU identification */
    uint32_t family;
    uint32_t model;
    uint32_t stepping;
    
    /* Brand string */
    char brand_string[49];
    
    /* Capabilities */
    uint32_t max_cpuid;
    uint32_t max_extended_cpuid;
} cpu_info_t;

/* Check if CPUID is supported */
int cpu_has_cpuid(void);

/* Initialize CPU detection */
void cpu_init(void);

/* Get CPU information */
void cpu_get_info(cpu_info_t* info);

/* Check if CPU has specific feature */
int cpu_has_feature(uint32_t feature);

/* Print CPU information */
void cpu_print_info(void);

/* Enable CPU features (SSE, etc) */
void cpu_enable_features(void);

#endif /* KERNEL_CPU_H */
