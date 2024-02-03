#ifndef TUBESOS_STR_H
#define TUBESOS_STR_H

#include "../lib-header/stdtype.h"

bool strcmp(char *str1, char *str2);

void strcpy(char *dest, char *src, uint32_t len);

uint32_t strlen(const char *str);

void strerase(char *str);

#endif //TUBESOS_STR_H
