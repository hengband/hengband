/* File: h-system.h */

#ifndef INCLUDED_H_SYSTEM_H
#define INCLUDED_H_SYSTEM_H

/*
 * Include the basic "system" files.
 *
 * Make sure all "system" constants/macros are defined.
 * Make sure all "system" functions have "extern" declarations.
 *
 * This file is a big hack to make other files less of a hack.
 * This file has been rebuilt -- it may need a little more work.
 *
 * It is (very) unlikely that VMS will work without help, primarily
 * because VMS does not use the "ASCII" character set.
 */


#include <stdio.h>
#include <ctype.h>
#include <errno.h>


#if defined(NeXT)
# include <libc.h>
#else
# include <stdlib.h>
#endif /* NeXT */


#ifdef SET_UID

# include <sys/types.h>

# if defined(Pyramid) || defined(NeXT) || defined(SUNOS) || \
     defined(NCR3K) || defined(SUNOS) || defined(ibm032) || \
     defined(__osf__) || defined(ISC) || defined(SGI) || \
     defined(linux)
#  include <sys/time.h>
# endif

# if !defined(SGI) && !defined(ULTRIX)
#  include <sys/timeb.h>
# endif

#endif /* SET_UID */


#include <time.h>


#if defined(MACINTOSH)
# if defined(MAC_MPW)
#  ifdef __STDC__
#   undef __STDC__
#  endif
# else
#  include <unix.h>
# endif
#endif


#if defined(WINDOWS) || defined(MSDOS) || defined(USE_EMX)
# include <io.h>
#endif

#if !defined(MACINTOSH) && !defined(AMIGA) && \
    !defined(ACORN) && !defined(VM)
# if defined(__TURBOC__) || defined(__WATCOMC__)
#  include <mem.h>
# else
#  include <memory.h>
# endif
#endif


#if !defined(NeXT) && !defined(__MWERKS__) && !defined(ACORN)
# include <fcntl.h>
#endif


#ifdef SET_UID

# ifndef USG
#  include <sys/param.h>
#  include <sys/file.h>
# endif /* !USG */

# ifdef linux
#  include <sys/file.h>
# endif

# include <pwd.h>

# include <unistd.h>

# include <sys/stat.h>

# ifdef SOLARIS
#  include <netdb.h>
# endif

#endif /* SET_UID */

#ifdef __DJGPP__
#include <unistd.h>
#endif /* __DJGPP__ */

#include <string.h>

#if !defined(linux) && !defined(__MWERKS__) && !defined(ACORN) && !defined(WIN32)
extern long atol();
#endif


#include <stdarg.h>

#endif /* INCLUDED_H_SYSTEM_H */
