#pragma once

#include "system/angband.h"
#include <filesystem>
#include <string_view>

/* Force definitions -- see fd_seek() */
#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

/* Force definitions -- see fd_lock() */
#ifndef F_UNLCK
#define F_UNLCK 0
#endif
#ifndef F_RDLCK
#define F_RDLCK 1
#endif
#ifndef F_WRLCK
#define F_WRLCK 2
#endif

#ifdef SET_UID
extern void user_name(char *buf, int id);
#ifndef HAVE_USLEEP
extern int usleep(ulong usecs);
#endif
#endif

#define FILE_READ_BUFF_SIZE 65535

enum class FileOpenMode {
    READ,
    WRITE,
    APPEND,
};

std::filesystem::path path_parse(std::string_view file);
std::filesystem::path path_build(const std::filesystem::path &path, std::string_view file);
FILE *angband_fopen(const std::filesystem::path &file, const FileOpenMode mode, const bool is_binary = false);
FILE *angband_fopen_temp(char *buf, int max);
errr angband_fgets(FILE *fff, char *buf, ulong n);
errr angband_fputs(FILE *fff, concptr buf, ulong n);
errr angband_fclose(FILE *fff);
void fd_kill(std::string_view file);
void fd_move(std::string_view from, std::string_view to);
int fd_make(std::string_view file, bool can_write_group = false);
int fd_open(std::string_view file, int mode);
errr fd_lock(int fd, int what);
errr fd_seek(int fd, ulong n);
errr fd_read(int fd, char *buf, ulong n);
errr fd_write(int fd, concptr buf, ulong n);
errr fd_close(int fd);
