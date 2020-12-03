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
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags4.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "mspell/assign-monster-spell.h"
#include "mspell/improper-mspell-remover.h"
#include "mspell/mspell-attack-util.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-lite.h"
#include "mspell/mspell-mask-definitions.h"
#include "mspell/mspell-selector.h"
#include "mspell/mspell-util.h"
#include "player/attack-defense-types.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "system/floor-type-definition.h"
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

    msa_ptr->f4 &= ~(RF4_NOMAGIC_MASK);
    msa_ptr->f5 &= ~(RF5_NOMAGIC_MASK);
    msa_ptr->f6 &= ~(RF6_NOMAGIC_MASK);
}

static void check_mspell_stupid(player_type *target_ptr, msa_type *msa_ptr)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    msa_ptr->in_no_magic_dungeon = (d_info[target_ptr->dungeon_idx].flags1 & DF1_NO_MAGIC) && floor_ptr->dun_level
        && (!floor_ptr->inside_quest || is_fixed_quest_idx(floor_ptr->inside_quest));
    if (!msa_ptr->in_no_magic_dungeon || ((msa_ptr->r_ptr->flags2 & RF2_STUPID) != 0))
        return;

    msa_ptr->f4 &= RF4_NOMAGIC_MASK;
    msa_ptr->f5 &= RF5_NOMAGIC_MASK;
    msa_ptr->f6 &= RF6_NOMAGIC_MASK;
}

static void check_mspell_smart(player_type *target_ptr, msa_type *msa_ptr)
{
    if ((msa_ptr->r_ptr->flags2 & RF2_SMART) == 0)
        return;

    if ((msa_ptr->m_ptr->hp < msa_ptr->m_ptr->maxhp / 10) && (randint0(100) < 50)) {
        msa_ptr->f4 &= (RF4_INT_MASK);
        msa_ptr->f5 &= (RF5_INT_MASK);
        msa_ptr->f6 &= (RF6_INT_MASK);
    }

    if ((msa_ptr->f6 & RF6_TELE_LEVEL) && is_teleport_level_ineffective(target_ptr, 0)) {
        msa_ptr->f6 &= ~(RF6_TELE_LEVEL);
    }
}

static void check_mspell_arena(player_type *target_ptr, msa_type *msa_ptr)
{
    if (!target_ptr->current_floor_ptr->inside_arena && !target_ptr->phase_out)
        return;

    msa_ptr->f4 &= ~(RF4_SUMMON_MASK);
    msa_ptr->f5 &= ~(RF5_SUMMON_MASK);
    msa_ptr->f6 &= ~(RF6_SUMMON_MASK | RF6_TELE_LEVEL);

    if (msa_ptr->m_ptr->r_idx == MON_ROLENTO)
        msa_ptr->f6 &= ~(RF6_SPECIAL);
}

static bool check_mspell_non_stupid(player_type *target_ptr, msa_type *msa_ptr)
{
    if ((msa_ptr->r_ptr->flags2 & RF2_STUPID) != 0)
        return TRUE;

    if (!target_ptr->csp)
        msa_ptr->f5 &= ~(RF5_DRAIN_MANA);

    if (((msa_ptr->f4 & RF4_BOLT_MASK) || (msa_ptr->f5 & RF5_BOLT_MASK) || (msa_ptr->f6 & RF6_BOLT_MASK))
        && !clean_shot(target_ptr, msa_ptr->m_ptr->fy, msa_ptr->m_ptr->fx, target_ptr->y, target_ptr->x, FALSE)) {
        msa_ptr->f4 &= ~(RF4_BOLT_MASK);
        msa_ptr->f5 &= ~(RF5_BOLT_MASK);
        msa_ptr->f6 &= ~(RF6_BOLT_MASK);
    }

    if (((msa_ptr->f4 & RF4_SUMMON_MASK) || (msa_ptr->f5 & RF5_SUMMON_MASK) || (msa_ptr->f6 & RF6_SUMMON_MASK))
        && !(summon_possible(target_ptr, msa_ptr->y, msa_ptr->x))) {
        msa_ptr->f4 &= ~(RF4_SUMMON_MASK);
        msa_ptr->f5 &= ~(RF5_SUMMON_MASK);
        msa_ptr->f6 &= ~(RF6_SUMMON_MASK);
    }

    if ((msa_ptr->f6 & RF6_RAISE_DEAD) && !raise_possible(target_ptr, msa_ptr->m_ptr))
        msa_ptr->f6 &= ~(RF6_RAISE_DEAD);

    if (((msa_ptr->f6 & RF6_SPECIAL) != 0) && (msa_ptr->m_ptr->r_idx == MON_ROLENTO) && !summon_possible(target_ptr, msa_ptr->y, msa_ptr->x))
        msa_ptr->f6 &= ~(RF6_SPECIAL);

    return (msa_ptr->f4 != 0) || (msa_ptr->f5 != 0) || (msa_ptr->f6 != 0);
}

