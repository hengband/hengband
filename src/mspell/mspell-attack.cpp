#include "mspell/mspell-attack.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/disturbance.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "monster-floor/monster-move.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "mspell/assign-monster-spell.h"
#include "mspell/improper-mspell-remover.h"
#include "mspell/mspell-attack-util.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-learn-checker.h"
#include "mspell/mspell-lite.h"
#include "mspell/mspell-result.h"
#include "mspell/mspell-selector.h"
#include "mspell/mspell-util.h"
#include "player-base/player-class.h"
#include "player-info/mane-data-type.h"
#include "player/attack-defense-types.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/timed-effects.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <iterator>
#ifdef JP
#else
#include "monster/monster-description-types.h"
#endif

static void set_no_magic_mask(msa_type *msa_ptr)
{
    if (!msa_ptr->no_inate) {
        return;
    }

    msa_ptr->ability_flags.reset(RF_ABILITY_NOMAGIC_MASK);
}

static void check_mspell_stupid(PlayerType *player_ptr, msa_type *msa_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto is_in_no_magic_dungeon = floor_ptr->get_dungeon_definition().flags.has(DungeonFeatureType::NO_MAGIC);
    is_in_no_magic_dungeon &= floor_ptr->is_in_dungeon();
    is_in_no_magic_dungeon &= !floor_ptr->is_in_quest() || QuestType::is_fixed(floor_ptr->quest_number);
    msa_ptr->in_no_magic_dungeon = is_in_no_magic_dungeon;
    if (!msa_ptr->in_no_magic_dungeon || (msa_ptr->r_ptr->behavior_flags.has(MonsterBehaviorType::STUPID))) {
        return;
    }

    msa_ptr->ability_flags &= RF_ABILITY_NOMAGIC_MASK;
}

static void check_mspell_smart(PlayerType *player_ptr, msa_type *msa_ptr)
{
    if (msa_ptr->r_ptr->behavior_flags.has_not(MonsterBehaviorType::SMART)) {
        return;
    }

    if ((msa_ptr->m_ptr->hp < msa_ptr->m_ptr->maxhp / 10) && (randint0(100) < 50)) {
        msa_ptr->ability_flags &= RF_ABILITY_INT_MASK;
    }

    if (msa_ptr->ability_flags.has(MonsterAbilityType::TELE_LEVEL) && is_teleport_level_ineffective(player_ptr, 0)) {
        msa_ptr->ability_flags.reset(MonsterAbilityType::TELE_LEVEL);
    }
}

static void check_mspell_arena(PlayerType *player_ptr, msa_type *msa_ptr)
{
    if (!player_ptr->current_floor_ptr->inside_arena && !player_ptr->phase_out) {
        return;
    }

    msa_ptr->ability_flags.reset(RF_ABILITY_SUMMON_MASK).reset(MonsterAbilityType::TELE_LEVEL);

    if (msa_ptr->m_ptr->r_idx == MonsterRaceId::ROLENTO) {
        msa_ptr->ability_flags.reset(MonsterAbilityType::SPECIAL);
    }
}

static bool check_mspell_non_stupid(PlayerType *player_ptr, msa_type *msa_ptr)
{
    if (msa_ptr->r_ptr->behavior_flags.has(MonsterBehaviorType::STUPID)) {
        return true;
    }

    if (!player_ptr->csp) {
        msa_ptr->ability_flags.reset(MonsterAbilityType::DRAIN_MANA);
    }

    if (msa_ptr->ability_flags.has_any_of(RF_ABILITY_BOLT_MASK) &&
        !clean_shot(player_ptr, msa_ptr->m_ptr->fy, msa_ptr->m_ptr->fx, player_ptr->y, player_ptr->x, false)) {
        msa_ptr->ability_flags.reset(RF_ABILITY_BOLT_MASK);
    }

    if (msa_ptr->ability_flags.has_any_of(RF_ABILITY_SUMMON_MASK) && !(summon_possible(player_ptr, msa_ptr->y, msa_ptr->x))) {
        msa_ptr->ability_flags.reset(RF_ABILITY_SUMMON_MASK);
    }

    if (msa_ptr->ability_flags.has(MonsterAbilityType::RAISE_DEAD) && !raise_possible(player_ptr, msa_ptr->m_ptr)) {
        msa_ptr->ability_flags.reset(MonsterAbilityType::RAISE_DEAD);
    }

    if (msa_ptr->ability_flags.has(MonsterAbilityType::SPECIAL) && (msa_ptr->m_ptr->r_idx == MonsterRaceId::ROLENTO) &&
        !summon_possible(player_ptr, msa_ptr->y, msa_ptr->x)) {
        msa_ptr->ability_flags.reset(MonsterAbilityType::SPECIAL);
    }

    return msa_ptr->ability_flags.any();
}

