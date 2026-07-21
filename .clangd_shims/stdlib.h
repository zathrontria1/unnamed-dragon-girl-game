#ifndef _CLANGD_SHIMS_STDLIB_H
#define _CLANGD_SHIMS_STDLIB_H

#include <stddef.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

void exit(int status);
int rand(void);
void srand(unsigned int seed);
int abs(int j);

#endif /* _CLANGD_SHIMS_STDLIB_H */

