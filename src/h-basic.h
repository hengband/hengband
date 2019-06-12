/*!
 * @file h-basic.h
 * @brief 変愚時追加された基本事項のヘッダーファイル /
 * The most basic "include" file. This file simply includes other low level header files.
 * @date 2014/08/15
 * @author
 * 不明(変愚蛮怒スタッフ？)
 */


#ifndef INCLUDED_H_BASIC_H
#define INCLUDED_H_BASIC_H

#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif /* HAVE_CONFIG_H */

/* System Configuration */
#include "h-config.h"

/* System includes/externs */
#include "h-system.h"

/* Basic types */
#include "h-type.h"

/* Basic constants and macros */
#include "h-define.h"

/* ゲーム調整値はこちらに */
#include "gamevalue.h"

#endif

/*
 * Hack -- Define NULL
 */
#ifndef NULL
# ifdef __STDC__
#  define NULL ((void*)0) /*!< コンパイル環境に定義がない場合のNULL定義 */
# else
#  define NULL ((char*)0) /*!< コンパイル環境に定義がない場合のNULL定義 */
# endif /* __STDC__ */
#endif /* NULL */

