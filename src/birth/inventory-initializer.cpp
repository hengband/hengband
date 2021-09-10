#include "birth/inventory-initializer.h"
#include "autopick/autopick.h"
#include "birth/initial-equipments-table.h"
#include "floor/floor-object.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "object-enchant/apply-magic.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/object-ego.h"
#include "object/object-info.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "realm/realm-types.h"
#include "sv-definition/sv-bow-types.h"
#include "sv-definition/sv-food-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-potion-types.h"
#include "sv-definition/sv-ring-types.h"
#include "sv-definition/sv-scroll-types.h"
#include "sv-definition/sv-staff-types.h"
#include "sv-definition/sv-wand-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"

/*!
 * @brief 所持状態にあるアイテムの中から一部枠の装備可能なものを装備させる。
 */
void wield_all(player_type *creature_ptr)
{
    object_type object_type_body;
    for (INVENTORY_IDX item = INVEN_PACK - 1; item >= 0; item--) {
        object_type *o_ptr;
        o_ptr = &creature_ptr->inventory_list[item];
        if (!o_ptr->k_idx)
            continue;

        int slot = wield_slot(creature_ptr, o_ptr);
        if (slot < INVEN_MAIN_HAND)
            continue;
        if (slot == INVEN_LITE)
            continue;
        if (creature_ptr->inventory_list[slot].k_idx)
            continue;

        object_type *i_ptr;
        i_ptr = &object_type_body;
        i_ptr->copy_from(o_ptr);
        i_ptr->number = 1;

        if (item >= 0) {
            inven_item_increase(creature_ptr, item, -1);
            inven_item_optimize(creature_ptr, item);
        } else {
            floor_item_increase(creature_ptr, 0 - item, -1);
            floor_item_optimize(creature_ptr, 0 - item);
        }

        o_ptr = &creature_ptr->inventory_list[slot];
        o_ptr->copy_from(i_ptr);
        creature_ptr->equip_cnt++;
    }
}

/*!
 * @brief 初期所持アイテムの処理 / Add an outfit object
 * @details アイテムを既知のものとした上でwield_all()関数により装備させる。
 * @param o_ptr 処理したいオブジェクト構造体の参照ポインタ
 */
void add_outfit(player_type *creature_ptr, object_type *o_ptr)
{
    object_aware(creature_ptr, o_ptr);
    object_known(o_ptr);
    int16_t slot = store_item_to_inventory(creature_ptr, o_ptr);
    autopick_alter_item(creature_ptr, slot, false);
    wield_all(creature_ptr);
}

static void decide_initial_items(player_type *creature_ptr, object_type *q_ptr)
{
    switch (creature_ptr->prace) {
    case player_race_type::VAMPIRE:
        /* Nothing! */
        /* Vampires can drain blood of creatures */
        break;
    case player_race_type::BALROG:
        /* Demon can drain vitality from humanoid corpse */
        get_mon_num_prep(creature_ptr, monster_hook_human, nullptr);
        for (int i = rand_range(3, 4); i > 0; i--) {
            q_ptr->prep(lookup_kind(TV_CORPSE, SV_CORPSE));
            q_ptr->pval = get_mon_num(creature_ptr, 0, 2, 0);
            if (q_ptr->pval) {
                q_ptr->number = 1;
                add_outfit(creature_ptr, q_ptr);
            }
        }

        break;
    case player_race_type::SKELETON:
    case player_race_type::GOLEM:
    case player_race_type::ZOMBIE:
    case player_race_type::SPECTRE:
        /* Staff (of Nothing) */
        q_ptr->prep(lookup_kind(TV_STAFF, SV_STAFF_NOTHING));
        q_ptr->number = 1;
        add_outfit(creature_ptr, q_ptr);
        break;
    case player_race_type::ENT:
        /* Potions of Water */
        q_ptr->prep(lookup_kind(TV_POTION, SV_POTION_WATER));
        q_ptr->number = (ITEM_NUMBER)rand_range(15, 23);
        add_outfit(creature_ptr, q_ptr);
        break;
    case player_race_type::ANDROID:
        /* Flasks of oil */
        q_ptr->prep(lookup_kind(TV_FLASK, SV_ANY));
        apply_magic_to_object(creature_ptr, q_ptr, 1, AM_NO_FIXED_ART);
        q_ptr->number = (ITEM_NUMBER)rand_range(7, 12);
        add_outfit(creature_ptr, q_ptr);
        break;
    default:
        /* Food rations */
        q_ptr->prep(lookup_kind(TV_FOOD, SV_FOOD_RATION));
        q_ptr->number = (ITEM_NUMBER)rand_range(3, 7);
        add_outfit(creature_ptr, q_ptr);
    }
}

