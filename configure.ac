dnl Process this file with autoconf to produce a configure script.
AC_INIT(hengband, 3.0.1.27-Beta)

AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_HEADERS(src/autoconf.h)
AC_CONFIG_SRCDIR(src/main.cpp)
AM_INIT_AUTOMAKE(tar-pax)
AM_SILENT_RULES([yes])

AC_ARG_WITH(setgid,
[  --with-setgid=NAME      install hengband as group NAME],
[case "${withval}" in
  no) GAMEGROUP="";;
  yes) AC_MSG_ERROR(missing argument for --with-setgid);;
  *) GAMEGROUP="$withval";;
esac])
AC_SUBST(GAMEGROUP)
AM_CONDITIONAL(SET_GID, test "$GAMEGROUP" != "")

dnl generate the installation path for the ./lib/ folder
if test "$GAMEGROUP" != ""; then
  MY_EXPAND_DIR(game_libpath, "$datarootdir/games/$PACKAGE/lib/")
else
  MY_EXPAND_DIR(game_libpath, "./lib/")
  bindir=".."
fi

dnl overwrite the path with an user-specified value
AC_ARG_WITH(libpath,
[  --with-libpath=path     specify the path to the Hengband lib folder],
[game_libpath="$withval"])

AC_ARG_WITH(varpath,
[  --with-varpath=path     specify the path to the Hengband var folder],
[game_varpath="$withval"],
[game_varpath="$game_libpath"])

AC_DEFINE_UNQUOTED(DEFAULT_LIB_PATH, "$game_libpath", [Path to the Hengband lib folder])
DEFAULT_LIB_PATH="$game_libpath"
AC_SUBST(DEFAULT_LIB_PATH)

AC_DEFINE_UNQUOTED(DEFAULT_VAR_PATH, "$game_varpath", [Path to the Hengband var folder])
DEFAULT_VAR_PATH="$game_varpath"
AC_SUBST(DEFAULT_VAR_PATH)

dnl Checks for programs.
AC_LANG(C++)
AC_PROG_CXX
saved_CXXFLAGS="$CXXFLAGS"
CXXFLAGS=$(echo "$CXXFLAGS" | sed 's/-Werror.* //g')
AX_CXX_COMPILE_STDCXX(20, [noext], [mandatory])
CXXFLAGS="$saved_CXXFLAGS"
PKG_PROG_PKG_CONFIG

AC_MSG_CHECKING([whether the compiler is Clang])
AC_COMPILE_IFELSE(
  [AC_LANG_SOURCE([[
    #ifdef __clang__
    int main() { return 0; }
    #else
    #error "Not Clang"
    #endif
  ]])],
  [AC_MSG_RESULT([yes]); is_clang=yes],
  [AC_MSG_RESULT([no]); is_clang=no]
)
AM_CONDITIONAL([COMPILER_IS_CLANG], [test "x$is_clang" = "xyes"])

AC_ARG_ENABLE(japanese,
[  --disable-japanese      build english version], use_japanese=no, [AC_DEFINE(JP, 1, [Enable Japanese]) AC_DEFINE(EUC, 1, [Use Extended Unix Code])])

AC_ARG_ENABLE(xim,
[  --disable-xim           disable xim support], use_xim=no, use_xim=yes)
AC_ARG_ENABLE(fontset,
[  --disable-fontset       disable fontset support], use_fontset=no, use_fontset=yes)
AC_ARG_ENABLE([xft],
	AS_HELP_STRING([--enable-xft], [Enable xft support]))
AC_ARG_ENABLE(net,
[  --disable-net           disable networking support], use_net=no)
AC_ARG_ENABLE(worldscore,
[  --disable-worldscore    disable worldscore support], worldscore=no)
AC_ARG_ENABLE([pch],
[  --disable-pch           disable use of precompiled headers],
enable_pch=no, enable_pch=yes)
AM_CONDITIONAL([PCH], [test x$enable_pch = xyes])

