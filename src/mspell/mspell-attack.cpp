#include "mspell/mspell-attack.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
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
#include "mspell/mspell-selector.h"
#include "mspell/mspell-util.h"
#include "mspell/mspell.h"
#include "player-base/player-class.h"
#include "player-info/mane-data-type.h"
#include "player/attack-defense-types.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"
#ifdef JP
#else
#include "monster/monster-description-types.h"
#endif

static void set_no_magic_mask(msa_type *msa_ptr)
{
    if (!msa_ptr->no_inate)
        return;

    msa_ptr->ability_flags.reset(RF_ABILITY_NOMAGIC_MASK);
}

static void check_mspell_stupid(player_type *player_ptr, msa_type *msa_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    msa_ptr->in_no_magic_dungeon = d_info[player_ptr->dungeon_idx].flags.has(DF::NO_MAGIC) && floor_ptr->dun_level
        && (!floor_ptr->inside_quest || is_fixed_quest_idx(floor_ptr->inside_quest));
    if (!msa_ptr->in_no_magic_dungeon || ((msa_ptr->r_ptr->flags2 & RF2_STUPID) != 0))
        return;

    msa_ptr->ability_flags &= RF_ABILITY_NOMAGIC_MASK;
}

static void check_mspell_smart(player_type *player_ptr, msa_type *msa_ptr)
{
    if ((msa_ptr->r_ptr->flags2 & RF2_SMART) == 0)
        return;

    if ((msa_ptr->m_ptr->hp < msa_ptr->m_ptr->maxhp / 10) && (randint0(100) < 50)) {
        msa_ptr->ability_flags &= RF_ABILITY_INT_MASK;
    }

    if (msa_ptr->ability_flags.has(RF_ABILITY::TELE_LEVEL) && is_teleport_level_ineffective(player_ptr, 0)) {
        msa_ptr->ability_flags.reset(RF_ABILITY::TELE_LEVEL);
    }
}

static void check_mspell_arena(player_type *player_ptr, msa_type *msa_ptr)
{
    if (!player_ptr->current_floor_ptr->inside_arena && !player_ptr->phase_out)
        return;

    msa_ptr->ability_flags.reset(RF_ABILITY_SUMMON_MASK).reset(RF_ABILITY::TELE_LEVEL);

    if (msa_ptr->m_ptr->r_idx == MON_ROLENTO)
        msa_ptr->ability_flags.reset(RF_ABILITY::SPECIAL);
}

static bool check_mspell_non_stupid(player_type *player_ptr, msa_type *msa_ptr)
{
    if ((msa_ptr->r_ptr->flags2 & RF2_STUPID) != 0)
        return true;

    if (!player_ptr->csp)
        msa_ptr->ability_flags.reset(RF_ABILITY::DRAIN_MANA);

    if (msa_ptr->ability_flags.has_any_of(RF_ABILITY_BOLT_MASK)
        && !clean_shot(player_ptr, msa_ptr->m_ptr->fy, msa_ptr->m_ptr->fx, player_ptr->y, player_ptr->x, false)) {
        msa_ptr->ability_flags.reset(RF_ABILITY_BOLT_MASK);
    }

    if (msa_ptr->ability_flags.has_any_of(RF_ABILITY_SUMMON_MASK)
        && !(summon_possible(player_ptr, msa_ptr->y, msa_ptr->x))) {
        msa_ptr->ability_flags.reset(RF_ABILITY_SUMMON_MASK);
    }

    if (msa_ptr->ability_flags.has(RF_ABILITY::RAISE_DEAD) && !raise_possible(player_ptr, msa_ptr->m_ptr))
        msa_ptr->ability_flags.reset(RF_ABILITY::RAISE_DEAD);

    if (msa_ptr->ability_flags.has(RF_ABILITY::SPECIAL) && (msa_ptr->m_ptr->r_idx == MON_ROLENTO) && !summon_possible(player_ptr, msa_ptr->y, msa_ptr->x))
        msa_ptr->ability_flags.reset(RF_ABILITY::SPECIAL);

    return msa_ptr->ability_flags.any();
}

static void set_mspell_list(msa_type *msa_ptr)
{
    EnumClassFlagGroup<RF_ABILITY>::get_flags(msa_ptr->ability_flags, std::back_inserter(msa_ptr->mspells));
}

static void describe_mspell_monster(player_type *player_ptr, msa_type *msa_ptr)
{
    monster_desc(player_ptr, msa_ptr->m_name, msa_ptr->m_ptr, 0x00);

#ifdef JP
#else
    /* Get the monster possessive ("his"/"her"/"its") */
    char m_poss[80];
    monster_desc(player_ptr, m_poss, msa_ptr->m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);
#endif
}

