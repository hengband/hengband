﻿/*!
 * @brief 特殊な行動を取るモンスターの具体的な行動定義 (r_infoのSPECIALフラグ)
 * @date 2020/05/16
 * @author Hourier
 */

#include "mspell/mspell-special.h"
#include "core/disturbance.h"
#include "core/player-update-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "grid/grid.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "melee/melee-postprocess.h"
#include "monster-floor/monster-death.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-summon.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-util.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-util.h"
#include "player/player-damage.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-crusade.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief バーノール・ルパートのRF6_SPECIALの処理。分裂・合体。 /
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 */
HIT_POINT spell_RF6_SPECIAL_BANORLUPART(player_type *target_ptr, MONSTER_IDX m_idx)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    HIT_POINT dummy_hp, dummy_maxhp;
    POSITION dummy_y = m_ptr->fy;
    POSITION dummy_x = m_ptr->fx;
    BIT_FLAGS mode = 0L;

    switch (m_ptr->r_idx) {
    case MON_BANORLUPART:
        dummy_hp = (m_ptr->hp + 1) / 2;
        dummy_maxhp = m_ptr->maxhp / 2;

        if (floor_ptr->inside_arena || target_ptr->phase_out || !summon_possible(target_ptr, m_ptr->fy, m_ptr->fx))
            return -1;

        delete_monster_idx(target_ptr, floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].m_idx);
        summon_named_creature(target_ptr, 0, dummy_y, dummy_x, MON_BANOR, mode);
        floor_ptr->m_list[hack_m_idx_ii].hp = dummy_hp;
        floor_ptr->m_list[hack_m_idx_ii].maxhp = dummy_maxhp;
        summon_named_creature(target_ptr, 0, dummy_y, dummy_x, MON_LUPART, mode);
        floor_ptr->m_list[hack_m_idx_ii].hp = dummy_hp;
        floor_ptr->m_list[hack_m_idx_ii].maxhp = dummy_maxhp;

        msg_print(_("『バーノール・ルパート』が分裂した！", "Banor=Rupart splits into two persons!"));
        break;

    case MON_BANOR:
    case MON_LUPART:
        dummy_hp = 0;
        dummy_maxhp = 0;

        if (!r_info[MON_BANOR].cur_num || !r_info[MON_LUPART].cur_num)
            return -1;

        for (MONSTER_IDX k = 1; k < floor_ptr->m_max; k++) {
            if (floor_ptr->m_list[k].r_idx == MON_BANOR || floor_ptr->m_list[k].r_idx == MON_LUPART) {
                dummy_hp += floor_ptr->m_list[k].hp;
                dummy_maxhp += floor_ptr->m_list[k].maxhp;
                if (floor_ptr->m_list[k].r_idx != m_ptr->r_idx) {
                    dummy_y = floor_ptr->m_list[k].fy;
                    dummy_x = floor_ptr->m_list[k].fx;
                }
                delete_monster_idx(target_ptr, k);
            }
        }
        summon_named_creature(target_ptr, 0, dummy_y, dummy_x, MON_BANORLUPART, mode);
        floor_ptr->m_list[hack_m_idx_ii].hp = dummy_hp;
        floor_ptr->m_list[hack_m_idx_ii].maxhp = dummy_maxhp;

        msg_print(_("『バーノール』と『ルパート』が合体した！", "Banor and Rupart combine into one!"));
        break;
    }

    return 0;
}

