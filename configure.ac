dnl Process this file with autoconf to produce a configure script.
AC_INIT(hengband, 3.0.2.5-Beta)

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

dnl lib/xtra (サブモジュール) がチェックアウトされているか
AM_CONDITIONAL([HAVE_XTRA], [test -f "$srcdir/lib/xtra/graf/8x8.bmp"])

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

dnl Detect macOS host
AC_CANONICAL_HOST
is_macos=no
case "${host_os}" in
  darwin*) is_macos=yes ;;
esac

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

dnl LTO (リンク時の最適化) を使うか。ゲームは速くなるが、1 ファイルを変えたときの
dnl 再ビルドが大幅に遅くなるので、既定では使わない。
AC_ARG_ENABLE([lto],
  [AS_HELP_STRING([--enable-lto], [enable link time optimization (faster game, much slower rebuilds)])],
  [], [enable_lto=no])
if test "x$enable_lto" = xyes; then
  dnl GCC は最適化を並列に行う -flto=auto、clang はコード生成を並列に行う ThinLTO を使う。
  dnl LTO のオブジェクトを静的ライブラリにまとめるには、それを扱える ar / ranlib が要る。
  dnl LTO_AR / LTO_RANLIB に探しておき、AR / RANLIB が指定されていなければ、AM_PROG_AR と
  dnl AC_PROG_RANLIB の後でそれに置き換える。見つからなければ通常の ar / ranlib を使う
  dnl (GCC は LTO のプラグインを通して扱え、macOS の ar も扱える)。
  if test "x$is_clang" = xyes; then
    lto_flags="-flto=thin"
    dnl llvm-ar は版番号付きの名前 (llvm-ar-18 など) でしか PATH に無いことがあるので、
    dnl clang 自身が知っている場所 (/usr/lib/llvm-18/bin など) も探す。
    lto_tool_path=$PATH
    lto_llvm_ar=`$CXX -print-prog-name=llvm-ar 2>/dev/null`
    AS_IF([test -x "$lto_llvm_ar"], [lto_tool_path=`dirname "$lto_llvm_ar"`$PATH_SEPARATOR$PATH])
    AC_PATH_TOOL([LTO_AR], [llvm-ar], [], [$lto_tool_path])
    AC_PATH_TOOL([LTO_RANLIB], [llvm-ranlib], [], [$lto_tool_path])
  else
    lto_flags="-flto=auto"
    AC_CHECK_TOOL([LTO_AR], [gcc-ar])
    AC_CHECK_TOOL([LTO_RANLIB], [gcc-ranlib])
  fi
  lto_ar_given=${AR:+yes}
  lto_ranlib_given=${RANLIB:+yes}
  dnl GNU ar は /usr/lib/bfd-plugins にある LLVMgold を通して LTO のオブジェクトを読むが、
  dnl 複数の版が入っていると clang より古い版が使われ、読めないオブジェクトが索引から漏れて
  dnl リンクできなくなる (小さな試験では再現しない)。macOS の ar は扱える。
  AS_IF([test "x$is_clang" = xyes && test "x$lto_ar_given" != xyes && test -z "$LTO_AR" && test "x$is_macos" != xyes],
    [AC_MSG_ERROR([llvm-ar is required to build with --enable-lto using Clang. Install llvm-ar, or specify AR=llvm-ar RANLIB=llvm-ranlib.])])
  CXXFLAGS="$CXXFLAGS $lto_flags"
  LDFLAGS="$LDFLAGS $lto_flags"
fi
dnl ARFLAGS が指定されている場合は、その値をそのまま使う。
dnl AM_PROG_AR は ARFLAGS が空の場合に既定値を設定する (automake 1.17 以降) ため、
dnl ユーザーが指定したかどうかは AM_PROG_AR を実行する前に調べておく。
AC_ARG_VAR([ARFLAGS],
  [archiver flags (default: crT if thin archives are supported, otherwise cru)])
arflags_given=${ARFLAGS+yes}

dnl ゲーム本体のソースをまとめた静的ライブラリ libhengband.a の生成に必要
AM_PROG_AR
AC_PROG_RANLIB