static bool switch_do_spell(player_type *player_ptr, msa_type *msa_ptr)
{
    switch (msa_ptr->do_spell) {
    case DO_SPELL_NONE: {
        int attempt = 10;
        while (attempt--) {
            msa_ptr->thrown_spell = choose_attack_spell(player_ptr, msa_ptr);
            if (msa_ptr->thrown_spell != RF_ABILITY::MAX)
                break;
        }

        return true;
    }
    case DO_SPELL_BR_LITE:
        msa_ptr->thrown_spell = RF_ABILITY::BR_LITE;
        return true;
    case DO_SPELL_BR_DISI:
        msa_ptr->thrown_spell = RF_ABILITY::BR_DISI;
        return true;
    case DO_SPELL_BA_LITE:
        msa_ptr->thrown_spell = RF_ABILITY::BA_LITE;
        return true;
    default:
        return false;
    }
}

static bool check_mspell_continuation(player_type *player_ptr, msa_type *msa_ptr)
{
    if (msa_ptr->ability_flags.none())
        return false;

    remove_bad_spells(msa_ptr->m_idx, player_ptr, msa_ptr->ability_flags);
    check_mspell_arena(player_ptr, msa_ptr);
    if (msa_ptr->ability_flags.none() || !check_mspell_non_stupid(player_ptr, msa_ptr))
        return false;

    set_mspell_list(msa_ptr);
    if (msa_ptr->mspells.empty() || !player_ptr->playing || player_ptr->is_dead || player_ptr->leaving)
        return false;

    describe_mspell_monster(player_ptr, msa_ptr);
    if (!switch_do_spell(player_ptr, msa_ptr) || (msa_ptr->thrown_spell == RF_ABILITY::MAX))
        return false;

    return true;
}

static bool check_mspell_unexploded(player_type *player_ptr, msa_type *msa_ptr)
{
    PERCENTAGE fail_rate = 25 - (msa_ptr->rlev + 3) / 4;
    if (msa_ptr->r_ptr->flags2 & RF2_STUPID)
        fail_rate = 0;

    if (!spell_is_inate(msa_ptr->thrown_spell)
        && (msa_ptr->in_no_magic_dungeon || (monster_stunned_remaining(msa_ptr->m_ptr) && one_in_(2)) || (randint0(100) < fail_rate))) {
        disturb(player_ptr, true, true);
        msg_format(_("%^sは呪文を唱えようとしたが失敗した。", "%^s tries to cast a spell, but fails."), msa_ptr->m_name);
        return true;
    }

    if (!spell_is_inate(msa_ptr->thrown_spell) && SpellHex(player_ptr).check_hex_barrier(msa_ptr->m_idx, HEX_ANTI_MAGIC)) {
        msg_format(_("反魔法バリアが%^sの呪文をかき消した。", "Anti magic barrier cancels the spell which %^s casts."), msa_ptr->m_name);
        return true;
    }

    return false;
}

/*!
 * @brief モンスターが使おうとする特技がプレイヤーに届くかどうかを返す
 *
 * ターゲット (msa_ptr->y, msa_ptr->x) は設定済みとする。
 */
static bool check_thrown_mspell(player_type *player_ptr, msa_type *msa_ptr)
{
    // プレイヤーがモンスターを正しく視認できていれば思い出に残る。
    // FIXME: ここで処理するのはおかしいような?
    msa_ptr->can_remember = is_original_ap_and_seen(player_ptr, msa_ptr->m_ptr);

    // ターゲットがプレイヤー位置なら直接射線が通っているので常に届く。
    bool direct = player_bold(player_ptr, msa_ptr->y, msa_ptr->x);
    if (direct)
        return true;

    // ターゲットがプレイヤー位置からずれているとき、直接の射線を必要とする特技
    // (ボルト系など)は届かないものとみなす。
    switch (msa_ptr->thrown_spell) {
    case RF_ABILITY::DISPEL:
    case RF_ABILITY::SHOOT:
    case RF_ABILITY::DRAIN_MANA:
    case RF_ABILITY::MIND_BLAST:
    case RF_ABILITY::BRAIN_SMASH:
    case RF_ABILITY::CAUSE_1:
    case RF_ABILITY::CAUSE_2:
    case RF_ABILITY::CAUSE_3:
    case RF_ABILITY::CAUSE_4:
    case RF_ABILITY::BO_ACID:
    case RF_ABILITY::BO_ELEC:
    case RF_ABILITY::BO_FIRE:
    case RF_ABILITY::BO_COLD:
    case RF_ABILITY::BO_NETH:
    case RF_ABILITY::BO_WATE:
    case RF_ABILITY::BO_MANA:
    case RF_ABILITY::BO_PLAS:
    case RF_ABILITY::BO_ICEE:
    case RF_ABILITY::MISSILE:
    case RF_ABILITY::SCARE:
    case RF_ABILITY::BLIND:
    case RF_ABILITY::CONF:
    case RF_ABILITY::SLOW:
    case RF_ABILITY::HOLD:
    case RF_ABILITY::HAND_DOOM:
    case RF_ABILITY::TELE_TO:
    case RF_ABILITY::TELE_AWAY:
    case RF_ABILITY::TELE_LEVEL:
    case RF_ABILITY::PSY_SPEAR:
    case RF_ABILITY::DARKNESS:
    case RF_ABILITY::FORGET:
        return false;
    default:
        return true;
    }
}

