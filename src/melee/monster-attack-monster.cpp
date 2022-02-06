/*!
 * @brief モンスター同士が乱闘する処理
 * @date 2020/05/23
 * @author Hourier
 */

#include "melee/monster-attack-monster.h"
#include "combat/attack-accuracy.h"
#include "combat/hallucination-attacks-table.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "melee/melee-postprocess.h"
#include "melee/melee-switcher.h"
#include "melee/melee-util.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-hex.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

static void heal_monster_by_melee(PlayerType *player_ptr, mam_type *mam_ptr)
{
    if (!monster_living(mam_ptr->m_idx) || (mam_ptr->damage <= 2))
        return;

    bool did_heal = mam_ptr->m_ptr->hp < mam_ptr->m_ptr->maxhp;
    mam_ptr->m_ptr->hp += damroll(4, mam_ptr->damage / 6);
    if (mam_ptr->m_ptr->hp > mam_ptr->m_ptr->maxhp)
        mam_ptr->m_ptr->hp = mam_ptr->m_ptr->maxhp;

    if (player_ptr->health_who == mam_ptr->m_idx)
        player_ptr->redraw |= (PR_HEALTH);

    if (player_ptr->riding == mam_ptr->m_idx)
        player_ptr->redraw |= (PR_UHEALTH);

    if (mam_ptr->see_m && did_heal)
        msg_format(_("%sは体力を回復したようだ。", "%^s appears healthier."), mam_ptr->m_name);
}

static void process_blow_effect(PlayerType *player_ptr, mam_type *mam_ptr)
{
    monster_race *r_ptr = &r_info[mam_ptr->m_ptr->r_idx];
    switch (mam_ptr->attribute) {
    case BLOW_EFFECT_TYPE_FEAR:
        project(player_ptr, mam_ptr->m_idx, 0, mam_ptr->t_ptr->fy, mam_ptr->t_ptr->fx, mam_ptr->damage,
            AttributeType::TURN_ALL, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED);
        break;
    case BLOW_EFFECT_TYPE_SLEEP:
        project(player_ptr, mam_ptr->m_idx, 0, mam_ptr->t_ptr->fy, mam_ptr->t_ptr->fx, r_ptr->level,
            AttributeType::OLD_SLEEP, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED);
        break;
    case BLOW_EFFECT_TYPE_HEAL:
        heal_monster_by_melee(player_ptr, mam_ptr);
        break;
    }
}

static void aura_fire_by_melee(PlayerType *player_ptr, mam_type *mam_ptr)
{
    monster_race *r_ptr = &r_info[mam_ptr->m_ptr->r_idx];
    monster_race *tr_ptr = &r_info[mam_ptr->t_ptr->r_idx];
    if (tr_ptr->aura_flags.has_not(MonsterAuraType::FIRE) || (mam_ptr->m_ptr->r_idx == 0))
        return;

    if (((r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK) != 0) && is_original_ap_and_seen(player_ptr, mam_ptr->m_ptr)) {
        r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK);
        return;
    }

    if (mam_ptr->see_either)
        msg_format(_("%^sは突然熱くなった！", "%^s is suddenly very hot!"), mam_ptr->m_name);

    if (mam_ptr->m_ptr->ml && is_original_ap_and_seen(player_ptr, mam_ptr->t_ptr))
        tr_ptr->aura_flags.set(MonsterAuraType::FIRE);

    project(player_ptr, mam_ptr->t_idx, 0, mam_ptr->m_ptr->fy, mam_ptr->m_ptr->fx, damroll(1 + ((tr_ptr->level) / 26), 1 + ((tr_ptr->level) / 17)), AttributeType::FIRE,
        PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED);
}

static void aura_cold_by_melee(PlayerType *player_ptr, mam_type *mam_ptr)
{
    monster_race *r_ptr = &r_info[mam_ptr->m_ptr->r_idx];
    monster_race *tr_ptr = &r_info[mam_ptr->t_ptr->r_idx];
    if (tr_ptr->aura_flags.has_not(MonsterAuraType::COLD) || (mam_ptr->m_ptr->r_idx == 0))
        return;

    if (((r_ptr->flagsr & RFR_EFF_IM_COLD_MASK) != 0) && is_original_ap_and_seen(player_ptr, mam_ptr->m_ptr)) {
        r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_COLD_MASK);
        return;
    }

    if (mam_ptr->see_either)
        msg_format(_("%^sは突然寒くなった！", "%^s is suddenly very cold!"), mam_ptr->m_name);

    if (mam_ptr->m_ptr->ml && is_original_ap_and_seen(player_ptr, mam_ptr->t_ptr))
        tr_ptr->aura_flags.set(MonsterAuraType::COLD);

    project(player_ptr, mam_ptr->t_idx, 0, mam_ptr->m_ptr->fy, mam_ptr->m_ptr->fx, damroll(1 + ((tr_ptr->level) / 26), 1 + ((tr_ptr->level) / 17)), AttributeType::COLD,
        PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED);
}

