#include "wizard/wizard-messages.h"
#include "game-option/cheat-options.h"
#include "game-option/cheat-types.h"
#include "io/write-diary.h"
#include "system/angband-exceptions.h"
#include "system/floor/floor-info.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include <array>
#include <string>
#include <string_view>

/*!
 * @brief 詐欺オプションの種類のメッセージを表示するかを返す
 * @param cheat_type 詐欺オプションの種類 (CHEAT_OBJECT など)
 * @return 対応する詐欺オプションが有効なら true
 * @details 範囲外の種類には true を返す。範囲の確認は msg_print_wizard() で行う。
 */
bool is_wizard_message_enabled(int cheat_type)
{
    switch (cheat_type) {
    case CHEAT_DUNGEON:
        return cheat_room;
    case CHEAT_OBJECT:
        return cheat_peek;
    case CHEAT_MONSTER:
        return cheat_hear;
    case CHEAT_MISC:
        return cheat_xtra;
    default:
        return true;
    }
}

/*!
 * @brief 詐欺オプションが有効なときにウィザードのメッセージを表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param cheat_type 詐欺オプションの種類 (CHEAT_OBJECT など)
 * @param msg メッセージ
 */
void msg_print_wizard(PlayerType *player_ptr, int cheat_type, std::string_view msg)
{
    constexpr auto max_type = 4;
    if ((cheat_type < 0) || (cheat_type >= max_type)) {
        THROW_EXCEPTION(std::logic_error, "Invalid cheat type is specified!");
    }

    if (!is_wizard_message_enabled(cheat_type)) {
        return;
    }

    static constexpr std::array<std::string_view, max_type> cheat_mes = { { "ITEM:", "MONS:", "DUNG:", "MISC:" } };
    const auto mes = fmt::format("WIZ-{}{}", cheat_mes[cheat_type], msg);
    msg_print(mes);
    if (cheat_diary_output) {
        exe_write_diary(*player_ptr->current_floor_ptr, DiaryKind::WIZARD_LOG, 0, mes);
    }
}
