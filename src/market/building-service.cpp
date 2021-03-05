﻿#include "market/building-service.h"
#include "cmd-building/cmd-building.h"
#include "realm/realm-names-table.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"

/*!
 * @brief 施設毎に設定された種族、職業、魔法領域フラグがプレイヤーと一致するかを判定する。
 * @details 各種ギルドや寺院など、特定の職業ならば優遇措置を得られる施設、
 * あるいは食堂など特定の種族では利用できない施設の判定処理を行う。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param bldg 施設構造体の参照ポインタ
 * @return 種族、職業、魔法領域のいずれかが一致しているかの是非。
 */
bool is_owner(player_type *player_ptr, building_type *bldg)
{
    if (bldg->member_class[player_ptr->pclass] == BUILDING_OWNER) {
        return TRUE;
    }

    if (bldg->member_race[player_ptr->prace] == BUILDING_OWNER) {
        return TRUE;
    }

    REALM_IDX realm1 = player_ptr->realm1;
    REALM_IDX realm2 = player_ptr->realm2;
    if ((is_magic(realm1) && (bldg->member_realm[realm1] == BUILDING_OWNER)) || (is_magic(realm2) && (bldg->member_realm[realm2] == BUILDING_OWNER))) {
        return TRUE;
    }

    return FALSE;
}

/*!
 * @brief 施設毎に設定された種族、職業、魔法領域フラグがプレイヤーと一致するかを判定する。
 （スペルマスターの特別判定つき）
 * @details 各種ギルドや寺院など、特定の職業ならば優遇措置を得られる施設、
 * あるいは食堂など特定の種族では利用できない施設の判定処理を行う。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param bldg 施設構造体の参照ポインタ
 * @return 種族、職業、魔法領域のいずれかが一致しているかの是非。
 * @todo is_owner()との実質的な多重実装なので、リファクタリングを行うべきである。
 */
bool is_member(player_type *player_ptr, building_type *bldg)
{
    if (bldg->member_class[player_ptr->pclass]) {
        return TRUE;
    }

    if (bldg->member_race[player_ptr->prace]) {
        return TRUE;
    }

    REALM_IDX realm1 = player_ptr->realm1;
    REALM_IDX realm2 = player_ptr->realm2;
    if ((is_magic(realm1) && bldg->member_realm[realm1]) || (is_magic(realm2) && bldg->member_realm[realm2])) {
        return TRUE;
    }

    if (player_ptr->pclass != CLASS_SORCERER)
        return FALSE;

    for (int i = 0; i < MAX_MAGIC; i++) {
        if (bldg->member_realm[i + 1])
            return TRUE;
    }

    return FALSE;
}

/*!
 * @brief 施設のサービス一覧を表示する / Display a building.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param bldg 施設構造体の参照ポインタ
 * @return なし
 */
void display_buikding_service(player_type *player_ptr, building_type *bldg)
{
    char buff[20];
    byte action_color;
    char tmp_str[80];

    term_clear();
    sprintf(tmp_str, "%s (%s) %35s", bldg->owner_name, bldg->owner_race, bldg->name);
    prt(tmp_str, 2, 1);

    for (int i = 0; i < 8; i++) {
        if (!bldg->letters[i])
            continue;

        if (bldg->action_restr[i] == 0) {
            if ((is_owner(player_ptr, bldg) && (bldg->member_costs[i] == 0)) || (!is_owner(player_ptr, bldg) && (bldg->other_costs[i] == 0))) {
                action_color = TERM_WHITE;
                buff[0] = '\0';
            } else if (is_owner(player_ptr, bldg)) {
                action_color = TERM_YELLOW;
                sprintf(buff, _("($%ld)", "(%ldgp)"), (long int)bldg->member_costs[i]);
            } else {
                action_color = TERM_YELLOW;
                sprintf(buff, _("($%ld)", "(%ldgp)"), (long int)bldg->other_costs[i]);
            }

            sprintf(tmp_str, " %c) %s %s", bldg->letters[i], bldg->act_names[i], buff);
            c_put_str(action_color, tmp_str, 19 + (i / 2), 35 * (i % 2));
            continue;
        }

        if (bldg->action_restr[i] == 1) {
            if (!is_member(player_ptr, bldg)) {
                action_color = TERM_L_DARK;
                strcpy(buff, _("(閉店)", "(closed)"));
            } else if ((is_owner(player_ptr, bldg) && (bldg->member_costs[i] == 0)) || (is_member(player_ptr, bldg) && (bldg->other_costs[i] == 0))) {
                action_color = TERM_WHITE;
                buff[0] = '\0';
            } else if (is_owner(player_ptr, bldg)) {
                action_color = TERM_YELLOW;
                sprintf(buff, _("($%ld)", "(%ldgp)"), (long int)bldg->member_costs[i]);
            } else {
                action_color = TERM_YELLOW;
                sprintf(buff, _("($%ld)", "(%ldgp)"), (long int)bldg->other_costs[i]);
            }

            sprintf(tmp_str, " %c) %s %s", bldg->letters[i], bldg->act_names[i], buff);
            c_put_str(action_color, tmp_str, 19 + (i / 2), 35 * (i % 2));
            continue;
        }

        if (!is_owner(player_ptr, bldg)) {
            action_color = TERM_L_DARK;
            strcpy(buff, _("(閉店)", "(closed)"));
        } else if (bldg->member_costs[i] != 0) {
            action_color = TERM_YELLOW;
            sprintf(buff, _("($%ld)", "(%ldgp)"), (long int)bldg->member_costs[i]);
        } else {
            action_color = TERM_WHITE;
            buff[0] = '\0';
        }

        sprintf(tmp_str, " %c) %s %s", bldg->letters[i], bldg->act_names[i], buff);
        c_put_str(action_color, tmp_str, 19 + (i / 2), 35 * (i % 2));
    }

    prt(_(" ESC) 建物を出る", " ESC) Exit building"), 23, 0);
}
