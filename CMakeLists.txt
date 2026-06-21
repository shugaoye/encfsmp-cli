cmake_minimum_required(VERSION 3.10)
project(encfsmp_cli C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------------
# Platform detection:
#   Linux/macOS:   full POSIX
#   Windows (MSYS2/MinGW): POSIX via MSYS2 runtime
# ---------------------------------------------------------------------
if(WIN32)
    # MSYS2/MinGW provides POSIX headers (dirent.h, unistd.h, etc.)
    # Enable 64-bit file offset support
    add_definitions(-D_FILE_OFFSET_BITS=64)
    add_definitions(-D_WIN32_WINNT=0x0601)
    # Don't define UNICODE/_UNICODE: EncFSMP code uses char/string
else()
    add_definitions(-DHAVE_LINUX -DEFS_64_BIT)
    add_definitions(-D_FILE_OFFSET_BITS=64)
endif()

# Common feature defines
add_definitions(
    -DHAVE_SYS_STAT_H
    -DHAVE_SYS_TYPES_H
    -DHAVE_STDLIB_H
    -DHAVE_STRING_H
    -DHAVE_MEMORY_H
    -DHAVE_FCNTL_H
)

if(NOT WIN32)
    add_definitions(-DHAVE_DIRENT_H -DHAVE_UNISTD_H -DHAVE_DLFCN_H -DHAVE_SYS_STATVFS_H)
else()
    # MSYS2/MinGW has dirent.h in its POSIX layer
    add_definitions(-DHAVE_DIRENT_H -DHAVE_UNISTD_H)
endif()

# Paths
set(ENCFSMP_ROOT "${CMAKE_SOURCE_DIR}/encfsmp-src")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${ENCFSMP_ROOT}
    ${ENCFSMP_ROOT}/encfs
    ${ENCFSMP_ROOT}/tinyxml2
    ${ENCFSMP_ROOT}/easyloggingpp
    ${ENCFSMP_ROOT}/intl
)

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.53.0 REQUIRED COMPONENTS
    filesystem thread date_time serialization system
)
include_directories(${Boost_INCLUDE_DIRS})

# EncFS core source files
set(ENCFS_SOURCES
    ${ENCFSMP_ROOT}/encfs/BlockFileIO.cpp
    ${ENCFSMP_ROOT}/encfs/BlockNameIO.cpp
    ${ENCFSMP_ROOT}/encfs/Cipher.cpp
    ${ENCFSMP_ROOT}/encfs/CipherFileIO.cpp
    ${ENCFSMP_ROOT}/encfs/CipherKey.cpp
    ${ENCFSMP_ROOT}/encfs/ConfigReader.cpp
    ${ENCFSMP_ROOT}/encfs/ConfigVar.cpp
    ${ENCFSMP_ROOT}/encfs/Context.cpp
    ${ENCFSMP_ROOT}/encfs/DirNode.cpp
    ${ENCFSMP_ROOT}/encfs/Error.cpp
    ${ENCFSMP_ROOT}/encfs/FileIO.cpp
    ${ENCFSMP_ROOT}/encfs/FileNode.cpp
    ${ENCFSMP_ROOT}/encfs/FileUtils.cpp
    ${ENCFSMP_ROOT}/encfs/Interface.cpp
    ${ENCFSMP_ROOT}/encfs/MACFileIO.cpp
    ${ENCFSMP_ROOT}/encfs/MemoryPool.cpp
    ${ENCFSMP_ROOT}/encfs/NameIO.cpp
    ${ENCFSMP_ROOT}/encfs/NullCipher.cpp
    ${ENCFSMP_ROOT}/encfs/NullNameIO.cpp
    ${ENCFSMP_ROOT}/encfs/openssl.cpp
    ${ENCFSMP_ROOT}/encfs/RawFileIO.cpp
    ${ENCFSMP_ROOT}/encfs/readpassphrase.cpp
    ${ENCFSMP_ROOT}/encfs/SSL_Cipher.cpp
    ${ENCFSMP_ROOT}/encfs/StreamNameIO.cpp
    ${ENCFSMP_ROOT}/encfs/XmlReader.cpp
    ${ENCFSMP_ROOT}/encfs/base64.cpp
    ${ENCFSMP_ROOT}/fs_layer.cpp
    ${ENCFSMP_ROOT}/portability.cpp
)

# Third-party libraries
set(THIRD_PARTY_SOURCES
    ${ENCFSMP_ROOT}/easyloggingpp/easylogging++.cc
    ${ENCFSMP_ROOT}/tinyxml2/tinyxml2.cpp
    ${ENCFSMP_ROOT}/intl/autosprintf.cpp
)

# The executable
add_executable(encfsmp_cli
    src/encfsmp_cli.cpp
    ${ENCFS_SOURCES}
    ${THIRD_PARTY_SOURCES}
)

# Link libraries
target_link_libraries(encfsmp_cli PRIVATE
    ${OPENSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    Threads::Threads
    $<$<NOT:$<BOOL:${WIN32}>>:rt>
    $<$<NOT:$<BOOL:${WIN32}>>:dl>
)

# On Windows/MSYS2 we need to link the socket/network libs
if(WIN32)
    target_link_libraries(encfsmp_cli PRIVATE ws2_32 mswsock advapi32)
endif()

# Installation
install(TARGETS encfsmp_cli DESTINATION bin)
