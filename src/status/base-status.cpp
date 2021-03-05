﻿#include "status/base-status.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "game-option/birth-options.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "perception/object-perception.h"
#include "player/player-status-flags.h"
#include "player-info/avatar.h"
#include "spell-kind/spells-floor.h"
#include "system/object-type-definition.h"
#include "view/display-messages.h"

/* Array of stat "descriptions" */
static concptr desc_stat_pos[]
    = { _("強く", "stronger"), _("知的に", "smarter"), _("賢く", "wiser"), _("器用に", "more dextrous"), _("健康に", "healthier"), _("美しく", "cuter") };

/* Array of stat "descriptions" */
static concptr desc_stat_neg[]
    = { _("弱く", "weaker"), _("無知に", "stupider"), _("愚かに", "more naive"), _("不器用に", "clumsier"), _("不健康に", "more sickly"), _("醜く", "uglier") };

/*!
 * @brief プレイヤーの基本能力値を増加させる / Increases a stat by one randomized level -RAK-
 * @param stat 上昇させるステータスID
 * @return 実際に上昇した場合TRUEを返す。
 * @details
 * Note that this function (used by stat potions) now restores\n
 * the stat BEFORE increasing it.\n
 */
bool inc_stat(player_type *creature_ptr, int stat)
{
    BASE_STATUS gain;
    BASE_STATUS value = creature_ptr->stat_cur[stat];
    if (value >= creature_ptr->stat_max_max[stat])
        return FALSE;

    if (value < 18) {
        gain = ((randint0(100) < 75) ? 1 : 2);
        value += gain;
    } else if (value < (creature_ptr->stat_max_max[stat] - 2)) {
        gain = (((creature_ptr->stat_max_max[stat]) - value) / 2 + 3) / 2;
        if (gain < 1)
            gain = 1;

        value += randint1(gain) + gain / 2;
        if (value > (creature_ptr->stat_max_max[stat] - 1))
            value = creature_ptr->stat_max_max[stat] - 1;
    } else {
        value++;
    }

    creature_ptr->stat_cur[stat] = value;
    if (value > creature_ptr->stat_max[stat]) {
        creature_ptr->stat_max[stat] = value;
    }

    creature_ptr->update |= PU_BONUS;
    return TRUE;
}

/*!
 * @brief プレイヤーの基本能力値を減少させる / Decreases a stat by an amount indended to vary from 0 to 100 percent.
 * @param stat 減少させるステータスID
 * @param amount 減少させる基本量
 * @param permanent TRUEならば現在の最大値を減少させる
 * @return 実際に減少した場合TRUEを返す。
 * @details
 *\n
 * Amount could be a little higher in extreme cases to mangle very high\n
 * stats from massive assaults.  -CWS\n
 *\n
 * Note that "permanent" means that the *given* amount is permanent,\n
 * not that the new value becomes permanent.  This may not work exactly\n
 * as expected, due to "weirdness" in the algorithm, but in general,\n
 * if your stat is already drained, the "max" value will not drop all\n
 * the way down to the "cur" value.\n
 */
bool dec_stat(player_type *creature_ptr, int stat, int amount, int permanent)
{
    bool res = FALSE;
    BASE_STATUS cur = creature_ptr->stat_cur[stat];
    BASE_STATUS max = creature_ptr->stat_max[stat];
    int same = (cur == max);
    if (cur > 3) {
        if (cur <= 18) {
            if (amount > 90)
                cur--;
            if (amount > 50)
                cur--;
            if (amount > 20)
                cur--;
            cur--;
        } else {
            int loss = (((cur - 18) / 2 + 1) / 2 + 1);
            if (loss < 1)
                loss = 1;

            loss = ((randint1(loss) + loss) * amount) / 100;
            if (loss < amount / 2)
                loss = amount / 2;

            cur = cur - loss;
            if (cur < 18)
                cur = (amount <= 20) ? 18 : 17;
        }

        if (cur < 3)
            cur = 3;

        if (cur != creature_ptr->stat_cur[stat])
            res = TRUE;
    }

    if (permanent && (max > 3)) {
        chg_virtue(creature_ptr, V_SACRIFICE, 1);
        if (stat == A_WIS || stat == A_INT)
            chg_virtue(creature_ptr, V_ENLIGHTEN, -2);

        if (max <= 18) {
            if (amount > 90)
                max--;
            if (amount > 50)
                max--;
            if (amount > 20)
                max--;
            max--;
        } else {
            int loss = (((max - 18) / 2 + 1) / 2 + 1);
            loss = ((randint1(loss) + loss) * amount) / 100;
            if (loss < amount / 2)
                loss = amount / 2;

            max = max - loss;
            if (max < 18)
                max = (amount <= 20) ? 18 : 17;
        }

        if (same || (max < cur))
            max = cur;

        if (max != creature_ptr->stat_max[stat])
            res = TRUE;
    }

    if (res) {
        creature_ptr->stat_cur[stat] = cur;
        creature_ptr->stat_max[stat] = max;
        creature_ptr->redraw |= (PR_STATS);
        creature_ptr->update |= (PU_BONUS);
    }

    return (res);
}