static void check_mspell_imitation(player_type *player_ptr, msa_type *msa_ptr)
{
    bool seen = (!player_ptr->blind && msa_ptr->m_ptr->ml);
    bool can_imitate = player_has_los_bold(player_ptr, msa_ptr->m_ptr->fy, msa_ptr->m_ptr->fx);
    if (!seen || !can_imitate || (w_ptr->timewalk_m_idx != 0) || (player_ptr->pclass != CLASS_IMITATOR))
        return;

    /* Not RF_ABILITY::SPECIAL */
    if (msa_ptr->thrown_spell == RF_ABILITY::SPECIAL)
        return;

    auto mane_data = PlayerClass(player_ptr).get_specific_data<mane_data_type>();

    if (mane_data->mane_list.size() == MAX_MANE) {
        mane_data->mane_list.pop_front();
    }

    mane_data->mane_list.push_back({ msa_ptr->thrown_spell, msa_ptr->dam });
    mane_data->new_mane = true;
    player_ptr->redraw |= PR_IMITATION;
}

static void remember_mspell(msa_type *msa_ptr)
{
    if (!msa_ptr->can_remember)
        return;

    msa_ptr->r_ptr->r_ability_flags.set(msa_ptr->thrown_spell);
    if (msa_ptr->r_ptr->r_cast_spell < MAX_UCHAR)
        msa_ptr->r_ptr->r_cast_spell++;
}

/*!
 * @brief モンスターの特殊技能メインルーチン /
 * Creatures can cast spells, shoot missiles, and breathe.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスター構造体配列のID
 * @return 実際に特殊技能を利用したらTRUEを返す
 */
bool make_attack_spell(player_type *player_ptr, MONSTER_IDX m_idx)
{
    msa_type tmp_msa;
    msa_type *msa_ptr = initialize_msa_type(player_ptr, &tmp_msa, m_idx);
    if (monster_confused_remaining(msa_ptr->m_ptr)) {
        reset_target(msa_ptr->m_ptr);
        return false;
    }

    if (msa_ptr->m_ptr->mflag.has(MFLAG::PREVENT_MAGIC) || !is_hostile(msa_ptr->m_ptr)
        || ((msa_ptr->m_ptr->cdis > get_max_range(player_ptr)) && !msa_ptr->m_ptr->target_y))
        return false;

    decide_lite_range(player_ptr, msa_ptr);
    if (!decide_lite_projection(player_ptr, msa_ptr))
        return false;

    reset_target(msa_ptr->m_ptr);
    msa_ptr->rlev = ((msa_ptr->r_ptr->level >= 1) ? msa_ptr->r_ptr->level : 1);
    set_no_magic_mask(msa_ptr);
    decide_lite_area(player_ptr, msa_ptr);
    check_mspell_stupid(player_ptr, msa_ptr);
    check_mspell_smart(player_ptr, msa_ptr);
    if (!check_mspell_continuation(player_ptr, msa_ptr))
        return false;

    if (check_mspell_unexploded(player_ptr, msa_ptr))
        return true;

    // 特技がプレイヤーに届かないなら使わない。
    if (!check_thrown_mspell(player_ptr, msa_ptr))
        return false;

    // 特技を使う。
    const auto monspell_res = monspell_to_player(player_ptr, msa_ptr->thrown_spell, msa_ptr->y, msa_ptr->x, m_idx);
    if (!monspell_res.valid)
        return false;

    msa_ptr->dam = monspell_res.dam;
    check_mspell_imitation(player_ptr, msa_ptr);
    remember_mspell(msa_ptr);
    if (player_ptr->is_dead && (msa_ptr->r_ptr->r_deaths < MAX_SHORT) && !player_ptr->current_floor_ptr->inside_arena)
        msa_ptr->r_ptr->r_deaths++;

    return true;
}
