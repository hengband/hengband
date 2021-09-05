#include "mind/mind-archer.h"
#include "action/action-limited.h"
#include "autopick/autopick.h"
#include "core/asking-player.h"
#include "core/player-update-types.h"
#include "flavor/flavor-describer.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/inventory-object.h"
#include "io/command-repeater.h"
#include "object-enchant/apply-magic.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/object-boost.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-kind-hook.h"
#include "perception/object-perception.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

enum ammo_creation_type {
    AMMO_NONE = 0,
    AMMO_SHOT = 1,
    AMMO_ARROW = 2,
    AMMO_BOLT = 3,
};

static bool select_ammo_creation_type(ammo_creation_type &type, PLAYER_LEVEL plev)
{
    COMMAND_CODE code;
    if (repeat_pull(&code)) {
        type = static_cast<ammo_creation_type>(code);
        switch (type) {
        case AMMO_SHOT:
        case AMMO_ARROW:
        case AMMO_BOLT:
            return true;
        case AMMO_NONE:
        default:
            break;
        }
    }

    concptr com;
    if (plev >= 20)
        com = _("[S]弾, [A]矢, [B]クロスボウの矢 :", "Create [S]hots, Create [A]rrow or Create [B]olt ?");
    else if (plev >= 10)
        com = _("[S]弾, [A]矢:", "Create [S]hots or Create [A]rrow ?");
    else
        com = _("[S]弾:", "Create [S]hots ?");

    while (type == AMMO_NONE) {
        char ch;

        if (!get_com(com, &ch, true)) {
            return false;
        }

        if (ch == 'S' || ch == 's') {
            type = AMMO_SHOT;
            break;
        }

        if ((ch == 'A' || ch == 'a') && (plev >= 10)) {
            type = AMMO_ARROW;
            break;
        }

        if ((ch == 'B' || ch == 'b') && (plev >= 20)) {
            type = AMMO_BOLT;
            break;
        }
    }

    repeat_push(static_cast<COMMAND_CODE>(type));
    return true;
}

/*!
 * @brief「弾/矢の製造」処理 / do_cmd_cast calls this function if the player's class is 'archer'.
 * Hook to determine if an object is contertible in an arrow/bolt
 * @return 製造を実際に行ったらTRUE、キャンセルしたらFALSEを返す
 */
bool create_ammo(player_type *creature_ptr)
{
    if (cmd_limit_confused(creature_ptr) || cmd_limit_blind(creature_ptr))
        return false;

    ammo_creation_type ext = AMMO_NONE;

    if (!select_ammo_creation_type(ext, creature_ptr->lev))
        return false;

    switch (ext) {
    case AMMO_SHOT: {
        DIRECTION dir;
        if (!get_rep_dir(creature_ptr, &dir, false))
            return false;

        POSITION y = creature_ptr->y + ddy[dir];
        POSITION x = creature_ptr->x + ddx[dir];
        grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
        if (f_info[g_ptr->get_feat_mimic()].flags.has_not(FF::CAN_DIG)) {
            msg_print(_("そこには岩石がない。", "You need a pile of rubble."));
            return false;
        }

        if (!g_ptr->cave_has_flag(FF::CAN_DIG) || !g_ptr->cave_has_flag(FF::HURT_ROCK)) {
            msg_print(_("硬すぎて崩せなかった。", "You failed to make ammo."));
            return true;
        }

        object_type forge;
        object_type *q_ptr = &forge;
        q_ptr->prep(lookup_kind(TV_SHOT, (OBJECT_SUBTYPE_VALUE)m_bonus(1, creature_ptr->lev) + 1));
        q_ptr->number = (byte)rand_range(15, 30);
        object_aware(creature_ptr, q_ptr);
        object_known(q_ptr);
        apply_magic_to_object(creature_ptr, q_ptr, creature_ptr->lev, AM_NO_FIXED_ART);
        q_ptr->discount = 99;
        int16_t slot = store_item_to_inventory(creature_ptr, q_ptr);
        GAME_TEXT o_name[MAX_NLEN];
        describe_flavor(creature_ptr, o_name, q_ptr, 0);
        msg_format(_("%sを作った。", "You make some ammo."), o_name);
        if (slot >= 0)
            autopick_alter_item(creature_ptr, slot, false);

        cave_alter_feat(creature_ptr, y, x, FF::HURT_ROCK);
        creature_ptr->update |= PU_FLOW;
        return true;
    }
    case AMMO_ARROW: {
        concptr q = _("どのアイテムから作りますか？ ", "Convert which item? ");
        concptr s = _("材料を持っていない。", "You have no item to convert.");
        OBJECT_IDX item;
        object_type *q_ptr = choose_object(creature_ptr, &item, q, s, USE_INVEN | USE_FLOOR, FuncItemTester(&object_type::is_convertible));
        if (!q_ptr)
            return false;

        object_type forge;
        q_ptr = &forge;
        q_ptr->prep(lookup_kind(TV_ARROW, (OBJECT_SUBTYPE_VALUE)m_bonus(1, creature_ptr->lev) + 1));
        q_ptr->number = (byte)rand_range(5, 10);
        object_aware(creature_ptr, q_ptr);
        object_known(q_ptr);
        apply_magic_to_object(creature_ptr, q_ptr, creature_ptr->lev, AM_NO_FIXED_ART);
        q_ptr->discount = 99;
        GAME_TEXT o_name[MAX_NLEN];
        describe_flavor(creature_ptr, o_name, q_ptr, 0);
        msg_format(_("%sを作った。", "You make some ammo."), o_name);
        vary_item(creature_ptr, item, -1);
        int16_t slot = store_item_to_inventory(creature_ptr, q_ptr);
        if (slot >= 0)
            autopick_alter_item(creature_ptr, slot, false);

        return true;
    }
    case AMMO_BOLT: {
        concptr q = _("どのアイテムから作りますか？ ", "Convert which item? ");
        concptr s = _("材料を持っていない。", "You have no item to convert.");
        OBJECT_IDX item;
        object_type *q_ptr = choose_object(creature_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), FuncItemTester(&object_type::is_convertible));
        if (!q_ptr)
            return false;

        object_type forge;
        q_ptr = &forge;
        q_ptr->prep(lookup_kind(TV_BOLT, (OBJECT_SUBTYPE_VALUE)m_bonus(1, creature_ptr->lev) + 1));
        q_ptr->number = (byte)rand_range(4, 8);
        object_aware(creature_ptr, q_ptr);
        object_known(q_ptr);
        apply_magic_to_object(creature_ptr, q_ptr, creature_ptr->lev, AM_NO_FIXED_ART);
        q_ptr->discount = 99;
        GAME_TEXT o_name[MAX_NLEN];
        describe_flavor(creature_ptr, o_name, q_ptr, 0);
        msg_format(_("%sを作った。", "You make some ammo."), o_name);
        vary_item(creature_ptr, item, -1);
        int16_t slot = store_item_to_inventory(creature_ptr, q_ptr);
        if (slot >= 0)
            autopick_alter_item(creature_ptr, slot, false);

        return true;
    }
    default:
        return true;
    }
}