static void aura_elec_by_melee(PlayerType *player_ptr, mam_type *mam_ptr)
{
    monster_race *r_ptr = &r_info[mam_ptr->m_ptr->r_idx];
    monster_race *tr_ptr = &r_info[mam_ptr->t_ptr->r_idx];
    if (tr_ptr->aura_flags.has_not(MonsterAuraType::ELEC) || (mam_ptr->m_ptr->r_idx == 0))
        return;

    if (((r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK) != 0) && is_original_ap_and_seen(player_ptr, mam_ptr->m_ptr)) {
        r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK);
        return;
    }

    if (mam_ptr->see_either)
        msg_format(_("%^sは電撃を食らった！", "%^s gets zapped!"), mam_ptr->m_name);

    if (mam_ptr->m_ptr->ml && is_original_ap_and_seen(player_ptr, mam_ptr->t_ptr))
        tr_ptr->aura_flags.set(MonsterAuraType::ELEC);

    project(player_ptr, mam_ptr->t_idx, 0, mam_ptr->m_ptr->fy, mam_ptr->m_ptr->fx, damroll(1 + ((tr_ptr->level) / 26), 1 + ((tr_ptr->level) / 17)), AttributeType::ELEC,
        PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED);
}

static bool check_same_monster(PlayerType *player_ptr, mam_type *mam_ptr)
{
    if (mam_ptr->m_idx == mam_ptr->t_idx)
        return false;

    monster_race *r_ptr = &r_info[mam_ptr->m_ptr->r_idx];
    if (r_ptr->behavior_flags.has(MonsterBehaviorType::NEVER_BLOW))
        return false;

    if (d_info[player_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_MELEE))
        return false;

    return true;
}

static void redraw_health_bar(PlayerType *player_ptr, mam_type *mam_ptr)
{
    if (!mam_ptr->t_ptr->ml)
        return;

    if (player_ptr->health_who == mam_ptr->t_idx)
        player_ptr->redraw |= (PR_HEALTH);

    if (player_ptr->riding == mam_ptr->t_idx)
        player_ptr->redraw |= (PR_UHEALTH);
}

static void describe_silly_melee(mam_type *mam_ptr)
{
    char temp[MAX_NLEN];
    if ((mam_ptr->act == nullptr) || !mam_ptr->see_either)
        return;

#ifdef JP
    if (mam_ptr->do_silly_attack)
        mam_ptr->act = silly_attacks2[randint0(MAX_SILLY_ATTACK)];

    strfmt(temp, mam_ptr->act, mam_ptr->t_name);
    msg_format("%^sは%s", mam_ptr->m_name, temp);
#else
    if (mam_ptr->do_silly_attack) {
        mam_ptr->act = silly_attacks[randint0(MAX_SILLY_ATTACK)];
        strfmt(temp, "%s %s.", mam_ptr->act, mam_ptr->t_name);
    } else
        strfmt(temp, mam_ptr->act, mam_ptr->t_name);

    msg_format("%^s %s", mam_ptr->m_name, temp);
#endif
}

static void process_monster_attack_effect(PlayerType *player_ptr, mam_type *mam_ptr)
{
    if (mam_ptr->pt == AttributeType::NONE)
        return;

    if (!mam_ptr->explode)
        project(player_ptr, mam_ptr->m_idx, 0, mam_ptr->t_ptr->fy, mam_ptr->t_ptr->fx, mam_ptr->damage, mam_ptr->pt,
            PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED);

    process_blow_effect(player_ptr, mam_ptr);
    if (!mam_ptr->touched)
        return;

    aura_fire_by_melee(player_ptr, mam_ptr);
    aura_cold_by_melee(player_ptr, mam_ptr);
    aura_elec_by_melee(player_ptr, mam_ptr);
}

static void process_melee(PlayerType *player_ptr, mam_type *mam_ptr)
{
    if (mam_ptr->effect != RaceBlowEffectType::NONE && !check_hit_from_monster_to_monster(mam_ptr->power, mam_ptr->rlev, mam_ptr->ac, monster_stunned_remaining(mam_ptr->m_ptr))) {
        describe_monster_missed_monster(player_ptr, mam_ptr);
        return;
    }

    (void)set_monster_csleep(player_ptr, mam_ptr->t_idx, 0);
    redraw_health_bar(player_ptr, mam_ptr);
    describe_melee_method(player_ptr, mam_ptr);
    describe_silly_melee(mam_ptr);
    mam_ptr->obvious = true;
    mam_ptr->damage = damroll(mam_ptr->d_dice, mam_ptr->d_side);
    mam_ptr->attribute = BLOW_EFFECT_TYPE_NONE;
    mam_ptr->pt = AttributeType::MONSTER_MELEE;
    decide_monster_attack_effect(player_ptr, mam_ptr);
    process_monster_attack_effect(player_ptr, mam_ptr);
}