/*!
 * @brief 種族/職業/性格などに基づき初期所持アイテムを設定するメインセット関数。 / Init players with some belongings
 * @details Having an item makes the player "aware" of its purpose.
 */
void player_outfit(player_type *creature_ptr)
{
    object_type *q_ptr;
    object_type forge;
    q_ptr = &forge;

    decide_initial_items(creature_ptr, q_ptr);
    q_ptr = &forge;

    if ((creature_ptr->prace == player_race_type::VAMPIRE) && (creature_ptr->pclass != CLASS_NINJA)) {
        q_ptr->prep(lookup_kind(TV_SCROLL, SV_SCROLL_DARKNESS));
        q_ptr->number = (ITEM_NUMBER)rand_range(2, 5);
        add_outfit(creature_ptr, q_ptr);
    } else if (creature_ptr->pclass != CLASS_NINJA) {
        q_ptr->prep(lookup_kind(TV_LITE, SV_LITE_TORCH));
        q_ptr->number = (ITEM_NUMBER)rand_range(3, 7);
        q_ptr->xtra4 = rand_range(3, 7) * 500;

        add_outfit(creature_ptr, q_ptr);
    }

    q_ptr = &forge;
    if (creature_ptr->prace == player_race_type::MERFOLK) {
        q_ptr->prep(lookup_kind(TV_RING, SV_RING_LEVITATION_FALL));
        q_ptr->number = 1;
        add_outfit(creature_ptr, q_ptr);
    }

    if ((creature_ptr->pclass == CLASS_RANGER) || (creature_ptr->pclass == CLASS_CAVALRY)) {
        q_ptr->prep(lookup_kind(TV_ARROW, SV_AMMO_NORMAL));
        q_ptr->number = (byte)rand_range(15, 20);
        add_outfit(creature_ptr, q_ptr);
    }

    if (creature_ptr->pclass == CLASS_RANGER) {
        q_ptr->prep(lookup_kind(TV_BOW, SV_SHORT_BOW));
        add_outfit(creature_ptr, q_ptr);
    } else if (creature_ptr->pclass == CLASS_ARCHER) {
        q_ptr->prep(lookup_kind(TV_ARROW, SV_AMMO_NORMAL));
        q_ptr->number = (ITEM_NUMBER)rand_range(15, 20);
        add_outfit(creature_ptr, q_ptr);
    } else if (creature_ptr->pclass == CLASS_HIGH_MAGE || creature_ptr->pclass == CLASS_ELEMENTALIST) {
        q_ptr->prep(lookup_kind(TV_WAND, SV_WAND_MAGIC_MISSILE));
        q_ptr->number = 1;
        q_ptr->pval = (PARAMETER_VALUE)rand_range(25, 30);
        add_outfit(creature_ptr, q_ptr);
    } else if (creature_ptr->pclass == CLASS_SORCERER) {
        for (auto book_tval = enum2i(TV_LIFE_BOOK); book_tval <= enum2i(TV_LIFE_BOOK) + enum2i(MAX_MAGIC) - 1; book_tval++) {
            q_ptr->prep(lookup_kind(static_cast<tval_type>(book_tval), 0));
            q_ptr->number = 1;
            add_outfit(creature_ptr, q_ptr);
        }
    } else if (creature_ptr->pclass == CLASS_TOURIST) {
        if (creature_ptr->pseikaku != PERSONALITY_SEXY) {
            q_ptr->prep(lookup_kind(TV_SHOT, SV_AMMO_LIGHT));
            q_ptr->number = rand_range(15, 20);
            add_outfit(creature_ptr, q_ptr);
        }

        q_ptr->prep(lookup_kind(TV_FOOD, SV_FOOD_BISCUIT));
        q_ptr->number = rand_range(2, 4);

        add_outfit(creature_ptr, q_ptr);

        q_ptr->prep(lookup_kind(TV_FOOD, SV_FOOD_WAYBREAD));
        q_ptr->number = rand_range(2, 4);

        add_outfit(creature_ptr, q_ptr);

        q_ptr->prep(lookup_kind(TV_FOOD, SV_FOOD_JERKY));
        q_ptr->number = rand_range(1, 3);

        add_outfit(creature_ptr, q_ptr);

        q_ptr->prep(lookup_kind(TV_FOOD, SV_FOOD_PINT_OF_ALE));
        q_ptr->number = rand_range(2, 4);

        add_outfit(creature_ptr, q_ptr);

        q_ptr->prep(lookup_kind(TV_FOOD, SV_FOOD_PINT_OF_WINE));
        q_ptr->number = rand_range(2, 4);

        add_outfit(creature_ptr, q_ptr);
    } else if (creature_ptr->pclass == CLASS_NINJA) {
        q_ptr->prep(lookup_kind(TV_SPIKE, 0));
        q_ptr->number = rand_range(15, 20);
        add_outfit(creature_ptr, q_ptr);
    } else if (creature_ptr->pclass == CLASS_SNIPER) {
        q_ptr->prep(lookup_kind(TV_BOLT, SV_AMMO_NORMAL));
        q_ptr->number = rand_range(15, 20);
        add_outfit(creature_ptr, q_ptr);
    }

    if (creature_ptr->pclass != CLASS_SORCERER) {
        if (creature_ptr->pseikaku == PERSONALITY_SEXY) {
            player_init[creature_ptr->pclass][2][0] = TV_HAFTED;
            player_init[creature_ptr->pclass][2][1] = SV_WHIP;
        } else if (creature_ptr->prace == player_race_type::MERFOLK) {
            player_init[creature_ptr->pclass][2][0] = TV_POLEARM;
            player_init[creature_ptr->pclass][2][1] = SV_TRIDENT;
        }
    }

    for (int i = 0; i < 3; i++) {
        int tv = player_init[creature_ptr->pclass][i][0];
        OBJECT_SUBTYPE_VALUE sv = player_init[creature_ptr->pclass][i][1];
        if ((creature_ptr->prace == player_race_type::ANDROID) && ((tv == TV_SOFT_ARMOR) || (tv == TV_HARD_ARMOR)))
            continue;

        if (tv == TV_SORCERY_BOOK)
            tv = TV_LIFE_BOOK + creature_ptr->realm1 - 1;
        else if (tv == TV_DEATH_BOOK)
            tv = TV_LIFE_BOOK + creature_ptr->realm2 - 1;
        else if (tv == TV_RING && sv == SV_RING_RES_FEAR && creature_ptr->prace == player_race_type::BARBARIAN)
            sv = SV_RING_SUSTAIN_STR;
        else if (tv == TV_RING && sv == SV_RING_SUSTAIN_INT && creature_ptr->prace == player_race_type::MIND_FLAYER) {
            tv = TV_POTION;
            sv = SV_POTION_RESTORE_MANA;
        }

        q_ptr = &forge;
        q_ptr->prep(lookup_kind(static_cast<tval_type>(tv), sv));
        if ((tv == TV_SWORD || tv == TV_HAFTED)
            && (creature_ptr->pclass == CLASS_ROGUE && creature_ptr->realm1 == REALM_DEATH)) /* Only assassins get a poisoned weapon */
            q_ptr->name2 = EGO_BRAND_POIS;

        add_outfit(creature_ptr, q_ptr);
    }

    k_info[lookup_kind(TV_POTION, SV_POTION_WATER)].aware = true;
}
