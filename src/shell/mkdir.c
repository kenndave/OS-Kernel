#include "shell-header/mkdir.h"
#include "shell-header/systemcall.h"
#include "shell-header/shell-state.h"
#include "shell-header/shell-helper.h"

#include "../lib-header/stdmem.h"
#include "../libc-header/str.h"

extern struct ShellState shell_state;

void mkdir_parent(char *folder_name, uint32_t parent_cluster) {
    if (strlen(folder_name) > 7) {
        char err[] = "max folder name length: 7 char\n";
        syscall(PUTS_SYSCALL, (uint32_t) err, strlen(err), 0xF);
        return;
    }

    struct FAT32DriverRequest request = {
            .buf                    = 0,
            .name                   = "",
            .ext                    = "\0\0\0",
            .parent_cluster_number  = parent_cluster,
            .buffer_size            = 0,
    };
    memcpy(request.name, folder_name, strlen(folder_name));

    uint32_t ret_code = 0;
    request.buffer_size = 0;
    syscall(WRITE_SYSCALL, (uint32_t) &request, (uint32_t) &ret_code, 0);

    if (ret_code == 1) {
        print_already_exists();
        return;
    }

    if (ret_code != 0) {
        print_weird_error();
        return;
    }
}

void mkdir(char *path) {
    uint32_t parent_cluster = shell_state.current_folder_cluster;
    char path_list[MAX_PATH_LENGTH][13] = {0};
    uint32_t last_index = parse_path(path, path_list);

    int32_t ret_code;
    struct FAT32DirectoryTable dir_table;

    if (prev_dir_table(path, &dir_table, &parent_cluster) == -1) {
        print_file_nfound();
        return;
    }

    if (strlen(path_list[last_index]) > 7) {
        char err[] = "error: max folder name length is 7 char\n";
        syscall(PUTS_SYSCALL, (uint32_t) err, strlen(err), 0xF);
        return;
    }

    struct FAT32DriverRequest request = {
            .buf                    = 0,
            .name                   = "",
            .ext                    = "\0\0\0",
            .parent_cluster_number  = parent_cluster,
            .buffer_size            = 0,
    };
    last_index = parse_path(path, path_list);

    memcpy(request.name, path_list[last_index], strlen(path_list[last_index]));

    syscall(WRITE_SYSCALL, (uint32_t) &request, (uint32_t) &ret_code, 0);

    if (ret_code == 1) {
        print_already_exists();
        return;
    }

    if (ret_code != 0) {
        print_weird_error();
        return;
    }
}