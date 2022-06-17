// borrowed from https://github.com/riscv/riscv-pk : util/snprintf.c
#ifndef _SNPRINTF_H
#define _SNPRINTF_H

#include <stdarg.h>

#include "util/types.h"

int vsnprintf(char* out, size_t n, const char* s, va_list vl);

#endif
