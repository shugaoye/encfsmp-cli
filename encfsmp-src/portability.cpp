/**
 * Simplified portability.cpp for command-line EncFS tool.
 * Provides basic string/number formatting compatible with original.
 */

#include "config.h"
#include "portability.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <string>
#include <sstream>
#include <iomanip>

// Simple snprintf compatibility wrapper
int efs_snprintf(char *str, size_t size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(str, size, format, args);
    va_end(args);
    return result;
}

// Simple strcasecmp
int efs_strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (tolower((unsigned char)*s1) != tolower((unsigned char)*s2))
            return 1;
        s1++; s2++;
    }
    if (*s1 || *s2) return 1;
    return 0;
}

// Simple strncasecmp
int efs_strncasecmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (!s1[i] && !s2[i]) return 0;
        if (!s1[i] || !s2[i]) return 1;
        if (tolower((unsigned char)s1[i]) != tolower((unsigned char)s2[i]))
            return 1;
    }
    return 0;
}
