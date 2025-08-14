/*!
 * @brief モンスターの逃走に関する処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster-floor/monster-runaway.h"
#include "core/disturbance.h"
#include "dungeon/quest-completion-checker.h"
#include "grid/grid.h"
#include "monster-floor/monster-remover.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-processor-util.h"
#include "pet/pet-fall-off.h"
#include "system/angband-system.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief monspeak.txt において、発話ではなく行動のみが描写されているモンスターであるかを調べる
 * @param r_idx モンスター種族ID
 * @return 会話内容が行動のみのモンスターか否か
 */
static bool is_acting_monster(const MonraceId r_idx)
{
    auto is_acting_monster = r_idx == MonraceId::GRIP;
    is_acting_monster |= r_idx == MonraceId::WOLF;
    is_acting_monster |= r_idx == MonraceId::FANG;
    return is_acting_monster;
}

/*!
 * @brief HPが1/3未満になった友好的なユニークモンスターの逃走処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param is_riding_mon 騎乗状態ならばTRUE
 * @param m_ptr モンスターへの参照ポインタ
 * @param m_name モンスター名称
 */
static void escape_monster(PlayerType *player_ptr, turn_flags *turn_flags_ptr, const MonsterEntity &monster, concptr m_name)
{
    if (turn_flags_ptr->is_riding_mon) {
        msg_format(_("%sはあなたの束縛から脱出した。", "%s^ succeeded to escape from your restriction!"), m_name);
        if (process_fall_off_horse(player_ptr, -1, false)) {
            msg_print(_("地面に落とされた。", "You have fallen from the pet you were riding."));
        }
    }

    if (turn_flags_ptr->see_m) {
        static constexpr auto flags = {
            MonsterSpeakType::SPEAK_ALL,
            MonsterSpeakType::SPEAK_BATTLE,
            MonsterSpeakType::SPEAK_FEAR,
        };

        auto speak = monster.get_monrace().speak_flags.has_any_of(flags);
        speak &= !is_acting_monster(monster.r_idx);
        const auto &floor = *player_ptr->current_floor_ptr;
        const auto p_pos = player_ptr->get_position();
        const auto m_pos = monster.get_position();
        speak &= player_ptr->current_floor_ptr->has_los_at(m_pos);
        speak &= projectable(floor, m_pos, p_pos);
        if (speak) {
            msg_format(_("%s^「ピンチだ！退却させてもらう！」", "%s^ says 'It is the pinch! I will retreat'."), m_name);
        }

        msg_format(_("%s^がテレポート・レベルの巻物を読んだ。", "%s^ reads a scroll of teleport level."), m_name);
        msg_format(_("%s^が消え去った。", "%s^ disappears."), m_name);
    }

    if (turn_flags_ptr->is_riding_mon && process_fall_off_horse(player_ptr, -1, false)) {
        msg_print(_("地面に落とされた。", "You have fallen from the pet you were riding."));
    }
}

/*!
 * @brief ペットや友好的なモンスターがフロアから逃げる処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param is_riding_mon 騎乗状態ならばTRUE
 * @param see_m モンスターが視界内にいたらTRUE
 * @return モンスターがフロアから消えたらTRUE
 */
bool runaway_monster(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx)
{
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    const auto &monrace = monster.get_monrace();
    bool can_runaway = monster.is_pet() || monster.is_friendly();
    can_runaway &= (monrace.kind_flags.has(MonsterKindType::UNIQUE)) || (monrace.population_flags.has(MonsterPopulationType::NAZGUL));
    can_runaway &= !AngbandSystem::get_instance().is_phase_out();
    if (!can_runaway) {
        return false;
    }

    static int riding_pinch = 0;

    if (monster.hp >= monster.maxhp / 3) {
        /* Reset the counter */
        if (turn_flags_ptr->is_riding_mon) {
            riding_pinch = 0;
        }

        return false;
    }

    const auto m_name = monster_desc(player_ptr, monster, 0);
    if (turn_flags_ptr->is_riding_mon && riding_pinch < 2) {
        msg_format(
            _("%sは傷の痛さの余りあなたの束縛から逃れようとしている。", "%s^ seems to be in so much pain and tries to escape from your restriction."), m_name.data());
        riding_pinch++;
        disturb(player_ptr, true, true);
        return false;
    }

    escape_monster(player_ptr, turn_flags_ptr, monster, m_name.data());
    QuestCompletionChecker(player_ptr, monster).complete();
    delete_monster_idx(player_ptr, m_idx);
    return true;
}
