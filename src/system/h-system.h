/*!
 * @file h-system.h
 * @brief 変愚蛮怒用システムヘッダーファイル /
 * The most basic "include" file.
 * This file simply includes other low level header files.
 * @date 2021/08/26
 * @author Hourier
 */

#pragma once

#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwctype>
#include <fcntl.h>
#include <memory.h>

// clang-format off

#ifdef WINDOWS
  #include <io.h>
#else
  #ifdef SET_UID
    #include <pwd.h>
    #include <sys/file.h>
    #include <sys/param.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>
  #endif
#endif

// clang-format on
