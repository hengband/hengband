/*!
 * @brief 特殊な行動を取るモンスターの具体的な行動定義 (MonsterRaceDefinitionsのSPECIALフラグ)
 * @date 2020/05/16
 * @author Hourier
 */

#include "mspell/mspell-special.h"
#include "core/disturbance.h"
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
#include "system/angband-system.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"
#include <algorithm>
#include <sstream>

/*!
 * @brief ユニークの分離・合体処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 */
static MonsterSpellResult spell_RF6_SPECIAL_UNIFICATION(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    auto dummy_y = m_ptr->fy;
    auto dummy_x = m_ptr->fx;
    if (see_monster(player_ptr, m_idx) && monster_near_player(floor_ptr, m_idx, 0)) {
        disturb(player_ptr, true, true);
    }

    const auto &monraces = MonraceList::get_instance();
    const auto &unified_uniques = MonraceList::get_unified_uniques();
    if (const auto it_unified = unified_uniques.find(m_ptr->r_idx); it_unified != unified_uniques.end()) {
        const int separates_size = it_unified->second.size();
        const auto separated_hp = (m_ptr->hp + 1) / separates_size;
        const auto separated_maxhp = m_ptr->maxhp / separates_size;
        if (floor_ptr->inside_arena || AngbandSystem::get_instance().is_phase_out() || !summon_possible(player_ptr, m_ptr->fy, m_ptr->fx)) {
            return MonsterSpellResult::make_invalid();
        }

        delete_monster_idx(player_ptr, floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].m_idx);
        for (const auto separate : it_unified->second) {
            summon_named_creature(player_ptr, 0, dummy_y, dummy_x, separate, MD_NONE);
            floor_ptr->m_list[hack_m_idx_ii].hp = separated_hp;
            floor_ptr->m_list[hack_m_idx_ii].maxhp = separated_maxhp;
        }

        const auto &m_name = monraces[it_unified->first].name;
        msg_format(_("%sが分離した！", "%s splits!"), m_name.data());
        return MonsterSpellResult::make_valid();
    }

    for (const auto &[unified_unique, separates] : unified_uniques) {
        if (!separates.contains(m_ptr->r_idx)) {
            continue;
        }

        if (!monraces.exists_separates(unified_unique)) {
            return MonsterSpellResult::make_invalid();
        }

        auto unified_hp = 0;
        auto unified_maxhp = 0;
        for (short k = 1; k < floor_ptr->m_max; k++) {
            const auto &monster = floor_ptr->m_list[k];
            if (!separates.contains(monster.r_idx)) {
                continue;
            }

            unified_hp += monster.hp;
            unified_maxhp += monster.maxhp;
            if (monster.r_idx != m_ptr->r_idx) {
                dummy_y = monster.fy;
                dummy_x = monster.fx;
            }

            delete_monster_idx(player_ptr, k);
        }

        summon_named_creature(player_ptr, 0, dummy_y, dummy_x, unified_unique, MD_NONE);
        floor_ptr->m_list[hack_m_idx_ii].hp = unified_hp;
        floor_ptr->m_list[hack_m_idx_ii].maxhp = unified_maxhp;
        std::vector<std::string> m_names;
        for (const auto &separate : separates) {
            const auto &monrace = monraces[separate];
            m_names.push_back(monrace.name);
        }

        std::stringstream ss;
        ss << *m_names.begin();
        for (size_t i = 1; i < m_names.size(); i++) { // @todo clang v14 はstd::views::drop() 非対応
            const auto &m_name = m_names[i];
            ss << _("と", " and ");
            ss << m_name;
        }

        const auto fmt = _("%sが合体した！", "%s combine into one!");
        msg_print(format(fmt, ss.str().data()));
        return MonsterSpellResult::make_valid();
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
    MonsterRaceInfo *tr_ptr = &t_ptr->get_monrace();
    bool monster_to_player = (target_type == MONSTER_TO_PLAYER);
    bool monster_to_monster = (target_type == MONSTER_TO_MONSTER);
    bool direct = player_ptr->is_located_at({ y, x });
    const auto m_name = monster_name(player_ptr, m_idx);

    disturb(player_ptr, true, true);
    if (one_in_(3) || !direct) {
        msg.to_player = _("%s^は突然視界から消えた!", "You lose sight of %s!");
        msg.to_mons = _("%s^は突然急上昇して視界から消えた!", "You lose sight of %s!");

        simple_monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

        teleport_away(player_ptr, m_idx, 10, TELEPORT_NONMAGICAL);
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
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
        int get_damage = take_hit(player_ptr, DAMAGE_NOESCAPE, dam, m_name);
        if (player_ptr->tim_eyeeye && get_damage > 0 && !player_ptr->is_dead) {
            const auto m_name_self = monster_desc(player_ptr, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);
            msg_print(_(format("攻撃が%s自身を傷つけた！", m_name.data()), format("The attack of %s has wounded %s!", m_name.data(), m_name_self.data())));
            project(player_ptr, 0, 0, m_ptr->fy, m_ptr->fx, get_damage, AttributeType::MISSILE, PROJECT_KILL);
            set_tim_eyeeye(player_ptr, player_ptr->tim_eyeeye - 5, true);
        }
    }

    if (monster_to_player && player_ptr->riding) {
        const auto &m_ref = floor_ptr->m_list[player_ptr->riding];
        mon_take_hit_mon(player_ptr, player_ptr->riding, dam, &dead, &fear, m_ref.get_died_message(), m_idx);
    }

    if (monster_to_monster) {
        mon_take_hit_mon(player_ptr, t_idx, dam, &dead, &fear, t_ptr->get_died_message(), m_idx);
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
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[m_idx];
    const auto &monrace = monster.get_monrace();
    const auto r_idx = monster.r_idx;
    if (MonraceList::get_instance().can_unify_separate(r_idx)) {
        return spell_RF6_SPECIAL_UNIFICATION(player_ptr, m_idx);
    }

    switch (r_idx) {
    case MonsterRaceId::OHMU:
        return MonsterSpellResult::make_invalid();
    case MonsterRaceId::ROLENTO:
        return spell_RF6_SPECIAL_ROLENTO(player_ptr, y, x, m_idx, t_idx, target_type);
    default:
        if (monrace.d_char == 'B') {
            return spell_RF6_SPECIAL_B(player_ptr, y, x, m_idx, t_idx, target_type);
        }

        return MonsterSpellResult::make_invalid();
    }
}
