#define NOB_IMPLEMENTATION
#define NOB_EXPERIMENTAL_DELETE_OLD
#include "3rdparty/nob.h"

typedef enum {
    HELP,
    INTERP,
    DBCACHE,
    TBCACHE,
    CACHE,
    DEBUG,
    TEST,
    LIBRARY,
    CLEAN,
    INVALID
} Flag;

static Flag what_flag(int argc, char **argv)
{
    if (argc == 1) return TBCACHE;
    if (argc > 2) return INVALID;
    const char *flag_str = argv[1];
    if (strcmp(flag_str, "help")    == 0) return HELP;
    if (strcmp(flag_str, "interp")  == 0) return INTERP;
    if (strcmp(flag_str, "dbcache") == 0) return DBCACHE;
    if (strcmp(flag_str, "tbcache") == 0) return TBCACHE;
    if (strcmp(flag_str, "cache")   == 0) return CACHE;
    if (strcmp(flag_str, "debug")   == 0) return DEBUG;
    if (strcmp(flag_str, "test")    == 0) return TEST;
    if (strcmp(flag_str, "lib")     == 0) return LIBRARY;
    if (strcmp(flag_str, "clean")   == 0) return CLEAN;
    return INVALID;
}

static void print_help_message(void)
{
    printf(
        "======================================\n"
        " ./nob [OPTION]                       \n"
        "======================================\n"
        "  help:     Display this information  \n"
        "  clean:    Clean generated files     \n"
        "  interp:   Interp only               \n"
        "  dbcache:  Interp with decode cache  \n"
        "  tbcache:  Interp with code cache    \n"
        "  lib:      Build library             \n"
        "  test:     Build test version        \n"
        "  debug:    Build debug version       \n"
        "======================================\n"
    );
}

static struct {
    char **items;
    size_t count;
    size_t capacity;
} cflags = {0}, cldflags = {0};

#define da_appendw(da, ...) \
    do { \
        char *new_items[] = { __VA_ARGS__ }; \
        da_append_many(da, new_items, ARRAY_LEN(new_items)); \
    } while (0)

#define INCLUDE "inc/"
#define SOURCE "src/"
#define CC "clang"

static const char *input = NULL;
static const char *output = NULL;

void build_interp(void)
{
    da_appendw(&cflags, "-O3", "-DINTERP");
    input = SOURCE"one.c";
    output = "rvsim";
}

void build_dbcache(void)
{
    da_appendw(&cflags, "-O3", "-DENABLE_DBCACHE");
    input = SOURCE"one.c";
    output = "rvsim";
}

void build_tbcache(void)
{
    da_appendw(&cflags, "-O3", "-DENABLE_TBCACHE");
    input = SOURCE"one.c";
    output = "rvsim";
}

void build_cache(void)
{
    da_appendw(&cflags, "-O3", "-DENABLE_TBCACHE", "-DENABLE_DBCACHE");
    input = SOURCE"one.c";
    output = "rvsim";
}

void build_test(void)
{
    da_append(&cflags, "-DTEST");
    input = SOURCE"one.c";
    output = "rvsim";
}

void build_debug(void)
{
    da_appendw(&cflags, "-DDEBUG", "-O0", "-ggdb");
    input = SOURCE"one.c";
    output = "rvsim";
}

bool clean(void)
{
    if (nob_file_exists("rvsim")) {
        if (!nob_delete_file("rvsim")) return false;
    }
    if (nob_file_exists("librvsim.so")) {
        if (!nob_delete_file("librvsim.so")) return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    da_appendw(&cflags, "-Wall", "-Wextra", "-Wno-unused-function", "-Wno-unused-parameter");
    da_appendw(&cflags, "-I"INCLUDE, "-I3rdparty");
    da_appendw(&cldflags, "-L3rdparty", "-lm", "-l:libtcc.a");

    Cmd cmd = {0};

    Flag flag = what_flag(argc, argv);
    switch (flag) {
    case HELP:
        print_help_message();
        break;
    case INTERP:
        build_interp();
        break;
    case DBCACHE:
        build_dbcache();
        break;
    case TBCACHE:
        build_tbcache();
        break;
    case CACHE:
        build_cache();
        break;
    case DEBUG:
        build_debug();
        break;
    case TEST:
        build_test();
        break;
    case LIBRARY:
        break;
    case CLEAN:
        if (!clean()) return 1;
        return 0;
    case INVALID:
        nob_log(NOB_ERROR, "try './nob help'");
        return 1;
    }

    cmd_append(&cmd, CC);
    da_foreach(char *, cflag, &cflags) { cmd_append(&cmd, *cflag); }
    cmd_append(&cmd, "-o", output, input);
    da_foreach(char *, cldflag, &cldflags) { cmd_append(&cmd, *cldflag); }

    if (!cmd_run(&cmd)) return 1;

    return 0;
}