/*!
 * @brief プレイヤーの基本能力値を回復させる / Restore a stat.  Return TRUE only if this actually makes a difference.
 * @param stat 回復ステータスID
 * @return 実際に回復した場合TRUEを返す。
 */
bool res_stat(player_type *creature_ptr, int stat)
{
    if (creature_ptr->stat_cur[stat] != creature_ptr->stat_max[stat]) {
        creature_ptr->stat_cur[stat] = creature_ptr->stat_max[stat];
        creature_ptr->update |= (PU_BONUS);
        creature_ptr->redraw |= (PR_STATS);
        return TRUE;
    }

    return FALSE;
}

/*
 * Lose a "point"
 */
bool do_dec_stat(player_type *creature_ptr, int stat)
{
    bool sust = FALSE;
    switch (stat) {
    case A_STR:
        if (has_sustain_str(creature_ptr))
            sust = TRUE;
        break;
    case A_INT:
        if (has_sustain_int(creature_ptr))
            sust = TRUE;
        break;
    case A_WIS:
        if (has_sustain_wis(creature_ptr))
            sust = TRUE;
        break;
    case A_DEX:
        if (has_sustain_dex(creature_ptr))
            sust = TRUE;
        break;
    case A_CON:
        if (has_sustain_con(creature_ptr))
            sust = TRUE;
        break;
    case A_CHR:
        if (has_sustain_chr(creature_ptr))
            sust = TRUE;
        break;
    }

    if (sust && (!ironman_nightmare || randint0(13))) {
        msg_format(_("%sなった気がしたが、すぐに元に戻った。", "You feel %s for a moment, but the feeling passes."), desc_stat_neg[stat]);
        return TRUE;
    }

    if (dec_stat(creature_ptr, stat, 10, (ironman_nightmare && !randint0(13)))) {
        msg_format(_("ひどく%sなった気がする。", "You feel %s."), desc_stat_neg[stat]);
        return TRUE;
    }

    return FALSE;
}

/*
 * Restore lost "points" in a stat
 */
bool do_res_stat(player_type *creature_ptr, int stat)
{
    if (res_stat(creature_ptr, stat)) {
        msg_format(_("元通りに%sなった気がする。", "You feel %s."), desc_stat_pos[stat]);
        return TRUE;
    }

    return FALSE;
}

/*
 * Gain a "point" in a stat
 */
bool do_inc_stat(player_type *creature_ptr, int stat)
{
    bool res = res_stat(creature_ptr, stat);
    if (inc_stat(creature_ptr, stat)) {
        if (stat == A_WIS) {
            chg_virtue(creature_ptr, V_ENLIGHTEN, 1);
            chg_virtue(creature_ptr, V_FAITH, 1);
        } else if (stat == A_INT) {
            chg_virtue(creature_ptr, V_KNOWLEDGE, 1);
            chg_virtue(creature_ptr, V_ENLIGHTEN, 1);
        } else if (stat == A_CON)
            chg_virtue(creature_ptr, V_VITALITY, 1);

        msg_format(_("ワーオ！とても%sなった！", "Wow! You feel %s!"), desc_stat_pos[stat]);
        return TRUE;
    }

    if (res) {
        msg_format(_("元通りに%sなった気がする。", "You feel %s."), desc_stat_pos[stat]);
        return TRUE;
    }

    return FALSE;
}

/*
 * Forget everything
 */
bool lose_all_info(player_type *creature_ptr)
{
    chg_virtue(creature_ptr, V_KNOWLEDGE, -5);
    chg_virtue(creature_ptr, V_ENLIGHTEN, -5);
    for (int i = 0; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &creature_ptr->inventory_list[i];
        if ((o_ptr->k_idx == 0) || object_is_fully_known(o_ptr))
            continue;

        o_ptr->feeling = FEEL_NONE;
        o_ptr->ident &= ~(IDENT_EMPTY);
        o_ptr->ident &= ~(IDENT_KNOWN);
        o_ptr->ident &= ~(IDENT_SENSE);
    }

    creature_ptr->update |= (PU_BONUS);
    creature_ptr->update |= (PU_COMBINE | PU_REORDER);
    creature_ptr->window_flags |= (PW_INVEN | PW_EQUIP | PW_PLAYER);
    wiz_dark(creature_ptr);
    return TRUE;
}
