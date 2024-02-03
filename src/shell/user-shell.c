#include "../lib-header/stdtype.h"
#include "../lib-header/fat32.h"
#include "../lib-header/stdmem.h"

#include "shell-header/systemcall.h"
#include "shell-header/shell-helper.h"
#include "shell-header/shell-state.h"
#include "shell-header/ls.h"
#include "shell-header/cd.h"
#include "shell-header/rm.h"
#include "shell-header/mkdir.h"
#include "shell-header/cat.h"
#include "shell-header/cp.h"
#include "shell-header/whereis.h"
#include "shell-header/mv.h"

#include "../libc-header/str.h"

struct ShellState shell_state = {
        .user_name = "tubesos",
        .host_name = "OS-IF2230",
        .current_folder_cluster = ROOT_CLUSTER_NUMBER
};

void write_signature() {
    syscall(PUTS_SYSCALL, (uint32_t) shell_state.user_name, (uint32_t) 7, (uint32_t) 0xA);
    syscall(PUTS_SYSCALL, (uint32_t) "@", 1, 0xA);
    syscall(PUTS_SYSCALL, (uint32_t) shell_state.host_name, 9, 0xA);
    syscall(PUTS_SYSCALL, (uint32_t) ":/", 2, 0x7);

    int i = 0;
    while (shell_state.current_path_list[i][0] != '\0') {
        if (i > 0) {
            syscall(PUTS_SYSCALL, (uint32_t) "/", 1, 0x7);
        }
        syscall(PUTS_SYSCALL, (uint32_t) shell_state.current_path_list[i], strlen(shell_state.current_path_list[i]), 0x7);
        ++i;
    }
    syscall(PUTS_SYSCALL, (uint32_t) " $\0", 3, 0xF);
}

