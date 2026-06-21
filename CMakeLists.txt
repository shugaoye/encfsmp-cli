cmake_minimum_required(VERSION 3.10)
project(encfsmp_cli C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Paths to EncFSMP source tree
# This repository contains encfsmp-src/ as a copy of EncFSMP/src
set(ENCFSMP_ROOT "${CMAKE_SOURCE_DIR}/encfsmp-src")
set(ENCFSMP_BUILD "${ENCFSMP_ROOT}/build")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${ENCFSMP_ROOT}
    ${ENCFSMP_ROOT}/encfs
    ${ENCFSMP_ROOT}/easyloggingpp
    ${ENCFSMP_ROOT}/tinyxml2
    ${ENCFSMP_ROOT}/intl
    ${ENCFSMP_BUILD}
)

# Platform checks
include(CheckIncludeFile)
include(CheckFunctionExists)

CHECK_INCLUDE_FILE(sys/stat.h HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE(dlfcn.h HAVE_DLFCN_H)
CHECK_INCLUDE_FILE(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILE(stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILE(string.h HAVE_STRING_H)
CHECK_INCLUDE_FILE(memory.h HAVE_MEMORY_H)

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.53.0 REQUIRED COMPONENTS
    filesystem thread date_time serialization system
)
include_directories(${Boost_INCLUDE_DIRS})

# Use the EncFSMP-generated config.h
add_definitions(-DHAVE_CONFIG_H=1)

# EncFS source files (core encryption library)
set(ENCFS_SOURCES
    ${ENCFSMP_ROOT}/encfs/BlockFileIO.cpp
    ${ENCFSMP_ROOT}/encfs/BlockNameIO.cpp
    ${ENCFSMP_ROOT}/encfs/Cipher.cpp
    ${ENCFSMP_ROOT}/encfs/CipherKey.cpp
    ${ENCFSMP_ROOT}/encfs/CipherFileIO.cpp
    ${ENCFSMP_ROOT}/encfs/Context.cpp
    ${ENCFSMP_ROOT}/encfs/DirNode.cpp
    ${ENCFSMP_ROOT}/encfs/Error.cpp
    ${ENCFSMP_ROOT}/encfs/FileNode.cpp
    ${ENCFSMP_ROOT}/encfs/FileUtils.cpp
    ${ENCFSMP_ROOT}/encfs/FS.cpp
    ${ENCFSMP_ROOT}/encfs/FSConfig.cpp
    ${ENCFSMP_ROOT}/encfs/Interface.cpp
    ${ENCFSMP_ROOT}/encfs/MemoryPool.cpp
    ${ENCFSMP_ROOT}/encfs/NameIO.cpp
    ${ENCFSMP_ROOT}/encfs/PathConfig.cpp
    ${ENCFSMP_ROOT}/encfs/Range.cpp
    ${ENCFSMP_ROOT}/encfs/RawFileIO.cpp
    ${ENCFSMP_ROOT}/encfs/SSL_Cipher.cpp
    ${ENCFSMP_ROOT}/encfs/StreamNameIO.cpp
    ${ENCFSMP_ROOT}/encfs/XmlReader.cpp
    ${ENCFSMP_ROOT}/encfs/XmlWriter.cpp
    ${ENCFSMP_ROOT}/encfs/base64.cpp
    ${ENCFSMP_ROOT}/encfs/posix/Mutex.cpp
    ${ENCFSMP_ROOT}/portability.cpp
    ${ENCFSMP_ROOT}/fs_layer.cpp
    ${ENCFSMP_ROOT}/encfs/FileStream.cpp
)

# Third-party libraries
set(THIRD_PARTY_SOURCES
    ${ENCFSMP_ROOT}/easyloggingpp/easyloggingpp.cpp
    ${ENCFSMP_ROOT}/tinyxml2/tinyxml2.cpp
    ${ENCFSMP_ROOT}/intl/fmt.cpp
    ${ENCFSMP_ROOT}/intl/pathconfig.cpp
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
    rt
    dl
)

# Installation
install(TARGETS encfsmp_cli DESTINATION bin)
