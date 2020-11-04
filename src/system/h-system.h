/*!
 * @file h-system.h
 * @brief 変愚蛮怒用システムヘッダーファイル /
 * The most basic "include" file. This file simply includes other low level header files.
 * @date 2014/08/16
 * @author
 * 不明(変愚蛮怒スタッフ？)
 * @details
 * Include the basic "system" files.
 * Make sure all "system" constants/macros are defined.
 * Make sure all "system" functions have "extern" declarations.
 * This file is a big hack to make other files less of a hack.
 * This file has been rebuilt -- it may need a little more work.
 *
 * It is (very) unlikely that VMS will work without help, primarily
 * because VMS does not use the "ASCII" character set.
 */

#ifndef INCLUDED_H_SYSTEM_H
#define INCLUDED_H_SYSTEM_H

#include <stdio.h>
#include <ctype.h>
#include <wctype.h>
#include <errno.h>
#include <stddef.h>
# include <stdlib.h>

#ifdef SET_UID

# include <sys/types.h>

# if defined(Pyramid) || defined(NCR3K) || defined(ibm032) || \
     defined(__osf__) || defined(ISC) || defined(linux)
#  include <sys/time.h>
# endif

#  include <sys/timeb.h>

#endif /* SET_UID */

#include <time.h>

#if defined(WINDOWS)
# include <io.h>
#endif

#if !defined(VM)
# if defined(__TURBOC__) || defined(__WATCOMC__)
#  include <mem.h>
# else
#  include <memory.h>
# endif
#endif

#include <fcntl.h>

#ifdef SET_UID

#  include <sys/param.h>
#  include <sys/file.h>

# ifdef linux
#  include <sys/file.h>
# endif

# include <pwd.h>

# include <unistd.h>

# include <sys/stat.h>

# ifdef si_ptr
/*
 * Linux (at least Debian 10) defines this to access a field in siginfo_t
 * (see the sigaction(2) man page).  Get rid of it since Hengband isn't
 * using siginfo_t and does use si_ptr in other contexts.
 */
#  undef si_ptr
# endif

#endif /* SET_UID */

#include <string.h>

#include <stdarg.h>

#endif /* INCLUDED_H_SYSTEM_H */
