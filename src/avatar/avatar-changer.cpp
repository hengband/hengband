/*
 * @brief 敵への攻撃によって徳を変化させる処理
 * @date 2021/08/05
 * @author Hourier
 */

#include "avatar/avatar-changer.h"
#include "avatar/avatar.h"
#include "monster-race/monster-kind-mask.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief AvaterChangerコンストラクタ
 */
AvatarChanger::AvatarChanger(PlayerType *player_ptr, MonsterEntity *m_ptr)
    : player_ptr(player_ptr)
    , m_ptr(m_ptr)
{
}

/*!
 * @brief 徳変化処理のメインルーチン
 */
void AvatarChanger::change_virtue()
{
    this->change_virtue_non_beginner();
    this->change_virtue_unique();
    const auto &r_ref = this->m_ptr->get_real_monrace();
    if (m_ptr->r_idx == MonsterRaceId::BEGGAR || m_ptr->r_idx == MonsterRaceId::LEPER) {
        chg_virtue(this->player_ptr, Virtue::COMPASSION, -1);
    }

    this->change_virtue_good_evil();
    if (r_ref.kind_flags.has(MonsterKindType::UNDEAD) && r_ref.kind_flags.has(MonsterKindType::UNIQUE)) {
        chg_virtue(this->player_ptr, Virtue::VITALITY, 2);
    }

    this->change_virtue_revenge();
    if (any_bits(r_ref.flags2, RF2_MULTIPLY) && (r_ref.r_akills > 1000) && one_in_(10)) {
        chg_virtue(this->player_ptr, Virtue::VALOUR, -1);
    }

    this->change_virtue_wild_thief();
    this->change_virtue_good_animal();
}

/*!
 * @brief 非BEGINNERダンジョン時に伴う徳の変化処理
 */
void AvatarChanger::change_virtue_non_beginner()
{
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto *r_ptr = &m_ptr->get_monrace();
    if (floor_ptr->get_dungeon_definition().flags.has(DungeonFeatureType::BEGINNER)) {
        return;
    }

    if ((floor_ptr->dun_level == 0) && !this->player_ptr->ambush_flag && !floor_ptr->inside_arena) {
        chg_virtue(this->player_ptr, Virtue::VALOUR, -1);
    } else if (r_ptr->level > floor_ptr->dun_level) {
        if (randint1(10) <= (r_ptr->level - floor_ptr->dun_level)) {
            chg_virtue(this->player_ptr, Virtue::VALOUR, 1);
        }
    }

    if (r_ptr->level > 60) {
        chg_virtue(this->player_ptr, Virtue::VALOUR, 1);
    }

    if (r_ptr->level >= 2 * (this->player_ptr->lev + 1)) {
        chg_virtue(this->player_ptr, Virtue::VALOUR, 2);
    }
}

/*!
 * @brief ユニークを攻撃対象にした場合限定の徳変化処理
 */
void AvatarChanger::change_virtue_unique()
{
    auto *r_ptr = &m_ptr->get_monrace();
    if (r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return;
    }

    if (r_ptr->kind_flags.has_any_of(alignment_mask)) {
        chg_virtue(this->player_ptr, Virtue::HARMONY, 2);
    }

    if (r_ptr->kind_flags.has(MonsterKindType::GOOD)) {
        chg_virtue(this->player_ptr, Virtue::UNLIFE, 2);
        chg_virtue(this->player_ptr, Virtue::VITALITY, -2);
    }

    if (one_in_(3)) {
        chg_virtue(this->player_ptr, Virtue::INDIVIDUALISM, -1);
    }
}

/*!
 * @brief 攻撃を与えたモンスターが天使か悪魔だった場合、徳を変化させる
 * @details 天使かつ悪魔だった場合、天使であることが優先される
 */
void AvatarChanger::change_virtue_good_evil()
{
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto *r_ptr = &m_ptr->get_monrace();
    if (r_ptr->kind_flags.has(MonsterKindType::GOOD) && ((r_ptr->level) / 10 + (3 * floor_ptr->dun_level) >= randint1(100))) {
        chg_virtue(this->player_ptr, Virtue::UNLIFE, 1);
    }

    if (r_ptr->kind_flags.has(MonsterKindType::ANGEL)) {
        if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
            chg_virtue(this->player_ptr, Virtue::FAITH, -2);
        } else if ((r_ptr->level) / 10 + (3 * floor_ptr->dun_level) >= randint1(100)) {
            auto change_value = r_ptr->kind_flags.has(MonsterKindType::GOOD) ? -1 : 1;
            chg_virtue(this->player_ptr, Virtue::FAITH, change_value);
        }

        return;
    }

    if (r_ptr->kind_flags.has(MonsterKindType::DEMON)) {
        if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
            chg_virtue(this->player_ptr, Virtue::FAITH, 2);
        } else if ((r_ptr->level) / 10 + (3 * floor_ptr->dun_level) >= randint1(100)) {
            chg_virtue(this->player_ptr, Virtue::FAITH, 1);
        }
    }
}

/*!
 * @brief 過去に＠を殺したことがあるユニークにリゾンべを果たせたら徳を上げる
 */
void AvatarChanger::change_virtue_revenge()
{
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto *r_ptr = &m_ptr->get_monrace();
    if (r_ptr->r_deaths == 0) {
        return;
    }

    if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        chg_virtue(this->player_ptr, Virtue::HONOUR, 10);
        return;
    }

    if ((r_ptr->level) / 10 + (2 * floor_ptr->dun_level) >= randint1(100)) {
        chg_virtue(this->player_ptr, Virtue::HONOUR, 1);
    }
}

/*!
 * @brief 盗み逃げをするモンスター及び地上のモンスターを攻撃した際に徳を変化させる
 */
void AvatarChanger::change_virtue_wild_thief()
{
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto *r_ptr = &m_ptr->get_monrace();
    auto innocent = true;
    auto thief = false;
    for (const auto &blow : r_ptr->blows) {
        if (blow.d_dice != 0) {
            innocent = false;
        }

        if ((blow.effect == RaceBlowEffectType::EAT_ITEM) || (blow.effect == RaceBlowEffectType::EAT_GOLD)) {
            thief = true;
        }
    }

    if (r_ptr->level > 0) {
        innocent = false;
    }

    if (thief) {
        if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
            chg_virtue(this->player_ptr, Virtue::JUSTICE, 3);
            return;
        }

        if (1 + ((r_ptr->level) / 10 + (2 * floor_ptr->dun_level)) >= randint1(100)) {
            chg_virtue(this->player_ptr, Virtue::JUSTICE, 1);
        }

        return;
    }

    if (innocent) {
        chg_virtue(this->player_ptr, Virtue::JUSTICE, -1);
    }
}

/*!
 * @brief 邪悪でなく、魔法も持たない動物を攻撃した時に徳を下げる
 */
void AvatarChanger::change_virtue_good_animal()
{
    auto *r_ptr = &m_ptr->get_monrace();
    auto magic_ability_flags = r_ptr->ability_flags;
    magic_ability_flags.reset(RF_ABILITY_NOMAGIC_MASK);
    if (r_ptr->kind_flags.has_not(MonsterKindType::ANIMAL) || r_ptr->kind_flags.has(MonsterKindType::EVIL) || magic_ability_flags.any()) {
        return;
    }

    if (one_in_(4)) {
        chg_virtue(this->player_ptr, Virtue::NATURE, -1);
    }
}
