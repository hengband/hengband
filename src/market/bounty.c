#include "system/angband.h"
#include "market/bounty.h"
#include "world/world.h"
#include "player/avatar.h"
#include "market/bounty-prize-table.h"
#include "object/object-flavor.h"
#include "autopick/autopick.h"
#include "core/stuff-handler.h"

/*!
 * @brief 賞金首の引き換え処理 / Get prize
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 各種賞金首のいずれかでも換金が行われたか否か。
 */
bool exchange_cash(player_type *player_ptr)
{
    bool change = FALSE;
    GAME_TEXT o_name[MAX_NLEN];
    object_type *o_ptr;

    for (INVENTORY_IDX i = 0; i <= INVEN_LARM; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        if ((o_ptr->tval == TV_CAPTURE) && (o_ptr->pval == MON_TSUCHINOKO)) {
            char buf[MAX_NLEN + 20];
            object_desc(player_ptr, o_name, o_ptr, 0);
            sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "), o_name);
            if (get_check(buf)) {
                msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)(1000000L * o_ptr->number));
                player_ptr->au += 1000000L * o_ptr->number;
                player_ptr->redraw |= (PR_GOLD);
                vary_item(player_ptr, i, -o_ptr->number);
            }

            change = TRUE;
        }
    }

    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        if ((o_ptr->tval == TV_CORPSE) && (o_ptr->sval == SV_CORPSE) && (o_ptr->pval == MON_TSUCHINOKO)) {
            char buf[MAX_NLEN + 20];
            object_desc(player_ptr, o_name, o_ptr, 0);
            sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "), o_name);
            if (get_check(buf)) {
                msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)(200000L * o_ptr->number));
                player_ptr->au += 200000L * o_ptr->number;
                player_ptr->redraw |= (PR_GOLD);
                vary_item(player_ptr, i, -o_ptr->number);
            }

            change = TRUE;
        }
    }

    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        if ((o_ptr->tval == TV_CORPSE) && (o_ptr->sval == SV_SKELETON) && (o_ptr->pval == MON_TSUCHINOKO)) {
            char buf[MAX_NLEN + 20];
            object_desc(player_ptr, o_name, o_ptr, 0);
            sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "), o_name);
            if (get_check(buf)) {
                msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)(100000L * o_ptr->number));
                player_ptr->au += 100000L * o_ptr->number;
                player_ptr->redraw |= (PR_GOLD);
                vary_item(player_ptr, i, -o_ptr->number);
            }

            change = TRUE;
        }
    }

    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        if ((o_ptr->tval == TV_CORPSE) && (o_ptr->sval == SV_CORPSE) && (streq(r_name + r_info[o_ptr->pval].name, r_name + r_info[today_mon].name))) {
            char buf[MAX_NLEN + 20];
            object_desc(player_ptr, o_name, o_ptr, 0);
            sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "), o_name);
            if (get_check(buf)) {
                msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)((r_info[today_mon].level * 50 + 100) * o_ptr->number));
                player_ptr->au += (r_info[today_mon].level * 50 + 100) * o_ptr->number;
                player_ptr->redraw |= (PR_GOLD);
                vary_item(player_ptr, i, -o_ptr->number);
            }

            change = TRUE;
        }
    }

    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        o_ptr = &player_ptr->inventory_list[i];

        if ((o_ptr->tval == TV_CORPSE) && (o_ptr->sval == SV_SKELETON) && (streq(r_name + r_info[o_ptr->pval].name, r_name + r_info[today_mon].name))) {
            char buf[MAX_NLEN + 20];
            object_desc(player_ptr, o_name, o_ptr, 0);
            sprintf(buf, _("%s を換金しますか？", "Convert %s into money? "), o_name);
            if (get_check(buf)) {
                msg_format(_("賞金 %ld＄を手に入れた。", "You get %ldgp."), (long int)((r_info[today_mon].level * 30 + 60) * o_ptr->number));
                player_ptr->au += (r_info[today_mon].level * 30 + 60) * o_ptr->number;
                player_ptr->redraw |= (PR_GOLD);
                vary_item(player_ptr, i, -o_ptr->number);
            }

            change = TRUE;
        }
    }

    for (int j = 0; j < MAX_BOUNTY; j++) {
        for (INVENTORY_IDX i = INVEN_PACK - 1; i >= 0; i--) {
            o_ptr = &player_ptr->inventory_list[i];
            if ((o_ptr->tval != TV_CORPSE) || (o_ptr->pval != current_world_ptr->bounty_r_idx[j]))
                continue;

            char buf[MAX_NLEN + 20];
            int num, k;
            INVENTORY_IDX item_new;
            object_type forge;

            object_desc(player_ptr, o_name, o_ptr, 0);
            sprintf(buf, _("%sを渡しますか？", "Hand %s over? "), o_name);
            if (!get_check(buf))
                continue;

            vary_item(player_ptr, i, -o_ptr->number);
            chg_virtue(player_ptr, V_JUSTICE, 5);
            current_world_ptr->bounty_r_idx[j] += 10000;

            for (num = 0, k = 0; k < MAX_BOUNTY; k++) {
                if (current_world_ptr->bounty_r_idx[k] >= 10000)
                    num++;
            }

            msg_format(_("これで合計 %d ポイント獲得しました。", "You earned %d point%s total."), num, (num > 1 ? "s" : ""));

            object_prep(&forge, lookup_kind(prize_list[num - 1].tval, prize_list[num - 1].sval));
            apply_magic(player_ptr, &forge, player_ptr->current_floor_ptr->object_level, AM_NO_FIXED_ART);

            object_aware(player_ptr, &forge);
            object_known(&forge);

            /*
			 * Hand it --- Assume there is an empty slot.
			 * Since a corpse is handed at first,
			 * there is at least one empty slot.
			 */
            item_new = inven_carry(player_ptr, &forge);
            object_desc(player_ptr, o_name, &forge, 0);
            msg_format(_("%s(%c)を貰った。", "You get %s (%c). "), o_name, index_to_label(item_new));

            autopick_alter_item(player_ptr, item_new, FALSE);
            handle_stuff(player_ptr);
            change = TRUE;
        }
    }

    if (change)
        return TRUE;

    msg_print(_("賞金を得られそうなものは持っていなかった。", "You have nothing."));
    msg_print(NULL);
    return FALSE;
}
