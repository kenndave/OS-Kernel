#include "shell-header/ls.h"
#include "shell-header/systemcall.h"
#include "shell-header/shell-state.h"
#include "shell-header/shell-helper.h"

#include "../libc-header/str.h"

extern struct ShellState shell_state;

void ls(char *path) {
    struct FAT32DirectoryTable dir_table;
    uint32_t parent_cluster = shell_state.current_folder_cluster;

    if (get_dir_table(path, &dir_table, &parent_cluster) == -1) {
        print_file_nfound();
        return;
    }

    for (uint8_t i = 2; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); ++i) {
        struct FAT32DirectoryEntry current = dir_table.table[i];
        if (current.user_attribute != UATTR_NOT_EMPTY) {
            continue;
        }

        if (current.attribute == ATTR_SUBDIRECTORY) {
            syscall(PUTS_SYSCALL, (uint32_t) current.name, strlen(current.name), 0x9);
        } else {
            char ext[4];
            uint32_t name_len = strlen(current.name), ext_len = strlen(current.ext);
            if (ext_len > 0) {
                strcpy(ext, ".", 1);
                strcpy(ext + 1, current.ext, ext_len);
                ext[ext_len+1] = '\0';
                ext_len += 1;
            }

            syscall(PUTS_SYSCALL, (uint32_t) current.name, name_len, 0xF);
            syscall(PUTS_SYSCALL, (uint32_t) ext, ext_len, 0xC);
        }
        syscall(PUTS_SYSCALL, (uint32_t) "\n", 1, 0x0);
    }
}
