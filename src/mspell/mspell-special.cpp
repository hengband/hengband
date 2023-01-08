/*!
 * @brief 特殊な行動を取るモンスターの具体的な行動定義 (MonsterRaceDefinitionsのSPECIALフラグ)
 * @date 2020/05/16
 * @author Hourier
 */

#include "mspell/mspell-special.h"
#include "core/disturbance.h"
#include "core/player-update-types.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
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
#include "mspell/mspell-result.h"
#include "mspell/mspell-util.h"
#include "player/player-damage.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-crusade.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

/*!
 * @brief バーノール・ルパートのRF6_SPECIALの処理。分裂・合体。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 */
static MonsterSpellResult spell_RF6_SPECIAL_BANORLUPART(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    int dummy_hp, dummy_maxhp;
    POSITION dummy_y = m_ptr->fy;
    POSITION dummy_x = m_ptr->fx;
    BIT_FLAGS mode = 0L;

    if (see_monster(player_ptr, m_idx) && monster_near_player(floor_ptr, m_idx, 0)) {
        disturb(player_ptr, true, true);
    }

    switch (m_ptr->r_idx) {
    case MonsterRaceId::BANORLUPART:
        dummy_hp = (m_ptr->hp + 1) / 2;
        dummy_maxhp = m_ptr->maxhp / 2;

        if (floor_ptr->inside_arena || player_ptr->phase_out || !summon_possible(player_ptr, m_ptr->fy, m_ptr->fx)) {
            return MonsterSpellResult::make_invalid();
        }

        delete_monster_idx(player_ptr, floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].m_idx);
        summon_named_creature(player_ptr, 0, dummy_y, dummy_x, MonsterRaceId::BANOR, mode);
        floor_ptr->m_list[hack_m_idx_ii].hp = dummy_hp;
        floor_ptr->m_list[hack_m_idx_ii].maxhp = dummy_maxhp;
        summon_named_creature(player_ptr, 0, dummy_y, dummy_x, MonsterRaceId::LUPART, mode);
        floor_ptr->m_list[hack_m_idx_ii].hp = dummy_hp;
        floor_ptr->m_list[hack_m_idx_ii].maxhp = dummy_maxhp;

        msg_print(_("『バーノール・ルパート』が分裂した！", "Banor=Rupart splits into two persons!"));
        break;

    case MonsterRaceId::BANOR:
    case MonsterRaceId::LUPART:
        dummy_hp = 0;
        dummy_maxhp = 0;

        if (!monraces_info[MonsterRaceId::BANOR].cur_num || !monraces_info[MonsterRaceId::LUPART].cur_num) {
            return MonsterSpellResult::make_invalid();
        }

        for (MONSTER_IDX k = 1; k < floor_ptr->m_max; k++) {
            if (floor_ptr->m_list[k].r_idx == MonsterRaceId::BANOR || floor_ptr->m_list[k].r_idx == MonsterRaceId::LUPART) {
                dummy_hp += floor_ptr->m_list[k].hp;
                dummy_maxhp += floor_ptr->m_list[k].maxhp;
                if (floor_ptr->m_list[k].r_idx != m_ptr->r_idx) {
                    dummy_y = floor_ptr->m_list[k].fy;
                    dummy_x = floor_ptr->m_list[k].fx;
                }
                delete_monster_idx(player_ptr, k);
            }
        }
        summon_named_creature(player_ptr, 0, dummy_y, dummy_x, MonsterRaceId::BANORLUPART, mode);
        floor_ptr->m_list[hack_m_idx_ii].hp = dummy_hp;
        floor_ptr->m_list[hack_m_idx_ii].maxhp = dummy_maxhp;

        msg_print(_("『バーノール』と『ルパート』が合体した！", "Banor and Rupart combine into one!"));
        break;

    default:
        break;
    }

    return MonsterSpellResult::make_valid();
}

