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
#include <tuple>

/*!
 * @brief 所持状態にあるアイテムの中から一部枠の装備可能なものを装備させる。
 */
void wield_all(PlayerType *player_ptr)
{
    object_type object_type_body;
    for (INVENTORY_IDX item = INVEN_PACK - 1; item >= 0; item--) {
        object_type *o_ptr;
        o_ptr = &player_ptr->inventory_list[item];
        if (!o_ptr->k_idx)
            continue;

        int slot = wield_slot(player_ptr, o_ptr);
        if (slot < INVEN_MAIN_HAND)
            continue;
        if (slot == INVEN_LITE)
            continue;
        if (player_ptr->inventory_list[slot].k_idx)
            continue;

        object_type *i_ptr;
        i_ptr = &object_type_body;
        i_ptr->copy_from(o_ptr);
        i_ptr->number = 1;

        if (item >= 0) {
            inven_item_increase(player_ptr, item, -1);
            inven_item_optimize(player_ptr, item);
        } else {
            floor_item_increase(player_ptr, 0 - item, -1);
            floor_item_optimize(player_ptr, 0 - item);
        }

        o_ptr = &player_ptr->inventory_list[slot];
        o_ptr->copy_from(i_ptr);
        player_ptr->equip_cnt++;
    }
}

/*!
 * @brief 初期所持アイテムの処理 / Add an outfit object
 * @details アイテムを既知のものとした上でwield_all()関数により装備させる。
 * @param o_ptr 処理したいオブジェクト構造体の参照ポインタ
 */
void add_outfit(PlayerType *player_ptr, object_type *o_ptr)
{
    object_aware(player_ptr, o_ptr);
    object_known(o_ptr);
    int16_t slot = store_item_to_inventory(player_ptr, o_ptr);
    autopick_alter_item(player_ptr, slot, false);
    wield_all(player_ptr);
}

static void decide_initial_items(PlayerType *player_ptr, object_type *q_ptr)
{
    switch (player_ptr->prace) {
    case PlayerRaceType::VAMPIRE:
        /* Nothing! */
        /* Vampires can drain blood of creatures */
        break;
    case PlayerRaceType::BALROG:
        /* Demon can drain vitality from humanoid corpse */
        get_mon_num_prep(player_ptr, monster_hook_human, nullptr);
        for (int i = rand_range(3, 4); i > 0; i--) {
            q_ptr->prep(lookup_kind(ItemKindType::CORPSE, SV_CORPSE));
            q_ptr->pval = get_mon_num(player_ptr, 0, 2, 0);
            if (q_ptr->pval) {
                q_ptr->number = 1;
                add_outfit(player_ptr, q_ptr);
            }
        }

        break;
    case PlayerRaceType::SKELETON:
    case PlayerRaceType::GOLEM:
    case PlayerRaceType::ZOMBIE:
    case PlayerRaceType::SPECTRE:
        /* Staff (of Nothing) */
        q_ptr->prep(lookup_kind(ItemKindType::STAFF, SV_STAFF_NOTHING));
        q_ptr->number = 1;
        add_outfit(player_ptr, q_ptr);
        break;
    case PlayerRaceType::ENT:
        /* Potions of Water */
        q_ptr->prep(lookup_kind(ItemKindType::POTION, SV_POTION_WATER));
        q_ptr->number = (ITEM_NUMBER)rand_range(15, 23);
        add_outfit(player_ptr, q_ptr);
        break;
    case PlayerRaceType::ANDROID:
        /* Flasks of oil */
        q_ptr->prep(lookup_kind(ItemKindType::FLASK, SV_ANY));
        apply_magic_to_object(player_ptr, q_ptr, 1, AM_NO_FIXED_ART);
        q_ptr->number = (ITEM_NUMBER)rand_range(7, 12);
        add_outfit(player_ptr, q_ptr);
        break;
    default:
        /* Food rations */
        q_ptr->prep(lookup_kind(ItemKindType::FOOD, SV_FOOD_RATION));
        q_ptr->number = (ITEM_NUMBER)rand_range(3, 7);
        add_outfit(player_ptr, q_ptr);
    }
}

