#ifndef TUBESOS_SYSCALL_HELPER_H
#define TUBESOS_SYSCALL_HELPER_H

#include "lib-header/stdtype.h"

/**
 *
 * @param cpu_ebx
 * @param cpu_ecx
 */
void syscall_read(uint32_t cpu_ebx, uint32_t cpu_ecx);

/**
 *
 * @param cpu_ebx
 * @param cpu_ecx
 */
void syscall_read_dir(uint32_t cpu_ebx, uint32_t cpu_ecx);

/**
 *
 * @param cpu_ebx
 * @param cpu_ecx
 */
void syscall_write(uint32_t cpu_ebx, uint32_t cpu_ecx);

/**
 *
 * @param cpu_ebx
 * @param cpu_ecx
 */
void syscall_del(uint32_t cpu_ebx, uint32_t cpu_ecx);

/**
 *
 * @param cpu_ebx
 * @param cpu_ecx
 * @param cpu_edx
 */
void syscall_move(uint32_t cpu_ebx, uint32_t cpu_ecx, uint32_t cpu_edx);

/**
 *
 * @param cpu_ebx
 * @param cpu_ecx
 */
void syscall_fgets(uint32_t cpu_ebx, uint32_t cpu_ecx);

/**
 *
 * @param cpu_ebx
 * @param cpu_ecx
 * @param cpu_edx
 */
void syscall_puts(uint32_t cpu_ebx, uint32_t cpu_ecx, uint32_t cpu_edx);



#endif //TUBESOS_SYSCALL_HELPER_H
