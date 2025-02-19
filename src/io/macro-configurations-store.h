/*
 * @brief マクロ設定定義
 * @author Hourier
 * @date 2024/02/19
 */

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

constexpr size_t MAX_MACRO_MOD = 12;
constexpr size_t MAX_MACRO_TRIG = 200; //!< 登録を許すマクロ（トリガー）の最大数

enum class ShiftStatus {
    OFF,
    ON,
};

extern size_t max_macrotrigger; //!<  現在登録中のマクロ(トリガー)の数
extern std::optional<std::string> macro_template; //!<  Angband設定ファイルのT: タグ情報から読み込んだ長いTコードを処理するために利用する文字列
extern std::optional<std::string> macro_modifier_chr; //!<  &x# で指定されるマクロトリガーに関する情報を記録する文字列
extern std::vector<std::string> macro_modifier_names; //!<  マクロ上で取り扱う特殊キーを文字列上で表現するためのフォーマットを記録した文字列配列
extern std::vector<std::string> macro_trigger_names; //!< マクロのトリガーコード
extern std::map<ShiftStatus, std::vector<std::string>> macro_trigger_keycodes; //!<  マクロの内容
