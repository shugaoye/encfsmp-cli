/**
 * Simplified POSIX fs_layer for EncFSMP CLI.
 * Works on Linux, macOS, and MSYS2/MinGW (Windows).
 * All filesystems use the same POSIX API.
 */

#include "config.h"
#include "fs_layer.h"
#include "FileStatCache.h"

#include <cerrno>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <utime.h>
#include <boost/filesystem.hpp>

// ---------------------------------------------------------------------------
// FileStatCache implementation (shared by fs_layer::stat)
// ---------------------------------------------------------------------------

static FileStatCache g_statCache;

FileStatCache::FileStatCache() : cacheSize_(128) {}
FileStatCache::~FileStatCache() {}

void FileStatCache::clearCache() { cache_.clear(); }
void FileStatCache::setCacheSize(int cacheSize) { cacheSize_ = cacheSize; }

void FileStatCache::addStatToCache(const char *path, int retVal, efs_stat *buffer) {
    if (cacheSize_ <= 0) return;
    CacheEntry entry;
    entry.time_ = time(nullptr);
    entry.retVal_ = retVal;
    if (retVal == 0 && buffer) entry.stat_ = *buffer;
    cache_[std::string(path)] = entry;
    while ((int)cache_.size() > cacheSize_) {
        cache_.erase(cache_.begin());
    }
}

int FileStatCache::stat(const char *path, efs_stat *buffer) {
    FileStatCacheType::iterator it = cache_.find(std::string(path));
    if (it != cache_.end()) {
        if (buffer && it->second.retVal_ == 0) *buffer = it->second.stat_;
        return it->second.retVal_;
    }
    int ret = ::stat(path, buffer);
    addStatToCache(path, ret, buffer);
    return ret;
}

void FileStatCache::forgetCachedStat(const char *path) {
    cache_.erase(std::string(path));
}

// ---------------------------------------------------------------------------
// fs_layer implementation - pure POSIX pass-through
// ---------------------------------------------------------------------------

int fs_layer::gettimeofday(struct fs_layer::timeval_fs *tv, void *tz) {
    return ::gettimeofday((struct timeval *)tv, (struct timezone *)tz);
}

boost::filesystem::path fs_layer::stringToFSPath(const std::string &str) {
    return boost::filesystem::path(str);
}

std::string fs_layer::readFileToString(const char *fn) {
    std::ifstream file(fn, std::ios::binary);
    if (!file.is_open()) return std::string();
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

bool fs_layer::writeFileFromString(const char *fn, const std::string &str) {
    std::ofstream file(fn, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) return false;
    file.write(str.c_str(), str.size());
    file.close();
    return true;
}

int fs_layer::fsync(int fd) { return ::fsync(fd); }
int fs_layer::fdatasync(int fd) { return ::fsync(fd); }
int fs_layer::fstat(int fd, efs_stat *buf) { return ::fstat(fd, buf); }

int64_t fs_layer::pread(int fd, void *buf, int64_t count, int64_t offset) {
    return (int64_t)::pread(fd, buf, (size_t)count, (off_t)offset);
}

int64_t fs_layer::pwrite(int fd, const void *buf, int64_t count, int64_t offset) {
    return (int64_t)::pwrite(fd, buf, (size_t)count, (off_t)offset);
}

int fs_layer::read(int fd, void *buf, unsigned int count) {
    return (int)::read(fd, buf, count);
}

int fs_layer::write(int fd, const void *buf, unsigned int count) {
    return (int)::write(fd, buf, count);
}

int fs_layer::truncate(const char *path, int64_t length) {
    return ::truncate(path, (off_t)length);
}

int fs_layer::ftruncate(int fd, int64_t length) {
    return ::ftruncate(fd, (off_t)length);
}

int fs_layer::statvfs(const char *path, struct statvfs *buf) {
    return ::statvfs(path, buf);
}

int fs_layer::utimes(const char *filename, const struct fs_layer::timeval_fs times[2]) {
    return ::utimes(filename, (const struct timeval *)times);
}

int fs_layer::futimes(int fd, const struct fs_layer::timeval_fs times[2]) {
    (void)fd; (void)times; return 0;
}

int fs_layer::utime(const char *filename, struct utimbuf *times) {
    return ::utime(filename, times);
}

int fs_layer::creat(const char *fn, unsigned short mode) {
    return ::open(fn, O_WRONLY | O_CREAT | O_TRUNC, (mode_t)mode);
}

int fs_layer::open(const char *fn, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = (mode_t)va_arg(args, int);
        va_end(args);
    }
    return ::open(fn, flags, mode);
}

int fs_layer::close(int fd) { return ::close(fd); }

int fs_layer::mkdir(const char *fn, int mode) { return ::mkdir(fn, (mode_t)mode); }

int fs_layer::rename(const char *oldpath, const char *newpath) {
    return ::rename(oldpath, newpath);
}

int fs_layer::unlink(const char *path) { return ::unlink(path); }
int fs_layer::rmdir(const char *path) { return ::rmdir(path); }

int fs_layer::stat(const char *path, efs_stat *buffer) {
    return g_statCache.stat(path, buffer);
}

int fs_layer::stat_cached(const char *path, efs_stat *buffer, void *pStatCache) {
    if (pStatCache) return static_cast<FileStatCache*>(pStatCache)->stat(path, buffer);
    return ::stat(path, buffer);
}

int fs_layer::chmod(const char *path, int mode) { return ::chmod(path, (mode_t)mode); }

fs_layer::DIR *fs_layer::opendir(const char *name) { return ::opendir(name); }
int fs_layer::closedir(fs_layer::DIR *dir) { return ::closedir(dir); }
struct dirent *fs_layer::readdir(fs_layer::DIR *dir) { return ::readdir(dir); }

bool fs_layer::capacity(const std::string &rootDir,
                        uint64_t &totalCapacity, uint64_t &availableCapacity) {
    boost::system::error_code ec;
    boost::filesystem::space_info info = boost::filesystem::space(rootDir, ec);
    if (ec) return false;
    totalCapacity = (uint64_t)info.capacity;
    availableCapacity = (uint64_t)info.available;
    return true;
}

std::string fs_layer::concat_path(const std::string &path1,
                                   const std::string &path2, bool genericPath) {
    boost::filesystem::path p1(path1), p2(path2);
    boost::filesystem::path p = p1 / p2;
    if (genericPath) return p.generic_string();
    return p.string();
}

bool fs_layer::is_same_path(const std::string &path1, const std::string &path2) {
    boost::system::error_code ec;
    return boost::filesystem::equivalent(path1, path2, ec);
}

std::string fs_layer::extract_path(const std::string &fullpath) {
    boost::filesystem::path p(fullpath);
    return p.parent_path().string();
}

std::string fs_layer::extract_filename(const std::string &fullpath) {
    boost::filesystem::path p(fullpath);
    return p.filename().string();
}

std::string fs_layer::canonical(const std::string &fullpath) {
    boost::system::error_code ec;
    boost::filesystem::path p = boost::filesystem::canonical(fullpath, ec);
    return p.string();
}
