/*!
 * @brief 武器でも防具でもアクセサリでもない、その他のアイテム群を生成・強化する処理
 * @date 2022/02/23
 * @author Hourier
 * @details 他との兼ね合いでEnchanterとなっているが、油つぼ・人形・死体・像は生成のみで強化はしない
 */

#include "object-enchant/others/apply-magic-others.h"
#include "artifact/random-art-generator.h"
#include "game-option/cheat-options.h"
#include "inventory/inventory-slot-types.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object/tval-types.h"
#include "perception/object-perception.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-other-types.h"
#include "system/baseitem-info.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include <unordered_map>

/*!
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたい/生成したいオブジェクトの構造体参照ポインタ
 * @param power 生成ランク
 * @details power > 2はデバッグ専用.
 */
OtherItemsEnchanter::OtherItemsEnchanter(PlayerType *player_ptr, ItemEntity *o_ptr)
    : player_ptr(player_ptr)
    , o_ptr(o_ptr)
{
}

/*!
 * @brief その他雑多のオブジェクトに生成ランクごとの強化を与える
 * @details power > 2はデバッグ専用.
 */
void OtherItemsEnchanter::apply_magic()
{
    const auto tval = this->o_ptr->bi_key.tval();
    switch (tval) {
    case ItemKindType::FLASK:
        this->o_ptr->fuel = this->o_ptr->pval;
        this->o_ptr->pval = 0;
        break;
    case ItemKindType::WAND:
    case ItemKindType::STAFF:
        this->enchant_wand_staff();
        break;
    case ItemKindType::ROD:
        this->o_ptr->pval = this->o_ptr->get_baseitem().pval;
        break;
    case ItemKindType::CAPTURE:
        this->o_ptr->pval = 0;
        object_aware(this->player_ptr, this->o_ptr);
        this->o_ptr->mark_as_known();
        break;
    case ItemKindType::FIGURINE:
        this->generate_figurine();
        break;
    case ItemKindType::CORPSE:
        this->generate_corpse();
        break;
    case ItemKindType::STATUE:
        this->generate_statue();
        break;
    case ItemKindType::CHEST:
        this->generate_chest();
        break;
    default:
        break;
    }
}

/*
 * @brief 杖を強化する
 * The wand or staff gets a number of initial charges equal
 * to between 1/2 (+1) and the full object kind's pval.
 */
void OtherItemsEnchanter::enchant_wand_staff()
{
    const auto &baseitem = this->o_ptr->get_baseitem();
    this->o_ptr->pval = baseitem.pval / 2 + randint1((baseitem.pval + 1) / 2);
}

/*
 * @brief ランダムに選択したモンスター種族IDからその人形を作る
 * @details
 * ツチノコの人形は作らない
 * レアリティが1～100のものだけ生成対象になる
 * レベルの高い人形ほど生成されにくい
 * たまに呪われる
 */
void OtherItemsEnchanter::generate_figurine()
{
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    MonsterRaceId r_idx;
    while (true) {
        r_idx = MonsterRace::pick_one_at_random();
        if (!item_monster_okay(this->player_ptr, r_idx) || (r_idx == MonsterRaceId::TSUCHINOKO)) {
            continue;
        }

        auto *r_ptr = &monraces_info[r_idx];
        auto check = (floor_ptr->dun_level < r_ptr->level) ? (r_ptr->level - floor_ptr->dun_level) : 0;
        if ((r_ptr->rarity == 0) || (r_ptr->rarity > 100) || (randint0(check) > 0)) {
            continue;
        }

        break;
    }

    this->o_ptr->pval = enum2i(r_idx);
    if (one_in_(6)) {
        this->o_ptr->curse_flags.set(CurseTraitType::CURSED);
    }
}

/*
 * @brief ランダムに選択したモンスター種族IDからその死体/骨を作る
 * @details
 * そもそも死体も骨も落とさないモンスターは対象外
 * ユニークやあやしい影等、そこらに落ちている死体としてふさわしくないものは弾く
 * レアリティが1～100のものだけ生成対象になる (はず)
 * レベルの高い死体/骨ほど生成されにくい
 */
void OtherItemsEnchanter::generate_corpse()
{
    const std::unordered_map<int, MonsterDropType> match = {
        { SV_SKELETON, MonsterDropType::DROP_SKELETON },
        { SV_CORPSE, MonsterDropType::DROP_CORPSE },
    };

    get_mon_num_prep(this->player_ptr, item_monster_okay, nullptr);
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    MonsterRaceId r_idx;
    while (true) {
        r_idx = get_mon_num(this->player_ptr, 0, floor_ptr->dun_level, PM_NONE);
        auto &r_ref = monraces_info[r_idx];
        auto check = (floor_ptr->dun_level < r_ref.level) ? (r_ref.level - floor_ptr->dun_level) : 0;
        const auto sval = this->o_ptr->bi_key.sval();
        if (!sval) {
            continue;
        }

        if ((r_ref.rarity == 0) || (match.find(sval.value()) != match.end() && r_ref.drop_flags.has_not(match.at(sval.value()))) || (randint0(check) > 0)) {
            continue;
        }

        break;
    }

    this->o_ptr->pval = enum2i(r_idx);
    object_aware(this->player_ptr, this->o_ptr);
    this->o_ptr->mark_as_known();
}

/*
 * @brief ランダムに選択したモンスター種族IDからその像を作る
 * @details レアリティが1以上のものだけ生成対象になる
 */
void OtherItemsEnchanter::generate_statue()
{
    auto pick_r_idx_for_statue = [] {
        while (true) {
            auto r_idx = MonsterRace::pick_one_at_random();
            if (monraces_info[r_idx].rarity > 0) {
                return r_idx;
            }
        }
    };
    auto r_idx = pick_r_idx_for_statue();
    auto *r_ptr = &monraces_info[r_idx];

    this->o_ptr->pval = enum2i(r_idx);
    if (cheat_peek) {
        msg_format(_("%sの像", "Statue of %s"), r_ptr->name.data());
    }

    object_aware(this->player_ptr, this->o_ptr);
    this->o_ptr->mark_as_known();
}

/*
 * @brief 箱を生成する
 * @details 箱にはレベルがあり、箱の召喚トラップが発動すると箱レベルと同等のモンスターが召喚される
 */
void OtherItemsEnchanter::generate_chest()
{
    auto obj_level = this->o_ptr->get_baseitem().level;
    if (obj_level <= 0) {
        return;
    }

    this->o_ptr->pval = randint1(obj_level);
    if (this->o_ptr->bi_key.sval() == SV_CHEST_KANDUME) {
        this->o_ptr->pval = 6;
    }

    this->o_ptr->chest_level = this->player_ptr->current_floor_ptr->dun_level + 5;
    if (this->o_ptr->pval > 55) {
        this->o_ptr->pval = 55 + randint0(5);
    }
}
