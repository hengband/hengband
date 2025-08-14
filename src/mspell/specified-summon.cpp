#include "mspell/specified-summon.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/floor-util.h"
#include "grid/grid.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-info.h"
#include "monster/monster-update.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-util.h"
#include "spell-kind/spells-launcher.h"
#include "spell/summon-types.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

/*!
 * @brief 鷹召喚の処理。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param rlev 呪文を唱えるモンスターのレベル
 * @param m_idx 呪文を唱えるモンスターID
 * @return 召喚したモンスターの数を返す。
 */
MONSTER_NUMBER summon_EAGLE(PlayerType *player_ptr, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx)
{
    int count = 0;
    int num = 4 + randint1(3);
    for (int k = 0; k < num; k++) {
        count += summon_specific(player_ptr, y, x, rlev, SUMMON_EAGLES, PM_ALLOW_GROUP | PM_ALLOW_UNIQUE, m_idx) ? 1 : 0;
    }

    return count;
}

/*!
 * @brief エッヂ召喚の処理。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param rlev 呪文を唱えるモンスターのレベル
 * @param m_idx 呪文を唱えるモンスターID
 * @return 召喚したモンスターの数を返す。
 */
MONSTER_NUMBER summon_EDGE(PlayerType *player_ptr, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx)
{
    int count = 0;
    int num = 2 + randint1(1 + rlev / 20);
    for (int k = 0; k < num; k++) {
        count += summon_named_creature(player_ptr, m_idx, y, x, MonraceId::EDGE, PM_NONE) ? 1 : 0;
    }

    return count;
}

/*!
 * @brief ダンジョンの主召喚の処理。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param rlev 呪文を唱えるモンスターのレベル
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return 召喚したモンスターの数を返す。
 */
MONSTER_NUMBER summon_guardian(PlayerType *player_ptr, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    int num = 2 + randint1(3);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    const auto &jormungand = MonraceList::get_instance().get_monrace(MonraceId::JORMUNGAND);
    if (jormungand.cur_num < jormungand.max_num && one_in_(6)) {
        mspell_cast_msg_simple msg(_("地面から水が吹き出した！", "Water blew off from the ground!"),
            _("地面から水が吹き出した！", "Water blew off from the ground!"));

        simple_monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

        if (mon_to_player) {
            fire_ball_hide(player_ptr, AttributeType::WATER_FLOW, Direction::self(), 3, 8);
        } else if (mon_to_mon) {
            project(player_ptr, t_idx, 8, y, x, 3, AttributeType::WATER_FLOW, PROJECT_GRID | PROJECT_HIDE);
        }
    }

    int count = 0;
    for (int k = 0; k < num; k++) {
        count += summon_specific(player_ptr, y, x, rlev, SUMMON_GUARDIANS, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE), m_idx) ? 1 : 0;
    }

    return count;
}

/*!
 * @brief ロックのクローン召喚の処理。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @return 召喚したモンスターの数を返す。
 */
MONSTER_NUMBER summon_LOCKE_CLONE(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx)
{
    int count = 0;
    int num = randint1(3);
    for (int k = 0; k < num; k++) {
        count += summon_named_creature(player_ptr, m_idx, y, x, MonraceId::LOCKE_CLONE, PM_NONE) ? 1 : 0;
    }

    return count;
}

/*!
 * @brief シラミ召喚の処理。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param rlev 呪文を唱えるモンスターのレベル
 * @param m_idx 呪文を唱えるモンスターID
 * @return 召喚したモンスターの数を返す。
 */
MONSTER_NUMBER summon_LOUSE(PlayerType *player_ptr, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx)
{
    int count = 0;
    int num = 2 + randint1(3);
    for (int k = 0; k < num; k++) {
        count += summon_specific(player_ptr, y, x, rlev, SUMMON_LOUSE, PM_ALLOW_GROUP, m_idx) ? 1 : 0;
    }

    return count;
}

MONSTER_NUMBER summon_MOAI(PlayerType *player_ptr, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx)
{
    int count = 0;
    int num = 3 + randint1(3);
    for (int k = 0; k < num; k++) {
        count += summon_specific(player_ptr, y, x, rlev, SUMMON_SMALL_MOAI, PM_NONE, m_idx) ? 1 : 0;
    }

    return count;
}