static void thief_runaway_by_melee(PlayerType *player_ptr, mam_type *mam_ptr)
{
    if (SpellHex(player_ptr).check_hex_barrier(mam_ptr->m_idx, HEX_ANTI_TELE)) {
        if (mam_ptr->see_m) {
            msg_print(_("泥棒は笑って逃げ...ようとしたがバリアに防がれた。", "The thief flees laughing...? But a magic barrier obstructs it."));
        } else if (mam_ptr->known) {
            player_ptr->current_floor_ptr->monster_noise = true;
        }
    } else {
        if (mam_ptr->see_m) {
            msg_print(_("泥棒は笑って逃げた！", "The thief flees laughing!"));
        } else if (mam_ptr->known) {
            player_ptr->current_floor_ptr->monster_noise = true;
        }

        teleport_away(player_ptr, mam_ptr->m_idx, MAX_SIGHT * 2 + 5, TELEPORT_SPONTANEOUS);
    }
}

static void explode_monster_by_melee(PlayerType *player_ptr, mam_type *mam_ptr)
{
    if (!mam_ptr->explode)
        return;

    sound(SOUND_EXPLODE);
    (void)set_monster_invulner(player_ptr, mam_ptr->m_idx, 0, false);
    mon_take_hit_mon(player_ptr, mam_ptr->m_idx, mam_ptr->m_ptr->hp + 1, &mam_ptr->dead, &mam_ptr->fear,
        _("は爆発して粉々になった。", " explodes into tiny shreds."), mam_ptr->m_idx);
    mam_ptr->blinked = false;
}

/*!
 * @brief r_infoで定義した攻撃回数の分だけ、モンスターからモンスターへの直接攻撃処理を繰り返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mam_ptr モンスター乱闘構造体への参照ポインタ
 */
void repeat_melee(PlayerType *player_ptr, mam_type *mam_ptr)
{
    monster_race *r_ptr = &r_info[mam_ptr->m_ptr->r_idx];
    for (int ap_cnt = 0; ap_cnt < MAX_NUM_BLOWS; ap_cnt++) {
        mam_ptr->effect = r_ptr->blow[ap_cnt].effect;
        mam_ptr->method = r_ptr->blow[ap_cnt].method;
        mam_ptr->d_dice = r_ptr->blow[ap_cnt].d_dice;
        mam_ptr->d_side = r_ptr->blow[ap_cnt].d_side;

        if (!monster_is_valid(mam_ptr->m_ptr) || (mam_ptr->t_ptr->fx != mam_ptr->x_saver) || (mam_ptr->t_ptr->fy != mam_ptr->y_saver) || mam_ptr->method == RaceBlowMethodType::NONE)
            break;

        if (mam_ptr->method == RaceBlowMethodType::SHOOT)
            continue;

        mam_ptr->power = mbe_info[enum2i(mam_ptr->effect)].power;
        process_melee(player_ptr, mam_ptr);
        if (!is_original_ap_and_seen(player_ptr, mam_ptr->m_ptr) || mam_ptr->do_silly_attack)
            continue;

        if (!mam_ptr->obvious && !mam_ptr->damage && (r_ptr->r_blows[ap_cnt] <= 10))
            continue;

        if (r_ptr->r_blows[ap_cnt] < MAX_UCHAR)
            r_ptr->r_blows[ap_cnt]++;
    }
}

/*!
 * @brief モンスターから敵モンスターへの打撃攻撃処理
 * @param m_idx 攻撃側モンスターの参照ID
 * @param t_idx 目標側モンスターの参照ID
 * @return 実際に打撃処理が行われた場合TRUEを返す
 */
bool monst_attack_monst(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx)
{
    mam_type tmp_mam;
    mam_type *mam_ptr = initialize_mam_type(player_ptr, &tmp_mam, m_idx, t_idx);

    if (!check_same_monster(player_ptr, mam_ptr))
        return false;

    monster_desc(player_ptr, mam_ptr->m_name, mam_ptr->m_ptr, 0);
    monster_desc(player_ptr, mam_ptr->t_name, mam_ptr->t_ptr, 0);
    if (!mam_ptr->see_either && mam_ptr->known)
        player_ptr->current_floor_ptr->monster_noise = true;

    if (player_ptr->riding && (m_idx == player_ptr->riding))
        disturb(player_ptr, true, true);

    repeat_melee(player_ptr, mam_ptr);
    explode_monster_by_melee(player_ptr, mam_ptr);
    if (!mam_ptr->blinked || mam_ptr->m_ptr->r_idx == 0)
        return true;

    thief_runaway_by_melee(player_ptr, mam_ptr);
    return true;
}