static void set_mspell_list(msa_type *msa_ptr)
{
    for (int k = 0; k < 32; k++)
        if (msa_ptr->f4 & (1L << k))
            msa_ptr->mspells[msa_ptr->num++] = k + RF4_SPELL_START;

    for (int k = 0; k < 32; k++)
        if (msa_ptr->f5 & (1L << k))
            msa_ptr->mspells[msa_ptr->num++] = k + RF5_SPELL_START;

    for (int k = 0; k < 32; k++)
        if (msa_ptr->f6 & (1L << k))
            msa_ptr->mspells[msa_ptr->num++] = k + RF6_SPELL_START;
}

static void describe_mspell_monster(player_type *target_ptr, msa_type *msa_ptr)
{
    monster_desc(target_ptr, msa_ptr->m_name, msa_ptr->m_ptr, 0x00);

#ifdef JP
#else
    /* Get the monster possessive ("his"/"her"/"its") */
    char m_poss[80];
    monster_desc(target_ptr, m_poss, msa_ptr->m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);
#endif
}

static bool switch_do_spell(player_type *target_ptr, msa_type *msa_ptr)
{
    switch (msa_ptr->do_spell) {
    case DO_SPELL_NONE: {
        int attempt = 10;
        while (attempt--) {
            msa_ptr->thrown_spell = choose_attack_spell(target_ptr, msa_ptr);
            if (msa_ptr->thrown_spell)
                break;
        }

        return TRUE;
    }
    case DO_SPELL_BR_LITE:
        msa_ptr->thrown_spell = 96 + 14; /* RF4_BR_LITE */
        return TRUE;
    case DO_SPELL_BR_DISI:
        msa_ptr->thrown_spell = 96 + 31; /* RF4_BR_DISI */
        return TRUE;
    case DO_SPELL_BA_LITE:
        msa_ptr->thrown_spell = 128 + 20; /* RF5_BA_LITE */
        return TRUE;
    default:
        return FALSE;
    }
}

static bool check_mspell_continuation(player_type *target_ptr, msa_type *msa_ptr)
{
    if ((msa_ptr->f4 == 0) && (msa_ptr->f5 == 0) && (msa_ptr->f6 == 0))
        return FALSE;

    remove_bad_spells(msa_ptr->m_idx, target_ptr, &msa_ptr->f4, &msa_ptr->f5, &msa_ptr->f6);
    check_mspell_arena(target_ptr, msa_ptr);
    if (((msa_ptr->f4 == 0) && (msa_ptr->f5 == 0) && (msa_ptr->f6 == 0)) || !check_mspell_non_stupid(target_ptr, msa_ptr))
        return FALSE;

    set_mspell_list(msa_ptr);
    if ((msa_ptr->num == 0) || !target_ptr->playing || target_ptr->is_dead || target_ptr->leaving)
        return FALSE;

    describe_mspell_monster(target_ptr, msa_ptr);
    if (!switch_do_spell(target_ptr, msa_ptr) || (msa_ptr->thrown_spell == 0))
        return FALSE;

    return TRUE;
}

