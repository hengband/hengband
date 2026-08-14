#pragma once

#include "system/angband.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <tl/optional.hpp>

/*!
 * @brief 長いコマンドライン引数の解釈結果
 */
enum class RuntimeArgumentResult {
    NOT_HANDLED, //!< 実行時オプションとしては解釈しなかった
    HANDLED, //!< 解釈して設定に反映した
    INVALID, //!< オプション名は一致したが値が不正
};

extern bool arg_music;
extern int arg_music_volume_table_index;
extern bool arg_sound;
extern int arg_sound_volume_table_index;
extern byte arg_graphics;
extern bool arg_monochrome;
extern bool arg_force_original;
extern bool arg_force_roguelike;
extern bool arg_bigtile;
extern bool arg_bot_json_output;
extern std::string arg_bot_json_output_path;
extern tl::optional<int> arg_control_port; //!< 制御サーバが待ち受けるTCPポート番号 (未指定なら制御サーバを起動しない)
extern tl::optional<uint32_t> arg_fixed_seed; //!< 乱数の初期シード (未指定なら従来通り実行毎にランダム)

RuntimeArgumentResult parse_runtime_argument(std::string_view option);
