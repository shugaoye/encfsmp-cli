#ifndef PORTABILITY_H
#define PORTABILITY_H

#include "config.h"

int efs_snprintf(char *str, size_t size, const char *format, ...);
int efs_strcasecmp(const char *s1, const char *s2);
int efs_strncasecmp(const char *s1, const char *s2, size_t n);

#endif