dnl AM_PROG_AR と AC_PROG_RANLIB は、前もって AR / RANLIB を設定しておいても configure -C の
dnl キャッシュの値を優先するので、LTO 用の ar / ranlib にはここで置き換える。
if test "x$enable_lto" = xyes; then
  AS_IF([test "x$lto_ar_given" != xyes && test -n "$LTO_AR"], [AR=$LTO_AR])
  AS_IF([test "x$lto_ranlib_given" != xyes && test -n "$LTO_RANLIB"], [RANLIB=$LTO_RANLIB])
fi

dnl libhengband.a は全オブジェクトを含むため 1GB 近くになる。
dnl thin archive (ar の T フラグ) が使えるなら、オブジェクトの実体をコピーせず
dnl パスのみを格納するようにして、ビルド時間とディスク使用量を削減する。
dnl GNU ar と llvm-ar のみ対応。T フラグを別の意味に解釈する ar (BSD/cctools では
dnl メンバ名の切り詰め) があるため、コマンドの成否ではなく、生成されたアーカイブの
dnl 先頭が thin archive のマジック "!<thin>" になっているかで判定する。
if test "x$arflags_given" != xyes; then
  AC_MSG_CHECKING([whether $AR supports thin archives])
  ar_thin_archive=no
  echo 'int hengband_ar_test;' > conftest_ar.cpp
  if $CXX -c conftest_ar.cpp -o conftest_ar.$OBJEXT >/dev/null 2>&1 &&
     $AR crT conftest_ar.a conftest_ar.$OBJEXT >/dev/null 2>&1 &&
     test -s conftest_ar.a; then
    case `dd if=conftest_ar.a bs=1 count=7 2>/dev/null` in
    ["!<thin>"]) ar_thin_archive=yes ;;
    esac
  fi
  rm -f conftest_ar.cpp conftest_ar.$OBJEXT conftest_ar.a
  AC_MSG_RESULT([$ar_thin_archive])
  if test "$ar_thin_archive" = yes; then
    ARFLAGS="crT"
  else
    ARFLAGS="cru"
  fi
fi

if test "x$enable_lto" = xyes; then
  AC_MSG_CHECKING([whether LTO objects can be archived and linked])
  echo 'int hengband_lto_test() { return 0; }' > conftest_lto.cpp
  echo 'int hengband_lto_test(); int main() { return hengband_lto_test(); }' > conftest_lto_main.cpp
  if $CXX $CPPFLAGS $CXXFLAGS -c conftest_lto.cpp -o conftest_lto.$OBJEXT >&AS_MESSAGE_LOG_FD 2>&1 &&
     $AR $ARFLAGS conftest_lto.a conftest_lto.$OBJEXT >&AS_MESSAGE_LOG_FD 2>&1 &&
     $RANLIB conftest_lto.a >&AS_MESSAGE_LOG_FD 2>&1 &&
     $CXX $CPPFLAGS $CXXFLAGS $LDFLAGS conftest_lto_main.cpp conftest_lto.a $LIBS -o conftest_lto$EXEEXT >&AS_MESSAGE_LOG_FD 2>&1; then
    lto_works=yes
  else
    lto_works=no
  fi
  rm -f conftest_lto.cpp conftest_lto_main.cpp conftest_lto.$OBJEXT conftest_lto.a conftest_lto$EXEEXT
  AC_MSG_RESULT([$lto_works ($lto_flags, AR=$AR, RANLIB=$RANLIB)])
  if test "$lto_works" != yes; then
    AC_MSG_ERROR([cannot build with --enable-lto. Specify an archiver that supports LTO objects (e.g. AR=gcc-ar RANLIB=gcc-ranlib for GCC, AR=llvm-ar RANLIB=llvm-ranlib for Clang), or a linker that supports LTO (e.g. LDFLAGS=-fuse-ld=lld), or build without --enable-lto.])
  fi
