#include "lib-header/syscall-helper.h"
#include "lib-header/fat32.h"
#include "lib-header/keyboard.h"
#include "lib-header/stdmem.h"

void syscall_read(uint32_t cpu_ebx, uint32_t cpu_ecx) {
    struct FAT32DriverRequest *request = (struct FAT32DriverRequest*) cpu_ebx;
    *((uint32_t*) cpu_ecx) = read(request);
}

void syscall_read_dir(uint32_t cpu_ebx, uint32_t cpu_ecx) {
    struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu_ebx;
    *((uint32_t*) cpu_ecx) = read_directory(request);
}

void syscall_write(uint32_t cpu_ebx, uint32_t cpu_ecx) {
    struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu_ebx;
    *((uint32_t*) cpu_ecx) = write(request);
}

void syscall_del(uint32_t cpu_ebx, uint32_t cpu_ecx) {
    struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu_ebx;
    *((uint32_t*) cpu_ecx) = delete(request);
}

void syscall_move(uint32_t cpu_ebx, uint32_t cpu_ecx, uint32_t cpu_edx) {
    struct FAT32DriverRequest request_src = *(struct FAT32DriverRequest*) cpu_ebx,
            request_dest = *(struct FAT32DriverRequest*) cpu_ecx;
    *((uint32_t*) cpu_edx) = move(request_src, request_dest);
}

void syscall_fgets(uint32_t cpu_ebx, uint32_t cpu_ecx) {
    keyboard_state_activate();
    __asm__("sti"); // Due IRQ is disabled when main_interrupt_handler() called
    while (is_keyboard_blocking());
    char buf[KEYBOARD_BUFFER_SIZE];
    get_keyboard_buffer(buf);
    memcpy((char *) cpu_ebx, buf, cpu_ecx);
}

void syscall_puts(uint32_t cpu_ebx, uint32_t cpu_ecx, uint32_t cpu_edx) {
    puts((char *) cpu_ebx, cpu_ecx, cpu_edx); // Modified puts() on kernel side
}
