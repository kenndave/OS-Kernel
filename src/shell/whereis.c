#include "shell-header/whereis.h"
#include "shell-header/shell-helper.h"
#include "shell-header/systemcall.h"

#include "../libc-header/str.h"

void whereis(char *search_name, uint32_t parent_cluster, char *current_path) {
    struct FAT32DirectoryTable dir_table;
    struct FAT32DriverRequest request_read = {
            .buf                    = &dir_table,
            .name                   = ".",
            .ext                    = "\0\0\0",
            .parent_cluster_number  = parent_cluster,
            .buffer_size            = sizeof(struct FAT32DirectoryTable),
    };

    int32_t ret_code;
    syscall(READ_DIR_SYSCALL, (uint32_t) &request_read, (uint32_t) &ret_code, 0);

    if (ret_code != 0) {
        print_weird_error();
        return;
    }

    for (uint32_t i = 2; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); ++i) {
        struct FAT32DirectoryEntry current = dir_table.table[i];
        if (current.user_attribute != UATTR_NOT_EMPTY) {
            continue;
        }

        char name[8] = {0}, ext[3] = {0};
        parse_filename(search_name, name, ext);

        if (strcmp(current.name, name) && strcmp(current.ext, ext)) {
            // ugly code, might change it later
            syscall(PUTS_SYSCALL, (uint32_t) current_path, strlen(current_path), 0xF);
            syscall(PUTS_SYSCALL, (uint32_t) "/", 1, 0xF);
            syscall(PUTS_SYSCALL, (uint32_t) search_name, strlen(search_name), 0xF);
            syscall(PUTS_SYSCALL, (uint32_t) "\n", 1, 0x0);
        }

        if (current.attribute == ATTR_SUBDIRECTORY) {
            uint32_t path_len = strlen(current_path), append_len = strlen(current.name);
            char print_str[path_len + append_len + 1];

            strcpy(print_str, current_path, path_len);
            strcpy(print_str+path_len, "/", 1);
            strcpy(print_str+path_len+1, current.name, append_len);

            whereis(search_name, form_cluster_number(current.cluster_high, current.cluster_low), print_str);
        }
    }
}