MONSTER_NUMBER summon_DEMON_SLAYER(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx)
{
    auto &monrace = MonraceList::get_instance().get_monrace(MonraceId::DEMON_SLAYER_MEMBER);
    if (monrace.max_num == 0) {
        msg_print(_("しかし、隊士は全滅していた…。", "However, all demon slayer members were murdered..."));
        return 0;
    }

    auto count = 0;
    for (auto k = 0; k < MAX_NAZGUL_NUM; k++) {
        count += summon_named_creature(player_ptr, m_idx, y, x, MonraceId::DEMON_SLAYER_MEMBER, PM_NONE) ? 1 : 0;
    }

    if (count == 0) {
        msg_print(_("しかし、隊士は誰も来てくれなかった。", "However, no demon slayer member answered the call..."));
    }

    return count;
}

/*!
 * @brief ナズグル戦隊召喚の処理。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @return 召喚したモンスターの数を返す。
 */
MONSTER_NUMBER summon_NAZGUL(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx)
{
    BIT_FLAGS mode = 0L;
    Pos2D pos_initial(y, x);
    auto pos = pos_initial;
    auto pos_scat = pos_initial;
    const auto m_name = monster_name(player_ptr, m_idx);

    if (player_ptr->effects()->blindness().is_blind()) {
        msg_format(_("%s^が何かをつぶやいた。", "%s^ mumbles."), m_name.data());
    } else {
        msg_format(_("%s^が魔法で幽鬼戦隊を召喚した！", "%s^ magically summons rangers of Nazgul!"), m_name.data());
    }

    msg_erase();
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    auto count = 0;
    for (auto k = 0; k < 30; k++) {
        if (!summon_possible(player_ptr, pos_scat.y, pos_scat.x) || !floor.is_empty_at(pos_scat) || (pos_scat == p_pos)) {
            int j;
            for (j = 100; j > 0; j--) {
                pos_scat = scatter(player_ptr, pos, 2, PROJECT_NONE);
                if (floor.is_empty_at(pos_scat) && (pos_scat != p_pos)) {
                    break;
                }
            }

            if (j == 0) {
                break;
            }
        }

        if (!floor.is_empty_at(pos_scat) || (pos_scat == p_pos)) {
            continue;
        }

        if (!summon_named_creature(player_ptr, m_idx, pos_scat.y, pos_scat.x, MonraceId::NAZGUL, mode)) {
            continue;
        }

        pos = pos_scat;
        count++;
        if (count == 1) {
            msg_format(_("「幽鬼戦隊%d号、ナズグル・ブラック！」", "A Nazgul says 'Nazgul-Rangers Number %d, Nazgul-Black!'"), count);
        } else {
            msg_format(_("「同じく%d号、ナズグル・ブラック！」", "Another one says 'Number %d, Nazgul-Black!'"), count);
        }

        msg_erase();
    }

    msg_format(_("「%d人そろって、リングレンジャー！」", "They say 'The %d meets! We are the Ring-Ranger!'."), count);
    msg_erase();
    return count;
}

MONSTER_NUMBER summon_APOCRYPHA(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx)
{
    int count = 0;
    int num = 4 + randint1(4);
    summon_type followers = one_in_(2) ? SUMMON_APOCRYPHA_FOLLOWERS : SUMMON_APOCRYPHA_DRAGONS;
    for (int k = 0; k < num; k++) {
        count += summon_specific(player_ptr, y, x, 200, followers, PM_ALLOW_UNIQUE, m_idx) ? 1 : 0;
    }

    return count;
}

MONSTER_NUMBER summon_HIGHEST_DRAGON(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx)
{
    int count = 0;
    int num = 4 + randint1(4);
    for (int k = 0; k < num; k++) {
        count += summon_specific(player_ptr, y, x, 100, SUMMON_APOCRYPHA_DRAGONS, PM_ALLOW_UNIQUE, m_idx) ? 1 : 0;
    }

    return count;
}

MONSTER_NUMBER summon_PYRAMID(PlayerType *player_ptr, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx)
{
    int count = 0;
    int num = 2 + randint1(3);
    for (int k = 0; k < num; k++) {
        count += summon_specific(player_ptr, y, x, rlev, SUMMON_PYRAMID, PM_NONE, m_idx) ? 1 : 0;
    }

    return count;
}

MONSTER_NUMBER summon_EYE_PHORN(PlayerType *player_ptr, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx)
{
    int count = 0;
    int num = 2 + randint1(1 + rlev / 20);
    for (int k = 0; k < num; k++) {
        count += summon_named_creature(player_ptr, m_idx, y, x, MonraceId::EYE_PHORN, PM_NONE) ? 1 : 0;
    }

    return count;
}

