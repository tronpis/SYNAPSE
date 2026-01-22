/* SYNAPSE SO - Early Boot Diagnostics */
/* Licensed under GPLv3 */

#ifndef KERNEL_EARLY_H
#define KERNEL_EARLY_H

#include <stdint.h>

/* Boot check results */
#define BOOT_CHECK_OK       0
#define BOOT_CHECK_WARNING  1
#define BOOT_CHECK_FATAL    2

/* Boot check structure */
typedef struct {
    const char* name;
    int (*check_func)(void);
    int is_fatal;
} boot_check_t;

/* Initialize early diagnostics */
void early_init(void);

/* Run all boot checks */
int early_run_checks(void);

/* Check minimum memory requirements */
int early_check_memory(void);

/* Check CPU requirements */
int early_check_cpu(void);

/* Check multiboot info validity */
int early_check_multiboot(void);

/* Verify kernel integrity (basic) */
int early_check_kernel(void);

/* Print boot summary */
void early_print_summary(void);

/* Panic with error message */
void early_panic(const char* message) __attribute__((noreturn));

#endif /* KERNEL_EARLY_H */
