﻿#include "player/mimic-info-table.h"
#include "player/player-class.h"
#include "player/player-personality.h"
#include "player/player-sex.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "view/display-player-stat-info.h"

/*!
 * @brief プレイヤーの特性フラグ一覧表示2a /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * Special display, part 2a
 * @return なし
 */
void display_player_misc_info(player_type *creature_ptr)
{
    put_str(_("名前  :", "Name  :"), 1, 26);
    put_str(_("性別  :", "Sex   :"), 3, 1);
    put_str(_("種族  :", "Race  :"), 4, 1);
    put_str(_("職業  :", "Class :"), 5, 1);

    char buf[80];
    char tmp[80];
    strcpy(tmp, ap_ptr->title);
#ifdef JP
    if (ap_ptr->no == 1)
        strcat(tmp, "の");
#else
    strcat(tmp, " ");
#endif
    strcat(tmp, creature_ptr->name);

    c_put_str(TERM_L_BLUE, tmp, 1, 34);
    c_put_str(TERM_L_BLUE, sp_ptr->title, 3, 9);
    c_put_str(TERM_L_BLUE, (creature_ptr->mimic_form ? mimic_info[creature_ptr->mimic_form].title : rp_ptr->title), 4, 9);
    c_put_str(TERM_L_BLUE, cp_ptr->title, 5, 9);

    put_str(_("レベル:", "Level :"), 6, 1);
    put_str(_("ＨＰ  :", "Hits  :"), 7, 1);
    put_str(_("ＭＰ  :", "Mana  :"), 8, 1);

    (void)sprintf(buf, "%d", (int)creature_ptr->lev);
    c_put_str(TERM_L_BLUE, buf, 6, 9);
    (void)sprintf(buf, "%d/%d", (int)creature_ptr->chp, (int)creature_ptr->mhp);
    c_put_str(TERM_L_BLUE, buf, 7, 9);
    (void)sprintf(buf, "%d/%d", (int)creature_ptr->csp, (int)creature_ptr->msp);
    c_put_str(TERM_L_BLUE, buf, 8, 9);
}
