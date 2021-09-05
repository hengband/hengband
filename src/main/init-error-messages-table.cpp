/*!
 * @file init-error-messages-table.cpp
 * @brief 変愚蛮怒のゲームデータ解析エラー名定義
 */

#include "main/init-error-messages-table.h"

/*!
 * エラーメッセージの名称定義 / Standard error message text
 */
concptr err_str[PARSE_ERROR_MAX] = {
    nullptr,
    _("文法エラー", "parse error"),
    _("古いファイル", "obsolete file"),
    _("記録ヘッダがない", "missing record header"),
    _("不連続レコード", "non-sequential records"),
    _("おかしなフラグ存在", "invalid flag specification"),
    _("未定義命令", "undefined directive"),
    _("メモリ不足", "out of memory"),
    _("座標範囲外", "coordinates out of bounds"),
    _("引数不足", "too few arguments"),
    _("未定義地形タグ", "undefined terrain tag"),
};