fi
saved_CXXFLAGS="$CXXFLAGS"
CXXFLAGS=$(echo "$CXXFLAGS" | sed 's/-Werror[[^ ]]*//g')
AX_CXX_COMPILE_STDCXX(20, [noext], [mandatory])
CXXFLAGS="$saved_CXXFLAGS"
PKG_PROG_PKG_CONFIG

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

dnl assert.h / TL_ASSERT の有効・無効。
dnl 既定は有効。出荷相当で消すときは --disable-assert。
dnl AC_HEADER_ASSERT は autoconf.h に NDEBUG を書くだけなので、
dnl stdafx.h が <tl/expected.hpp> を autoconf.h より先に読む経路でも
dnl 効くよう、無効時はコマンドラインにも -DNDEBUG を足す。
AC_HEADER_ASSERT
AS_IF([test "x$ac_enable_assert" = xno],
  [case " $CPPFLAGS $CXXFLAGS " in
   *" -DNDEBUG "*) ;;
   *) CPPFLAGS="$CPPFLAGS -DNDEBUG" ;;
   esac])

dnl Checks for libraries.
dnl Replace `main' with a function in -lncurses:
AC_CHECK_LIB(ncursesw, initscr, [
  AC_DEFINE(USE_GCU, 1, [Allow -mGCU environment]) 
  AC_DEFINE(USE_NCURSES, 1, [Use ncurses]) 
  LIBS="$LIBS -lncursesw"
  
  dnl Define MACOS_TERMINAL_UTF8 for macOS ncursesw builds
  if test "x$is_macos" = "xyes"; then
     AC_DEFINE(MACOS_TERMINAL_UTF8, 1, [Enable UTF-8 width fix for macOS terminal])
  fi
])
if test "$ac_cv_lib_ncursesw_initscr" != yes; then
  AC_CHECK_LIB(ncurses, initscr, [
    AC_DEFINE(USE_GCU, 1, [Allow -mGCU environment]) 
    AC_DEFINE(USE_NCURSES, 1, [Use ncurses]) 
    LIBS="$LIBS -lncurses"
    
    dnl Define MACOS_TERMINAL_UTF8 for macOS ncurses builds
    if test "x$is_macos" = "xyes"; then
       AC_DEFINE(MACOS_TERMINAL_UTF8, 1, [Enable UTF-8 width fix for macOS terminal])
    fi
  ])
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

dnl On Linux, backtrace() may need additional linker flags to produce useful
dnl symbol names.  With the GNU linker, the necessary option is
dnl --export-dynamic or, if using the compiler to do the linking, -rdynamic.
dnl Use -rdynamic on any platform where it is accepted.  If a platform
dnl requires linking additional libraries to use backtrace(), this may have
dnl to be modified as it only accounts for FreeBSD/NetBSD/OpenBSD where
dnl libexecinfo needs to be linked to have access to backtrace().
AC_SEARCH_LIBS([backtrace], [execinfo],
  [AC_LINK_IFELSE(
    [AC_LANG_SOURCE([[
      #include <execinfo.h>
      int main() { void *a[100]; int n; n = (int)backtrace(a, 100); return (n != 0) ? 0 : 1; }
    ]])],
    [AC_DEFINE([HAVE_BACKTRACE], [1], [Define if backtrace and backtrace_symbols exist.])
    ac_save_LDFLAGS="$LDFLAGS"
    LDFLAGS="$LDFLAGS -rdynamic"
    AC_LINK_IFELSE(
      [AC_LANG_SOURCE([[
        int main() { return 0; }
      ]])],
      [],
      [LDFLAGS="$ac_save_LDFLAGS"])])])

AC_CONFIG_FILES(Makefile src/Makefile lib/Makefile lib/apex/Makefile \
	lib/bone/Makefile lib/data/Makefile \
	lib/edit/Makefile lib/edit/quests/Makefile lib/edit/towns/Makefile \
	lib/file/Makefile lib/file/books/Makefile lib/help/Makefile \
	lib/info/Makefile lib/pref/Makefile lib/save/Makefile \
	lib/script/Makefile lib/user/Makefile)
AC_OUTPUT()
