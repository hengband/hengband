#include "mind/mind-archer.h"
#include "action/action-limited.h"
#include "autopick/autopick.h"
#include "core/asking-player.h"
#include "core/player-update-types.h"
#include "flavor/flavor-describer.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/inventory-object.h"
#include "object-enchant/apply-magic.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/object-boost.h"
#include "object-hook/hook-bow.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-generator.h"
#include "object/object-kind-hook.h"
#include "perception/object-perception.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

typedef enum ammo_creation_type {
    AMMO_NONE = 0,
    AMMO_SHOT = 1,
    AMMO_ARROW = 2,
    AMMO_BOLT = 3,
} ammo_creation_type;

/*!
 * @brief「弾/矢の製造」処理 / do_cmd_cast calls this function if the player's class is 'archer'.
 * Hook to determine if an object is contertible in an arrow/bolt
 * @return 製造を実際に行ったらTRUE、キャンセルしたらFALSEを返す
 */
bool create_ammo(player_type *creature_ptr)
{
    char com[80];
    if (creature_ptr->lev >= 20)
        sprintf(com, _("[S]弾, [A]矢, [B]クロスボウの矢 :", "Create [S]hots, Create [A]rrow or Create [B]olt ?"));
    else if (creature_ptr->lev >= 10)
        sprintf(com, _("[S]弾, [A]矢:", "Create [S]hots or Create [A]rrow ?"));
    else
        sprintf(com, _("[S]弾:", "Create [S]hots ?"));

    if (cmd_limit_confused(creature_ptr) || cmd_limit_blind(creature_ptr))
        return FALSE;

    ammo_creation_type ext = AMMO_NONE;
    char ch;
    while (TRUE) {
        if (!get_com(com, &ch, TRUE)) {
            return FALSE;
        }

        if (ch == 'S' || ch == 's') {
            ext = AMMO_SHOT;
            break;
        }

        if ((ch == 'A' || ch == 'a') && (creature_ptr->lev >= 10)) {
            ext = AMMO_ARROW;
            break;
        }

        if ((ch == 'B' || ch == 'b') && (creature_ptr->lev >= 20)) {
            ext = AMMO_BOLT;
            break;
        }
    }

    switch (ext) {
    case AMMO_SHOT: {
        DIRECTION dir;
        if (!get_rep_dir(creature_ptr, &dir, FALSE))
            return FALSE;

        POSITION y = creature_ptr->y + ddy[dir];
        POSITION x = creature_ptr->x + ddx[dir];
        grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
        if (!has_flag(f_info[get_feat_mimic(g_ptr)].flags, FF_CAN_DIG)) {
            msg_print(_("そこには岩石がない。", "You need a pile of rubble."));
            return FALSE;
        }

        if (!cave_has_flag_grid(g_ptr, FF_CAN_DIG) || !cave_has_flag_grid(g_ptr, FF_HURT_ROCK)) {
            msg_print(_("硬すぎて崩せなかった。", "You failed to make ammo."));
            return TRUE;
        }

        object_type forge;
        object_type *q_ptr = &forge;
        object_prep(creature_ptr, q_ptr, lookup_kind(TV_SHOT, (OBJECT_SUBTYPE_VALUE)m_bonus(1, creature_ptr->lev) + 1));
        q_ptr->number = (byte)rand_range(15, 30);
        object_aware(creature_ptr, q_ptr);
        object_known(q_ptr);
        apply_magic(creature_ptr, q_ptr, creature_ptr->lev, AM_NO_FIXED_ART);
        q_ptr->discount = 99;
        s16b slot = store_item_to_inventory(creature_ptr, q_ptr);
        GAME_TEXT o_name[MAX_NLEN];
        describe_flavor(creature_ptr, o_name, q_ptr, 0);
        msg_format(_("%sを作った。", "You make some ammo."), o_name);
        if (slot >= 0)
            autopick_alter_item(creature_ptr, slot, FALSE);

        cave_alter_feat(creature_ptr, y, x, FF_HURT_ROCK);
        creature_ptr->update |= PU_FLOW;
        return TRUE;
    }
    case AMMO_ARROW: {
        item_tester_hook = item_tester_hook_convertible;
        concptr q = _("どのアイテムから作りますか？ ", "Convert which item? ");
        concptr s = _("材料を持っていない。", "You have no item to convert.");
        OBJECT_IDX item;
        object_type *q_ptr = choose_object(creature_ptr, &item, q, s, USE_INVEN | USE_FLOOR, 0);
        if (!q_ptr)
            return FALSE;

        object_type forge;
        q_ptr = &forge;
        object_prep(creature_ptr, q_ptr, lookup_kind(TV_ARROW, (OBJECT_SUBTYPE_VALUE)m_bonus(1, creature_ptr->lev) + 1));
        q_ptr->number = (byte)rand_range(5, 10);
        object_aware(creature_ptr, q_ptr);
        object_known(q_ptr);
        apply_magic(creature_ptr, q_ptr, creature_ptr->lev, AM_NO_FIXED_ART);
        q_ptr->discount = 99;
        GAME_TEXT o_name[MAX_NLEN];
        describe_flavor(creature_ptr, o_name, q_ptr, 0);
        msg_format(_("%sを作った。", "You make some ammo."), o_name);
        vary_item(creature_ptr, item, -1);
        s16b slot = store_item_to_inventory(creature_ptr, q_ptr);
        if (slot >= 0)
            autopick_alter_item(creature_ptr, slot, FALSE);

        return TRUE;
    }
    case AMMO_BOLT: {
        item_tester_hook = item_tester_hook_convertible;
        concptr q = _("どのアイテムから作りますか？ ", "Convert which item? ");
        concptr s = _("材料を持っていない。", "You have no item to convert.");
        OBJECT_IDX item;
        object_type *q_ptr = choose_object(creature_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), 0);
        if (!q_ptr)
            return FALSE;

        object_type forge;
        q_ptr = &forge;
        object_prep(creature_ptr, q_ptr, lookup_kind(TV_BOLT, (OBJECT_SUBTYPE_VALUE)m_bonus(1, creature_ptr->lev) + 1));
        q_ptr->number = (byte)rand_range(4, 8);
        object_aware(creature_ptr, q_ptr);
        object_known(q_ptr);
        apply_magic(creature_ptr, q_ptr, creature_ptr->lev, AM_NO_FIXED_ART);
        q_ptr->discount = 99;
        GAME_TEXT o_name[MAX_NLEN];
        describe_flavor(creature_ptr, o_name, q_ptr, 0);
        msg_format(_("%sを作った。", "You make some ammo."), o_name);
        vary_item(creature_ptr, item, -1);
        s16b slot = store_item_to_inventory(creature_ptr, q_ptr);
        if (slot >= 0)
            autopick_alter_item(creature_ptr, slot, FALSE);

        return TRUE;
    }
    default:
        return TRUE;
    }
}