static bool check_mspell_unexploded(player_type *target_ptr, msa_type *msa_ptr)
{
    PERCENTAGE fail_rate = 25 - (msa_ptr->rlev + 3) / 4;
    if (msa_ptr->r_ptr->flags2 & RF2_STUPID)
        fail_rate = 0;

    if (!spell_is_inate(msa_ptr->thrown_spell)
        && (msa_ptr->in_no_magic_dungeon || (monster_stunned_remaining(msa_ptr->m_ptr) && one_in_(2)) || (randint0(100) < fail_rate))) {
        disturb(target_ptr, TRUE, TRUE);
        msg_format(_("%^sは呪文を唱えようとしたが失敗した。", "%^s tries to cast a spell, but fails."), msa_ptr->m_name);
        return TRUE;
    }

    if (!spell_is_inate(msa_ptr->thrown_spell) && magic_barrier(target_ptr, msa_ptr->m_idx)) {
        msg_format(_("反魔法バリアが%^sの呪文をかき消した。", "Anti magic barrier cancels the spell which %^s casts."), msa_ptr->m_name);
        return TRUE;
    }

    return FALSE;
}

static bool check_thrown_mspell(player_type *target_ptr, msa_type *msa_ptr)
{
    bool direct = player_bold(target_ptr, msa_ptr->y, msa_ptr->x);
    msa_ptr->can_remember = is_original_ap_and_seen(target_ptr, msa_ptr->m_ptr);
    if (direct)
        return TRUE;

    switch (msa_ptr->thrown_spell) {
    case 96 + 2: /* RF4_DISPEL */
    case 96 + 4: /* RF4_SHOOT */
    case 128 + 9: /* RF5_DRAIN_MANA */
    case 128 + 10: /* RF5_MIND_BLAST */
    case 128 + 11: /* RF5_BRAIN_SMASH */
    case 128 + 12: /* RF5_CAUSE_1 */
    case 128 + 13: /* RF5_CAUSE_2 */
    case 128 + 14: /* RF5_CAUSE_3 */
    case 128 + 15: /* RF5_CAUSE_4 */
    case 128 + 16: /* RF5_BO_ACID */
    case 128 + 17: /* RF5_BO_ELEC */
    case 128 + 18: /* RF5_BO_FIRE */
    case 128 + 19: /* RF5_BO_COLD */
    case 128 + 21: /* RF5_BO_NETH */
    case 128 + 22: /* RF5_BO_WATE */
    case 128 + 23: /* RF5_BO_MANA */
    case 128 + 24: /* RF5_BO_PLAS */
    case 128 + 25: /* RF5_BO_ICEE */
    case 128 + 26: /* RF5_MISSILE */
    case 128 + 27: /* RF5_SCARE */
    case 128 + 28: /* RF5_BLIND */
    case 128 + 29: /* RF5_CONF */
    case 128 + 30: /* RF5_SLOW */
    case 128 + 31: /* RF5_HOLD */
    case 160 + 1: /* RF6_HAND_DOOM */
    case 160 + 8: /* RF6_TELE_TO */
    case 160 + 9: /* RF6_TELE_AWAY */
    case 160 + 10: /* RF6_TELE_LEVEL */
    case 160 + 11: /* RF6_PSY_SPEAR */
    case 160 + 12: /* RF6_DARKNESS */
    case 160 + 14: /* RF6_FORGET */
        return FALSE;
    default:
        return TRUE;
    }
}

static void check_mspell_imitation(player_type *target_ptr, msa_type *msa_ptr)
{
    bool seen = (!target_ptr->blind && msa_ptr->m_ptr->ml);
    bool can_imitate = player_has_los_bold(target_ptr, msa_ptr->m_ptr->fy, msa_ptr->m_ptr->fx);
    if (!seen || !can_imitate || (current_world_ptr->timewalk_m_idx != 0) || (target_ptr->pclass != CLASS_IMITATOR))
        return;

    /* Not RF6_SPECIAL */
    if (msa_ptr->thrown_spell == 167)
        return;

    if (target_ptr->mane_num == MAX_MANE) {
        target_ptr->mane_num--;
        for (int i = 0; i < target_ptr->mane_num; i++) {
            target_ptr->mane_spell[i] = target_ptr->mane_spell[i + 1];
            target_ptr->mane_dam[i] = target_ptr->mane_dam[i + 1];
        }
    }

    target_ptr->mane_spell[target_ptr->mane_num] = msa_ptr->thrown_spell - 96;
    target_ptr->mane_dam[target_ptr->mane_num] = msa_ptr->dam;
    target_ptr->mane_num++;
    target_ptr->new_mane = TRUE;
    target_ptr->redraw |= PR_IMITATION;
}

