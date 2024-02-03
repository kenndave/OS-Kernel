#include "shell-header/cp.h"
#include "shell-header/shell-helper.h"
#include "shell-header/shell-state.h"
#include "shell-header/systemcall.h"
#include "shell-header/mkdir.h"

#include "../libc-header/str.h"

extern struct ShellState shell_state;

void get_file_names(struct FAT32DirectoryTable dir_table, char filenames[256][13]) {
    int filename_idx = 0;
    for (uint32_t i = 2; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); ++i) {
        struct FAT32DirectoryEntry current = dir_table.table[i];
        if (current.user_attribute != UATTR_NOT_EMPTY || current.filesize == 0) {
            continue;
        }
        uint32_t len_current = strlen(current.name);
        strcpy(filenames[filename_idx], current.name, len_current);
        strcpy(filenames[filename_idx]+len_current, ".", 1);
        strcpy(filenames[filename_idx]+len_current+1, current.ext, strlen(current.ext));
        ++filename_idx;

    }
}

void cp(char *src_path, char *dest_path, bool recursive) {
    uint32_t src_parent_cluster = shell_state.current_folder_cluster,
                dest_parent_cluster = shell_state.current_folder_cluster;
    char src_path_list[MAX_PATH_LENGTH][13] = {0}, dest_path_list[MAX_PATH_LENGTH][13] = {0};
    uint32_t src_last_index = parse_path(src_path, src_path_list), dest_last_index = 0;

    struct FAT32DirectoryTable src_dir_table, dest_dir_table, temp_dir_table;

    if (prev_dir_table(src_path, &src_dir_table, &src_parent_cluster) == -1) {
        // src path not valid
        char err[] = "error: source not valid\n";
        syscall(PUTS_SYSCALL, (uint32_t) err, strlen(err), 0xF);
        return;
    }

    // setup src
    bool src_is_dir = TRUE;
    uint32_t temp_cluster = shell_state.current_folder_cluster;
    if (get_dir_table(src_path_list[src_last_index], &temp_dir_table, &temp_cluster) == -1) {
        src_is_dir = FALSE;
    }

    // setup dest
    bool dest_is_dir = TRUE;
    // check if dest path is a directory
    if (get_dir_table(dest_path, &dest_dir_table, &dest_parent_cluster) == -1) {
        // check dest parent folder
        dest_parent_cluster = shell_state.current_folder_cluster;
        if (prev_dir_table(dest_path, &dest_dir_table, &dest_parent_cluster) == -1) {
            // dest path not valid
            char err[] = "error: destination not valid\n";
            syscall(PUTS_SYSCALL, (uint32_t) err, strlen(err), 0xF);
            return;
        }
        dest_is_dir = FALSE;
    }

    if (!recursive || !src_is_dir) {
        struct FAT32DriverRequest request_read = {
                .buf                    = 0,
                .name                   = "",
                .ext                    = "\0\0\0",
                .parent_cluster_number  = src_parent_cluster,
                .buffer_size            = 0,
        };

        if (src_is_dir) {
            char err[] = "error: recursive flag not enabled\n";
            syscall(PUTS_SYSCALL, (uint32_t) err, strlen(err), 0xF);
            return;
        }

        char name[8], ext[3];
        parse_filename(src_path_list[src_last_index], name, ext);

        bool found_file = FALSE;
        for (uint32_t i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); ++i) {
            struct FAT32DirectoryEntry current = src_dir_table.table[i];

            if (current.user_attribute != UATTR_NOT_EMPTY) {
                continue;
            }

            if (strcmp(current.name, name) && strcmp(current.ext, ext)) {
                found_file = TRUE;
                strcpy(request_read.name, current.name, strlen(current.name));
                strcpy(request_read.ext, current.ext, strlen(current.ext));
                request_read.buffer_size = current.filesize;

                break;
            }
        }

        if (!found_file) {
            print_file_nfound();
            return;
        }

        uint32_t size = request_read.buffer_size+1;
        char src[request_read.buffer_size+1];
        request_read.buf = src;
        request_read.buffer_size = size;
        request_read.parent_cluster_number = src_parent_cluster;

        int32_t ret_code = 0;
        syscall(READ_SYSCALL, (uint32_t) &request_read, (uint32_t) &ret_code, 0);

        if (ret_code != 0) {
            print_weird_error();
            return;
        }

        struct FAT32DriverRequest request_write = {
                .buf                    = src,
                .name                   = "",
                .ext                    = "",
                .parent_cluster_number  = dest_parent_cluster,
                .buffer_size            = size,
        };

        if (dest_is_dir) {
            parse_filename(src_path_list[src_last_index], request_write.name, request_write.ext);
        } else {
            dest_last_index = parse_path(dest_path, dest_path_list);
            parse_filename(dest_path_list[dest_last_index], request_write.name, request_write.ext);
        }
        request_write.parent_cluster_number = dest_parent_cluster;
        request_write.buffer_size = size;

        syscall(WRITE_SYSCALL, (uint32_t) &request_write, (uint32_t) &ret_code, 0);

        if (ret_code == 1) {
            char err[] = "error: file already exists or invalid format.\n";
            syscall(PUTS_SYSCALL, (uint32_t) err, strlen(err), 0xF);
            return;
        }
        if (ret_code != 0) {
            print_weird_error();
            return;
        }

        return;
    }

    struct FAT32DirectoryTable new_dir_table;
    struct FAT32DriverRequest request_new_dir = {
            .buf                    = &new_dir_table,
            .name                   = "",
            .ext                    = "",
            .parent_cluster_number  = dest_parent_cluster,
            .buffer_size            = sizeof(struct FAT32DirectoryTable),
    };

    if (dest_is_dir) {
        src_last_index = parse_path(src_path, src_path_list);
        mkdir_parent(src_path_list[src_last_index], dest_parent_cluster);
        strcpy(request_new_dir.name, src_path_list[src_last_index], strlen(src_path_list[src_last_index]));
    } else {
        dest_last_index = parse_path(dest_path, dest_path_list);
        mkdir_parent(dest_path_list[dest_last_index], dest_parent_cluster);
        strcpy(request_new_dir.name, dest_path_list[dest_last_index], strlen(dest_path_list[dest_last_index]));
    }

    int32_t ret_code;
    syscall(READ_DIR_SYSCALL, (uint32_t) &request_new_dir, (uint32_t) &ret_code, 0);

    if (ret_code != 0) {
        print_weird_error();
        return;
    }

    struct FAT32DirectoryEntry new_entry = new_dir_table.table[1];
    dest_parent_cluster = form_cluster_number(new_entry.cluster_high, new_entry.cluster_low);

    for (uint32_t i = 2; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); ++i) {
        struct FAT32DirectoryEntry current = temp_dir_table.table[i];
        if (current.user_attribute != UATTR_NOT_EMPTY) {
            continue;
        }

        // use cp_recursive to avoid this big mess
        if (current.attribute == ATTR_SUBDIRECTORY) {
            uint32_t old_src_len = strlen(src_path), old_dest_len = strlen(dest_path),
                        append_len = strlen(current.name);
            char new_src_path[old_src_len + append_len + 1], new_dest_path[old_dest_len + append_len + 1],
                    slash[] = "/";

            // setup new path for src
            strcpy(new_src_path, src_path, old_src_len);
            strcpy(new_src_path+old_src_len, slash, 1);
            strcpy(new_src_path+old_src_len+1, current.name, append_len);

            // setup new path for destination
            strcpy(new_dest_path, dest_path, old_dest_len);
            strcpy(new_dest_path+old_dest_len, slash, 1);
            strcpy(new_dest_path+old_dest_len+1, current.name, append_len);

            syscall(PUTS_SYSCALL, (uint32_t) new_src_path, strlen(new_src_path), 0xF);
            syscall(PUTS_SYSCALL, (uint32_t) "\n", 1, 0x0);
            syscall(PUTS_SYSCALL, (uint32_t) new_dest_path, strlen(new_dest_path), 0xF);
            syscall(PUTS_SYSCALL, (uint32_t) "\n", 1, 0x0);
            cp(new_src_path, new_dest_path, recursive);
            continue;
        }

        struct FAT32DriverRequest request_write = {
                .buf                    = 0,
                .name                   = "",
                .ext                    = "",
                .parent_cluster_number  = dest_parent_cluster,
                .buffer_size            = 0,
        };

        char src[current.filesize+1];
        request_write.buf = src; request_write.buffer_size = current.filesize+1;

        strcpy(request_write.name, current.name, strlen(current.name));
        strcpy(request_write.ext, current.ext, strlen(current.ext));

        int32_t ret_code = 0;
        syscall(WRITE_SYSCALL, (uint32_t) &request_write, (uint32_t) &ret_code, 0);

        if (ret_code == 1) {
            char err[] = "error: file already exists or invalid format.\n";
            syscall(PUTS_SYSCALL, (uint32_t) err, strlen(err), 0xF);
            return;
        }
        if (ret_code != 0) {
            print_weird_error();
            return;
        }

    }

}