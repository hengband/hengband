/*!
 * @brief 防具系のアイテムを強化して(恐らく床に)生成する処理
 * @date 2020/06/02
 * @author Hourier
 * @todo ちょっと長い。要分割
 */

#include "object-enchant/others/apply-magic-others.h"
#include "artifact/random-art-generator.h"
#include "game-option/cheat-options.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags9.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-other-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief その他雑多のオブジェクトに生成ランクごとの強化を与えるサブルーチン
 * Apply magic to an item known to be "boring"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param power 生成ランク
 * @details power > 2はデバッグ専用.
 */
void apply_magic_others(PlayerType *player_ptr, ObjectType *o_ptr)
{
    auto *k_ptr = &k_info[o_ptr->k_idx];

    auto *floor_ptr = player_ptr->current_floor_ptr;
    switch (o_ptr->tval) {
    case ItemKindType::WHISTLE: {
        break;
    }
    case ItemKindType::FLASK: {
        o_ptr->xtra4 = o_ptr->pval;
        o_ptr->pval = 0;
        break;
    }
    case ItemKindType::WAND:
    case ItemKindType::STAFF: {
        /* The wand or staff gets a number of initial charges equal
         * to between 1/2 (+1) and the full object kind's pval. -LM-
         */
        o_ptr->pval = k_ptr->pval / 2 + randint1((k_ptr->pval + 1) / 2);
        break;
    }
    case ItemKindType::ROD: {
        o_ptr->pval = k_ptr->pval;
        break;
    }
    case ItemKindType::CAPTURE: {
        o_ptr->pval = 0;
        object_aware(player_ptr, o_ptr);
        object_known(o_ptr);
        break;
    }
    case ItemKindType::FIGURINE: {
        PARAMETER_VALUE i = 1;
        int check;
        monster_race *r_ptr;
        while (true) {
            i = randint1(r_info.size() - 1);

            if (!item_monster_okay(player_ptr, i))
                continue;
            if (i == MON_TSUCHINOKO)
                continue;

            r_ptr = &r_info[i];
            check = (floor_ptr->dun_level < r_ptr->level) ? (r_ptr->level - floor_ptr->dun_level) : 0;
            if (!r_ptr->rarity)
                continue;
            if (r_ptr->rarity > 100)
                continue;
            if (randint0(check))
                continue;

            break;
        }

        o_ptr->pval = i;
        if (one_in_(6))
            o_ptr->curse_flags.set(CurseTraitType::CURSED);

        break;
    }
    case ItemKindType::CORPSE: {
        PARAMETER_VALUE i = 1;
        int check;
        uint32_t match = 0;
        monster_race *r_ptr;
        if (o_ptr->sval == SV_SKELETON) {
            match = RF9_DROP_SKELETON;
        } else if (o_ptr->sval == SV_CORPSE) {
            match = RF9_DROP_CORPSE;
        }

        get_mon_num_prep(player_ptr, item_monster_okay, nullptr);
        while (true) {
            i = get_mon_num(player_ptr, 0, floor_ptr->dun_level, 0);
            r_ptr = &r_info[i];
            check = (floor_ptr->dun_level < r_ptr->level) ? (r_ptr->level - floor_ptr->dun_level) : 0;
            if (!r_ptr->rarity)
                continue;
            if (!(r_ptr->flags9 & match))
                continue;
            if (randint0(check))
                continue;

            break;
        }

        o_ptr->pval = i;
        object_aware(player_ptr, o_ptr);
        object_known(o_ptr);
        break;
    }
    case ItemKindType::STATUE: {
        PARAMETER_VALUE i = 1;
        monster_race *r_ptr;
        while (true) {
            i = randint1(r_info.size() - 1);
            r_ptr = &r_info[i];
            if (!r_ptr->rarity)
                continue;

            break;
        }

        o_ptr->pval = i;
        if (cheat_peek) {
            msg_format(_("%sの像", "Statue of %s"), r_ptr->name.c_str());
        }

        object_aware(player_ptr, o_ptr);
        object_known(o_ptr);
        break;
    }
    case ItemKindType::CHEST: {
        DEPTH obj_level = k_info[o_ptr->k_idx].level;
        if (obj_level <= 0)
            break;

        o_ptr->pval = randint1(obj_level);
        if (o_ptr->sval == SV_CHEST_KANDUME)
            o_ptr->pval = 6;

        o_ptr->chest_level = floor_ptr->dun_level + 5;
        if (o_ptr->pval > 55)
            o_ptr->pval = 55 + (byte)randint0(5);

        break;
    }

    default:
        break;
    }
}