dnl Checks for libraries.
dnl Replace `main' with a function in -lncurses:
AC_CHECK_LIB(ncursesw, initscr, [AC_DEFINE(USE_GCU, 1, [Allow -mGCU environment]) AC_DEFINE(USE_NCURSES, 1, [Use ncurses]) LIBS="$LIBS -lncursesw"])
if test "$ac_cv_lib_ncursesw_initscr" != yes; then
  AC_CHECK_LIB(ncurses, initscr, [AC_DEFINE(USE_GCU, 1, [Allow -mGCU environment]) AC_DEFINE(USE_NCURSES, 1, [Use ncurses]) LIBS="$LIBS -lncurses"])
  if test "$ac_cv_lib_ncurses_initscr" != yes; then
    AC_CHECK_LIB(curses, initscr, [AC_DEFINE(USE_GCU, 1, [Allow -mGCU environment]) LIBS="$LIBS -lcurses"])
    if test "$ac_cv_lib_curses_initscr" != yes; then
      AC_CHECK_LIB(termcap, tgetent, [AC_DEFINE(USE_CAP, 1, [Allow -mCAP environment]) LIBS="$LIBS -ltermcap"])
    fi
  fi
fi

AC_CHECK_LIB(iconv, iconv_open)

if test "$use_net" = no; then
  AC_DEFINE(DISABLE_NET, 1, [Disable networking support])
  worldscore=no;
else
  PKG_CHECK_MODULES(libcurl, [libcurl])
fi

dnl The world score server is currently only available in Japanese.
if test "$use_japanese" = no; then
  worldscore=no
fi
if test "$worldscore" != no; then
  AC_DEFINE(WORLD_SCORE, 1, [Allow the game to send scores to the score server])
fi

dnl Checks for header files.
AC_PATH_XTRA
if test "$have_x" = yes; then
  LIBS="$LIBS -lX11"
  AC_DEFINE(USE_X11, 1, [Allow -mX11 environment])
  CXXFLAGS="$X_CFLAGS $CXXFLAGS"
  LDFLAGS="$X_LIBS $LDFLAGS"

  if test "$use_fontset" = yes; then
    AC_DEFINE(USE_FONTSET, 1, [Use FontSet])
  else
    if test "$use_japanese" != no; then
      AC_DEFINE(USE_JP_FONTSTRUCT, 1, [Use Kanji without FontSet])
    fi
  fi

  if test "$use_xim" = yes; then
    AC_DEFINE(USE_XIM, 1, [Use X Imput Method])
  fi

  if test "$use_fontset" = yes || test "$use_xim" = yes; then
    AC_DEFINE(USE_LOCALE, 1, [Support Locale])
    if test "$use_japanese" != no; then
      AC_DEFINE(DEFAULT_LOCALE, "ja_JP.eucJP", Default Japanese Locale)
    fi
    AC_CHECK_LIB(X11, _Xsetlocale, AC_DEFINE(X_LOCALE, 1, [Use XLocale]))
    AC_CHECK_LIB(xpg4, setlocale, LIBS="$LIBS -lxpg4")
  fi

  AS_IF([test "x$enable_xft" = "xyes"], [
    PKG_CHECK_MODULES(XFT, [xft],
      [AC_DEFINE([USE_XFT], [1], [Use XFT])])
  ])
fi

if test "$use_japanese" != no; then
  AC_MSG_CHECKING([whether the compiler supports -fexec-charset=euc-jp-ms])
  saved_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -fexec-charset=euc-jp-ms"
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [],
      [return 0;]
    )],
    [AC_MSG_RESULT([yes])
      ac_cv_compile_exec_charset=yes],
    [AC_MSG_RESULT([no])
      ac_cv_compile_exec_charset=no]
  )
  CXXFLAGS="$saved_CXXFLAGS"

  if test x"$ac_cv_compile_exec_charset" = xno; then
    AC_CHECK_PROG(NKF, nkf, yes)
    if test x"$NKF" != x"yes"; then
      AC_MSG_ERROR([nkf is not found. Please install nkf.])
    fi
  fi
fi

AM_CONDITIONAL([COMPILER_HAVE_EXEC_CHARSET_EUC_JP_MS], [test x"$use_japanese" != xno -a x"$ac_cv_compile_exec_charset" = xyes])
AM_CONDITIONAL([USE_NKF], [test x"$use_japanese" != xno -a x"$ac_cv_compile_exec_charset" = xno])

if test "$enable_pch" != no; then
  dnl md5sum, sha224sum, sha256sum, sha384sum, sha512sum and b2sum are
  dnl frequently available on Linux.  md5 is available on macOS and OpenBSD.
  dnl sha1, sha256, and sha512 are available on OpenBSD.
  AC_PATH_PROGS(PCH_CHECKSUMMER, [md5sum md5 sha1sum sha1 sha224sum sha256sum sha256 sha384sum sha512sum sha512 b2sum], [none])
  AC_ARG_VAR([PCH_CHECKSUMMER], [full path to a utility to compute the checksum for the precompiled header; checksum is for ccache's pch_external_checksum])
fi

AC_CHECK_HEADERS(fcntl.h sys/file.h sys/ioctl.h sys/time.h termio.h unistd.h stdint.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM

dnl Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(gethostname mkdir select socket strtol mkstemp usleep)

AC_CONFIG_FILES(Makefile src/Makefile lib/Makefile lib/apex/Makefile \
	lib/bone/Makefile lib/data/Makefile \
	lib/edit/Makefile lib/edit/quests/Makefile lib/edit/towns/Makefile \
	lib/file/Makefile lib/file/books/Makefile lib/help/Makefile \
	lib/info/Makefile lib/pref/Makefile lib/save/Makefile \
	lib/script/Makefile lib/user/Makefile lib/xtra/Makefile \
	lib/xtra/graf/Makefile lib/xtra/music/Makefile lib/xtra/sound/Makefile)
AC_OUTPUT()
