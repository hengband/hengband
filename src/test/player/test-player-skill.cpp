/*!
 * @brief 熟練度の獲得のテスト
 *
 * player/player-skill.h の PlayerSkill の熟練度を加える処理が、能力の再計算 (StatusRecalculatingFlag::BONUS) を
 * 必要なときだけ要求することを確かめる。
 * 武器の熟練度は、実際に増えたときだけ要求する。
 * 呪文の熟練度は能力の計算に使わないので、増えても能力の再計算を要求せず、呪文の情報だけ描き直す。
 */

#include "object/tval-types.h"
#include "player-info/class-info.h"
#include "player/player-skill.h"
#include "realm/realm-types.h"
#include "system/baseitem/baseitem-key.h"
#include "system/deferred-update-requests.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "test/scoped-redrawing-flags.h"
#include "test/scoped-rng.h"
#include "util/enum-converter.h"
#include "util/finalizer.h"
#include <doctest/doctest.h>

TEST_CASE("PlayerSkill::gain_melee_weapon_exp requests recalculating the bonuses only when the proficiency increases")
{
    const auto restore_srf = test::scoped_restore_redrawing_flags<StatusRecalculatingFlag>();
    const auto restore_rng = test::scoped_rng();
    auto &dur = DeferredUpdateRequests::get_instance();
    dur.reset_flag(StatusRecalculatingFlag::BONUS);

    PlayerType player;
    ItemEntity item;
    item.bi_key = BaseitemKey(ItemKindType::SWORD, 1);
    const auto skilled = PlayerSkill::weapon_exp_at(PlayerSkillRank::SKILLED);
    player.weapon_exp[ItemKindType::SWORD].fill(skilled);
    player.weapon_exp_max[ItemKindType::SWORD].fill(PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER));
    auto &exp = player.weapon_exp[ItemKindType::SWORD][1];

    SUBCASE("does not request when no proficiency is gained")
    {
        // レベル 19 以下では、熟練 (SKILLED) から先の熟練度は増えない
        player.lev = 19;
        PlayerSkill(&player).gain_melee_weapon_exp(&item);
        CHECK(exp == skilled);
        CHECK_FALSE(dur.has(StatusRecalculatingFlag::BONUS));
    }

    SUBCASE("requests when the proficiency increases")
    {
        player.lev = 20;
        PlayerSkill(&player).gain_melee_weapon_exp(&item);
        CHECK(exp == skilled + 1);
        CHECK(player.weapon_exp[ItemKindType::SWORD][0] == skilled);
        CHECK(dur.has(StatusRecalculatingFlag::BONUS));
    }
}

TEST_CASE("PlayerSkill::gain_spell_skill_exp_over_learning requests redrawing the spell info but not recalculating the bonuses")
{
    const auto restore_srf = test::scoped_restore_redrawing_flags<StatusRecalculatingFlag>();
    const auto restore_swrf = test::scoped_restore_redrawing_flags<SubWindowRedrawingFlag>();
    auto &dur = DeferredUpdateRequests::get_instance();
    dur.reset_flag(StatusRecalculatingFlag::BONUS);
    dur.reset_flag(SubWindowRedrawingFlag::SPELL);
    dur.reset_flag(SubWindowRedrawingFlag::ITEM_KNOWLEDGE);

    PlayerType player;
    player.spell_exp[0] = PlayerSkill::spell_exp_at(PlayerSkillRank::UNSKILLED);
    CHECK(PlayerSkill(&player).gain_spell_skill_exp_over_learning(0) == PlayerSkillRank::BEGINNER);
    CHECK_FALSE(dur.has(StatusRecalculatingFlag::BONUS));
    CHECK(dur.has(SubWindowRedrawingFlag::SPELL));
    CHECK(dur.has(SubWindowRedrawingFlag::ITEM_KNOWLEDGE));
}

TEST_CASE("PlayerSkill::gain_spell_skill_exp requests redrawing the spell info only when the proficiency increases")
{
    const auto restore_srf = test::scoped_restore_redrawing_flags<StatusRecalculatingFlag>();
    const auto restore_swrf = test::scoped_restore_redrawing_flags<SubWindowRedrawingFlag>();
    const auto restore_mp_ptr = util::make_finalizer([saved = mp_ptr] { mp_ptr = saved; });
    auto &dur = DeferredUpdateRequests::get_instance();
    dur.reset_flag(StatusRecalculatingFlag::BONUS);
    dur.reset_flag(SubWindowRedrawingFlag::SPELL);
    dur.reset_flag(SubWindowRedrawingFlag::ITEM_KNOWLEDGE);

    player_magic magic;
    magic.info[enum2i(RealmType::LIFE)][0].slevel = 1;
    mp_ptr = &magic;
    FloorType floor;
    PlayerType player;
    player.current_floor_ptr = &floor;
    player.realm1 = RealmType::LIFE;
    player.lev = 10;
    auto &exp = player.spell_exp[0];

    SUBCASE("does not request when no proficiency is gained")
    {
        // 入門者 (BEGINNER) から先の熟練度は、5 階より浅いところでは増えない
        exp = PlayerSkill::spell_exp_at(PlayerSkillRank::BEGINNER);
        PlayerSkill(&player).gain_spell_skill_exp(RealmType::LIFE, 0);
        CHECK(exp == PlayerSkill::spell_exp_at(PlayerSkillRank::BEGINNER));
        CHECK_FALSE(dur.has(SubWindowRedrawingFlag::SPELL));
        CHECK_FALSE(dur.has(SubWindowRedrawingFlag::ITEM_KNOWLEDGE));
    }

    SUBCASE("requests redrawing the spell info when the proficiency increases")
    {
        exp = PlayerSkill::spell_exp_at(PlayerSkillRank::UNSKILLED);
        PlayerSkill(&player).gain_spell_skill_exp(RealmType::LIFE, 0);
        CHECK(exp > PlayerSkill::spell_exp_at(PlayerSkillRank::UNSKILLED));
        CHECK(dur.has(SubWindowRedrawingFlag::SPELL));
        CHECK(dur.has(SubWindowRedrawingFlag::ITEM_KNOWLEDGE));
    }

    CHECK_FALSE(dur.has(StatusRecalculatingFlag::BONUS));
}
