/* SYNAPSE SO - CPU Detection Implementation */
/* Licensed under GPLv3 */

#include <kernel/cpu.h>
#include <kernel/vga.h>
#include <kernel/string.h>

/* Global CPU info */
static cpu_info_t cpu_info;

/* Execute CPUID instruction */
static inline void cpuid(uint32_t code, uint32_t* eax, uint32_t* ebx, 
                         uint32_t* ecx, uint32_t* edx) {
    __asm__ volatile(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(code)
    );
}

/* Check if CPUID is supported */
int cpu_has_cpuid(void) {
    uint32_t eflags_before, eflags_after;
    
    /* Try to flip ID bit (bit 21) in EFLAGS */
    __asm__ volatile(
        "pushfd\n"
        "pop %0\n"
        "mov %0, %1\n"
        "xor $0x200000, %0\n"
        "push %0\n"
        "popfd\n"
        "pushfd\n"
        "pop %0\n"
        : "=&r"(eflags_after), "=&r"(eflags_before)
    );
    
    /* If bit changed, CPUID is supported */
    return ((eflags_before ^ eflags_after) & 0x200000) != 0;
}

/* Initialize CPU detection */
void cpu_init(void) {
    uint32_t eax, ebx, ecx, edx;
    
    memset(&cpu_info, 0, sizeof(cpu_info_t));
    
    if (!cpu_has_cpuid()) {
        vga_print("[-] CPUID not supported (ancient CPU)\n");
        cpu_info.vendor_id = CPU_VENDOR_UNKNOWN;
        return;
    }
    
    /* Get vendor string */
    cpuid(0, &eax, &ebx, &ecx, &edx);
    cpu_info.max_cpuid = eax;
    
    /* Copy vendor string (EBX, EDX, ECX order) */
    *(uint32_t*)(cpu_info.vendor_string + 0) = ebx;
    *(uint32_t*)(cpu_info.vendor_string + 4) = edx;
    *(uint32_t*)(cpu_info.vendor_string + 8) = ecx;
    cpu_info.vendor_string[12] = '\0';
    
    /* Determine vendor ID */
    if (strcmp(cpu_info.vendor_string, "GenuineIntel") == 0) {
        cpu_info.vendor_id = CPU_VENDOR_INTEL;
    } else if (strcmp(cpu_info.vendor_string, "AuthenticAMD") == 0) {
        cpu_info.vendor_id = CPU_VENDOR_AMD;
    } else {
        cpu_info.vendor_id = CPU_VENDOR_UNKNOWN;
    }
    
    /* Get CPU features */
    if (cpu_info.max_cpuid >= 1) {
        cpuid(1, &eax, &ebx, &ecx, &edx);
        
        cpu_info.features_edx = edx;
        cpu_info.features_ecx = ecx;
        
        /* Extract family, model, stepping */
        cpu_info.stepping = eax & 0xF;
        cpu_info.model = (eax >> 4) & 0xF;
        cpu_info.family = (eax >> 8) & 0xF;
        
        /* Extended model/family */
        if (cpu_info.family == 0xF) {
            cpu_info.family += (eax >> 20) & 0xFF;
        }
        if (cpu_info.family == 0xF || cpu_info.family == 0x6) {
            cpu_info.model += ((eax >> 16) & 0xF) << 4;
        }
    }
    
    /* Get extended CPUID info */
    cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    cpu_info.max_extended_cpuid = eax;
    
    /* Get brand string if available */
    if (cpu_info.max_extended_cpuid >= 0x80000004) {
        uint32_t* brand = (uint32_t*)cpu_info.brand_string;
        
        cpuid(0x80000002, &brand[0], &brand[1], &brand[2], &brand[3]);
        cpuid(0x80000003, &brand[4], &brand[5], &brand[6], &brand[7]);
        cpuid(0x80000004, &brand[8], &brand[9], &brand[10], &brand[11]);
        
        cpu_info.brand_string[48] = '\0';
        
        /* Trim leading spaces */
        char* p = cpu_info.brand_string;
        while (*p == ' ') p++;
        if (p != cpu_info.brand_string) {
            int i = 0;
            while (p[i]) {
                cpu_info.brand_string[i] = p[i];
                i++;
            }
            cpu_info.brand_string[i] = '\0';
        }
    }
}

/* Get CPU information */
void cpu_get_info(cpu_info_t* info) {
    if (info) {
        *info = cpu_info;
    }
}

/* Check if CPU has specific feature */
int cpu_has_feature(uint32_t feature) {
    /* Check standard features (EDX) */
    if (feature & cpu_info.features_edx) {
        return 1;
    }
    /* Check extended features (ECX) */
    if (feature & cpu_info.features_ecx) {
        return 1;
    }
    return 0;
}

/* Print CPU information */
void cpu_print_info(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\n=== CPU Information ===\n");
    
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    /* Vendor */
    vga_print("Vendor: ");
    vga_print(cpu_info.vendor_string);
    vga_print("\n");
    
    /* Brand string */
    if (cpu_info.brand_string[0]) {
        vga_print("CPU: ");
        vga_print(cpu_info.brand_string);
        vga_print("\n");
    }
    
    /* Family/Model/Stepping */
    vga_print("Family: ");
    vga_print_dec(cpu_info.family);
    vga_print(", Model: ");
    vga_print_dec(cpu_info.model);
    vga_print(", Stepping: ");
    vga_print_dec(cpu_info.stepping);
    vga_print("\n");
    
    /* Features */
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("Features: ");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    if (cpu_has_feature(CPU_FEATURE_FPU)) vga_print("FPU ");
    if (cpu_has_feature(CPU_FEATURE_PSE)) vga_print("PSE ");
    if (cpu_has_feature(CPU_FEATURE_TSC)) vga_print("TSC ");
    if (cpu_has_feature(CPU_FEATURE_PAE)) vga_print("PAE ");
    if (cpu_has_feature(CPU_FEATURE_APIC)) vga_print("APIC ");
    if (cpu_has_feature(CPU_FEATURE_SEP)) vga_print("SEP ");
    if (cpu_has_feature(CPU_FEATURE_MMX)) vga_print("MMX ");
    if (cpu_has_feature(CPU_FEATURE_SSE)) vga_print("SSE ");
    if (cpu_has_feature(CPU_FEATURE_SSE2)) vga_print("SSE2 ");
    vga_print("\n");
}

/* Enable CPU features */
void cpu_enable_features(void) {
    /* Enable SSE if available */
    if (cpu_has_feature(CPU_FEATURE_SSE)) {
        uint32_t cr0, cr4;
        
        /* Clear CR0.EM (emulation) and set CR0.MP (monitor coprocessor) */
        __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
        cr0 &= ~(1 << 2);  /* Clear EM */
        cr0 |= (1 << 1);   /* Set MP */
        __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
        
        /* Set CR4.OSFXSR and CR4.OSXMMEXCPT */
        __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
        cr4 |= (1 << 9);   /* OSFXSR */
        cr4 |= (1 << 10);  /* OSXMMEXCPT */
        __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
        
        vga_print("    SSE enabled\n");
    }
    
    /* Enable global pages if available */
    if (cpu_has_feature(CPU_FEATURE_PGE)) {
        uint32_t cr4;
        __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
        cr4 |= (1 << 7);  /* PGE */
        __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
        vga_print("    Global pages enabled\n");
    }
}
