#ifndef ERR_H
#define ERR_H

#include <errno.h>
#include <stdarg.h>

#include "common.h"

typedef struct {
    const char *func;
    const char *info;
    const char *file;
    int line;
} SimErr;

#define SIM_ERR_NEW(func, info) sim_err_new(func, info, __FILE__, __LINE__)
#define SIM_ERR_NEWF(func, fmt, ...) sim_err_newf(func, __FILE__, __LINE__, fmt, __VA_ARGS__)

static __ForceInline void sim_err_print(SimErr err)
{
    fprintf(stderr, "%s:%d: %s:%s\n", err.file, err.line, err.func, err.info);
}

static __ForceInline SimErr sim_err_new(const char *func, const char *info, const char *file, int line)
{
    return (SimErr) {
        .func = func,
        .info = info,
        .file = file,
        .line = line,
    };
}

static __ForceInline SimErr sim_err_newf(const char *func, const char *file, int line, const char *fmt, ...)
{
    char buf[256];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    return (SimErr) {
        .func = func,
        .info = buf,
        .file = file,
        .line = line,
    };
}

#define Result(T) __Result_##T

typedef struct {
    bool ok;
    SimErr err;
} ResultVoid;

#define DEFINE_RESULT(T) \
    typedef struct { \
        bool ok; \
        SimErr err; \
        T value; \
    } __Result_##T

#define ALIAS_RESULT(old, new) typedef __Result_##old __Result_##new

#define OK_VOID           ((ResultVoid) { .ok = true                                             })
#define ERR_VOID(e)       ((ResultVoid) { .ok = false, .err = (e)                                })
#define SYSERR_VOID(func) ((ResultVoid) { .ok = false, .err = SIM_ERR_NEW(func, strerror(errno)) })

#define OK(T, v)        ((Result(T)) { .ok = true,  .value = (v)                              })
#define ERR(T, e)       ((Result(T)) { .ok = false, .err = (e)                                })
#define SYSERR(T, func) ((Result(T)) { .ok = false, .err = SIM_ERR_NEW(func, strerror(errno)) })

#define return__defer(r) do { res = (r); goto defer; } while (0)

DEFINE_RESULT(int);
DEFINE_RESULT(u64);
ALIAS_RESULT(u64, GuestVAddr);

#endif // ERR_H
