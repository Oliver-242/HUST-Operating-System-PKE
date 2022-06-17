/*
 * vsnprintf() is borrowed from pk.
 */

//#include <stdint.h>
//#include <stdarg.h>
//#include <stdbool.h>

#include "util/snprintf.h"

int32 vsnprintf(char* out, size_t n, const char* s, va_list vl) {
  bool format = FALSE;
  bool longarg = FALSE;
  size_t pos = 0;

  for (; *s; s++) {
    if (format) {
      switch (*s) {
        case 'l':
          longarg = TRUE;
          break;
        case 'p':
          longarg = TRUE;
          if (++pos < n) out[pos - 1] = '0';
          if (++pos < n) out[pos - 1] = 'x';
        case 'x': {
          long num = longarg ? va_arg(vl, long) : va_arg(vl, int);
          for (int i = 2 * (longarg ? sizeof(long) : sizeof(int)) - 1; i >= 0; i--) {
            int d = (num >> (4 * i)) & 0xF;
            if (++pos < n) out[pos - 1] = (d < 10 ? '0' + d : 'a' + d - 10);
          }
          longarg = FALSE;
          format = FALSE;
          break;
        }
        case 'd': {
          long num = longarg ? va_arg(vl, long) : va_arg(vl, int);
          if (num < 0) {
            num = -num;
            if (++pos < n) out[pos - 1] = '-';
          }
          long digits = 1;
          for (long nn = num; nn /= 10; digits++)
            ;
          for (int i = digits - 1; i >= 0; i--) {
            if (pos + i + 1 < n) out[pos + i] = '0' + (num % 10);
            num /= 10;
          }
          pos += digits;
          longarg = FALSE;
          format = FALSE;
          break;
        }
        case 's': {
          const char* s2 = va_arg(vl, const char*);
          while (*s2) {
            if (++pos < n) out[pos - 1] = *s2;
            s2++;
          }
          longarg = FALSE;
          format = FALSE;
          break;
        }
        case 'c': {
          if (++pos < n) out[pos - 1] = (char)va_arg(vl, int);
          longarg = FALSE;
          format = FALSE;
          break;
        }
        default:
          break;
      }
    } else if (*s == '%')
      format = TRUE;
    else if (++pos < n)
      out[pos - 1] = *s;
  }
  if (pos < n)
    out[pos] = 0;
  else if (n)
    out[n - 1] = 0;
  return pos;
}
