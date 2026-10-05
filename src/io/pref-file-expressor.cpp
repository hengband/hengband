#include "io/pref-file-expressor.h"
#include "game-option/runtime-arguments.h"
#include "io/condition-expression.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-realm.h"
#include "player/process-name.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "term/z-form.h"

/*!
 * @brief 設定ファイルと町のマップの条件式に共通の変数の値を返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name 先頭の「$」を除いた変数名
 * @return 変数の値。共通の変数でなければ tl::nullopt
 */
tl::optional<std::string> resolve_common_expression_variable(PlayerType *player_ptr, std::string_view name)
{
    if (name == "SYS") {
        return std::string(ANGBAND_SYS);
    }
    if (name == "GRAF") {
        return std::string(ANGBAND_GRAF);
    }
    if (name == "MONOCHROME") {
        return arg_monochrome ? "ON" : "OFF";
    }
    if (name == "RACE") {
        return rp_ptr->title.en_string();
    }
    if (name == "CLASS") {
        return cp_ptr->title.en_string();
    }
    if (name == "PLAYER") {
        return make_player_name_for_expression(player_ptr->name);
    }
    if (name == "REALM1") {
        return PlayerRealm(player_ptr).realm1().get_name().en_string();
    }
    if (name == "REALM2") {
        return PlayerRealm(player_ptr).realm2().get_name().en_string();
    }

    return tl::nullopt;
}

/*!
 * @brief 設定ファイルの条件式の変数の値を返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name 先頭の「$」を除いた変数名
 * @return 変数の値。知らない変数なら tl::nullopt
 */
static tl::optional<std::string> resolve_pref_file_variable(PlayerType *player_ptr, std::string_view name)
{
    if (name == "KEYBOARD") {
        return std::string(ANGBAND_KEYBOARD);
    }
    if (name == "LEVEL") {
        return format("%02d", player_ptr->lev);
    }
    if (name == "AUTOREGISTER") {
        return player_ptr->autopick_autoregister ? "1" : "0";
    }
    if (name == "MONEY") {
        return format("%09ld", (long int)player_ptr->au);
    }

    return resolve_common_expression_variable(player_ptr, name);
}

/*!
 * @brief 設定ファイルの条件式 (「?:」の行) を評価する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param expr 条件式
 * @return 評価の結果。条件を満たさなければ "0"
 * @details 書式と返す値は evaluate_condition_expression() を参照。
 */
std::string process_pref_file_expr(PlayerType *player_ptr, std::string_view expr)
{
    return evaluate_condition_expression(expr, [player_ptr](std::string_view name) { return resolve_pref_file_variable(player_ptr, name); });
}
