// EncFS MP - Command Line Encrypted File System Tool
// Copyright (C) 2025
// Based on EncFSMP and EncFS libraries

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <iomanip>

// Use POSIX headers on all platforms:
//   - Linux/macOS: native POSIX
//   - Windows via MSYS2/MinGW: POSIX layer included with the toolchain
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/statvfs.h>

#include <boost/filesystem.hpp>

#include <FileUtils.h>
#include <FileNode.h>
#include <DirNode.h>
#include <Context.h>
#include <Cipher.h>
#include <CipherKey.h>
#include <Error.h>
#include <FSConfig.h>
#include <Interface.h>
#include <MemoryPool.h>
#include <NameIO.h>
#include <Range.h>

namespace fs = boost::filesystem;
using namespace std;

// =================================================================
// Utilities
// =================================================================

static std::string normalizePath(const std::string& p) {
    std::string s = p;
    while (!s.empty() && s[0] == '/') s.erase(s.begin());
    return s;
}

static std::string ensureTrailingSlash(const std::string& p) {
    if (p.empty() || p[p.size()-1] != '/') return p + "/";
    return p;
}

// =================================================================
// Common: set up encfs opts and open volume
// =================================================================

struct VolState {
    encfs::RootPtr root;
    std::string cipherDir;

    bool open(const std::string& dir, bool usePass, const std::string& password) {
        cipherDir = ensureTrailingSlash(dir);
        encfs::EncFS_Context* ctx = new encfs::EncFS_Context();
        std::shared_ptr<encfs::EncFS_Opts> opts(new encfs::EncFS_Opts());
        opts->rootDir = cipherDir;
        if (usePass) {
            // In EncFSMP, passwordProgram is treated as the literal password,
            // not as an external program. This is a key difference from the
            // original encfs project
            opts->useStdin = false;
            opts->passwordProgram = password;
        } else {
            opts->useStdin = true;
        }
        opts->annotate = false;
        root = encfs::initFS(ctx, opts, std::cerr);
        if (!root) std::cerr << "Error: cannot open encrypted volume\n";
        return root.operator bool();
    }

    bool create(const std::string& dir, bool usePass, const std::string& password) {
        cipherDir = ensureTrailingSlash(dir);
        boost::system::error_code ec;
        fs::create_directories(cipherDir, ec);
        encfs::EncFS_Context* ctx = new encfs::EncFS_Context();
        std::shared_ptr<encfs::EncFS_Opts> opts(new encfs::EncFS_Opts());
        opts->rootDir = cipherDir;
        if (usePass) {
            opts->useStdin = false;
            opts->passwordProgram = password;
        } else {
            opts->useStdin = true;
        }
        opts->annotate = false;
        root = encfs::createV6Config(ctx, opts);
        if (!root) std::cerr << "Error: volume creation failed\n";
        return root.operator bool();
    }
};

// =================================================================
// Commands
// =================================================================

static void showUsage(const char* prog) {
    std::cout <<
      "EncFS MP - Command Line Tool\n"
      "=============================\n\n"
      "Usage: " << prog << " <command> [options] <arguments>\n\n"
      "Commands:\n"
      "  create <cipher-dir> [-p password]   Create new encrypted volume\n"
      "  info   <cipher-dir> [-p password]   Show volume info\n"
      "  list   <cipher-dir> [path] [-p pwd] List files\n"
      "  cat    <cipher-dir> <path> [-p pwd] Decrypt file to stdout\n"
      "  mkdir  <cipher-dir> <path> [-p pwd] Create directory\n"
      "  rm     <cipher-dir> <path> [-p pwd] Delete file/directory\n"
      "  put    <cipher-dir> <src> <dst> [-p pwd]  Copy file INTO volume\n"
      "  get    <cipher-dir> <src> <dst> [-p pwd]  Extract file FROM volume\n\n"
      "Notes:\n"
      "  - All paths inside the encrypted volume are relative to the volume root\n"
      "  - Use -p to provide a password; otherwise read from stdin\n";
}

// ================================================================
// CREATE
// ================================================================
static int cmdCreate(int argc, char** argv) {
    if (argc < 2) { std::cerr << "Need cipher directory\n"; return 1; }
    std::string cipherDir = argv[1];
    std::string password;
    bool usePass = false;
    for (int i = 2; i < argc; i++) {
        if (std::string(argv[i]) == "-p" && i+1 < argc) {
            password = argv[++i];
            usePass = true;
        }
    }
    VolState vol;
    if (!vol.create(cipherDir, usePass, password)) return 1;
    std::cout << "Successfully created encrypted volume\n"
              << "  Directory: " << vol.cipherDir << "\n";
    return 0;
}