/*!
 * @brief 種族/職業/性格などに基づき初期所持アイテムを設定するメインセット関数。 / Init players with some belongings
 * @details Having an item makes the player "aware" of its purpose.
 */
void player_outfit(PlayerType *player_ptr)
{
    object_type *q_ptr;
    object_type forge;
    q_ptr = &forge;

    decide_initial_items(player_ptr, q_ptr);
    q_ptr = &forge;

    if ((player_ptr->prace == PlayerRaceType::VAMPIRE) && (player_ptr->pclass != PlayerClassType::NINJA)) {
        q_ptr->prep(lookup_kind(ItemKindType::SCROLL, SV_SCROLL_DARKNESS));
        q_ptr->number = (ITEM_NUMBER)rand_range(2, 5);
        add_outfit(player_ptr, q_ptr);
    } else if (player_ptr->pclass != PlayerClassType::NINJA) {
        q_ptr->prep(lookup_kind(ItemKindType::LITE, SV_LITE_TORCH));
        q_ptr->number = (ITEM_NUMBER)rand_range(3, 7);
        q_ptr->xtra4 = rand_range(3, 7) * 500;

        add_outfit(player_ptr, q_ptr);
    }

    q_ptr = &forge;
    if (player_ptr->prace == PlayerRaceType::MERFOLK) {
        q_ptr->prep(lookup_kind(ItemKindType::RING, SV_RING_LEVITATION_FALL));
        q_ptr->number = 1;
        add_outfit(player_ptr, q_ptr);
    }

    if ((player_ptr->pclass == PlayerClassType::RANGER) || (player_ptr->pclass == PlayerClassType::CAVALRY)) {
        q_ptr->prep(lookup_kind(ItemKindType::ARROW, SV_AMMO_NORMAL));
        q_ptr->number = (byte)rand_range(15, 20);
        add_outfit(player_ptr, q_ptr);
    }

    if (player_ptr->pclass == PlayerClassType::RANGER) {
        q_ptr->prep(lookup_kind(ItemKindType::BOW, SV_SHORT_BOW));
        add_outfit(player_ptr, q_ptr);
    } else if (player_ptr->pclass == PlayerClassType::ARCHER) {
        q_ptr->prep(lookup_kind(ItemKindType::ARROW, SV_AMMO_NORMAL));
        q_ptr->number = (ITEM_NUMBER)rand_range(15, 20);
        add_outfit(player_ptr, q_ptr);
    } else if (player_ptr->pclass == PlayerClassType::HIGH_MAGE || player_ptr->pclass == PlayerClassType::ELEMENTALIST) {
        q_ptr->prep(lookup_kind(ItemKindType::WAND, SV_WAND_MAGIC_MISSILE));
        q_ptr->number = 1;
        q_ptr->pval = (PARAMETER_VALUE)rand_range(25, 30);
        add_outfit(player_ptr, q_ptr);
    } else if (player_ptr->pclass == PlayerClassType::SORCERER) {
        for (auto book_tval = enum2i(ItemKindType::LIFE_BOOK); book_tval <= enum2i(ItemKindType::LIFE_BOOK) + MAX_MAGIC - 1; book_tval++) {
            q_ptr->prep(lookup_kind(i2enum<ItemKindType>(book_tval), 0));
            q_ptr->number = 1;
            add_outfit(player_ptr, q_ptr);
        }
    } else if (player_ptr->pclass == PlayerClassType::TOURIST) {
        if (player_ptr->ppersonality != PERSONALITY_SEXY) {
            q_ptr->prep(lookup_kind(ItemKindType::SHOT, SV_AMMO_LIGHT));
            q_ptr->number = rand_range(15, 20);
            add_outfit(player_ptr, q_ptr);
        }

        q_ptr->prep(lookup_kind(ItemKindType::FOOD, SV_FOOD_BISCUIT));
        q_ptr->number = rand_range(2, 4);

        add_outfit(player_ptr, q_ptr);

        q_ptr->prep(lookup_kind(ItemKindType::FOOD, SV_FOOD_WAYBREAD));
        q_ptr->number = rand_range(2, 4);

        add_outfit(player_ptr, q_ptr);

        q_ptr->prep(lookup_kind(ItemKindType::FOOD, SV_FOOD_JERKY));
        q_ptr->number = rand_range(1, 3);

        add_outfit(player_ptr, q_ptr);

        q_ptr->prep(lookup_kind(ItemKindType::FOOD, SV_FOOD_PINT_OF_ALE));
        q_ptr->number = rand_range(2, 4);

        add_outfit(player_ptr, q_ptr);

        q_ptr->prep(lookup_kind(ItemKindType::FOOD, SV_FOOD_PINT_OF_WINE));
        q_ptr->number = rand_range(2, 4);

        add_outfit(player_ptr, q_ptr);
    } else if (player_ptr->pclass == PlayerClassType::NINJA) {
        q_ptr->prep(lookup_kind(ItemKindType::SPIKE, 0));
        q_ptr->number = rand_range(15, 20);
        add_outfit(player_ptr, q_ptr);
    } else if (player_ptr->pclass == PlayerClassType::SNIPER) {
        q_ptr->prep(lookup_kind(ItemKindType::BOLT, SV_AMMO_NORMAL));
        q_ptr->number = rand_range(15, 20);
        add_outfit(player_ptr, q_ptr);
    }

    // @todo 本来read-onlyであるべきプリセットテーブルを書き換えている. 良くないパターン.
    // 「状況によって特別に持たせたいアイテム」は別途定義すべき.
    if (player_ptr->pclass != PlayerClassType::SORCERER) {
        auto short_pclass = enum2i(player_ptr->pclass);
        if (player_ptr->ppersonality == PERSONALITY_SEXY) {
            player_init[short_pclass][2] = std::make_tuple(ItemKindType::HAFTED, SV_WHIP);            
        } else if (player_ptr->prace == PlayerRaceType::MERFOLK) {
            player_init[short_pclass][2] = std::make_tuple(ItemKindType::POLEARM, SV_TRIDENT);
        }
    }

    for (auto i = 0; i < 3; i++) {
        auto &[tv, sv] = player_init[enum2i(player_ptr->pclass)][i];
        if ((player_ptr->prace == PlayerRaceType::ANDROID) && ((tv == ItemKindType::SOFT_ARMOR) || (tv == ItemKindType::HARD_ARMOR)))
            continue;

        if (tv == ItemKindType::SORCERY_BOOK)
            tv = i2enum<ItemKindType>(enum2i(ItemKindType::LIFE_BOOK) + player_ptr->realm1 - 1);
        else if (tv == ItemKindType::DEATH_BOOK)
            tv = i2enum<ItemKindType>(enum2i(ItemKindType::LIFE_BOOK) + player_ptr->realm2 - 1);
        else if (tv == ItemKindType::RING && sv == SV_RING_RES_FEAR && player_ptr->prace == PlayerRaceType::BARBARIAN)
            sv = SV_RING_SUSTAIN_STR;
        else if (tv == ItemKindType::RING && sv == SV_RING_SUSTAIN_INT && player_ptr->prace == PlayerRaceType::MIND_FLAYER) {
            tv = ItemKindType::POTION;
            sv = SV_POTION_RESTORE_MANA;
        }

        q_ptr = &forge;
        q_ptr->prep(lookup_kind(tv, sv));

        /* Only assassins get a poisoned weapon */
        if (((tv == ItemKindType::SWORD) || (tv == ItemKindType::HAFTED)) && ((player_ptr->pclass == PlayerClassType::ROGUE) && (player_ptr->realm1 == REALM_DEATH))) {
            q_ptr->name2 = EGO_BRAND_POIS;
        }
        
        add_outfit(player_ptr, q_ptr);
    }

    k_info[lookup_kind(ItemKindType::POTION, SV_POTION_WATER)].aware = true;
}
