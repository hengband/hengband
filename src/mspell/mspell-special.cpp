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
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "melee/melee-postprocess.h"
#include "monster-floor/monster-death.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-summon.h"
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
#include "system/deferred-update-requests.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"
#include <algorithm>
#include <range/v3/view.hpp>
#include <sstream>

/*!
 * @brief ユニークの分離・合体処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 */
static MonsterSpellResult spell_RF6_SPECIAL_UNIFICATION(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[m_idx];
    auto dummy_y = monster.fy;
    auto dummy_x = monster.fx;
    if (see_monster(player_ptr, m_idx) && monster_near_player(floor, m_idx, 0)) {
        disturb(player_ptr, true, true);
    }

    const auto &monraces = MonraceList::get_instance();
    const auto &unified_uniques = MonraceList::get_unified_uniques();
    if (const auto it_unified = unified_uniques.find(monster.r_idx); it_unified != unified_uniques.end()) {
        const int separates_size = it_unified->second.size();
        const auto separated_hp = (monster.hp + 1) / separates_size;
        const auto separated_maxhp = monster.maxhp / separates_size;
        if (floor.inside_arena || AngbandSystem::get_instance().is_phase_out() || !summon_possible(player_ptr, monster.fy, monster.fx)) {
            return MonsterSpellResult::make_invalid();
        }

        delete_monster_idx(player_ptr, floor.grid_array[monster.fy][monster.fx].m_idx);
        for (const auto separate : it_unified->second) {
            auto summoned_m_idx = summon_named_creature(player_ptr, 0, dummy_y, dummy_x, separate, MD_NONE);
            if (!summoned_m_idx) {
                continue;
            }

            floor.m_list[*summoned_m_idx].hp = separated_hp;
            floor.m_list[*summoned_m_idx].maxhp = separated_maxhp;
        }

        const auto &m_name = monraces.get_monrace(it_unified->first).name;
        msg_print(_("{}が分離した！", "{} splits!"), m_name);
        return MonsterSpellResult::make_valid();
    }

    for (const auto &[unified_unique, separates] : unified_uniques) {
        if (!separates.contains(monster.r_idx)) {
            continue;
        }

        if (!monraces.exists_separates(unified_unique)) {
            return MonsterSpellResult::make_invalid();
        }

        auto unified_hp = 0;
        auto unified_maxhp = 0;
        for (short k = 1; k < floor.m_max; k++) {
            const auto &monster_separate = floor.m_list[k];
            if (!separates.contains(monster_separate.r_idx)) {
                continue;
            }

            unified_hp += monster_separate.hp;
            unified_maxhp += monster_separate.maxhp;
            if (monster_separate.r_idx != monster.r_idx) {
                dummy_y = monster_separate.fy;
                dummy_x = monster_separate.fx;
            }

            delete_monster_idx(player_ptr, k);
        }

        if (auto summoned_m_idx = summon_named_creature(player_ptr, 0, dummy_y, dummy_x, unified_unique, MD_NONE)) {
            floor.m_list[*summoned_m_idx].hp = unified_hp;
            floor.m_list[*summoned_m_idx].maxhp = unified_maxhp;
        }

        using namespace std::string_view_literals;
        const auto monrace_id_to_name = [&monraces](auto id) { return monraces.get_monrace(id).name.string(); };
        const auto separates_names =
            separates |
            ranges::views::transform(monrace_id_to_name) |
            ranges::views::join(_("と"sv, " and "sv)) |
            ranges::to<std::string>;
        msg_print(_("{}が合体した！", "{} combine into one!"), separates_names);
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
    const auto &floor = *player_ptr->current_floor_ptr;
    bool see_either = see_monster(player_ptr, m_idx) || see_monster(player_ptr, t_idx);
    bool mon_to_mon = target_type == MONSTER_TO_MONSTER;
    bool mon_to_player = target_type == MONSTER_TO_PLAYER;
    bool known = monster_near_player(floor, m_idx, t_idx);

    mspell_cast_msg_blind msg(_("{}が何か大量に投げた。", "{} spreads something."),
        _("{}は手榴弾をばらまいた。", "{} throws some hand grenades."), _("{}は手榴弾をばらまいた。", "{} throws some hand grenades."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);
    if (mon_to_player || (mon_to_mon && known && see_either)) {
        disturb(player_ptr, true, true);
    }

    for (k = 0; k < num; k++) {
        if (summon_named_creature(player_ptr, m_idx, y, x, MonraceId::GRENADE, mode)) {
            count++;
        }
    }
    if (player_ptr->effects()->blindness().is_active() && count) {
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
    mspell_cast_msg_simple msg{};
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster = floor.m_list[m_idx];
    const auto &monster_target = floor.m_list[t_idx];
    const auto &monrace_target = monster_target.get_monrace();
    bool monster_to_player = (target_type == MONSTER_TO_PLAYER);
    bool monster_to_monster = (target_type == MONSTER_TO_MONSTER);
    bool direct = player_ptr->is_located_at({ y, x });
    const auto m_name = monster_name(player_ptr, m_idx);

    disturb(player_ptr, true, true);
    // プレイヤーを狙ったのに、プレイヤーがその地点にいなければ掴めない (モンスターを狙ったときは掴める)
    if (one_in_(3) || (monster_to_player && !direct)) {
        msg.to_player = _("{}は突然視界から消えた!", "You lose sight of {2}!");
        msg.to_mons = _("{}は突然急上昇して視界から消えた!", "You lose sight of {2}!");

        simple_monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

        teleport_away(player_ptr, m_idx, 10, TELEPORT_NONMAGICAL);
        DeferredUpdateRequests::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
        return MonsterSpellResult::make_valid();
    }

    msg.to_player = _("{}があなたを掴んで空中から投げ落とした。", "{} snatches you, soars into the sky, and drops you.");
    msg.to_mons = _("{}が{}を掴んで空中から投げ落とした。", "{} snatches {}, soars into the sky, and releases its grip.");

    // 音は、プレイヤー (乗っている馬を含む) が掴まれたときと、モンスター同士で掴んだメッセージを出したときだけ出す。
    // プレイヤーが狙われたときはメッセージを必ず出すが、乗っている馬が狙われたときは見えないと出さないので direct も見る
    const auto is_noticed = simple_monspell_message(player_ptr, m_idx, t_idx, msg, target_type);
    if (direct || is_noticed) {
        sound(SoundKind::FALL);
    }

    bool fear, dead; /* dummy */
    int dam = Dice::roll(4, 8);

    if (monster_to_player || monster_target.is_riding()) {
        teleport_player_to(player_ptr, monster.fy, monster.fx, i2enum<teleport_flags>(TELEPORT_NONMAGICAL | TELEPORT_PASSIVE));
    } else {
        teleport_monster_to(player_ptr, t_idx, monster.fy, monster.fx, 100, i2enum<teleport_flags>(TELEPORT_NONMAGICAL | TELEPORT_PASSIVE));
    }

    if ((monster_to_player && player_ptr->levitation) || (monster_to_monster && monrace_target.feature_flags.has(MonsterFeatureType::CAN_FLY))) {
        msg.to_player = _("あなたは静かに着地した。", "You float gently down to the ground.");
        msg.to_mons = _("{4}は静かに着地した。", "{4} floats gently down to the ground.");
    } else {
        msg.to_player = _("あなたは地面に叩きつけられた。", "You crashed into the ground.");
        msg.to_mons = _("{4}は地面に叩きつけられた。", "{4} crashed into the ground.");
        dam += Dice::roll(6, 8);
    }

    simple_monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    if (monster_to_player || (monster_to_monster && monster_target.is_riding())) {
        int get_damage = take_hit(player_ptr, DAMAGE_NOESCAPE, dam, m_name);
        if (player_ptr->tim_eyeeye && get_damage > 0 && !player_ptr->is_dead) {
            const auto m_name_self = monster_desc(player_ptr, monster, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);
            msg_print(_("攻撃が{}自身を傷つけた！", "The attack of {} has wounded {}!"), m_name, m_name_self);
            project(player_ptr, 0, 0, monster.fy, monster.fx, get_damage, AttributeType::MISSILE, PROJECT_KILL);
            set_tim_eyeeye(player_ptr, player_ptr->tim_eyeeye - 5, true);
        }
    }

    if (monster_to_player && player_ptr->riding) {
        const auto &m_ref = floor.m_list[player_ptr->riding];
        mon_take_hit_mon(player_ptr, player_ptr->riding, dam, &dead, &fear, m_ref.get_died_message(), m_idx);
    }

    if (monster_to_monster) {
        mon_take_hit_mon(player_ptr, t_idx, dam, &dead, &fear, monster_target.get_died_message(), m_idx);
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
    case MonraceId::OHMU:
        return MonsterSpellResult::make_invalid();
    case MonraceId::ROLENTO:
        return spell_RF6_SPECIAL_ROLENTO(player_ptr, y, x, m_idx, t_idx, target_type);
    default:
        if (monrace.symbol_char_is_any_of("B")) {
            return spell_RF6_SPECIAL_B(player_ptr, y, x, m_idx, t_idx, target_type);
        }

        return MonsterSpellResult::make_invalid();
    }
}
