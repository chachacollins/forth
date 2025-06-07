// nob.c
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};
    char* cc = "gcc";
    cmd_append(&cmd, cc);
    cmd_append(&cmd, "-o", "forth");
    cmd_append(&cmd, "main.c", "lexer.c", "compiler.c");
    cmd_append(&cmd, "-Wextra", "-Wall", "-Werror", "-std=c11");
    if (!nob_cmd_run_sync(cmd)) return 1;
    return 0;
}