static void set_mspell_list(msa_type *msa_ptr)
{
    EnumClassFlagGroup<MonsterAbilityType>::get_flags(msa_ptr->ability_flags, std::back_inserter(msa_ptr->mspells));
}

static bool switch_do_spell(PlayerType *player_ptr, msa_type *msa_ptr)
{
    switch (msa_ptr->do_spell) {
    case DO_SPELL_NONE: {
        int attempt = 10;
        while (attempt--) {
            msa_ptr->thrown_spell = choose_attack_spell(player_ptr, msa_ptr);
            if (msa_ptr->thrown_spell != MonsterAbilityType::MAX) {
                break;
            }
        }

        return true;
    }
    case DO_SPELL_BR_LITE:
        msa_ptr->thrown_spell = MonsterAbilityType::BR_LITE;
        return true;
    case DO_SPELL_BR_DISI:
        msa_ptr->thrown_spell = MonsterAbilityType::BR_DISI;
        return true;
    case DO_SPELL_BA_LITE:
        msa_ptr->thrown_spell = MonsterAbilityType::BA_LITE;
        return true;
    default:
        return false;
    }
}

static bool check_mspell_continuation(PlayerType *player_ptr, msa_type *msa_ptr)
{
    if (msa_ptr->ability_flags.none()) {
        return false;
    }

    remove_bad_spells(msa_ptr->m_idx, player_ptr, msa_ptr->ability_flags);
    check_mspell_arena(player_ptr, msa_ptr);
    if (msa_ptr->ability_flags.none() || !check_mspell_non_stupid(player_ptr, msa_ptr)) {
        return false;
    }

    set_mspell_list(msa_ptr);
    if (msa_ptr->mspells.empty() || !player_ptr->playing || player_ptr->is_dead || player_ptr->leaving) {
        return false;
    }

    msa_ptr->m_name = monster_desc(player_ptr, msa_ptr->m_ptr, 0x00);
    if (!switch_do_spell(player_ptr, msa_ptr) || (msa_ptr->thrown_spell == MonsterAbilityType::MAX)) {
        return false;
    }

    return true;
}

static bool check_mspell_unexploded(PlayerType *player_ptr, msa_type *msa_ptr)
{
    PERCENTAGE fail_rate = 25 - (msa_ptr->rlev + 3) / 4;
    if (msa_ptr->r_ptr->behavior_flags.has(MonsterBehaviorType::STUPID)) {
        fail_rate = 0;
    }

    if (!spell_is_inate(msa_ptr->thrown_spell) && (msa_ptr->in_no_magic_dungeon || (msa_ptr->m_ptr->get_remaining_stun() && one_in_(2)) || (randint0(100) < fail_rate))) {
        disturb(player_ptr, true, true);
        msg_format(_("%s^は呪文を唱えようとしたが失敗した。", "%s^ tries to cast a spell, but fails."), msa_ptr->m_name.data());
        return true;
    }

    if (!spell_is_inate(msa_ptr->thrown_spell) && SpellHex(player_ptr).check_hex_barrier(msa_ptr->m_idx, HEX_ANTI_MAGIC)) {
        msg_format(_("反魔法バリアが%s^の呪文をかき消した。", "Anti magic barrier cancels the spell which %s^ casts."), msa_ptr->m_name.data());
        return true;
    }

    return false;
}

/*!
 * @brief モンスターが使おうとする特技がプレイヤーに届くかどうかを返す
 *
 * ターゲット (msa_ptr->y, msa_ptr->x) は設定済みとする。
 */
static bool check_thrown_mspell(PlayerType *player_ptr, msa_type *msa_ptr)
{
    // プレイヤーがモンスターを正しく視認できていれば思い出に残る。
    // FIXME: ここで処理するのはおかしいような?
    msa_ptr->can_remember = is_original_ap_and_seen(player_ptr, msa_ptr->m_ptr);

    // ターゲットがプレイヤー位置なら直接射線が通っているので常に届く。
    bool direct = player_bold(player_ptr, msa_ptr->y, msa_ptr->x);
    if (direct) {
        return true;
    }

    // ターゲットがプレイヤー位置からずれているとき、直接の射線を必要とする特技
    // (ボルト系など)は届かないものとみなす。
    switch (msa_ptr->thrown_spell) {
    case MonsterAbilityType::DISPEL:
    case MonsterAbilityType::SHOOT:
    case MonsterAbilityType::DRAIN_MANA:
    case MonsterAbilityType::MIND_BLAST:
    case MonsterAbilityType::BRAIN_SMASH:
    case MonsterAbilityType::CAUSE_1:
    case MonsterAbilityType::CAUSE_2:
    case MonsterAbilityType::CAUSE_3:
    case MonsterAbilityType::CAUSE_4:
    case MonsterAbilityType::BO_ACID:
    case MonsterAbilityType::BO_ELEC:
    case MonsterAbilityType::BO_FIRE:
    case MonsterAbilityType::BO_COLD:
    case MonsterAbilityType::BO_NETH:
    case MonsterAbilityType::BO_WATE:
    case MonsterAbilityType::BO_MANA:
    case MonsterAbilityType::BO_PLAS:
    case MonsterAbilityType::BO_ICEE:
    case MonsterAbilityType::BO_VOID:
    case MonsterAbilityType::BO_ABYSS:
    case MonsterAbilityType::MISSILE:
    case MonsterAbilityType::SCARE:
    case MonsterAbilityType::BLIND:
    case MonsterAbilityType::CONF:
    case MonsterAbilityType::SLOW:
    case MonsterAbilityType::HOLD:
    case MonsterAbilityType::HAND_DOOM:
    case MonsterAbilityType::TELE_TO:
    case MonsterAbilityType::TELE_AWAY:
    case MonsterAbilityType::TELE_LEVEL:
    case MonsterAbilityType::PSY_SPEAR:
    case MonsterAbilityType::DARKNESS:
    case MonsterAbilityType::FORGET:
        return false;
    default:
        return true;
    }
}

