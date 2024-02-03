#ifndef TUBESOS_SHELL_STATE_H
#define TUBESOS_SHELL_STATE_H

#include "../../lib-header/stdtype.h"
#include "shell-helper.h"

/*
 *
 * */
struct ShellState {
    char        *user_name;
    char        *host_name;
    char        current_path_list[MAX_PATH_LENGTH][8];
    uint32_t    current_folder_cluster;
    bool        debug_mode;
};

#endif //TUBESOS_SHELL_STATE_H
