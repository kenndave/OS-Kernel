#include "shell-header/shell-helper.h"
#include "shell-header/systemcall.h"
#include "shell-header/shell-state.h"

#include "../lib-header/stdmem.h"

#include "../libc-header/str.h"

extern struct ShellState shell_state;

uint32_t form_cluster_number(uint16_t cluster_high, uint16_t cluster_low) {
    return ((uint32_t)cluster_high << 16) | cluster_low;
}

int parse_path(const char *path, char path_list[MAX_PATH_LENGTH][13]) {
    int i = 0, j = 0, k = 0;

    if (path[0] == '/') {
        k += 1;
    }

    while (path[k] != '\0') {
        if (path[k] == '/') {
            path_list[i][j] = '\0';
            ++i;++k;
            j = 0;
            continue;
        }
        path_list[i][j] = path[k];
        ++j;
        ++k;
    }
    if (j > 0) {
        path_list[i][j] = '\0';
    }
    path_list[i+1][0] = '\0';

    return i;
}

int32_t get_path_cluster(const char *path, uint32_t *parent_cluster) {
    struct FAT32DirectoryTable dir_table;
    return get_dir_table(path, &dir_table, parent_cluster);
}

int32_t get_dir_table(const char *path, struct FAT32DirectoryTable *dir_table, uint32_t *parent_cluster) {
    char path_list[MAX_PATH_LENGTH][13] = {0};
    parse_path(path, path_list);
    int i = 0;

    if (path[0] == '/') {
        *parent_cluster = ROOT_CLUSTER_NUMBER;
    }

    int32_t ret_code;

    while (i < MAX_PATH_LENGTH && path_list[i][0] != '\0') {
        char *current_path = path_list[i];

        if (strlen(current_path) > 8) {
            return -1;
        }

        struct FAT32DriverRequest request = {
                .buf                    = dir_table,
                .name                   = "",
                .ext                    = "\0\0\0",
                .parent_cluster_number  = *parent_cluster,
                .buffer_size            = sizeof(struct FAT32DirectoryTable),
        };
        memcpy(request.name, current_path, 8);

        syscall(READ_DIR_SYSCALL, (uint32_t) &request, (uint32_t) &ret_code, 0);

        if (ret_code != 0) {
            return -1;
        }

        // assuming dir_table is in the correct format (0 for parent reference, 1 for self reference)
        struct FAT32DirectoryEntry self_entry = dir_table->table[1];
        *parent_cluster = form_cluster_number(self_entry.cluster_high, self_entry.cluster_low);

        ++i;
    }

    return 0;
}

int32_t prev_path_cluster(const char *path, uint32_t *parent_cluster) {
    struct FAT32DirectoryTable dir_table;
    return prev_dir_table(path, &dir_table, parent_cluster);
}

int32_t prev_dir_table(const char *path, struct FAT32DirectoryTable *dir_table, uint32_t *parent_cluster) {
    char path_list[MAX_PATH_LENGTH][13] = {};
    parse_path(path, path_list);
    int i = 0;

    if (path[0] == '/') {
        *parent_cluster = ROOT_CLUSTER_NUMBER;
    }

    int32_t ret_code;

    while ((i < MAX_PATH_LENGTH -1 && path_list[i + 1][0] != '\0') || i == 0) {
        char *current_path = path_list[i];

        if (strlen(current_path) > 8) {
            return -1;
        }

        // to handle a condition where this called on root dir
        if (path_list[i+1][0] == '\0') {
            strcpy(current_path, ".", 1);
        }

        struct FAT32DriverRequest request = {
                .buf                    = dir_table,
                .name                   = "",
                .ext                    = "\0\0\0",
                .parent_cluster_number  = *parent_cluster,
                .buffer_size            = sizeof(struct FAT32DirectoryTable),
        };
        memcpy(request.name, current_path, strlen(current_path));

        syscall(READ_DIR_SYSCALL, (uint32_t) &request, (uint32_t) &ret_code, 0);

        if (ret_code != 0) {;
            return -1;
        }

        // assuming dir_table is in the correct format (0 for parent reference, 1 for self reference)
        struct FAT32DirectoryEntry self_entry = dir_table->table[1];
        *parent_cluster = form_cluster_number(self_entry.cluster_high, self_entry.cluster_low);

        ++i;
    }

    return 0;
}

void parse_filename(char *origin, char *filename, char *ext) {
    int i = 0, j = -1, k = 0;
    while (origin[i] != '\0') {
        if (origin[i] == '.') {
            j = i;
        }
        i++;
    }

    if (j == -1) {
        i = 0;
        while (origin[i] != '\0') {
            filename[i] = origin[i];
            i++;
        }
        filename[i] = '\0';
        ext[0] = '\0';
    } else {
        i = 0;
        while (i < j) {
            filename[i] = origin[i];
            i++;
        }
        filename[i] = '\0';

        j++;
        while (origin[j] != '\0') {
            if (origin[j] == '.') {
                k = 0;
            }
            ext[k] = origin[j];
            j++;
            k++;
        }
        ext[k] = '\0';
    }
}


uint32_t parse_command(const char *command, char res[10][32]){
    if (command[0] == '\0') {
        return 0;
    }

    int i, j, k;
    i = j = k = 0;

    while (command[i] != '\0') {
        if (command[i] == ' ') {
            res[j][k] = '\0';
            j++;
            k = 0;
        }

        else {
            res[j][k] = command[i];
            k++;
        }
        i++;
    }


    res[j][k] = '\0';
    res[j+1][0] = '\0';

    return j+1;
}

bool compare_filename(char *name, char *ext, char *filename){
    char parsed_filename[8];
    char parsed_ext[3];

    parse_filename(filename, parsed_filename, parsed_ext);

    if (strcmp(parsed_filename, name) && strcmp(parsed_ext, ext)){
        return TRUE;
    }
    else{
        return FALSE;
    }
}

void print_weird_error() {
    char err[] = "error: sumthin weird happened\n";
    syscall(PUTS_SYSCALL, (uint32_t) err, strlen(err), 0xF);
}

void print_file_nfound() {
    char err[] = "error: file not found\n";
    syscall(PUTS_SYSCALL, (uint32_t) err, strlen(err), 0xF);
}

void print_already_exists() {
    char err[] = "error: file already exists\n";
    syscall(PUTS_SYSCALL, (uint32_t) err, strlen(err), 0xF);
}