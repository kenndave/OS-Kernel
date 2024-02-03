#ifndef TUBESOS_MKDIR_H
#define TUBESOS_MKDIR_H

#include "../../lib-header/stdtype.h"

void mkdir(char *path);
void mkdir_parent(char *folder_name, uint32_t parent_cluster);

#endif //TUBESOS_MKDIR_H
