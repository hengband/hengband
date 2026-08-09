#include "game-option/runtime-arguments.h"
#include "term/gameterm.h"
#include "util/string-processor.h"
#include <limits>

bool arg_sound; /* Command arg -- Request special sounds */
int arg_sound_volume_table_index;
bool arg_music; /* Command arg -- Request special musics */
int arg_music_volume_table_index;
byte arg_graphics; /* Command arg -- Request graphics mode */
bool arg_monochrome; /* Command arg -- Request monochrome mode */
bool arg_force_original; /* Command arg -- Request original keyset */
bool arg_force_roguelike; /* Command arg -- Request roguelike keyset */
bool arg_bigtile = false; /* Command arg -- Request big tile mode */
bool arg_bot_json_output = false; /* Command arg -- Output bot-readable JSON snapshots */
std::string arg_bot_json_output_path = "bot-state.jsonl";
tl::optional<int> arg_headless_port; /* Command arg -- Listen port of the headless terminal */
tl::optional<uint32_t> arg_fixed_seed; /* Command arg -- Fixed initial seed of the RNG */
int arg_headless_term_count = 1; /* Command arg -- Number of terminals the headless frontend creates */

namespace {

/*!
 * @brief 「オプション名=値」形式のコマンドライン引数から値の部分を取り出す
 * @param option 先頭の「--」を除いたコマンドライン引数
 * @param name オプション名
 * @return 値の文字列。オプション名が一致しない場合はnullopt
 */
tl::optional<std::string_view> extract_option_value(std::string_view option, std::string_view name)
{
    if (!option.starts_with(name) || (option.size() <= name.size()) || (option[name.size()] != '=')) {
        return tl::nullopt;
    }

    return option.substr(name.size() + 1);
}

/*!
 * @brief 数値を値に取るオプションを解釈する
 * @param option 先頭の「--」を除いたコマンドライン引数
 * @param name オプション名
 * @param setter 解釈した数値を設定に反映する関数
 * @param min 許容する最小値
 * @param max 許容する最大値
 * @return 解釈結果
 * @details
 * 有効範囲外の値を黙って切り詰めると、指定した値と実際の挙動が食い違って原因が分からなくなるため、
 * 範囲外は不正な値として扱う。
 */
template <typename T, typename Setter>
RuntimeArgumentResult parse_number_option(std::string_view option, std::string_view name, Setter setter,
    T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max())
{
    const auto value = extract_option_value(option, name);
    if (!value) {
        return RuntimeArgumentResult::NOT_HANDLED;
    }

    const auto number = str_to_num<T>(*value);
    if (!number || (*number < min) || (*number > max)) {
        return RuntimeArgumentResult::INVALID;
    }

    setter(*number);
    return RuntimeArgumentResult::HANDLED;
}

}

/*!
 * @brief プラットフォームに依存しない長いコマンドライン引数を解釈する
 * @param option 先頭の「--」を除いたコマンドライン引数
 * @return 解釈結果
 * @details Unix版とWindows版の双方から呼ばれる。オプションの綴りを一箇所で管理するためのもの。
 */
RuntimeArgumentResult parse_runtime_argument(std::string_view option)
{
    static constexpr std::string_view bot_json_output = "bot-json-output";
    if (option == bot_json_output) {
        arg_bot_json_output = true;
        return RuntimeArgumentResult::HANDLED;
    }

    if (const auto path = extract_option_value(option, bot_json_output); path) {
        arg_bot_json_output = true;
        if (!path->empty()) {
            arg_bot_json_output_path = *path;
        }

        return RuntimeArgumentResult::HANDLED;
    }

    if (const auto result = parse_number_option<int>(
            option, "headless-port", [](int port) { arg_headless_port = port; }, 1, 65535);
        result != RuntimeArgumentResult::NOT_HANDLED) {
        return result;
    }

    if (const auto result = parse_number_option<uint32_t>(option, "fixed-seed", [](uint32_t seed) { arg_fixed_seed = seed; });
        result != RuntimeArgumentResult::NOT_HANDLED) {
        return result;
    }

    return parse_number_option<int>(
        option, "headless-term-count", [](int count) { arg_headless_term_count = count; }, 1, MAX_TERM_DATA);
}
