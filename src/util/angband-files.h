#pragma once

#include "system/angband.h"

errr path_parse(char *buf, int max, concptr file);
errr path_build(char *buf, int max, concptr path, concptr file);
FILE *angband_fopen(concptr file, concptr mode);
FILE *angband_fopen_temp(char *buf, int max);
errr angband_fgets(FILE *fff, char *buf, huge n);
errr angband_fputs(FILE *fff, concptr buf, huge n);
errr angband_fclose(FILE *fff);
errr fd_kill(concptr file);
errr fd_move(concptr file, concptr what);
errr fd_copy(concptr file, concptr what);
int fd_make(concptr file, BIT_FLAGS mode);
int fd_open(concptr file, int flags);
errr fd_lock(int fd, int what);
errr fd_seek(int fd, huge n);
errr fd_chop(int fd, huge n);
errr fd_read(int fd, char *buf, huge n);
errr fd_write(int fd, concptr buf, huge n);
errr fd_close(int fd);
