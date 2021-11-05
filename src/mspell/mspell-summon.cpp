﻿#include "mspell/mspell-summon.h"
#include "core/disturbance.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "game-option/birth-options.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "mspell/mspell-util.h"
#include "mspell/mspell.h"
#include "mspell/specified-summon.h"
#include "spell-kind/spells-launcher.h"
#include "spell/spell-types.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/* summoning number */
constexpr int S_NUM_6 = 6;
constexpr int S_NUM_4 = 4;

/*!
 * @brief 特定条件のモンスター召喚のみPM_ALLOW_UNIQUEを許可する /
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @param m_idx モンスターID
 * @return 召喚可能であればPM_ALLOW_UNIQUEを返す。
 */
static BIT_FLAGS monster_u_mode(floor_type *floor_ptr, MONSTER_IDX m_idx)
{
    BIT_FLAGS u_mode = 0L;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    bool pet = is_pet(m_ptr);
    if (!pet)
        u_mode |= PM_ALLOW_UNIQUE;
    return u_mode;
}

/*!
 * @brief 救援召喚の通常処理。同シンボルのモンスターを召喚する。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param rlev 呪文を唱えるモンスターのレベル
 * @param m_idx 呪文を唱えるモンスターID
 * @return 召喚したモンスターの数を返す。
 */
static MONSTER_NUMBER summon_Kin(PlayerType *player_ptr, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx)
{
    int count = 0;
    for (int k = 0; k < 4; k++) {
        count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_KIN, PM_ALLOW_GROUP);
    }

    return count;
}

static void decide_summon_kin_caster(
    PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type, concptr m_name, concptr m_poss, const bool known)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    bool see_either = see_monster(player_ptr, m_idx) || see_monster(player_ptr, t_idx);
    bool mon_to_mon = target_type == MONSTER_TO_MONSTER;
    bool mon_to_player = target_type == MONSTER_TO_PLAYER;

    if (m_ptr->r_idx == MON_SERPENT || m_ptr->r_idx == MON_ZOMBI_SERPENT) {
        mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
            _("%^sがダンジョンの主を召喚した。", "%^s magically summons guardians of dungeons."),
            _("%^sがダンジョンの主を召喚した。", "%^s magically summons guardians of dungeons."));

        monspell_message(player_ptr, m_idx, t_idx, msg, target_type);
        return;
    }

    if (mon_to_player || (mon_to_mon && known && see_either))
        disturb(player_ptr, true, true);

    if (player_ptr->blind) {
        if (mon_to_player)
            msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
    } else if (mon_to_player || (mon_to_mon && known && see_either)) {
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
#ifdef JP
        (void)m_poss;
#endif
        _(msg_format("%sが魔法で%sを召喚した。", m_name, ((r_ptr->flags1 & RF1_UNIQUE) ? "手下" : "仲間")),
            msg_format("%^s magically summons %s %s.", m_name, m_poss, ((r_ptr->flags1 & RF1_UNIQUE) ? "minions" : "kin")));
    }

    if (mon_to_mon && known && !see_either)
        floor_ptr->monster_noise = true;
}