static void remember_mspell(msa_type *msa_ptr)
{
    if (!msa_ptr->can_remember)
        return;

    if (msa_ptr->thrown_spell < 32 * 4) {
        msa_ptr->r_ptr->r_flags4 |= (1L << (msa_ptr->thrown_spell - 32 * 3));
        if (msa_ptr->r_ptr->r_cast_spell < MAX_UCHAR)
            msa_ptr->r_ptr->r_cast_spell++;

        return;
    }
    
    if (msa_ptr->thrown_spell < 32 * 5) {
        msa_ptr->r_ptr->r_flags5 |= (1L << (msa_ptr->thrown_spell - 32 * 4));
        if (msa_ptr->r_ptr->r_cast_spell < MAX_UCHAR)
            msa_ptr->r_ptr->r_cast_spell++;

        return;
    }
    
    if (msa_ptr->thrown_spell < 32 * 6) {
        msa_ptr->r_ptr->r_flags6 |= (1L << (msa_ptr->thrown_spell - 32 * 5));
        if (msa_ptr->r_ptr->r_cast_spell < MAX_UCHAR)
            msa_ptr->r_ptr->r_cast_spell++;
    }
}

/*!
 * @brief モンスターの特殊技能メインルーチン /
 * Creatures can cast spells, shoot missiles, and breathe.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスター構造体配列のID
 * @return 実際に特殊技能を利用したらTRUEを返す
 */
bool make_attack_spell(player_type *target_ptr, MONSTER_IDX m_idx)
{
    msa_type tmp_msa;
    msa_type *msa_ptr = initialize_msa_type(target_ptr, &tmp_msa, m_idx);
    if (monster_confused_remaining(msa_ptr->m_ptr)) {
        reset_target(msa_ptr->m_ptr);
        return FALSE;
    }

    if (((msa_ptr->m_ptr->mflag & MFLAG_NICE) != 0) || !is_hostile(msa_ptr->m_ptr)
        || ((msa_ptr->m_ptr->cdis > get_max_range(target_ptr)) && !msa_ptr->m_ptr->target_y))
        return FALSE;

    decide_lite_range(target_ptr, msa_ptr);
    if (!decide_lite_projection(target_ptr, msa_ptr))
        return FALSE;

    reset_target(msa_ptr->m_ptr);
    msa_ptr->rlev = ((msa_ptr->r_ptr->level >= 1) ? msa_ptr->r_ptr->level : 1);
    set_no_magic_mask(msa_ptr);
    decide_lite_area(target_ptr, msa_ptr);
    check_mspell_stupid(target_ptr, msa_ptr);
    check_mspell_smart(target_ptr, msa_ptr);
    if (!check_mspell_continuation(target_ptr, msa_ptr))
        return FALSE;

    if (check_mspell_unexploded(target_ptr, msa_ptr))
        return TRUE;

    if (!check_thrown_mspell(target_ptr, msa_ptr))
        return FALSE;

    msa_ptr->dam = monspell_to_player(target_ptr, msa_ptr->thrown_spell, msa_ptr->y, msa_ptr->x, m_idx);
    if (msa_ptr->dam < 0)
        return FALSE;

    if ((target_ptr->action == ACTION_LEARN) && msa_ptr->thrown_spell > 175)
        learn_spell(target_ptr, msa_ptr->thrown_spell - 96);

    check_mspell_imitation(target_ptr, msa_ptr);
    remember_mspell(msa_ptr);
    if (target_ptr->is_dead && (msa_ptr->r_ptr->r_deaths < MAX_SHORT) && !target_ptr->current_floor_ptr->inside_arena)
        msa_ptr->r_ptr->r_deaths++;

    return TRUE;
}
