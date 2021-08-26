/*!
 * @file h-system.h
 * @brief 変愚蛮怒用システムヘッダーファイル /
 * The most basic "include" file.
 * This file simply includes other low level header files.
 * @date 2021/08/26
 * @author Hourier
 */

#pragma once

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wctype.h>

#ifdef WINDOWS
#include <io.h>
#endif

#ifdef SET_UID
#include <pwd.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <unistd.h>
#if defined(Pyramid) || defined(NCR3K) || defined(ibm032) || defined(__osf__) || defined(ISC) || defined(linux)
#include <sys/time.h>
#endif
#endif /* SET_UID */
