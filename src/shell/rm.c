#include "shell-header/rm.h"
#include "shell-header/shell-helper.h"
#include "shell-header/shell-state.h"
#include "shell-header/systemcall.h"

#include "../lib-header/stdmem.h"

#include "../libc-header/str.h"

extern struct ShellState shell_state;

void rm(char *src_path, bool recursive) {
    uint32_t src_parent_cluster = shell_state.current_folder_cluster;
    char src_path_list[MAX_PATH_LENGTH][13] = {0};
    uint32_t src_last_index = parse_path(src_path, src_path_list);

    struct FAT32DirectoryTable src_dir_table;

    // check if src path is a directory
    bool src_is_dir = TRUE;
    if (get_dir_table(src_path, &src_dir_table, &src_parent_cluster) == -1) {
        // check src parent folder
        src_is_dir = FALSE;
    }

    if (!recursive || !src_is_dir) {
        // trying to delete non-empty folder with recursive flag not enabled
        src_parent_cluster = shell_state.current_folder_cluster;
        if (prev_dir_table(src_path, &src_dir_table, &src_parent_cluster) == -1) {
            // src path not valid
            char err[] = "error: source not available.\n";
            syscall(PUTS_SYSCALL, (uint32_t) err, strlen(err), 0xF);
            return;
        }

        struct FAT32DriverRequest request_delete = {
                .buf                    = 0,
                .name                   = "",
                .ext                    = "\0\0\0",
                .parent_cluster_number  = src_parent_cluster,
                .buffer_size            = 0,
        };
        src_last_index = parse_path(src_path, src_path_list);
        if (!src_is_dir) {
            parse_filename(src_path_list[src_last_index], request_delete.name, request_delete.ext);
        } else {
            char *foldername = src_path_list[src_last_index];
            memcpy(request_delete.name, foldername, strlen(foldername));
        }

        int32_t ret_code;
        syscall(DELETE_SYSCALL, (uint32_t) &request_delete, (uint32_t) &ret_code, 0);

        if (ret_code == 2) {
            char err[] = "error: target directory is not empty.\n";
            syscall(PUTS_SYSCALL, (uint32_t) err, strlen(err), 0xF);
        }

        if (ret_code == 1) {
            print_file_nfound();
        }

        return;
    }


    for (uint32_t i = 2; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); ++i) {
        struct FAT32DirectoryEntry current = src_dir_table.table[i];
        if (current.user_attribute != UATTR_NOT_EMPTY) {
            continue;
        }

        if (current.attribute == ATTR_SUBDIRECTORY) {
            uint32_t old_src_len = strlen(src_path), append_len = strlen(current.name);
            char new_src_path[old_src_len + append_len + 1], slash[] = "/";
            strcpy(new_src_path, src_path, old_src_len);
            strcpy(new_src_path+old_src_len, slash, 1);
            strcpy(new_src_path+old_src_len+1, current.name, append_len);
            rm(new_src_path, recursive);
            continue;
        }

        struct FAT32DriverRequest request_delete = {
                .buf                    = 0,
                .name                   = "",
                .ext                    = "\0\0\0",
                .parent_cluster_number  = src_parent_cluster,
                .buffer_size            = 0,
        };

        memcpy(request_delete.name, current.name, strlen(current.name));
        memcpy(request_delete.ext, current.ext, strlen(current.ext));

        int32_t ret_code;
        syscall(DELETE_SYSCALL, (uint32_t) &request_delete, (uint32_t) &ret_code, 0);

        if (ret_code != 0) {
            char err[] = "error: error while deleting file.\n";
            syscall(PUTS_SYSCALL, (uint32_t) err, strlen(err), 0xF);
            return;
        }
    }

    // remove folder
    struct FAT32DirectoryEntry dir_entry = src_dir_table.table[0];
    src_parent_cluster = form_cluster_number(dir_entry.cluster_high, dir_entry.cluster_low);
    struct FAT32DriverRequest request_delete = {
            .buf                    = 0,
            .name                   = "",
            .ext                    = "\0\0\0",
            .parent_cluster_number  = src_parent_cluster,
            .buffer_size            = 0,
    };
    src_last_index = parse_path(src_path, src_path_list);
    memcpy(request_delete.name, src_path_list[src_last_index], strlen(src_path_list[src_last_index]));

    int32_t ret_code;
    syscall(DELETE_SYSCALL, (uint32_t) &request_delete, (uint32_t) &ret_code, 0);

    if (ret_code != 0) {
        print_weird_error();
    }

}