// ================================================================
// INFO
// ================================================================
static int cmdInfo(int argc, char** argv) {
    if (argc < 2) { std::cerr << "Need cipher directory\n"; return 1; }
    std::string cipherDir = argv[1];
    std::string password;
    bool usePass = false;
    for (int i = 2; i < argc; i++) {
        if (std::string(argv[i]) == "-p" && i+1 < argc) {
            password = argv[++i];
            usePass = true;
        }
    }
    VolState vol;
    if (!vol.open(cipherDir, usePass, password)) return 1;
    std::cout << "EncFS MP Volume\n"
              << "  Directory: " << vol.cipherDir << "\n";
    return 0;
}

// ================================================================
// LIST
// ================================================================
static int cmdList(int argc, char** argv) {
    if (argc < 2) { std::cerr << "Need cipher directory\n"; return 1; }
    std::string cipherDir = argv[1];
    std::string path = "";
    std::string password;
    bool usePass = false;
    for (int i = 2; i < argc; i++) {
        std::string a(argv[i]);
        if (a == "-p" && i+1 < argc) { password = argv[++i]; usePass = true; continue; }
        path = normalizePath(a);
    }
    VolState vol;
    if (!vol.open(cipherDir, usePass, password)) return 1;
    try {
        std::string base = vol.root->root->cipherPath(path.c_str());
        DIR* d = opendir(base.c_str());
        if (!d) { std::cerr << "Cannot list: " << (path.empty() ? "/" : path) << "\n"; return 1; }
        std::cout << "[" << (path.empty() ? std::string("/") : "/" + path) << "]\n";
        std::string encRel = path.empty() ? "" : vol.root->root->cipherPathWithoutRoot(path.c_str());
        struct dirent* e;
        while ((e = readdir(d)) != NULL) {
            std::string encName(e->d_name);
            if (encName == "." || encName == ".." || encName.find(".encfs") == 0) continue;
            std::string fullEncPath = base + "/" + encName;
            std::string plainName;
            try {
                std::string fullCipherRel = encRel.empty() ? encName : (encRel + "/" + encName);
                std::string decoded = vol.root->root->plainPath(fullCipherRel.c_str());
                size_t lastSlash = decoded.rfind('/');
                plainName = (lastSlash == std::string::npos) ? decoded : decoded.substr(lastSlash + 1);
            } catch (...) {
                plainName = encName;
            }
            struct stat st;
            std::string type = "FILE";
            uint64_t size = 0;
            if (stat(fullEncPath.c_str(), &st) == 0) {
                if (S_ISDIR(st.st_mode)) type = "DIR";
                else if (S_ISLNK(st.st_mode)) type = "LNK";
                size = (uint64_t)st.st_size;
            }
            std::cout << std::left << std::setw(45) << plainName
                      << std::right << std::setw(15) << size
                      << std::setw(10) << type << "\n";
        }
        closedir(d);
        return 0;
    } catch (std::exception& ex) {
        std::cerr << "List failed: " << ex.what() << "\n";
        return 1;
    }
}

// ================================================================
// CAT
// ================================================================
static int cmdCat(int argc, char** argv) {
    if (argc < 3) { std::cerr << "Need: cat <cipher-dir> <path>\n"; return 1; }
    std::string cipherDir = argv[1];
    std::string path = "";
    std::string password;
    bool usePass = false;
    for (int i = 2; i < argc; i++) {
        std::string a(argv[i]);
        if (a == "-p" && i+1 < argc) { password = argv[++i]; usePass = true; continue; }
        path = a;
    }
    path = normalizePath(path);
    VolState vol;
    if (!vol.open(cipherDir, usePass, password)) return 1;
    try {
        int res = 0;
        auto node = vol.root->root->openNode(path.c_str(), "cat", O_RDONLY, &res);
        if (!node) { std::cerr << "Cannot open: " << path << "\n"; return 1; }
        const size_t BUF = 32768;
        std::vector<unsigned char> buf(BUF);
        off_t offset = 0;
        while (true) {
            ssize_t n = node->read(offset, buf.data(), BUF);
            if (n <= 0) break;
            fwrite(buf.data(), 1, (size_t)n, stdout);
            offset += n;
        }
        return 0;
    } catch (std::exception& ex) { std::cerr << "Cat failed: " << ex.what() << "\n"; return 1; }
}

