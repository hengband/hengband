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
#include "system/h-config.h"

/* System includes/externs */
#include "system/h-system.h"

/* Basic types */
#include "system/h-type.h"

/* ゲーム調整値はこちらに */
#include "system/gamevalue.h"

#endif
