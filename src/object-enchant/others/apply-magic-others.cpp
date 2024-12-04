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
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object/tval-types.h"
#include "perception/object-perception.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-other-types.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
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
        this->o_ptr->pval = this->o_ptr->get_baseitem_pval();
        break;
    case ItemKindType::CAPTURE:
        this->o_ptr->pval = 0;
        object_aware(this->player_ptr, *this->o_ptr);
        this->o_ptr->mark_as_known();
        break;
    case ItemKindType::FIGURINE:
        this->generate_figurine();
        break;
    case ItemKindType::MONSTER_REMAINS:
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
    const auto base_pval = this->o_ptr->get_baseitem_pval();
    this->o_ptr->pval = base_pval / 2 + randint1((base_pval + 1) / 2);
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
    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &monraces = MonraceList::get_instance();
    MonraceId monrace_id;
    while (true) {
        monrace_id = monraces.pick_id_at_random();
        if (!item_monster_okay(this->player_ptr, monrace_id) || (monrace_id == MonraceId::TSUCHINOKO)) {
            continue;
        }

        const auto &monrace = monraces.get_monrace(monrace_id);
        auto check = (floor.dun_level < monrace.level) ? (monrace.level - floor.dun_level) : 0;
        if ((monrace.rarity > 100) || (randint0(check) > 0)) {
            continue;
        }

        break;
    }

    this->o_ptr->pval = enum2i(monrace_id);
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
    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &monraces = MonraceList::get_instance();
    MonraceId monrace_id;
    while (true) {
        monrace_id = get_mon_num(this->player_ptr, 0, floor.dun_level, PM_NONE);
        const auto &monrace = monraces.get_monrace(monrace_id);
        const auto check = (floor.dun_level < monrace.level) ? (monrace.level - floor.dun_level) : 0;
        const auto sval = this->o_ptr->bi_key.sval();
        if (!sval) {
            continue;
        }

        if ((match.find(*sval) != match.end() && monrace.drop_flags.has_not(match.at(*sval))) || (randint0(check) > 0)) {
            continue;
        }

        break;
    }

    this->o_ptr->pval = enum2i(monrace_id);
    object_aware(this->player_ptr, *this->o_ptr);
    this->o_ptr->mark_as_known();
}

/*
 * @brief ランダムに選択したモンスター種族IDからその像を作る
 */
void OtherItemsEnchanter::generate_statue()
{
    const auto &monraces = MonraceList::get_instance();
    const auto pick_monrace_id_for_statue = [&monraces] {
        while (true) {
            auto &monrace = monraces.pick_monrace_at_random();
            return monrace.idx;
        }
    };
    const auto monrace_id = pick_monrace_id_for_statue();
    this->o_ptr->pval = enum2i(monrace_id);
    if (cheat_peek) {
        const auto &monrace = monraces.get_monrace(monrace_id);
        msg_format(_("%sの像", "Statue of %s"), monrace.name.data());
    }

    object_aware(this->player_ptr, *this->o_ptr);
    this->o_ptr->mark_as_known();
}

/*
 * @brief 箱を生成する
 * @details 箱にはレベルがあり、箱の召喚トラップが発動すると箱レベルと同等のモンスターが召喚される
 */
void OtherItemsEnchanter::generate_chest()
{
    const auto item_level = this->o_ptr->get_baseitem_level();
    if (item_level <= 0) {
        return;
    }

    this->o_ptr->pval = randnum1<short>(item_level);
    if (this->o_ptr->bi_key.sval() == SV_CHEST_KANDUME) {
        this->o_ptr->pval = 6;
    }

    this->o_ptr->chest_level = this->player_ptr->current_floor_ptr->dun_level + 5;
    if (this->o_ptr->pval > 55) {
        this->o_ptr->pval = 55 + randint0(5);
    }
}