/*!
 * @brief ロレントのRF6_SPECIALの処理。手榴弾の召喚。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
static MonsterSpellResult spell_RF6_SPECIAL_ROLENTO(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    int count = 0, k;
    int num = 1 + randint1(3);
    BIT_FLAGS mode = 0L;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    bool see_either = see_monster(player_ptr, m_idx) || see_monster(player_ptr, t_idx);
    bool mon_to_mon = target_type == MONSTER_TO_MONSTER;
    bool mon_to_player = target_type == MONSTER_TO_PLAYER;
    bool known = monster_near_player(floor_ptr, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("%s^が何か大量に投げた。", "%s^ spreads something."),
        _("%s^は手榴弾をばらまいた。", "%s^ throws some hand grenades."), _("%s^は手榴弾をばらまいた。", "%s^ throws some hand grenades."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);
    if (mon_to_player || (mon_to_mon && known && see_either)) {
        disturb(player_ptr, true, true);
    }

    for (k = 0; k < num; k++) {
        count += summon_named_creature(player_ptr, m_idx, y, x, MonsterRaceId::GRENADE, mode);
    }
    if (player_ptr->effects()->blindness()->is_blind() && count) {
        msg_print(_("多くのものが間近にばらまかれる音がする。", "You hear many things scattered nearby."));
    }

    return MonsterSpellResult::make_valid();
}

/*!
 * @brief BシンボルのRF6_SPECIALの処理。投げ落とす攻撃。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
static MonsterSpellResult spell_RF6_SPECIAL_B(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_simple msg;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    MonsterEntity *t_ptr = &floor_ptr->m_list[t_idx];
    MonsterRaceInfo *tr_ptr = &monraces_info[t_ptr->r_idx];
    bool monster_to_player = (target_type == MONSTER_TO_PLAYER);
    bool monster_to_monster = (target_type == MONSTER_TO_MONSTER);
    bool direct = player_bold(player_ptr, y, x);
    const auto m_name = monster_name(player_ptr, m_idx);

    disturb(player_ptr, true, true);
    if (one_in_(3) || !direct) {
        msg.to_player = _("%s^は突然視界から消えた!", "You lose sight of %s!");
        msg.to_mons = _("%s^は突然急上昇して視界から消えた!", "You lose sight of %s!");

        simple_monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

        teleport_away(player_ptr, m_idx, 10, TELEPORT_NONMAGICAL);
        player_ptr->update |= (PU_MONSTERS);
        return MonsterSpellResult::make_valid();
    }

    if (direct) {
        sound(SOUND_FALL);
    }

    msg.to_player = _("%s^があなたを掴んで空中から投げ落とした。", "%s^ snatches you, soars into the sky, and drops you.");
    msg.to_mons = _("%s^が%sを掴んで空中から投げ落とした。", "%s^ snatches %s, soars into the sky, and releases its grip.");

    simple_monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    bool fear, dead; /* dummy */
    int dam = damroll(4, 8);

    if (monster_to_player || t_idx == player_ptr->riding) {
        teleport_player_to(player_ptr, m_ptr->fy, m_ptr->fx, i2enum<teleport_flags>(TELEPORT_NONMAGICAL | TELEPORT_PASSIVE));
    } else {
        teleport_monster_to(player_ptr, t_idx, m_ptr->fy, m_ptr->fx, 100, i2enum<teleport_flags>(TELEPORT_NONMAGICAL | TELEPORT_PASSIVE));
    }

    if ((monster_to_player && player_ptr->levitation) || (monster_to_monster && tr_ptr->feature_flags.has(MonsterFeatureType::CAN_FLY))) {
        msg.to_player = _("あなたは静かに着地した。", "You float gently down to the ground.");
        msg.to_mons = _("%s^は静かに着地した。", "%s^ floats gently down to the ground.");
    } else {
        msg.to_player = _("あなたは地面に叩きつけられた。", "You crashed into the ground.");
        msg.to_mons = _("%s^は地面に叩きつけられた。", "%s^ crashed into the ground.");
    }

    simple_monspell_message(player_ptr, m_idx, t_idx, msg, target_type);
    dam += damroll(6, 8);

    if (monster_to_player || (monster_to_monster && player_ptr->riding == t_idx)) {
        int get_damage = take_hit(player_ptr, DAMAGE_NOESCAPE, dam, m_name.data());
        if (player_ptr->tim_eyeeye && get_damage > 0 && !player_ptr->is_dead) {
            const auto m_name_self = monster_desc(player_ptr, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);
            msg_print(_(format("攻撃が%s自身を傷つけた！", m_name.data()), format("The attack of %s has wounded %s!", m_name.data(), m_name_self.data())));
            project(player_ptr, 0, 0, m_ptr->fy, m_ptr->fx, get_damage, AttributeType::MISSILE, PROJECT_KILL);
            set_tim_eyeeye(player_ptr, player_ptr->tim_eyeeye - 5, true);
        }
    }

    if (monster_to_player && player_ptr->riding) {
        const auto &m_ref = floor_ptr->m_list[player_ptr->riding];
        mon_take_hit_mon(player_ptr, player_ptr->riding, dam, &dead, &fear, extract_note_dies(m_ref.get_real_r_idx()), m_idx);
    }

    if (monster_to_monster) {
        mon_take_hit_mon(player_ptr, t_idx, dam, &dead, &fear, extract_note_dies(t_ptr->get_real_r_idx()), m_idx);
    }

    return MonsterSpellResult::make_valid();
}

/*!
 * @brief RF6_SPECIALの処理。モンスターの種類によって実処理に振り分ける。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * ラーニング不可。
 */
MonsterSpellResult spell_RF6_SPECIAL(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];

    switch (m_ptr->r_idx) {
    case MonsterRaceId::OHMU:
        return MonsterSpellResult::make_invalid();

    case MonsterRaceId::BANORLUPART:
    case MonsterRaceId::BANOR:
    case MonsterRaceId::LUPART:
        return spell_RF6_SPECIAL_BANORLUPART(player_ptr, m_idx);

    case MonsterRaceId::ROLENTO:
        return spell_RF6_SPECIAL_ROLENTO(player_ptr, y, x, m_idx, t_idx, target_type);
        break;

    default:
        if (r_ptr->d_char == 'B') {
            return spell_RF6_SPECIAL_B(player_ptr, y, x, m_idx, t_idx, target_type);
            break;
        } else {
            return MonsterSpellResult::make_invalid();
        }
    }
}
