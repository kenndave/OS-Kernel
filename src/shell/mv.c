#include "shell-header/mv.h"
#include "shell-header/systemcall.h"
#include "shell-header/shell-state.h"
#include "shell-header/shell-helper.h"

#include "../libc-header/str.h"

extern struct ShellState shell_state;

void handle_error(int32_t return_code) {
    if (return_code == 1) {
        print_file_nfound();
    }
    if (return_code == 3) {
        print_already_exists();
    }
    if (return_code != 0) {
        print_weird_error();
    }
}

void mv(char *src_path, char *dest_path) {
    uint32_t src_parent_cluster = shell_state.current_folder_cluster,
                dest_parent_cluster = shell_state.current_folder_cluster;
    char src_path_list[MAX_PATH_LENGTH][13] = {0}, dest_path_list[MAX_PATH_LENGTH][13] = {0};
    uint32_t src_last_index = parse_path(src_path, src_path_list), dest_last_index;

    struct FAT32DirectoryTable src_dir_table, dest_dir_table;

    // setup src
    bool src_is_dir = TRUE;
    if (get_dir_table(src_path_list[src_last_index], &src_dir_table, &src_parent_cluster) == -1) {
        src_parent_cluster = shell_state.current_folder_cluster;
        if (prev_dir_table(src_path, &src_dir_table, &src_parent_cluster) == -1) {
            // src path not valid
            char err[] = "error: source not valid\n";
            syscall(PUTS_SYSCALL, (uint32_t) err, strlen(err), 0xF);
            return;
        }
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

    struct FAT32DriverRequest request_src = {
            .name = {0},
            .ext = {0}
    },
    request_dest = {
            .name = {0},
            .ext = {0}
    };
    struct FAT32DirectoryEntry src_parent_folder;

    if (src_is_dir) {
        dest_last_index = parse_path(dest_path, dest_path_list);
        char *src_folder_name = src_path_list[src_last_index], *dest_folder_name = dest_path_list[dest_last_index];

        src_parent_folder = src_dir_table.table[0];

        request_src.parent_cluster_number = form_cluster_number(src_parent_folder.cluster_high,
                                                                src_parent_folder.cluster_low);

        // setup src
        strcpy(request_src.name, src_folder_name, strlen(src_folder_name));
        request_src.buffer_size = 0;

        // setup dest
        if (dest_is_dir) {
            strcpy(request_dest.name, src_folder_name, strlen(src_folder_name));
        } else {
            strcpy(request_dest.name, dest_folder_name, strlen(dest_folder_name));
        }
        request_dest.parent_cluster_number = dest_parent_cluster;
        request_dest.buffer_size = 0;

        // make system call
        int32_t ret_code;
        syscall(MOVE_SYSCALL, (uint32_t) &request_src, (uint32_t) &request_dest, (uint32_t) &ret_code);

        if (ret_code != 0) {
            handle_error(ret_code);
        }
        return;
    }

    // setup source request
    request_src.buffer_size = 1;
    char name[8] = {0}, ext[3] = {0};
    parse_filename(src_path_list[src_last_index], name, ext);

    strcpy(request_src.name, name, strlen(name));
    strcpy(request_src.ext, ext, strlen(ext));

    // setup destination request
    if (dest_is_dir) {
        strcpy(request_dest.name, name, strlen(name));
        strcpy(request_dest.ext, ext, strlen(ext));
    } else {
        dest_last_index = parse_path(dest_path, dest_path_list);
        parse_filename(dest_path_list[dest_last_index], request_dest.name, request_dest.ext);
    }

    request_dest.parent_cluster_number = dest_parent_cluster;
    request_dest.buffer_size = 1;

    src_parent_folder = src_dir_table.table[1];
    request_src.parent_cluster_number = form_cluster_number(src_parent_folder.cluster_high,
                                                            src_parent_folder.cluster_low);
    // make system call
    int32_t ret_code = 0;
    syscall(MOVE_SYSCALL, (uint32_t) &request_src, (uint32_t) &request_dest, (uint32_t) &ret_code);

    if (ret_code != 0) {
        handle_error(ret_code);
    }

}