int main(void) {
    // 0xf = putih 0xe = kuning 0xd = ungu 0xc = merah 0xb = biru 0xa = hijau
    char keyboard_buffer[256] = {0};
    shell_state.current_path_list[0][0] = '\0';
    shell_state.debug_mode = TRUE;
    uint32_t i = 0;

    while (TRUE) {
        write_signature();
        syscall(FGETS_SYSCALL, (uint32_t) keyboard_buffer, 256, 0);

        char commands[10][32];
        uint32_t argc = parse_command(keyboard_buffer, commands);

        if (argc == 0) {
            continue;
        }

        char lsc[] = "ls", mkdirc[] = "mkdir", cdc[] = "cd", catc[] = "cat", cpc[] = "cp", rmc[] = "rm",
                whereisc[] = "whereis", mvc[] = "mv", d_r[] = "-r";

        if (strcmp(commands[0], lsc) != 0) {
            char root_folder[] = ".";
            if (argc == 1) {
                ls(root_folder);

            } else {
                for(i = 1 ; i<argc ; i++) {
                    syscall(PUTS_SYSCALL, (uint32_t) commands[i], strlen(commands[i]), 0xb);
                    syscall(PUTS_SYSCALL, (uint32_t) "\n", 1, 0xF);
                    ls(commands[i]);
                    //syscall(PUTS_SYSCALL, "\n", 1, 0xF);
                }
            }
        } else if (strcmp(commands[0], mkdirc) != 0) {
            if(argc > 1 ) {
                for(i = 1 ; i<argc ; i++) {
                    mkdir(commands[i]);
                }
            }
            else{
                syscall(PUTS_SYSCALL, (uint32_t) "wrong command\n", 14, 0xc);
                syscall(PUTS_SYSCALL, (uint32_t) "How to use : mkdir [Name1] [Name2] ...\n", 39, 0xF);
                continue;
            }

        } else if (strcmp(commands[0], cdc) != 0) {
            if (argc != 2) {
                syscall(PUTS_SYSCALL, (uint32_t) "wrong command\n", 14, 0xc);
                syscall(PUTS_SYSCALL, (uint32_t) "How to use : cd [Path]\n", 23, 0xF);
                continue;
            }
            else{
                cd(commands[1]);
            }

        } else if (strcmp(commands[0], catc) != 0) {
            if(argc < 2 ){
                syscall(PUTS_SYSCALL, (uint32_t) "wrong command\n", 14, 0xc);
                syscall(PUTS_SYSCALL, (uint32_t) "How to use : cat [File1] [File2] ...\n", 37, 0xF);

            }
            else {
                for(i = 1 ; i < argc ; i++) {
                    syscall(PUTS_SYSCALL, (uint32_t) commands[i], strlen(commands[i]), 0xb);
                    syscall(PUTS_SYSCALL, (uint32_t) ": ", 2, 0xb);
                    cat(commands[i]);
                }
            }
        } else if (strcmp(commands[0], cpc) != 0) {
            bool boolcp = TRUE ;
            if(argc<2){
                syscall(PUTS_SYSCALL, (uint32_t) "wrong command\n", 14, 0xc);
                syscall(PUTS_SYSCALL, (uint32_t) "How to use : cp [source1] [source2] ... [destination] \n", 55, 0xF);
                continue;
            }
            else{
                if(strcmp(commands[1],d_r)) {
                    if(argc < 4){
                        syscall(PUTS_SYSCALL, (uint32_t) "wrong command\n", 14, 0xc);
                        syscall(PUTS_SYSCALL, (uint32_t) "How to use : cp -r [source1] [source2] ... [destination] \n", 58, 0xF);
                        continue;
                    }
                    else{
                        for (i = 2; i < argc - 1; i++) {
                            cp(commands[i], commands[(argc - 1)], boolcp);
                        }
                    }
                }
                else{
                    boolcp = FALSE;
                    for(i = 1 ; i<argc-1 ; i++){
                        cp(commands[i], commands[(argc-1)], boolcp );
                    }

                }
            }


        } else if (strcmp(commands[0], rmc) != 0) {
            bool boolrm = TRUE;
            if(argc < 2){
                syscall(PUTS_SYSCALL, (uint32_t) "wrong command\n", 14, 0xc);
                syscall(PUTS_SYSCALL, (uint32_t) "How to use : rm [file1] [file2] ... \n", 37, 0xF);
                continue;
            }
            else{
                if(strcmp(commands[1],d_r)) {
                    if(argc < 3) {
                        syscall(PUTS_SYSCALL, (uint32_t) "wrong command\n", 14, 0xc);
                        syscall(PUTS_SYSCALL, (uint32_t) "How to use : rm -r [dir1] ... \n",
                                31, 0xF);
                        continue;
                    }
                    else{
                    for(i = 2 ; i<argc ; i++) {
                        rm(commands[i], boolrm);
                    }
                    }
                }
                else {
                    boolrm = FALSE;
                    for (i = 1; i < argc; i++) {
                    rm(commands[i], boolrm);
                    }
                }
            }


        }
        else if (strcmp(commands[0], mvc) != 0){
            if(argc != 3 ){
                syscall(PUTS_SYSCALL, (uint32_t) "wrong command\n", 14, 0xc);
                syscall(PUTS_SYSCALL, (uint32_t) "How to use : mv [file1] [file2] ... \n", 37, 0xF);
                continue;
            }
            else{
                    mv(commands[1],commands[2]);


            }

        }
        else if (strcmp(commands[0], whereisc) != 0) {
            if (argc < 2){
                syscall(PUTS_SYSCALL, (uint32_t) "wrong command\n", 14, 0xc);
                syscall(PUTS_SYSCALL, (uint32_t) "how to use : whereis [options]\n", 31, 0xc);
                continue;
            }
            else {
                for (i = 1; i < argc; i++) {
                    whereis(commands[i], ROOT_CLUSTER_NUMBER, "");
                }
            }

        } else {
            syscall(PUTS_SYSCALL, (uint32_t) "wrong command\n", 14, 0xc);
            syscall(PUTS_SYSCALL, (uint32_t) "command list : \ncd\nls\nmkdir\ncat\ncp\nrm\nmv\nwhereis\n", 49, 0xF);
        }

    }

}