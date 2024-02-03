#include "shell-header/cat.h"
#include "shell-header/shell-helper.h"
#include "shell-header/shell-state.h"
#include "shell-header/systemcall.h"

#include "../libc-header/str.h"

extern struct ShellState shell_state;

void cat(char *file_path) {
    uint32_t parent_cluster = shell_state.current_folder_cluster;
    char path_list[MAX_PATH_LENGTH][13] = {0};
    uint32_t last_index = parse_path(file_path, path_list);

    struct FAT32DirectoryTable dir_table;

    if (prev_dir_table(file_path, &dir_table, &parent_cluster) == -1) {
        print_file_nfound();
        return;
    }

    struct FAT32DriverRequest request = {
            .buf                    = 0,
            .name                   = "",
            .ext                    = "\0\0\0",
            .parent_cluster_number  = parent_cluster,
            .buffer_size            = 0,
    };
    last_index = parse_path(file_path, path_list);
    parse_filename(path_list[last_index], request.name, request.ext);
    request.parent_cluster_number = parent_cluster;

    int32_t ret_code;
    syscall(READ_SYSCALL, (uint32_t) &request, (uint32_t) &ret_code, 0);

    // weird
    if (ret_code == 0) {
        print_weird_error();
        return;
    }

    if (ret_code == 2) {
        char buff[request.buffer_size/sizeof(char)];
        request.buf = buff;

        syscall(READ_SYSCALL, (uint32_t) &request, (uint32_t) &ret_code, 0);

        if (ret_code != 0) {
            char err[] = "error while reading the file\n";
            syscall(PUTS_SYSCALL, (uint32_t) err, strlen(err), 0xF);
            return;
        }

        syscall(PUTS_SYSCALL, (uint32_t) buff, strlen(buff), 0xF);
        syscall(PUTS_SYSCALL, (uint32_t) "\n", 1, 0);
        return;
    }

    print_file_nfound();
}