static void check_mspell_imitation(PlayerType *player_ptr, msa_type *msa_ptr)
{
    const auto seen = (!player_ptr->effects()->blindness()->is_blind() && msa_ptr->m_ptr->ml);
    const auto can_imitate = player_has_los_bold(player_ptr, msa_ptr->m_ptr->fy, msa_ptr->m_ptr->fx);
    PlayerClass pc(player_ptr);
    if (!seen || !can_imitate || (w_ptr->timewalk_m_idx != 0) || !pc.equals(PlayerClassType::IMITATOR)) {
        return;
    }

    /* Not RF_ABILITY::SPECIAL */
    if (msa_ptr->thrown_spell == MonsterAbilityType::SPECIAL) {
        return;
    }

    auto mane_data = pc.get_specific_data<mane_data_type>();

    if (mane_data->mane_list.size() == MAX_MANE) {
        mane_data->mane_list.pop_front();
    }

    mane_data->mane_list.push_back({ msa_ptr->thrown_spell, msa_ptr->dam });
    mane_data->new_mane = true;
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::IMITATION);
}

static void remember_mspell(msa_type *msa_ptr)
{
    if (!msa_ptr->can_remember) {
        return;
    }

    msa_ptr->r_ptr->r_ability_flags.set(msa_ptr->thrown_spell);
    if (msa_ptr->r_ptr->r_cast_spell < MAX_UCHAR) {
        msa_ptr->r_ptr->r_cast_spell++;
    }
}

/*!
 * @brief モンスターの特殊技能メインルーチン /
 * Creatures can cast spells, shoot missiles, and breathe.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスター構造体配列のID
 * @return 実際に特殊技能を利用したらTRUEを返す
 */
bool make_attack_spell(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    msa_type tmp_msa(player_ptr, m_idx);
    msa_type *msa_ptr = &tmp_msa;
    if (msa_ptr->m_ptr->is_confused()) {
        reset_target(msa_ptr->m_ptr);
        return false;
    }

    const auto &m_ref = *msa_ptr->m_ptr;
    if (m_ref.mflag.has(MonsterTemporaryFlagType::PREVENT_MAGIC) || !m_ref.is_hostile() || ((m_ref.cdis > get_max_range(player_ptr)) && !m_ref.target_y)) {
        return false;
    }

    decide_lite_range(player_ptr, msa_ptr);
    if (!decide_lite_projection(player_ptr, msa_ptr)) {
        return false;
    }

    reset_target(msa_ptr->m_ptr);
    msa_ptr->rlev = ((msa_ptr->r_ptr->level >= 1) ? msa_ptr->r_ptr->level : 1);
    set_no_magic_mask(msa_ptr);
    decide_lite_area(player_ptr, msa_ptr);
    check_mspell_stupid(player_ptr, msa_ptr);
    check_mspell_smart(player_ptr, msa_ptr);
    if (!check_mspell_continuation(player_ptr, msa_ptr)) {
        return false;
    }

    if (check_mspell_unexploded(player_ptr, msa_ptr)) {
        return true;
    }

    // 特技がプレイヤーに届かないなら使わない。
    if (!check_thrown_mspell(player_ptr, msa_ptr)) {
        return false;
    }

    // 特技を使う。
    const auto monspell_res = monspell_to_player(player_ptr, msa_ptr->thrown_spell, msa_ptr->y, msa_ptr->x, m_idx);
    if (!monspell_res.valid) {
        return false;
    }

    msa_ptr->dam = monspell_res.dam;
    check_mspell_imitation(player_ptr, msa_ptr);
    remember_mspell(msa_ptr);
    if (player_ptr->is_dead && (msa_ptr->r_ptr->r_deaths < MAX_SHORT) && !player_ptr->current_floor_ptr->inside_arena) {
        msa_ptr->r_ptr->r_deaths++;
    }

    return true;
}
