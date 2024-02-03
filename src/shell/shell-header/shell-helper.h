#ifndef TUBESOS_SYSCALL_HELPER_H
#define TUBESOS_SYSCALL_HELPER_H

#include "../../lib-header/stdtype.h"
#include "../../lib-header/fat32.h"

#define MAX_PATH_LENGTH         32

#define READ_SYSCALL            0
#define READ_DIR_SYSCALL        1
#define WRITE_SYSCALL           2
#define DELETE_SYSCALL          3
#define MOVE_SYSCALL            4
#define FGETS_SYSCALL           5
#define PUTS_SYSCALL            6

// return last element index
int parse_path(const char *path, char path_list[MAX_PATH_LENGTH][13]);

uint32_t form_cluster_number(uint16_t cluster_high, uint16_t cluster_low);

void parse_filename(char *origin, char *filename, char *ext);

int32_t get_path_cluster(const char *path, uint32_t *parent_cluster);

int32_t get_dir_table(const char *path, struct FAT32DirectoryTable *dir_table, uint32_t *parent_cluster);

int32_t prev_path_cluster(const char *path, uint32_t *parent_cluster);

int32_t prev_dir_table(const char *path, struct FAT32DirectoryTable *dir_table, uint32_t *parent_cluster);

uint32_t parse_command(const char *command, char res[10][32]);

bool compare_filename(char *name, char *ext, char *filename);

void print_weird_error();

void print_file_nfound();

void print_already_exists();

#endif //TUBESOS_SYSCALL_HELPER_H