/*!
 * @brief ロレントのRF6_SPECIALの処理。手榴弾の召喚。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF6_SPECIAL_ROLENTO(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    int count = 0, k;
    int num = 1 + randint1(3);
    BIT_FLAGS mode = 0L;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何か大量に投げた。", "%^s spreads something."),
        _("%^sは手榴弾をばらまいた。", "%^s throws some hand grenades."), _("%^sは手榴弾をばらまいた。", "%^s throws some hand grenades."), TARGET_TYPE);

    for (k = 0; k < num; k++) {
        count += summon_named_creature(target_ptr, m_idx, y, x, MON_GRENADE, mode);
    }
    if (target_ptr->blind && count) {
        msg_print(_("多くのものが間近にばらまかれる音がする。", "You hear many things scattered nearby."));
    }
    return 0;
}

/*!
 * @brief BシンボルのRF6_SPECIALの処理。投げ落とす攻撃。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF6_SPECIAL_B(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    monster_type *t_ptr = &floor_ptr->m_list[t_idx];
    monster_race *tr_ptr = &r_info[t_ptr->r_idx];
    bool monster_to_player = (TARGET_TYPE == MONSTER_TO_PLAYER);
    bool monster_to_monster = (TARGET_TYPE == MONSTER_TO_MONSTER);
    bool direct = player_bold(target_ptr, y, x);
    GAME_TEXT m_name[MAX_NLEN];
    monster_name(target_ptr, m_idx, m_name);

    disturb(target_ptr, TRUE, TRUE);
    if (one_in_(3) || !direct) {
        simple_monspell_message(target_ptr, m_idx, t_idx, _("%^sは突然視界から消えた!", "You lose sight of %s!"),
            _("%^sは突然急上昇して視界から消えた!", "You lose sight of %s!"), TARGET_TYPE);

        teleport_away(target_ptr, m_idx, 10, TELEPORT_NONMAGICAL);
        target_ptr->update |= (PU_MONSTERS);
        return 0;
    }


    simple_monspell_message(target_ptr, m_idx, t_idx, _("%^sがあなたを掴んで空中から投げ落とした。", "%^s snatches you, soars into the sky, and drops you."),
        _("%^sが%sを掴んで空中から投げ落とした。", "%^s snatches %s, soars into the sky, and releases its grip."), TARGET_TYPE);

    bool fear, dead; /* dummy */
    HIT_POINT dam = damroll(4, 8);

    if (monster_to_player || t_idx == target_ptr->riding)
        teleport_player_to(target_ptr, m_ptr->fy, m_ptr->fx, static_cast<teleport_flags>(TELEPORT_NONMAGICAL | TELEPORT_PASSIVE));
    else
        teleport_monster_to(target_ptr, t_idx, m_ptr->fy, m_ptr->fx, 100, static_cast<teleport_flags>(TELEPORT_NONMAGICAL | TELEPORT_PASSIVE));

    sound(SOUND_FALL);

    if ((monster_to_player && target_ptr->levitation) || (monster_to_monster && (tr_ptr->flags7 & RF7_CAN_FLY))) {
        simple_monspell_message(target_ptr, m_idx, t_idx, _("あなたは静かに着地した。", "You float gently down to the ground."),
            _("%^sは静かに着地した。", "%^s floats gently down to the ground."), TARGET_TYPE);
    } else {
        simple_monspell_message(target_ptr, m_idx, t_idx, _("あなたは地面に叩きつけられた。", "You crashed into the ground."),
            _("%^sは地面に叩きつけられた。", "%^s crashed into the ground."), TARGET_TYPE);
        dam += damroll(6, 8);
    }

    if (monster_to_player || (monster_to_monster && target_ptr->riding == t_idx)) {
        int get_damage = take_hit(target_ptr, DAMAGE_NOESCAPE, dam, m_name, -1);
        if (target_ptr->tim_eyeeye && get_damage > 0 && !target_ptr->is_dead) {
            GAME_TEXT m_name_self[MAX_MONSTER_NAME];
            monster_desc(target_ptr, m_name_self, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);
            msg_format(_("攻撃が%s自身を傷つけた！", "The attack of %s has wounded %s!"), m_name, m_name_self);
            project(target_ptr, 0, 0, m_ptr->fy, m_ptr->fx, get_damage, GF_MISSILE, PROJECT_KILL, -1);
            set_tim_eyeeye(target_ptr, target_ptr->tim_eyeeye - 5, TRUE);
        }
    }

    if (monster_to_player && target_ptr->riding)
        mon_take_hit_mon(target_ptr, target_ptr->riding, dam, &dead, &fear, extract_note_dies(real_r_idx(&floor_ptr->m_list[target_ptr->riding])), m_idx);

    if (monster_to_monster)
        mon_take_hit_mon(target_ptr, t_idx, dam, &dead, &fear, extract_note_dies(real_r_idx(t_ptr)), m_idx);
    return dam;
}

/*!
 * @brief RF6_SPECIALの処理。モンスターの種類によって実処理に振り分ける。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF6_SPECIAL(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    disturb(target_ptr, TRUE, TRUE);
    switch (m_ptr->r_idx) {
    case MON_OHMU:
        return -1;

    case MON_BANORLUPART:
    case MON_BANOR:
    case MON_LUPART:
        return spell_RF6_SPECIAL_BANORLUPART(target_ptr, m_idx);

    case MON_ROLENTO:
        return spell_RF6_SPECIAL_ROLENTO(target_ptr, y, x, m_idx, t_idx, TARGET_TYPE);
        break;

    default:
        if (r_ptr->d_char == 'B') {
            return spell_RF6_SPECIAL_B(target_ptr, y, x, m_idx, t_idx, TARGET_TYPE);
            break;
        }

        else
            return -1;
    }
}
