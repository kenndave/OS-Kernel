#include "libc-header/str.h"

uint32_t strlen(const char *str) {
    uint32_t len = 0;
    while (str[len] != '\0') {
        ++len;
    }
    return len;
}

void strcpy(char *str1, char *str2, uint32_t len) {
    uint32_t i;
    for (i = 0; i < len && str2[i] != '\0'; i++) {
        str1[i] = str2[i];
    }
    str1[i] = '\0';
}

bool strcmp(char *str1, char *str2) {
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) {
            return FALSE;
        }
        i++;
    }
    return (str1[i] == '\0' && str2[i] == '\0');
}

void strerase(char *str) {
    int i = 0;
    while (str[i] != '\0') {
        str[i] = '\0';
    }
}