// ================================================================
// PUT
// ================================================================
static int cmdPut(int argc, char** argv) {
    if (argc < 4) { std::cerr << "Need: put <cipher-dir> <local-src> <plain-dst-path>\n"; return 1; }
    std::string cipherDir = argv[1];
    std::string srcFile = "";
    std::string dstPath = "";
    std::string password;
    bool usePass = false;
    std::vector<std::string> positional;
    for (int i = 2; i < argc; i++) {
        std::string a(argv[i]);
        if (a == "-p" && i+1 < argc) { password = argv[++i]; usePass = true; continue; }
        positional.push_back(a);
    }
    if (positional.size() < 2) { std::cerr << "Need source and destination paths\n"; return 1; }
    srcFile = positional[0];
    dstPath = normalizePath(positional[1]);
    if (!fs::exists(srcFile)) { std::cerr << "Source file not found: " << srcFile << "\n"; return 1; }
    VolState vol;
    if (!vol.open(cipherDir, usePass, password)) return 1;
    try {
        size_t slashPos = dstPath.rfind('/');
        if (slashPos != std::string::npos && slashPos > 0) {
            std::string parentPath = dstPath.substr(0, slashPos);
            std::string current = "";
            std::istringstream iss(parentPath);
            std::string part;
            while (std::getline(iss, part, '/')) {
                if (part.empty()) continue;
                current = current.empty() ? part : current + "/" + part;
                vol.root->root->mkdir(current.c_str(), 0755, 0, 0);
            }
        }
        auto node = vol.root->root->lookupNode(dstPath.c_str(), "put");
        if (!node) { std::cerr << "Cannot create FileNode for: " << dstPath << "\n"; return 1; }
        int mkres = node->mknod(S_IFREG | 0644, 0, 0, 0);
        if (mkres < 0) { std::cerr << "Cannot create encrypted file: " << dstPath << " (err " << mkres << ")\n"; return 1; }
        int openRes = node->open(O_RDWR);
        if (openRes < 0) { std::cerr << "Cannot open for writing: " << dstPath << "\n"; return 1; }
        std::ifstream in(srcFile.c_str(), std::ios::binary);
        if (!in) { std::cerr << "Cannot open source file: " << srcFile << "\n"; return 1; }
        const size_t BUF = 32768;
        std::vector<unsigned char> buf(BUF);
        off_t offset = 0;
        while (in.good()) {
            in.read((char*)buf.data(), BUF);
            std::streamsize n = in.gcount();
            if (n > 0) {
                if (!node->write(offset, buf.data(), (size_t)n)) {
                    std::cerr << "Write failed at offset " << offset << "\n";
                    return 1;
                }
                offset += n;
            }
        }
        std::cout << "Encrypted: " << srcFile << " -> " << dstPath
                  << " (" << offset << " bytes)\n";
        return 0;
    } catch (std::exception& ex) { std::cerr << "Put failed: " << ex.what() << "\n"; return 1; }
}

// ================================================================
// GET
// ================================================================
static int cmdGet(int argc, char** argv) {
    if (argc < 4) { std::cerr << "Need: get <cipher-dir> <plain-src-path> <local-dst>\n"; return 1; }
    std::string cipherDir = argv[1];
    std::string srcPath = "";
    std::string dstFile = "";
    std::string password;
    bool usePass = false;
    std::vector<std::string> positional;
    for (int i = 2; i < argc; i++) {
        std::string a(argv[i]);
        if (a == "-p" && i+1 < argc) { password = argv[++i]; usePass = true; continue; }
        positional.push_back(a);
    }
    if (positional.size() < 2) { std::cerr << "Need source and destination paths\n"; return 1; }
    srcPath = normalizePath(positional[0]);
    dstFile = positional[1];
    VolState vol;
    if (!vol.open(cipherDir, usePass, password)) return 1;
    try {
        int res = 0;
        auto node = vol.root->root->openNode(srcPath.c_str(), "get", O_RDONLY, &res);
        if (!node) { std::cerr << "Cannot open: " << srcPath << "\n"; return 1; }
        std::ofstream out(dstFile.c_str(), std::ios::binary | std::ios::trunc);
        if (!out) { std::cerr << "Cannot create: " << dstFile << "\n"; return 1; }
        const size_t BUF = 32768;
        std::vector<unsigned char> buf(BUF);
        off_t offset = 0;
        while (true) {
            ssize_t n = node->read(offset, buf.data(), BUF);
            if (n <= 0) break;
            out.write((const char*)buf.data(), n);
            offset += n;
        }
        std::cout << "Decrypted: " << srcPath << " -> " << dstFile
                  << " (" << offset << " bytes)\n";
        return 0;
    } catch (std::exception& ex) { std::cerr << "Get failed: " << ex.what() << "\n"; return 1; }
}

