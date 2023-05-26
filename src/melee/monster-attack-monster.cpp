/*!
 * @brief モンスター同士が乱闘する処理
 * @date 2020/05/23
 * @author Hourier
 */

#include "melee/monster-attack-monster.h"
#include "combat/attack-accuracy.h"
#include "combat/hallucination-attacks-table.h"
#include "core/disturbance.h"
#include "dungeon/dungeon-flag-types.h"
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
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

static void heal_monster_by_melee(PlayerType *player_ptr, mam_type *mam_ptr)
{
    if (!mam_ptr->t_ptr->has_living_flag() || (mam_ptr->damage <= 2)) {
        return;
    }

    bool did_heal = mam_ptr->m_ptr->hp < mam_ptr->m_ptr->maxhp;
    mam_ptr->m_ptr->hp += damroll(4, mam_ptr->damage / 6);
    if (mam_ptr->m_ptr->hp > mam_ptr->m_ptr->maxhp) {
        mam_ptr->m_ptr->hp = mam_ptr->m_ptr->maxhp;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (player_ptr->health_who == mam_ptr->m_idx) {
        rfu.set_flag(MainWindowRedrawingFlag::HEALTH);
    }

    if (player_ptr->riding == mam_ptr->m_idx) {
        rfu.set_flag(MainWindowRedrawingFlag::UHEALTH);
    }

    if (mam_ptr->see_m && did_heal) {
        msg_format(_("%sは体力を回復したようだ。", "%s^ appears healthier."), mam_ptr->m_name);
    }
}

static void process_blow_effect(PlayerType *player_ptr, mam_type *mam_ptr)
{
    auto *r_ptr = &monraces_info[mam_ptr->m_ptr->r_idx];
    switch (mam_ptr->attribute) {
    case BlowEffectType::FEAR:
        project(player_ptr, mam_ptr->m_idx, 0, mam_ptr->t_ptr->fy, mam_ptr->t_ptr->fx, mam_ptr->damage,
            AttributeType::TURN_ALL, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED);
        break;
    case BlowEffectType::SLEEP:
        project(player_ptr, mam_ptr->m_idx, 0, mam_ptr->t_ptr->fy, mam_ptr->t_ptr->fx, r_ptr->level,
            AttributeType::OLD_SLEEP, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED);
        break;
    case BlowEffectType::HEAL:
        heal_monster_by_melee(player_ptr, mam_ptr);
        break;
    default:
        break;
    }
}

static void aura_fire_by_melee(PlayerType *player_ptr, mam_type *mam_ptr)
{
    auto *r_ptr = &monraces_info[mam_ptr->m_ptr->r_idx];
    MonsterRaceInfo *tr_ptr = &monraces_info[mam_ptr->t_ptr->r_idx];
    if (tr_ptr->aura_flags.has_not(MonsterAuraType::FIRE) || !MonsterRace(mam_ptr->m_ptr->r_idx).is_valid()) {
        return;
    }

    if (r_ptr->resistance_flags.has_any_of(RFR_EFF_IM_FIRE_MASK) && is_original_ap_and_seen(player_ptr, mam_ptr->m_ptr)) {
        r_ptr->r_resistance_flags.set(r_ptr->resistance_flags & RFR_EFF_IM_FIRE_MASK);
        return;
    }

    if (mam_ptr->see_either) {
        msg_format(_("%s^は突然熱くなった！", "%s^ is suddenly very hot!"), mam_ptr->m_name);
    }

    if (mam_ptr->m_ptr->ml && is_original_ap_and_seen(player_ptr, mam_ptr->t_ptr)) {
        tr_ptr->aura_flags.set(MonsterAuraType::FIRE);
    }

    const auto dam = damroll(1 + ((tr_ptr->level) / 26), 1 + ((tr_ptr->level) / 17));
    constexpr auto flags = PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED;
    project(player_ptr, mam_ptr->t_idx, 0, mam_ptr->m_ptr->fy, mam_ptr->m_ptr->fx, dam, AttributeType::FIRE, flags);
}

static void aura_cold_by_melee(PlayerType *player_ptr, mam_type *mam_ptr)
{
    const auto *m_ptr = mam_ptr->m_ptr;
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    MonsterRaceInfo *tr_ptr = &monraces_info[mam_ptr->t_ptr->r_idx];
    if (tr_ptr->aura_flags.has_not(MonsterAuraType::COLD) || !MonsterRace(m_ptr->r_idx).is_valid()) {
        return;
    }

    if (r_ptr->resistance_flags.has_any_of(RFR_EFF_IM_COLD_MASK) && is_original_ap_and_seen(player_ptr, m_ptr)) {
        r_ptr->r_resistance_flags.set(r_ptr->resistance_flags & RFR_EFF_IM_COLD_MASK);
        return;
    }

    if (mam_ptr->see_either) {
        msg_format(_("%s^は突然寒くなった！", "%s^ is suddenly very cold!"), mam_ptr->m_name);
    }

    if (m_ptr->ml && is_original_ap_and_seen(player_ptr, mam_ptr->t_ptr)) {
        tr_ptr->aura_flags.set(MonsterAuraType::COLD);
    }

    const auto dam = damroll(1 + ((tr_ptr->level) / 26), 1 + ((tr_ptr->level) / 17));
    constexpr auto flags = PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED;
    project(player_ptr, mam_ptr->t_idx, 0, m_ptr->fy, m_ptr->fx, dam, AttributeType::COLD, flags);
}

static void aura_elec_by_melee(PlayerType *player_ptr, mam_type *mam_ptr)
{
    const auto *m_ptr = mam_ptr->m_ptr;
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    MonsterRaceInfo *tr_ptr = &monraces_info[mam_ptr->t_ptr->r_idx];
    if (tr_ptr->aura_flags.has_not(MonsterAuraType::ELEC) || !MonsterRace(m_ptr->r_idx).is_valid()) {
        return;
    }

    if (r_ptr->resistance_flags.has_any_of(RFR_EFF_IM_ELEC_MASK) && is_original_ap_and_seen(player_ptr, m_ptr)) {
        r_ptr->r_resistance_flags.set(r_ptr->resistance_flags & RFR_EFF_IM_ELEC_MASK);
        return;
    }

    if (mam_ptr->see_either) {
        msg_format(_("%s^は電撃を食らった！", "%s^ gets zapped!"), mam_ptr->m_name);
    }

    if (m_ptr->ml && is_original_ap_and_seen(player_ptr, mam_ptr->t_ptr)) {
        tr_ptr->aura_flags.set(MonsterAuraType::ELEC);
    }

    const auto dam = damroll(1 + ((tr_ptr->level) / 26), 1 + ((tr_ptr->level) / 17));
    constexpr auto flags = PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED;
    project(player_ptr, mam_ptr->t_idx, 0, m_ptr->fy, m_ptr->fx, dam, AttributeType::ELEC, flags);
}

static bool check_same_monster(PlayerType *player_ptr, mam_type *mam_ptr)
{
    if (mam_ptr->m_idx == mam_ptr->t_idx) {
        return false;
    }

    auto *r_ptr = &monraces_info[mam_ptr->m_ptr->r_idx];
    if (r_ptr->behavior_flags.has(MonsterBehaviorType::NEVER_BLOW)) {
        return false;
    }

    if (dungeons_info[player_ptr->current_floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_MELEE)) {
        return false;
    }

    return true;
}

static void redraw_health_bar(PlayerType *player_ptr, mam_type *mam_ptr)
{
    if (!mam_ptr->t_ptr->ml) {
        return;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (player_ptr->health_who == mam_ptr->t_idx) {
        rfu.set_flag(MainWindowRedrawingFlag::HEALTH);
    }

    if (player_ptr->riding == mam_ptr->t_idx) {
        rfu.set_flag(MainWindowRedrawingFlag::UHEALTH);
    }
}

static void describe_silly_melee(mam_type *mam_ptr)
{
    char temp[MAX_NLEN];
    if ((mam_ptr->act == nullptr) || !mam_ptr->see_either) {
        return;
    }

#ifdef JP
    if (mam_ptr->do_silly_attack) {
        mam_ptr->act = rand_choice(silly_attacks2);
    }

    strnfmt(temp, sizeof(temp), mam_ptr->act, mam_ptr->t_name);
    msg_format("%s^は%s", mam_ptr->m_name, temp);
#else
    if (mam_ptr->do_silly_attack) {
        mam_ptr->act = rand_choice(silly_attacks);
        strnfmt(temp, sizeof(temp), "%s %s.", mam_ptr->act, mam_ptr->t_name);
    } else {
        strnfmt(temp, sizeof(temp), mam_ptr->act, mam_ptr->t_name);
    }

    msg_format("%s^ %s", mam_ptr->m_name, temp);
#endif
}

static void process_monster_attack_effect(PlayerType *player_ptr, mam_type *mam_ptr)
{
    if (mam_ptr->pt == AttributeType::NONE) {
        return;
    }

    if (!mam_ptr->explode) {
        project(player_ptr, mam_ptr->m_idx, 0, mam_ptr->t_ptr->fy, mam_ptr->t_ptr->fx, mam_ptr->damage, mam_ptr->pt,
            PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED);
    }

    process_blow_effect(player_ptr, mam_ptr);
    if (!mam_ptr->touched) {
        return;
    }

    aura_fire_by_melee(player_ptr, mam_ptr);
    aura_cold_by_melee(player_ptr, mam_ptr);
    aura_elec_by_melee(player_ptr, mam_ptr);
}

static void process_melee(PlayerType *player_ptr, mam_type *mam_ptr)
{
    const auto remaining_stun = mam_ptr->m_ptr->get_remaining_stun();
    if (mam_ptr->effect != RaceBlowEffectType::NONE && !check_hit_from_monster_to_monster(mam_ptr->power, mam_ptr->rlev, mam_ptr->ac, remaining_stun)) {
        describe_monster_missed_monster(player_ptr, mam_ptr);
        return;
    }

    (void)set_monster_csleep(player_ptr, mam_ptr->t_idx, 0);
    redraw_health_bar(player_ptr, mam_ptr);
    describe_melee_method(player_ptr, mam_ptr);
    describe_silly_melee(mam_ptr);
    mam_ptr->obvious = true;
    mam_ptr->damage = damroll(mam_ptr->d_dice, mam_ptr->d_side);
    mam_ptr->attribute = BlowEffectType::NONE;
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

        teleport_away(player_ptr, mam_ptr->m_idx, MAX_PLAYER_SIGHT * 2 + 5, TELEPORT_SPONTANEOUS);
    }
}

static void explode_monster_by_melee(PlayerType *player_ptr, mam_type *mam_ptr)
{
    if (!mam_ptr->explode) {
        return;
    }

    sound(SOUND_EXPLODE);
    (void)set_monster_invulner(player_ptr, mam_ptr->m_idx, 0, false);
    mon_take_hit_mon(player_ptr, mam_ptr->m_idx, mam_ptr->m_ptr->hp + 1, &mam_ptr->dead, &mam_ptr->fear,
        _("は爆発して粉々になった。", " explodes into tiny shreds."), mam_ptr->m_idx);
    mam_ptr->blinked = false;
}

/*!
 * @brief MonsterRaceDefinitionで定義した攻撃回数の分だけ、モンスターからモンスターへの直接攻撃処理を繰り返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mam_ptr モンスター乱闘構造体への参照ポインタ
 */
void repeat_melee(PlayerType *player_ptr, mam_type *mam_ptr)
{
    const auto *m_ptr = mam_ptr->m_ptr;
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    for (int ap_cnt = 0; ap_cnt < MAX_NUM_BLOWS; ap_cnt++) {
        mam_ptr->effect = r_ptr->blows[ap_cnt].effect;
        mam_ptr->method = r_ptr->blows[ap_cnt].method;
        mam_ptr->d_dice = r_ptr->blows[ap_cnt].d_dice;
        mam_ptr->d_side = r_ptr->blows[ap_cnt].d_side;

        if (!m_ptr->is_valid()) {
            break;
        }

        const auto x_saver = mam_ptr->t_ptr->fx != mam_ptr->x_saver;
        const auto y_saver = mam_ptr->t_ptr->fy != mam_ptr->y_saver;
        if (x_saver || y_saver || mam_ptr->method == RaceBlowMethodType::NONE) {
            break;
        }

        if (mam_ptr->method == RaceBlowMethodType::SHOOT) {
            continue;
        }

        mam_ptr->power = mbe_info[enum2i(mam_ptr->effect)].power;
        process_melee(player_ptr, mam_ptr);
        if (!is_original_ap_and_seen(player_ptr, mam_ptr->m_ptr) || mam_ptr->do_silly_attack) {
            continue;
        }

        if (!mam_ptr->obvious && !mam_ptr->damage && (r_ptr->r_blows[ap_cnt] <= 10)) {
            continue;
        }

        if (r_ptr->r_blows[ap_cnt] < MAX_UCHAR) {
            r_ptr->r_blows[ap_cnt]++;
        }
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

    if (!check_same_monster(player_ptr, mam_ptr)) {
        return false;
    }

    angband_strcpy(mam_ptr->m_name, monster_desc(player_ptr, mam_ptr->m_ptr, 0).data(), sizeof(mam_ptr->m_name));
    angband_strcpy(mam_ptr->t_name, monster_desc(player_ptr, mam_ptr->t_ptr, 0).data(), sizeof(mam_ptr->t_name));
    if (!mam_ptr->see_either && mam_ptr->known) {
        player_ptr->current_floor_ptr->monster_noise = true;
    }

    if (player_ptr->riding && (m_idx == player_ptr->riding)) {
        disturb(player_ptr, true, true);
    }

    repeat_melee(player_ptr, mam_ptr);
    explode_monster_by_melee(player_ptr, mam_ptr);
    if (!mam_ptr->blinked || !MonsterRace(mam_ptr->m_ptr->r_idx).is_valid()) {
        return true;
    }

    thief_runaway_by_melee(player_ptr, mam_ptr);
    return true;
}
