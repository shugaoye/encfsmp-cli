#ifndef FS_LAYER_H
#define FS_LAYER_H

// config.h defines efs_stat and efs_off_t in a platform-independent way
#include "config.h"

#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/statvfs.h>
#include <utime.h>

#include <boost/filesystem.hpp>

// efs_stat and efs_off_t are defined in config.h

class fs_layer
{
public:
    struct timeval_fs
    {
        long tv_sec;
        long tv_usec;
    };

    static int gettimeofday(struct fs_layer::timeval_fs *, void *);

    static boost::filesystem::path stringToFSPath(const std::string &str);
    static std::string readFileToString(const char *fn);
    static bool writeFileFromString(const char *fn, const std::string &str);

    static int fsync(int fd);
    static int fdatasync(int fd);
    static int fstat(int fd, efs_stat *buf);

    static int64_t pread(int fd, void *buf, int64_t count, int64_t offset);
    static int64_t pwrite(int fd, const void *buf, int64_t count, int64_t offset);

    static int read(int fd, void *buf, unsigned int count);
    static int write(int fd, const void *buf, unsigned int count);

    static int truncate(const char *path, int64_t length);
    static int ftruncate(int fd, int64_t length);
    static int statvfs(const char *path, struct statvfs *buf);
    static int utimes(const char *filename, const struct fs_layer::timeval_fs times[2]);
    static int futimes(int fd, const struct fs_layer::timeval_fs times[2]);
    static int utime(const char *filename, struct utimbuf *times);
    static int creat(const char *fn, unsigned short mode);
    static int open(const char *fn, int flags, ...);
    static int close(int fd);
    static int mkdir(const char *fn, int mode);
    static int rename(const char *oldpath, const char *newpath);
    static int unlink(const char *path);
    static int rmdir(const char *path);
    static int stat(const char *path, efs_stat *buffer);
    static int stat_cached(const char *path, efs_stat *buffer, void *pStatCache);
    static inline int lstat(const char *path, efs_stat *buffer) {
        return stat(path, buffer);
    }
    static int chmod(const char*, int);

    // Use POSIX DIR and dirent directly on all platforms (Linux, macOS, MSYS2/MinGW)
    typedef ::DIR DIR;
    typedef struct ::dirent fs_dirent;
    static DIR *opendir(const char *name);
    static int closedir(DIR *dir);
    static fs_dirent *readdir(DIR *dir);

    static std::string concat_path(const std::string &path1,
        const std::string &path2, bool genericPath = false);
    static bool is_same_path(const std::string &path1,
        const std::string &path2);
    static std::string extract_path(const std::string &fullpath);
    static std::string extract_filename(const std::string &fullpath);
    static std::string canonical(const std::string &fullpath);

    static bool capacity(const std::string &rootDir,
        uint64_t &totalCapacity, uint64_t &availableCapacity);
};

#endif // FS_LAYER_H
