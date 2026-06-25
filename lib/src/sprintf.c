#include <stdarg.h>
#include "libultra_internal.h"
#include "printf.h"
#include <string.h>

char *proutSprintf(char *dst, const char *src, size_t count);

int sprintf(char *dst, const char *fmt, ...) {
    s32 written;
    va_list args;
    va_start(args, fmt);
    written = _Printf(proutSprintf, dst, fmt, args);
    if (written >= 0) {
        dst[written] = 0;
    }
    return written;
}

char *proutSprintf(char *dst, const char *src, size_t count) {
    return (char *) memcpy((u8 *) dst, (u8 *) src, count) + count;
}

typedef struct {
    char *ptr;
    size_t remaining;
} SnprintfCtx;

static char *proutSnprintf(char *dst, const char *src, size_t count) {
    SnprintfCtx *ctx = (SnprintfCtx *) dst;
    if (ctx->remaining > 0) {
        size_t to_copy = count < ctx->remaining ? count : ctx->remaining;
        memcpy(ctx->ptr, src, to_copy);
        ctx->ptr += to_copy;
        ctx->remaining -= to_copy;
    }
    return dst;
}

int snprintf(char *dst, size_t n, const char *fmt, ...) {
    SnprintfCtx ctx;
    s32 written;
    va_list args;
    ctx.ptr = dst;
    ctx.remaining = n > 0 ? n - 1 : 0;
    va_start(args, fmt);
    written = _Printf((char *(*)(char *, const char *, size_t)) proutSnprintf, (char *) &ctx, fmt, args);
    va_end(args);
    if (n > 0) {
        ctx.ptr[0] = '\0';
    }
    return written;
}
