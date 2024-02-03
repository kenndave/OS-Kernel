#include "shell-header/cd.h"
#include "shell-header/shell-state.h"
#include "shell-header/shell-helper.h"
#include "shell-header/systemcall.h"

#include "../libc-header/str.h"

extern struct ShellState shell_state;

void cd(char *path) {
    uint32_t parent_cluster = shell_state.current_folder_cluster;

    if (get_path_cluster(path, &parent_cluster) == -1) {
        print_file_nfound();
        return;
    }

    shell_state.current_folder_cluster = parent_cluster;

    // weird but funny bug: calling the same command twice causes path_list to have the exact
    // same memory address so that it'll leave the memory trace as it is
    // to avoid this annoying thing, please initialize every array with = {0}
    char path_list[MAX_PATH_LENGTH][13] = {0};
    parse_path(path, path_list);

    // i: path_list index; k: shell_state.current_path_list index
    uint32_t i = 0, k = 0;
    if (path[0] != '/') {
        while (shell_state.current_path_list[k][0] != '\0') {
            ++k;
        }
    }

    while (path_list[i][0] != '\0') {
        if (strcmp("..", path_list[i])) {
            if (k > 0)
                --k;
            ++i;
            continue;
        }
        if (strcmp(".", path_list[i])) {
            ++i;
            continue;
        }

        strcpy(shell_state.current_path_list[k], path_list[i], strlen(path_list[i]));
        ++i; ++k;
    }

    shell_state.current_path_list[k][0] = '\0';
}