MONSTER_NUMBER summon_VESPOID(PlayerType *player_ptr, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx)
{
    int count = 0;
    int num = 2 + randint1(3);
    for (int k = 0; k < num; k++) {
        count += summon_specific(player_ptr, y, x, rlev, SUMMON_VESPOID, PM_NONE, m_idx) ? 1 : 0;
    }

    return count;
}

MONSTER_NUMBER summon_THUNDERS(PlayerType *player_ptr, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx)
{
    auto count = (MONSTER_NUMBER)0;
    auto num = 11;
    for (auto k = 0; k < num; k++) {
        count += summon_specific(player_ptr, y, x, rlev, SUMMON_ANTI_TIGERS, PM_NONE, m_idx) ? 1 : 0;
    }

    return count;
}

/*!
 * @brief イェンダーの魔法使いの召喚の処理。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @return 召喚したモンスターの数を返す。
 */
MONSTER_NUMBER summon_YENDER_WIZARD(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx)
{
    auto &monrace = MonraceList::get_instance().get_monrace(MonraceId::YENDOR_WIZARD_2);
    if (monrace.max_num == 0) {
        msg_print(_("しかし、誰も来なかった…。", "However, no kin was appeared..."));
        return 0;
    }

    auto count = summon_named_creature(player_ptr, m_idx, y, x, MonraceId::YENDOR_WIZARD_2, PM_NONE) ? 1 : 0;
    if (count == 0) {
        msg_print(_("どこからか声が聞こえる…「三重苦は負わぬ。。。」", "Heard a voice from somewhere... 'I will deny the triple suffering...'"));
        return 0;
    }

    msg_print(_("二重苦だ。。。", "THIS is double suffering..."));
    return count;
}

MONSTER_NUMBER summon_PLASMA(PlayerType *player_ptr, POSITION y, POSITION x, int rlev, MONSTER_IDX m_idx)
{
    auto count = 0;
    auto num = 2 + randint1(1 + rlev / 20);
    for (auto k = 0; k < num; k++) {
        count += summon_named_creature(player_ptr, m_idx, y, x, MonraceId::PLASMA_VORTEX, PM_NONE) ? 1 : 0;
    }

    msg_print(_("プーラーズーマーッ！！", "P--la--s--ma--!!"));
    return count;
}

/*!
 * @brief ウサウサストライカー召喚の処理。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @return 召喚したモンスターの数を返す。
 */
MONSTER_NUMBER summon_LAFFEY_II(PlayerType *player_ptr, const Pos2D &position, MONSTER_IDX m_idx)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto count = 0;
    constexpr auto summon_num = 2;
    const auto real_num = summon_num - MonraceList::get_instance().get_monrace(MonraceId::BUNBUN_STRIKERS).cur_num;

    if (!floor.inside_arena && real_num < MAX_BUNBUN_NUM) {
        for (auto &monster : floor.m_list) {
            if (monster.r_idx == MonraceId::BUNBUN_STRIKERS) {
                const auto current_position = monster.get_position();
                auto &current_grid = floor.get_grid(current_position);
                auto target_m_idx = current_grid.m_idx;
                const auto attract_position = mon_scatter(player_ptr, MonraceId::BUNBUN_STRIKERS, position, 2);
                if (!attract_position) {
                    continue;
                }

                current_grid.m_idx = 0;
                floor.get_grid(*attract_position).m_idx = target_m_idx;
                monster.set_position(*attract_position);
                update_monster(player_ptr, target_m_idx, true);
                lite_spot(player_ptr, current_position);
                lite_spot(player_ptr, *attract_position);

                count++;
            }
        }
    }
    for (auto k = 0; k < real_num; k++) {
        count += summon_named_creature(player_ptr, m_idx, position.y, position.x, MonraceId::BUNBUN_STRIKERS, PM_NONE) ? 1 : 0;
    }
    return count;
}

MONSTER_NUMBER summon_POLYGON(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx)
{
    auto count = 0;
    auto num = 2 + randint1(3);
    for (auto k = 0; k < num; k++) {
        count += summon_named_creature(player_ptr, m_idx, y, x, MonraceId::POLYGON_SPIN, PM_NONE) ? 1 : 0;
    }

    return count;
}
