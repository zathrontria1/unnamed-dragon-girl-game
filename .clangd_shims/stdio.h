#ifndef _CLANGD_SHIMS_STDIO_H
#define _CLANGD_SHIMS_STDIO_H

#include <stdarg.h>
#include <stddef.h>

int snprintf(char *s, size_t n, const char *format, ...);
int sprintf(char *s, const char *format, ...);
int printf(const char *format, ...);

#endif /* _CLANGD_SHIMS_STDIO_H */
