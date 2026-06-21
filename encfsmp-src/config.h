#ifndef CONFIG_H
#define CONFIG_H

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdint.h>
#include <unistd.h>

// On modern 64-bit Linux, struct stat is already 64-bit capable.
// Just use it directly.
typedef struct stat efs_stat;

// EncFS uses its own off_t type.
// On modern Linux, off_t is 64-bit when _FILE_OFFSET_BITS=64 is defined.
// For safety, use int64_t directly.
typedef int64_t efs_off_t;

#ifndef HAVE_SSIZE_T
typedef signed long ssize_t;
#define HAVE_SSIZE_T 1
#endif

// EncFS configuration
#define VERSION "1.9.5"

// Common feature defines used by source files
#ifndef HAVE_SYS_STAT_H
#define HAVE_SYS_STAT_H 1
#endif
#ifndef HAVE_SYS_TYPES_H
#define HAVE_SYS_TYPES_H 1
#endif
#ifndef HAVE_UNISTD_H
#define HAVE_UNISTD_H 1
#endif
#ifndef HAVE_STDLIB_H
#define HAVE_STDLIB_H 1
#endif
#ifndef HAVE_STRING_H
#define HAVE_STRING_H 1
#endif
#ifndef HAVE_MEMORY_H
#define HAVE_MEMORY_H 1
#endif
#ifndef HAVE_DLFCN_H
#define HAVE_DLFCN_H 1
#endif
#ifndef HAVE_FCNTL_H
#define HAVE_FCNTL_H 1
#endif
#ifndef HAVE_DIRENT_H
#define HAVE_DIRENT_H 1
#endif
#ifndef HAVE_UTIME_H
#define HAVE_UTIME_H 1
#endif
#ifndef HAVE_SSIZE_T
#define HAVE_SSIZE_T 1
#endif
#ifndef HAVE_MODE_T
#define HAVE_MODE_T 1
#endif
#ifndef HAVE_PID_T
#define HAVE_PID_T 1
#endif
#ifndef HAVE_GID_T
#define HAVE_GID_T 1
#endif
#ifndef HAVE_UID_T
#define HAVE_UID_T 1
#endif

#ifndef EFS_64_BIT
#define EFS_64_BIT 1
#endif

#ifndef HAVE_SSL
#define HAVE_SSL 1
#endif

#ifndef HAVE_BOOST
#define HAVE_BOOST 1
#endif

#endif // CONFIG_H
