#include "market/building-service.h"
#include "player-base/player-class.h"
#include "player/player-realm.h"
#include "system/building-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/enum-converter.h"

/*!
 * @brief 施設毎に設定された種族、職業、魔法領域フラグがプレイヤーと一致するかを判定する。
 * @details 各種ギルドや寺院など、特定の職業ならば優遇措置を得られる施設、
 * あるいは食堂など特定の種族では利用できない施設の判定処理を行う。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param bldg 施設構造体の参照ポインタ
 * @return 種族、職業、魔法領域のいずれかが一致しているかの是非。
 */
bool is_owner(PlayerType *player_ptr, const building_type &bldg)
{
    constexpr auto building_owner = 2;
    if (bldg.member_class[enum2i(player_ptr->pclass)] == building_owner) {
        return true;
    }

    if (bldg.member_race[enum2i(player_ptr->prace)] == building_owner) {
        return true;
    }

    PlayerRealm pr(player_ptr);
    const auto realm1 = pr.realm1().to_enum();
    const auto realm2 = pr.realm2().to_enum();
    if ((PlayerRealm::is_magic(realm1) && (bldg.member_realm[enum2i(realm1)] == building_owner)) || (PlayerRealm::is_magic(realm2) && (bldg.member_realm[enum2i(realm2)] == building_owner))) {
        return true;
    }

    return false;
}

/*!
 * @brief 施設毎に設定された種族、職業、魔法領域フラグがプレイヤーと一致するかを判定する。
 （スペルマスターの特別判定つき）
 * @details 各種ギルドや寺院など、特定の職業ならば優遇措置を得られる施設、
 * あるいは食堂など特定の種族では利用できない施設の判定処理を行う。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param bldg 施設構造体の参照ポインタ
 * @return 種族、職業、魔法領域のいずれかが一致しているかの是非。
 * @todo is_owner()との実質的な多重実装なので、リファクタリングを行うべきである。
 */
bool is_member(PlayerType *player_ptr, const building_type &bldg)
{
    if (static_cast<bool>(bldg.member_class[enum2i(player_ptr->pclass)])) {
        return true;
    }

    if (static_cast<bool>(bldg.member_race[enum2i(player_ptr->prace)])) {
        return true;
    }

    PlayerRealm pr(player_ptr);
    const auto realm1 = pr.realm1().to_enum();
    const auto realm2 = pr.realm2().to_enum();
    if ((PlayerRealm::is_magic(realm1) && bldg.member_realm[enum2i(realm1)]) || (PlayerRealm::is_magic(realm2) && bldg.member_realm[enum2i(realm2)])) {
        return true;
    }

    if (!PlayerClass(player_ptr).equals(PlayerClassType::SORCERER)) {
        return false;
    }

    for (int i = 0; i < MAX_MAGIC; i++) {
        if (bldg.member_realm[i + 1]) {
            return true;
        }
    }

    return false;
}

/*!
 * @brief 施設のサービス一覧を表示する / Display a building.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param bldg 施設構造体の参照ポインタ
 */
void display_buikding_service(PlayerType *player_ptr, const building_type &bldg)
{
    byte action_color;

    term_clear();
    prt(format("%s (%s) %35s", bldg.owner_name, bldg.owner_race, bldg.name), 2, 1);

    for (int i = 0; i < 8; i++) {
        if (!bldg.letters[i]) {
            continue;
        }

        std::string buff;
        if (bldg.action_restr[i] == 0) {
            if ((is_owner(player_ptr, bldg) && (bldg.member_costs[i] == 0)) || (!is_owner(player_ptr, bldg) && (bldg.other_costs[i] == 0))) {
                action_color = TERM_WHITE;
            } else if (is_owner(player_ptr, bldg)) {
                action_color = TERM_YELLOW;
                buff = format(_("($%d)", "(%dgp)"), bldg.member_costs[i]);
            } else {
                action_color = TERM_YELLOW;
                buff = format(_("($%d)", "(%dgp)"), bldg.other_costs[i]);
            }

            c_put_str(action_color, format(" %c) %s %s", bldg.letters[i], bldg.act_names[i], buff.data()), 19 + (i / 2), 35 * (i % 2));
            continue;
        }

        if (bldg.action_restr[i] == 1) {
            if (!is_member(player_ptr, bldg)) {
                action_color = TERM_L_DARK;
                buff = _("(閉店)", "(closed)");
            } else if ((is_owner(player_ptr, bldg) && (bldg.member_costs[i] == 0)) || (is_member(player_ptr, bldg) && (bldg.other_costs[i] == 0))) {
                action_color = TERM_WHITE;
            } else if (is_owner(player_ptr, bldg)) {
                action_color = TERM_YELLOW;
                buff = format(_("($%d)", "(%dgp)"), bldg.member_costs[i]);
            } else {
                action_color = TERM_YELLOW;
                buff = format(_("($%d)", "(%dgp)"), bldg.other_costs[i]);
            }

            c_put_str(action_color, format(" %c) %s %s", bldg.letters[i], bldg.act_names[i], buff.data()), 19 + (i / 2), 35 * (i % 2));
            continue;
        }

        if (!is_owner(player_ptr, bldg)) {
            action_color = TERM_L_DARK;
            buff = _("(閉店)", "(closed)");
        } else if (bldg.member_costs[i] != 0) {
            action_color = TERM_YELLOW;
            buff = format(_("($%d)", "(%dgp)"), bldg.member_costs[i]);
        } else {
            action_color = TERM_WHITE;
        }

        c_put_str(action_color, format(" %c) %s %s", bldg.letters[i], bldg.act_names[i], buff.data()), 19 + (i / 2), 35 * (i % 2));
    }

    prt(_(" ESC) 建物を出る", " ESC) Exit building"), 23, 0);
}