/*!
 * @brief RF6_S_KINの処理。救援召喚。使用するモンスターの種類により、実処理に分岐させる。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_KIN(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN], m_poss[80];
    monster_name(player_ptr, m_idx, m_name);
    monster_name(player_ptr, t_idx, t_name);
    monster_desc(player_ptr, m_poss, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);

    disturb(player_ptr, true, true);
    bool known = monster_near_player(floor_ptr, m_idx, t_idx);
    decide_summon_kin_caster(player_ptr, m_idx, t_idx, target_type, m_name, m_poss, known);
    int count = 0;
    switch (m_ptr->r_idx) {
    case MON_MENELDOR:
    case MON_GWAIHIR:
    case MON_THORONDOR:
        count += summon_EAGLE(player_ptr, y, x, rlev, m_idx);
        break;
    case MON_BULLGATES:
        count += summon_EDGE(player_ptr, y, x, rlev, m_idx);
        break;
    case MON_SERPENT:
    case MON_ZOMBI_SERPENT:
        count += summon_guardian(player_ptr, y, x, rlev, m_idx, t_idx, target_type);
        break;
    case MON_TIAMAT:
        count += summon_HIGHEST_DRAGON(player_ptr, y, x, m_idx);
        break;
    case MON_CALDARM:
        count += summon_LOCKE_CLONE(player_ptr, y, x, m_idx);
        break;
    case MON_LOUSY:
        count += summon_LOUSE(player_ptr, y, x, rlev, m_idx);
        break;
    case MON_VAIF:
        count += summon_MOAI(player_ptr, y, x, rlev, m_idx);
        break;
    case MON_DEMON_SLAYER_SENIOR:
        count += summon_DEMON_SLAYER(player_ptr, y, x, m_idx);
        break;
    case MON_ALDUIN:
        count += summon_HIGHEST_DRAGON(player_ptr, y, x, m_idx);
        break;
    case MON_MIRAAK:
        count += summon_APOCRYPHA(player_ptr, y, x, m_idx);
        break;
    case MON_IMHOTEP:
        count += summon_PYRAMID(player_ptr, y, x, rlev, m_idx);
        break;
    case MON_JOBZ:
        count += summon_EYE_PHORN(player_ptr, y, x, rlev, m_idx);
        break;
    case MON_QUEEN_VESPOID:
        count += summon_VESPOID(player_ptr, y, x, rlev, m_idx);
        break;
    case MON_YENDOR_WIZARD_1:
        count += summon_YENDER_WIZARD(player_ptr, y, x, m_idx);
        break;
    case MON_LEE_QIEZI:
        msg_print(_("しかし、誰も来てくれなかった…。", "However, no one answered the call..."));
        break;
    case MON_THUNDERS:
        count += summon_THUNDERS(player_ptr, y, x, rlev, m_idx);
        break;
    case MON_OOTSUKI:
        count += summon_PLASMA(player_ptr, y, x, rlev, m_idx);
        break;
    default:
        count += summon_Kin(player_ptr, y, x, rlev, m_idx);
        break;
    }

    if (player_ptr->blind && count && (target_type == MONSTER_TO_PLAYER))
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));

    if (known && !see_monster(player_ptr, t_idx) && count && (target_type == MONSTER_TO_MONSTER))
        floor_ptr->monster_noise = true;

    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_CYBERの処理。サイバー・デーモン召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_CYBER(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    int count = 0;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    bool mon_to_player = (TARGET_TYPE == MONSTER_TO_PLAYER);
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがサイバーデーモンを召喚した！", "%^s magically summons Cyberdemons!"),
        _("%^sがサイバーデーモンを召喚した！", "%^s magically summons Cyberdemons!"));

    monspell_message(player_ptr, m_idx, t_idx, msg, TARGET_TYPE);

    if (is_friendly(m_ptr) && mon_to_mon) {
        count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_CYBER, (PM_ALLOW_GROUP));
    } else {
        count += summon_cyber(player_ptr, m_idx, y, x);
    }

    if (player_ptr->blind && count && mon_to_player)
        msg_print(_("重厚な足音が近くで聞こえる。", "You hear heavy steps nearby."));

    if (monster_near_player(floor_ptr, m_idx, t_idx) && !see_monster(player_ptr, t_idx) && count && mon_to_mon)
        floor_ptr->monster_noise = true;

    auto res = MonsterSpellResult::make_valid();
    res.learnable = TARGET_TYPE == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_MONSTERの処理。モンスター一体召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_MONSTER(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが魔法で仲間を召喚した！", "%^s magically summons help!"),
        _("%^sが魔法で仲間を召喚した！", "%^s magically summons help!"));

    monspell_message(player_ptr, m_idx, t_idx, msg, TARGET_TYPE);

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    int count = 0;
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    bool mon_to_player = (TARGET_TYPE == MONSTER_TO_PLAYER);
    for (int k = 0; k < 1; k++) {
        if (mon_to_player)
            count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_NONE, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));

        if (mon_to_mon)
            count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_NONE, (monster_u_mode(floor_ptr, m_idx)));
    }

    if (player_ptr->blind && count && mon_to_player)
        msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));

    if (monster_near_player(floor_ptr, m_idx, t_idx) && !see_monster(player_ptr, t_idx) && count && mon_to_mon)
        floor_ptr->monster_noise = true;

    auto res = MonsterSpellResult::make_valid();
    res.learnable = TARGET_TYPE == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_MONSTERSの処理。モンスター複数召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_MONSTERS(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sが魔法でモンスターを召喚した！", "%^s magically summons monsters!"), _("%^sが魔法でモンスターを召喚した！", "%^s magically summons monsters!"));

    monspell_message(player_ptr, m_idx, t_idx, msg, TARGET_TYPE);

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    int count = 0;
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    bool mon_to_player = (TARGET_TYPE == MONSTER_TO_PLAYER);
    for (auto k = 0; k < S_NUM_6; k++) {
        if (mon_to_player)
            count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_NONE, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));

        if (mon_to_mon)
            count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_NONE, (PM_ALLOW_GROUP | monster_u_mode(floor_ptr, m_idx)));
    }

    if (player_ptr->blind && count && mon_to_player)
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));

    if (monster_near_player(floor_ptr, m_idx, t_idx) && !see_monster(player_ptr, t_idx) && count && mon_to_mon)
        floor_ptr->monster_noise = true;

    auto res = MonsterSpellResult::make_valid();
    res.learnable = TARGET_TYPE == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_ANTの処理。アリ召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_ANT(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが魔法でアリを召喚した。", "%^s magically summons ants."),
        _("%^sが魔法でアリを召喚した。", "%^s magically summons ants."));

    monspell_message(player_ptr, m_idx, t_idx, msg, TARGET_TYPE);

    int count = 0;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    bool mon_to_player = (TARGET_TYPE == MONSTER_TO_PLAYER);
    for (auto k = 0; k < S_NUM_6; k++) {
        count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_ANT, PM_ALLOW_GROUP);
    }

    if (player_ptr->blind && count && mon_to_player)
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));

    if (monster_near_player(floor_ptr, m_idx, t_idx) && !see_monster(player_ptr, t_idx) && count && mon_to_mon)
        floor_ptr->monster_noise = true;

    auto res = MonsterSpellResult::make_valid();
    res.learnable = TARGET_TYPE == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_SPIDERの処理。クモ召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_SPIDER(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが魔法でクモを召喚した。", "%^s magically summons spiders."),
        _("%^sが魔法でクモを召喚した。", "%^s magically summons spiders."));

    monspell_message(player_ptr, m_idx, t_idx, msg, TARGET_TYPE);

    int count = 0;
    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    bool mon_to_player = (TARGET_TYPE == MONSTER_TO_PLAYER);
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    for (auto k = 0; k < S_NUM_6; k++) {
        count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_SPIDER, PM_ALLOW_GROUP);
    }

    if (player_ptr->blind && count && mon_to_player)
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));

    if (monster_near_player(floor_ptr, m_idx, t_idx) && !see_monster(player_ptr, t_idx) && count && mon_to_mon)
        floor_ptr->monster_noise = true;

    auto res = MonsterSpellResult::make_valid();
    res.learnable = TARGET_TYPE == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_HOUNDの処理。ハウンド召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_HOUND(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sが魔法でハウンドを召喚した。", "%^s magically summons hounds."), _("%^sが魔法でハウンドを召喚した。", "%^s magically summons hounds."));

    monspell_message(player_ptr, m_idx, t_idx, msg, TARGET_TYPE);

    int count = 0;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    bool mon_to_player = (TARGET_TYPE == MONSTER_TO_PLAYER);
    for (auto k = 0; k < S_NUM_4; k++) {
        count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_HOUND, PM_ALLOW_GROUP);
    }

    if (player_ptr->blind && count && mon_to_player)
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));

    if (monster_near_player(floor_ptr, m_idx, t_idx) && !see_monster(player_ptr, t_idx) && count && mon_to_mon)
        floor_ptr->monster_noise = true;

    auto res = MonsterSpellResult::make_valid();
    res.learnable = TARGET_TYPE == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_HYDRAの処理。ヒドラ召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_HYDRA(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sが魔法でヒドラを召喚した。", "%^s magically summons hydras."), _("%^sが魔法でヒドラを召喚した。", "%^s magically summons hydras."));

    monspell_message(player_ptr, m_idx, t_idx, msg, TARGET_TYPE);

    int count = 0;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    bool mon_to_player = (TARGET_TYPE == MONSTER_TO_PLAYER);
    for (auto k = 0; k < S_NUM_4; k++) {
        count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_HYDRA, PM_ALLOW_GROUP);
    }

    if (player_ptr->blind && count && mon_to_player)
        msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));

    if (monster_near_player(floor_ptr, m_idx, t_idx) && !see_monster(player_ptr, t_idx) && count && mon_to_mon)
        floor_ptr->monster_noise = true;

    auto res = MonsterSpellResult::make_valid();
    res.learnable = TARGET_TYPE == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_ANGELの処理。天使一体召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_ANGEL(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sが魔法で天使を召喚した！", "%^s magically summons an angel!"), _("%^sが魔法で天使を召喚した！", "%^s magically summons an angel!"));

    monspell_message(player_ptr, m_idx, t_idx, msg, TARGET_TYPE);

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    int num = 1;
    if (any_bits(r_ptr->flags1, RF1_UNIQUE)) {
        num += r_ptr->level / 40;
    }

    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    int count = 0;
    for (int k = 0; k < num; k++) {
        count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_ANGEL, PM_ALLOW_GROUP);
    }

    if (count < 2) {
        if (player_ptr->blind && count)
            msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));
    } else {
        if (player_ptr->blind)
            msg_print(_("多くのものが間近に現れた音がする。", "You hear many things appear nearby."));
    }

    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    if (monster_near_player(floor_ptr, m_idx, t_idx) && !see_monster(player_ptr, t_idx) && count && mon_to_mon)
        floor_ptr->monster_noise = true;

    auto res = MonsterSpellResult::make_valid();
    res.learnable = TARGET_TYPE == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_DEMONの処理。デーモン一体召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_DEMON(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sは魔法で混沌の宮廷から悪魔を召喚した！", "%^s magically summons a demon from the Courts of Chaos!"),
        _("%^sは魔法で混沌の宮廷から悪魔を召喚した！", "%^s magically summons a demon from the Courts of Chaos!"));

    monspell_message(player_ptr, m_idx, t_idx, msg, TARGET_TYPE);

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    int count = 0;
    for (int k = 0; k < 1; k++) {
        count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_DEMON, PM_ALLOW_GROUP);
    }

    if (player_ptr->blind && count)
        msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));

    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    if (monster_near_player(floor_ptr, m_idx, t_idx) && !see_monster(player_ptr, t_idx) && count && mon_to_mon)
        floor_ptr->monster_noise = true;

    auto res = MonsterSpellResult::make_valid();
    res.learnable = TARGET_TYPE == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_UNDEADの処理。アンデッド一体召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_UNDEAD(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sが魔法でアンデッドの強敵を召喚した！", "%^s magically summons an undead adversary!"),
        _("%sが魔法でアンデッドを召喚した。", "%^s magically summons undead."));

    monspell_message(player_ptr, m_idx, t_idx, msg, TARGET_TYPE);

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    int count = 0;
    for (int k = 0; k < 1; k++) {
        count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_UNDEAD, PM_ALLOW_GROUP);
    }

    if (player_ptr->blind && count)
        msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));

    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    if (monster_near_player(floor_ptr, m_idx, t_idx) && !see_monster(player_ptr, t_idx) && count && mon_to_mon)
        floor_ptr->monster_noise = true;

    auto res = MonsterSpellResult::make_valid();
    res.learnable = TARGET_TYPE == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_DRAGONの処理。ドラゴン一体召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_DRAGON(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sが魔法でドラゴンを召喚した！", "%^s magically summons a dragon!"), _("%^sが魔法でドラゴンを召喚した！", "%^s magically summons a dragon!"));

    monspell_message(player_ptr, m_idx, t_idx, msg, TARGET_TYPE);

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    bool mon_to_player = (TARGET_TYPE == MONSTER_TO_PLAYER);
    int count = 0;
    if (mon_to_player)
        count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_DRAGON, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));

    if (mon_to_mon)
        count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_DRAGON, (PM_ALLOW_GROUP | monster_u_mode(floor_ptr, m_idx)));

    if (player_ptr->blind && count)
        msg_print(_("何かが間近に現れた音がする。", "You hear something appear nearby."));

    if (monster_near_player(floor_ptr, m_idx, t_idx) && !see_monster(player_ptr, t_idx) && count && mon_to_mon)
        floor_ptr->monster_noise = true;

    auto res = MonsterSpellResult::make_valid();
    res.learnable = TARGET_TYPE == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_HI_UNDEADの処理。強力なアンデッド召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_HI_UNDEAD(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    GAME_TEXT m_name[MAX_NLEN];
    monster_name(player_ptr, m_idx, m_name);

    disturb(player_ptr, true, true);

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    bool mon_to_player = (TARGET_TYPE == MONSTER_TO_PLAYER);
    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    int count = 0;
    if (((m_ptr->r_idx == MON_MORGOTH) || (m_ptr->r_idx == MON_SAURON) || (m_ptr->r_idx == MON_ANGMAR)) && ((r_info[MON_NAZGUL].cur_num + 2) < r_info[MON_NAZGUL].max_num) && mon_to_player) {
        count += summon_NAZGUL(player_ptr, y, x, m_idx);
    } else {
        mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
            _("%^sが魔法で強力なアンデッドを召喚した！", "%^s magically summons greater undead!"),
            _("%sが魔法でアンデッドを召喚した。", "%^s magically summons undead."));

        monspell_message(player_ptr, m_idx, t_idx, msg, TARGET_TYPE);

        DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
        for (auto k = 0; k < S_NUM_6; k++) {
            if (mon_to_player)
                count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_HI_UNDEAD, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));

            if (mon_to_mon)
                count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_HI_UNDEAD, (PM_ALLOW_GROUP | monster_u_mode(floor_ptr, m_idx)));
        }
    }

    if (player_ptr->blind && count && mon_to_player) {
        msg_print(_("間近で何か多くのものが這い回る音が聞こえる。", "You hear many creepy things appear nearby."));
    }

    if (monster_near_player(floor_ptr, m_idx, t_idx) && !see_monster(player_ptr, t_idx) && count && mon_to_mon)
        floor_ptr->monster_noise = true;

    auto res = MonsterSpellResult::make_valid();
    res.learnable = TARGET_TYPE == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_HI_DRAGONの処理。古代ドラゴン召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_HI_DRAGON(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sが魔法で古代ドラゴンを召喚した！", "%^s magically summons ancient dragons!"),
        _("%^sが魔法で古代ドラゴンを召喚した！", "%^s magically summons ancient dragons!"));

    monspell_message(player_ptr, m_idx, t_idx, msg, TARGET_TYPE);

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    bool mon_to_player = (TARGET_TYPE == MONSTER_TO_PLAYER);
    int count = 0;
    for (auto k = 0; k < S_NUM_4; k++) {
        if (mon_to_player)
            count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_HI_DRAGON, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));

        if (mon_to_mon)
            count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_HI_DRAGON, (PM_ALLOW_GROUP | monster_u_mode(floor_ptr, m_idx)));
    }

    if (player_ptr->blind && count && mon_to_player) {
        msg_print(_("多くの力強いものが間近に現れた音が聞こえる。", "You hear many powerful things appear nearby."));
    }

    if (monster_near_player(floor_ptr, m_idx, t_idx) && !see_monster(player_ptr, t_idx) && count && mon_to_mon)
        floor_ptr->monster_noise = true;

    auto res = MonsterSpellResult::make_valid();
    res.learnable = TARGET_TYPE == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_AMBERITESの処理。アンバーの王族召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_AMBERITES(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがアンバーの王族を召喚した！", "%^s magically summons Lords of Amber!"),
        _("%^sがアンバーの王族を召喚した！", "%^s magically summons Lords of Amber!"));

    monspell_message(player_ptr, m_idx, t_idx, msg, TARGET_TYPE);

    int count = 0;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    bool mon_to_player = (TARGET_TYPE == MONSTER_TO_PLAYER);
    for (auto k = 0; k < S_NUM_4; k++) {
        count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_AMBERITES, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));
    }

    if (player_ptr->blind && count && mon_to_player) {
        msg_print(_("不死の者が近くに現れるのが聞こえた。", "You hear immortal beings appear nearby."));
    }

    if (monster_near_player(floor_ptr, m_idx, t_idx) && !see_monster(player_ptr, t_idx) && count && mon_to_mon)
        floor_ptr->monster_noise = true;

    auto res = MonsterSpellResult::make_valid();
    res.learnable = TARGET_TYPE == MONSTER_TO_PLAYER;

    return res;
}

/*!
 * @brief RF6_S_UNIQUEの処理。ユニーク・モンスター召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_S_UNIQUE(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sが魔法で特別な強敵を召喚した！", "%^s magically summons special opponents!"),
        _("%^sが魔法で特別な強敵を召喚した！", "%^s magically summons special opponents!"));

    monspell_message(player_ptr, m_idx, t_idx, msg, TARGET_TYPE);

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    bool mon_to_player = (TARGET_TYPE == MONSTER_TO_PLAYER);
    bool uniques_are_summoned = false;
    int count = 0;
    for (auto k = 0; k < S_NUM_4; k++) {
        count += summon_specific(player_ptr, m_idx, y, x, rlev, SUMMON_UNIQUE, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));
    }

    if (count)
        uniques_are_summoned = true;

    summon_type non_unique_type = SUMMON_HI_UNDEAD;
    if ((m_ptr->sub_align & (SUB_ALIGN_GOOD | SUB_ALIGN_EVIL)) == (SUB_ALIGN_GOOD | SUB_ALIGN_EVIL))
        non_unique_type = SUMMON_NONE;
    else if (m_ptr->sub_align & SUB_ALIGN_GOOD)
        non_unique_type = SUMMON_ANGEL;

    for (auto k = count; k < S_NUM_4; k++) {
        count += summon_specific(player_ptr, m_idx, y, x, rlev, non_unique_type, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));
    }

    if (player_ptr->blind && count && mon_to_player) {
        msg_format(_("多くの%sが間近に現れた音が聞こえる。", "You hear many %s appear nearby."),
            uniques_are_summoned ? _("力強いもの", "powerful things") : _("もの", "things"));
    }

    if (monster_near_player(floor_ptr, m_idx, t_idx) && !see_monster(player_ptr, t_idx) && count && mon_to_mon)
        floor_ptr->monster_noise = true;

    auto res = MonsterSpellResult::make_valid();
    res.learnable = TARGET_TYPE == MONSTER_TO_PLAYER;

    return res;
}