// ================================================================
// MKDIR
// ================================================================
static int cmdMkdir(int argc, char** argv) {
    if (argc < 3) { std::cerr << "Need: mkdir <cipher-dir> <path>\n"; return 1; }
    std::string cipherDir = argv[1];
    std::string path = "";
    std::string password;
    bool usePass = false;
    for (int i = 2; i < argc; i++) {
        std::string a(argv[i]);
        if (a == "-p" && i+1 < argc) { password = argv[++i]; usePass = true; continue; }
        path = normalizePath(a);
    }
    VolState vol;
    if (!vol.open(cipherDir, usePass, password)) return 1;
    try {
        int res = vol.root->root->mkdir(path.c_str(), 0755, 0, 0);
        if (res < 0) { std::cerr << "Cannot mkdir: " << path << " (error " << res << ")\n"; return 1; }
        std::cout << "Created directory: " << path << "\n";
        return 0;
    } catch (std::exception& ex) { std::cerr << "Mkdir failed: " << ex.what() << "\n"; return 1; }
}

// ================================================================
// RM
// ================================================================
static int cmdRm(int argc, char** argv) {
    if (argc < 3) { std::cerr << "Need: rm <cipher-dir> <path>\n"; return 1; }
    std::string cipherDir = argv[1];
    std::string path = "";
    std::string password;
    bool usePass = false;
    for (int i = 2; i < argc; i++) {
        std::string a(argv[i]);
        if (a == "-p" && i+1 < argc) { password = argv[++i]; usePass = true; continue; }
        path = normalizePath(a);
    }
    VolState vol;
    if (!vol.open(cipherDir, usePass, password)) return 1;
    try {
        std::string cipherPath = vol.root->root->cipherPath(path.c_str());
        struct stat st;
        if (stat(cipherPath.c_str(), &st) != 0) {
            std::cerr << "Not found: " << path << "\n";
            return 1;
        }
        int res;
        if (S_ISDIR(st.st_mode)) {
            res = ::rmdir(cipherPath.c_str());
            if (res != 0) {
                std::cerr << "rmdir failed (directory not empty?)\n";
                return 1;
            }
        } else {
            res = vol.root->root->unlink(path.c_str());
            if (res < 0) {
                std::cerr << "unlink failed (error " << res << ")\n";
                return 1;
            }
        }
        std::cout << "Deleted: " << path << "\n";
        return 0;
    } catch (std::exception& ex) { std::cerr << "Rm failed: " << ex.what() << "\n"; return 1; }
}

// ================================================================
// Main dispatcher
// ================================================================
int main(int argc, char* argv[]) {
    if (argc < 2) { showUsage(argv[0]); return 0; }
    std::string cmd = argv[1];
    std::vector<char*> args(argv + 1, argv + argc);
    args.push_back(NULL);

    if (cmd == "create") return cmdCreate((int)args.size() - 1, args.data());
    if (cmd == "info") return cmdInfo((int)args.size() - 1, args.data());
    if (cmd == "list") return cmdList((int)args.size() - 1, args.data());
    if (cmd == "cat") return cmdCat((int)args.size() - 1, args.data());
    if (cmd == "mkdir") return cmdMkdir((int)args.size() - 1, args.data());
    if (cmd == "rm") return cmdRm((int)args.size() - 1, args.data());
    if (cmd == "put") return cmdPut((int)args.size() - 1, args.data());
    if (cmd == "get") return cmdGet((int)args.size() - 1, args.data());
    if (cmd == "-h" || cmd == "--help" || cmd == "help") {
        showUsage(argv[0]);
        return 0;
    }
    std::cerr << "Unknown command: " << cmd << "\n\n";
    showUsage(argv[0]);
    return 1;
}
