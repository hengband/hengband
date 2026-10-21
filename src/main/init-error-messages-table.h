#pragma once
/*!
 * @file init-error-messages-table.h
 * @brief 変愚蛮怒のゲームデータ解析エラー名ヘッダ
 */

#include "info-reader/parse-error-types.h"
#include "system/angband.h"
#include "util/enum-converter.h"

extern concptr err_str[enum2i(ParseErrorType::MAX)];
