#define NOB_IMPLEMENTATION
#define NOB_WARN_DEPRECATED
#define NOB_EXPERIMENTAL_DELETE_OLD

#include "nob.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define SEPARATOR_GREEN  "\033[32m==============================\033[0m\n"
#define SEPARATOR_RED    "\033[31m==============================\033[0m\n"

#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_RESET "\033[0m"

int main(int argc, char **argv){
    NOB_GO_REBUILD_URSELF(argc, argv);

    Nob_Cmd cmd = {0};
    // 1. gcc
    printf(ANSI_GREEN"=================[nob_build]================="ANSI_RESET"\n");
    nob_cmd_append(
        &cmd,
        "gcc",
        "src/main.c",
        "-o",
        "build/main",
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-fdiagnostics-color=always",
        "-lm"
    );


    if (!nob_cmd_run(&cmd)){
        printf(ANSI_RED"=================[nob_build]================="ANSI_RESET"\n");
        return 1;
    }

    // 2. run 
    printf(ANSI_GREEN"=================[nob_run]================="ANSI_RESET"\n");
    nob_cmd_append(&cmd, "./build/main");
    for (int i = 1; i < argc; i ++){
        nob_cmd_append(&cmd, argv[i]);
    }

    if (!nob_cmd_run(&cmd)){
        printf(ANSI_RED"=================[nob_run]================="ANSI_RESET"\n");
        return 1;
    }

    // nob_cmd_free(&cmd);
    
    return 0;
}