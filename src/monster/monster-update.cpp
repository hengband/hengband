/*!
 * @brief モンスター情報のアップデート処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster/monster-update.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "mind/drs-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-processor-util.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "player-base/player-class.h"
#include "player-info/samurai-data-type.h"
#include "player-info/sniper-data-type.h"
#include "player/player-move.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "status/element-resistance.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "world/world.h"

// Update Monster.
struct um_type {
    monster_type *m_ptr;
    bool do_disturb;
    POSITION fy;
    POSITION fx;
    bool flag;
    bool easy;
    bool in_darkness;
    bool full;
};

/*!
 * @brief 騎乗中のモンスター情報を更新する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param oy 移動前の、モンスターのY座標
 * @param ox 移動前の、モンスターのX座標
 * @param ny 移動後の、モンスターのY座標
 * @param ox 移動後の、モンスターのX座標
 * @return アイテム等に影響を及ぼしたらTRUE
 */
bool update_riding_monster(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox, POSITION ny, POSITION nx)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[ny][nx];
    monster_type *y_ptr = &player_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
    if (turn_flags_ptr->is_riding_mon) {
        return move_player_effect(player_ptr, ny, nx, MPE_DONT_PICKUP);
    }

    player_ptr->current_floor_ptr->grid_array[oy][ox].m_idx = g_ptr->m_idx;
    if (g_ptr->m_idx) {
        y_ptr->fy = oy;
        y_ptr->fx = ox;
        update_monster(player_ptr, g_ptr->m_idx, true);
    }

    g_ptr->m_idx = m_idx;
    m_ptr->fy = ny;
    m_ptr->fx = nx;
    update_monster(player_ptr, m_idx, true);

    lite_spot(player_ptr, oy, ox);
    lite_spot(player_ptr, ny, nx);
    return true;
}

/*!
 * @brief updateフィールドを更新する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 */
void update_player_type(PlayerType *player_ptr, turn_flags *turn_flags_ptr, monster_race *r_ptr)
{
    if (turn_flags_ptr->do_view) {
        player_ptr->update |= PU_FLOW;
        player_ptr->window_flags |= PW_OVERHEAD | PW_DUNGEON;
    }

    if (turn_flags_ptr->do_move && ((r_ptr->flags7 & (RF7_SELF_LD_MASK | RF7_HAS_DARK_1 | RF7_HAS_DARK_2)) || ((r_ptr->flags7 & (RF7_HAS_LITE_1 | RF7_HAS_LITE_2)) && !player_ptr->phase_out))) {
        player_ptr->update |= PU_MON_LITE;
    }
}

/*!
 * @brief モンスターのフラグを更新する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 */
void update_monster_race_flags(PlayerType *player_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr)
{
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    if (!is_original_ap_and_seen(player_ptr, m_ptr)) {
        return;
    }

    if (turn_flags_ptr->did_open_door) {
        r_ptr->r_behavior_flags.set(MonsterBehaviorType::OPEN_DOOR);
    }

    if (turn_flags_ptr->did_bash_door) {
        r_ptr->r_behavior_flags.set(MonsterBehaviorType::BASH_DOOR);
    }

    if (turn_flags_ptr->did_take_item) {
        r_ptr->r_behavior_flags.set(MonsterBehaviorType::TAKE_ITEM);
    }

    if (turn_flags_ptr->did_kill_item) {
        r_ptr->r_behavior_flags.set(MonsterBehaviorType::KILL_ITEM);
    }

    if (turn_flags_ptr->did_move_body) {
        r_ptr->r_behavior_flags.set(MonsterBehaviorType::MOVE_BODY);
    }

    if (turn_flags_ptr->did_pass_wall) {
        r_ptr->r_feature_flags.set(MonsterFeatureType::PASS_WALL);
    }

    if (turn_flags_ptr->did_kill_wall) {
        r_ptr->r_feature_flags.set(MonsterFeatureType::KILL_WALL);
    }
}

/*!
 * @brief モンスターフラグの更新に基づき、モンスター表示を更新する
 * @param monster_race_idx モンスターID
 * @param window ウィンドウフラグ
 * @param old_race_flags_ptr モンスターフラグへの参照ポインタ
 */
void update_player_window(PlayerType *player_ptr, old_race_flags *old_race_flags_ptr)
{
    monster_race *r_ptr;
    r_ptr = &monraces_info[player_ptr->monster_race_idx];
    if ((old_race_flags_ptr->old_r_flags1 != r_ptr->r_flags1) || (old_race_flags_ptr->old_r_flags2 != r_ptr->r_flags2) ||
        (old_race_flags_ptr->old_r_flags3 != r_ptr->r_flags3) || (old_race_flags_ptr->old_r_ability_flags != r_ptr->r_ability_flags) ||
        (old_race_flags_ptr->old_r_resistance_flags != r_ptr->r_resistance_flags) || (old_race_flags_ptr->old_r_blows0 != r_ptr->r_blows[0]) ||
        (old_race_flags_ptr->old_r_blows1 != r_ptr->r_blows[1]) || (old_race_flags_ptr->old_r_blows2 != r_ptr->r_blows[2]) ||
        (old_race_flags_ptr->old_r_blows3 != r_ptr->r_blows[3]) || (old_race_flags_ptr->old_r_cast_spell != r_ptr->r_cast_spell) ||
        (old_race_flags_ptr->old_r_behavior_flags != r_ptr->r_behavior_flags) || (old_race_flags_ptr->old_r_kind_flags != r_ptr->r_kind_flags) ||
        (old_race_flags_ptr->old_r_drop_flags != r_ptr->r_drop_flags) || (old_race_flags_ptr->old_r_feature_flags != r_ptr->r_feature_flags)) {
        player_ptr->window_flags |= PW_MONSTER;
    }
}

static um_type *initialize_um_type(PlayerType *player_ptr, um_type *um_ptr, MONSTER_IDX m_idx, bool full)
{
    um_ptr->m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    um_ptr->do_disturb = disturb_move;
    um_ptr->fy = um_ptr->m_ptr->fy;
    um_ptr->fx = um_ptr->m_ptr->fx;
    um_ptr->flag = false;
    um_ptr->easy = false;
    um_ptr->in_darkness = dungeons_info[player_ptr->dungeon_idx].flags.has(DungeonFeatureType::DARKNESS) && !player_ptr->see_nocto;
    um_ptr->full = full;
    return um_ptr;
}

static POSITION decide_updated_distance(PlayerType *player_ptr, um_type *um_ptr)
{
    if (!um_ptr->full) {
        return um_ptr->m_ptr->cdis;
    }

    int dy = (player_ptr->y > um_ptr->fy) ? (player_ptr->y - um_ptr->fy) : (um_ptr->fy - player_ptr->y);
    int dx = (player_ptr->x > um_ptr->fx) ? (player_ptr->x - um_ptr->fx) : (um_ptr->fx - player_ptr->x);
    POSITION distance = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));
    if (distance > 255) {
        distance = 255;
    }

    if (!distance) {
        distance = 1;
    }

    um_ptr->m_ptr->cdis = distance;
    return distance;
}

static void update_smart_stupid_flags(monster_race *r_ptr)
{
    if (r_ptr->r_behavior_flags.has(MonsterBehaviorType::SMART)) {
        r_ptr->r_behavior_flags.set(MonsterBehaviorType::SMART);
    }

    if (r_ptr->r_behavior_flags.has(MonsterBehaviorType::STUPID)) {
        r_ptr->r_behavior_flags.set(MonsterBehaviorType::STUPID);
    }
}

/*!
 * @brief WEIRD_MINDフラグ持ちのモンスターを1/10の確率でテレパシーに引っかける
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param um_ptr モンスター情報アップデート構造体への参照ポインタ
 * @param m_idx モンスターID
 * @return WEIRD_MINDフラグがあるならTRUE
 */
static bool update_weird_telepathy(PlayerType *player_ptr, um_type *um_ptr, MONSTER_IDX m_idx)
{
    auto *m_ptr = um_ptr->m_ptr;
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    if ((r_ptr->flags2 & RF2_WEIRD_MIND) == 0) {
        return false;
    }

    if ((m_idx % 10) != (w_ptr->game_turn % 10)) {
        return true;
    }

    um_ptr->flag = true;
    m_ptr->mflag.set(MonsterTemporaryFlagType::ESP);
    if (m_ptr->is_original_ap() && !player_ptr->effects()->hallucination()->is_hallucinated()) {
        r_ptr->r_flags2 |= RF2_WEIRD_MIND;
        update_smart_stupid_flags(r_ptr);
    }

    return true;
}

static void update_telepathy_sight(PlayerType *player_ptr, um_type *um_ptr, MONSTER_IDX m_idx)
{
    auto *m_ptr = um_ptr->m_ptr;
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    if (PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::MUSOU)) {
        um_ptr->flag = true;
        um_ptr->m_ptr->mflag.set(MonsterTemporaryFlagType::ESP);
        if (um_ptr->m_ptr->is_original_ap() && !player_ptr->effects()->hallucination()->is_hallucinated()) {
            update_smart_stupid_flags(r_ptr);
        }

        return;
    }

    if (!player_ptr->telepathy) {
        return;
    }

    auto is_hallucinated = player_ptr->effects()->hallucination()->is_hallucinated();
    if (r_ptr->flags2 & RF2_EMPTY_MIND) {
        if (m_ptr->is_original_ap() && !is_hallucinated) {
            r_ptr->r_flags2 |= RF2_EMPTY_MIND;
        }

        return;
    }

    if (update_weird_telepathy(player_ptr, um_ptr, m_idx)) {
        return;
    }

    um_ptr->flag = true;
    m_ptr->mflag.set(MonsterTemporaryFlagType::ESP);
    if (m_ptr->is_original_ap() && !is_hallucinated) {
        update_smart_stupid_flags(r_ptr);
    }
}

static void update_specific_race_telepathy(PlayerType *player_ptr, um_type *um_ptr)
{
    auto *m_ptr = um_ptr->m_ptr;
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    auto is_hallucinated = player_ptr->effects()->hallucination()->is_hallucinated();
    if ((player_ptr->esp_animal) && r_ptr->kind_flags.has(MonsterKindType::ANIMAL)) {
        um_ptr->flag = true;
        m_ptr->mflag.set(MonsterTemporaryFlagType::ESP);
        if (m_ptr->is_original_ap() && !is_hallucinated) {
            r_ptr->r_kind_flags.set(MonsterKindType::ANIMAL);
        }
    }

    if ((player_ptr->esp_undead) && r_ptr->kind_flags.has(MonsterKindType::UNDEAD)) {
        um_ptr->flag = true;
        m_ptr->mflag.set(MonsterTemporaryFlagType::ESP);
        if (m_ptr->is_original_ap() && !is_hallucinated) {
            r_ptr->r_kind_flags.set(MonsterKindType::UNDEAD);
        }
    }

    if ((player_ptr->esp_demon) && r_ptr->kind_flags.has(MonsterKindType::DEMON)) {
        um_ptr->flag = true;
        m_ptr->mflag.set(MonsterTemporaryFlagType::ESP);
        if (m_ptr->is_original_ap() && !is_hallucinated) {
            r_ptr->r_kind_flags.set(MonsterKindType::DEMON);
        }
    }

    if ((player_ptr->esp_orc) && r_ptr->kind_flags.has(MonsterKindType::ORC)) {
        um_ptr->flag = true;
        m_ptr->mflag.set(MonsterTemporaryFlagType::ESP);
        if (m_ptr->is_original_ap() && !is_hallucinated) {
            r_ptr->r_kind_flags.set(MonsterKindType::ORC);
        }
    }

    if ((player_ptr->esp_troll) && r_ptr->kind_flags.has(MonsterKindType::TROLL)) {
        um_ptr->flag = true;
        m_ptr->mflag.set(MonsterTemporaryFlagType::ESP);
        if (m_ptr->is_original_ap() && !is_hallucinated) {
            r_ptr->r_kind_flags.set(MonsterKindType::TROLL);
        }
    }

    if ((player_ptr->esp_giant) && r_ptr->kind_flags.has(MonsterKindType::GIANT)) {
        um_ptr->flag = true;
        m_ptr->mflag.set(MonsterTemporaryFlagType::ESP);
        if (m_ptr->is_original_ap() && !is_hallucinated) {
            r_ptr->r_kind_flags.set(MonsterKindType::GIANT);
        }
    }

    if ((player_ptr->esp_dragon) && r_ptr->kind_flags.has(MonsterKindType::DRAGON)) {
        um_ptr->flag = true;
        m_ptr->mflag.set(MonsterTemporaryFlagType::ESP);
        if (m_ptr->is_original_ap() && !is_hallucinated) {
            r_ptr->r_kind_flags.set(MonsterKindType::DRAGON);
        }
    }

    if ((player_ptr->esp_human) && r_ptr->kind_flags.has(MonsterKindType::HUMAN)) {
        um_ptr->flag = true;
        m_ptr->mflag.set(MonsterTemporaryFlagType::ESP);
        if (m_ptr->is_original_ap() && !is_hallucinated) {
            r_ptr->r_kind_flags.set(MonsterKindType::HUMAN);
        }
    }

    if ((player_ptr->esp_evil) && r_ptr->kind_flags.has(MonsterKindType::EVIL)) {
        um_ptr->flag = true;
        m_ptr->mflag.set(MonsterTemporaryFlagType::ESP);
        if (m_ptr->is_original_ap() && !is_hallucinated) {
            r_ptr->r_kind_flags.set(MonsterKindType::EVIL);
        }
    }

    if ((player_ptr->esp_good) && r_ptr->kind_flags.has(MonsterKindType::GOOD)) {
        um_ptr->flag = true;
        m_ptr->mflag.set(MonsterTemporaryFlagType::ESP);
        if (m_ptr->is_original_ap() && !is_hallucinated) {
            r_ptr->r_kind_flags.set(MonsterKindType::GOOD);
        }
    }

    if ((player_ptr->esp_nonliving) && (r_ptr->kind_flags.has(MonsterKindType::NONLIVING) && r_ptr->kind_flags.has_none_of({ MonsterKindType::DEMON, MonsterKindType::UNDEAD }))) {
        um_ptr->flag = true;
        m_ptr->mflag.set(MonsterTemporaryFlagType::ESP);
        if (m_ptr->is_original_ap() && !is_hallucinated) {
            r_ptr->r_kind_flags.set(MonsterKindType::NONLIVING);
        }
    }

    if ((player_ptr->esp_unique) && r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        um_ptr->flag = true;
        m_ptr->mflag.set(MonsterTemporaryFlagType::ESP);
        if (m_ptr->is_original_ap() && !is_hallucinated) {
            r_ptr->r_kind_flags.set(MonsterKindType::UNIQUE);
        }
    }
}

static bool check_cold_blood(PlayerType *player_ptr, um_type *um_ptr, const POSITION distance)
{
    if (distance > player_ptr->see_infra) {
        return false;
    }

    auto *r_ptr = &monraces_info[um_ptr->m_ptr->r_idx];
    if (any_bits(r_ptr->flags2, RF2_COLD_BLOOD) && r_ptr->aura_flags.has_not(MonsterAuraType::FIRE)) {
        return false;
    }

    um_ptr->easy = true;
    um_ptr->flag = true;
    return true;
}

static bool check_invisible(PlayerType *player_ptr, um_type *um_ptr)
{
    if (!player_can_see_bold(player_ptr, um_ptr->fy, um_ptr->fx)) {
        return false;
    }

    auto *r_ptr = &monraces_info[um_ptr->m_ptr->r_idx];
    if (r_ptr->flags2 & RF2_INVISIBLE) {
        if (player_ptr->see_inv) {
            um_ptr->easy = true;
            um_ptr->flag = true;
        }
    } else {
        um_ptr->easy = true;
        um_ptr->flag = true;
    }

    return true;
}

/*!
 * @brief テレパシー・赤外線視力・可視透明によってモンスターを感知できるかどうかの判定
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param um_ptr モンスター情報アップデート構造体への参照ポインタ
 */
static void decide_sight_invisible_monster(PlayerType *player_ptr, um_type *um_ptr, MONSTER_IDX m_idx)
{
    POSITION distance = decide_updated_distance(player_ptr, um_ptr);
    auto *m_ptr = um_ptr->m_ptr;
    auto *r_ptr = &monraces_info[m_ptr->r_idx];

    m_ptr->mflag.reset(MonsterTemporaryFlagType::ESP);

    if (distance > (um_ptr->in_darkness ? MAX_PLAYER_SIGHT / 2 : MAX_PLAYER_SIGHT)) {
        return;
    }

    if (!um_ptr->in_darkness || (distance <= MAX_PLAYER_SIGHT / 4)) {
        update_telepathy_sight(player_ptr, um_ptr, m_idx);
        update_specific_race_telepathy(player_ptr, um_ptr);
    }

    if (!player_has_los_bold(player_ptr, um_ptr->fy, um_ptr->fx) || player_ptr->effects()->blindness()->is_blind()) {
        return;
    }

    auto sniper_data = PlayerClass(player_ptr).get_specific_data<sniper_data_type>();
    if (sniper_data && (sniper_data->concent >= CONCENT_RADAR_THRESHOLD)) {
        um_ptr->easy = true;
        um_ptr->flag = true;
    }

    bool do_cold_blood = check_cold_blood(player_ptr, um_ptr, distance);
    bool do_invisible = check_invisible(player_ptr, um_ptr);
    if (!um_ptr->flag || !m_ptr->is_original_ap() || player_ptr->effects()->hallucination()->is_hallucinated()) {
        return;
    }

    if (do_invisible) {
        r_ptr->r_flags2 |= RF2_INVISIBLE;
    }

    if (do_cold_blood) {
        r_ptr->r_flags2 |= RF2_COLD_BLOOD;
    }
}

/*!
 * @brief 壁の向こうにいるモンスターへのテレパシー・赤外線視力による冷血動物以外の透明モンスター・可視透明能力による透明モンスター
 * 以上を感知する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param um_ptr モンスター情報アップデート構造体への参照ポインタ
 * @param m_idx フロアのモンスター番号
 * @details 感知した結果、エルドリッチホラー持ちがいたら精神を破壊する
 */
static void update_invisible_monster(PlayerType *player_ptr, um_type *um_ptr, MONSTER_IDX m_idx)
{
    auto *m_ptr = um_ptr->m_ptr;
    if (m_ptr->ml) {
        return;
    }

    m_ptr->ml = true;
    lite_spot(player_ptr, um_ptr->fy, um_ptr->fx);

    if (player_ptr->health_who == m_idx) {
        player_ptr->redraw |= PR_HEALTH;
    }

    if (player_ptr->riding == m_idx) {
        player_ptr->redraw |= PR_UHEALTH;
    }

    if (!player_ptr->effects()->hallucination()->is_hallucinated()) {
        auto *r_ptr = &monraces_info[m_ptr->r_idx];
        if ((m_ptr->ap_r_idx == MonsterRaceId::KAGE) && (monraces_info[MonsterRaceId::KAGE].r_sights < MAX_SHORT)) {
            monraces_info[MonsterRaceId::KAGE].r_sights++;
        } else if (m_ptr->is_original_ap() && (r_ptr->r_sights < MAX_SHORT)) {
            r_ptr->r_sights++;
        }
    }

    if (w_ptr->is_loading_now && w_ptr->character_dungeon && !player_ptr->phase_out && monraces_info[m_ptr->ap_r_idx].flags2 & RF2_ELDRITCH_HORROR) {
        m_ptr->mflag.set(MonsterTemporaryFlagType::SANITY_BLAST);
    }

    const auto projectable_from_monster = projectable(player_ptr, m_ptr->fy, m_ptr->fx, player_ptr->y, player_ptr->x);
    const auto projectable_from_player = projectable(player_ptr, player_ptr->y, player_ptr->x, m_ptr->fy, m_ptr->fx);
    if (disturb_near && projectable_from_monster && projectable_from_player) {
        if (disturb_pets || m_ptr->is_hostile()) {
            disturb(player_ptr, true, true);
        }
    }
}

static void update_visible_monster(PlayerType *player_ptr, um_type *um_ptr, MONSTER_IDX m_idx)
{
    if (!um_ptr->m_ptr->ml) {
        return;
    }

    um_ptr->m_ptr->ml = false;
    lite_spot(player_ptr, um_ptr->fy, um_ptr->fx);

    if (player_ptr->health_who == m_idx) {
        player_ptr->redraw |= PR_HEALTH;
    }

    if (player_ptr->riding == m_idx) {
        player_ptr->redraw |= PR_UHEALTH;
    }

    if (um_ptr->do_disturb && (disturb_pets || um_ptr->m_ptr->is_hostile())) {
        disturb(player_ptr, true, true);
    }
}

static bool update_clear_monster(PlayerType *player_ptr, um_type *um_ptr)
{
    if (!um_ptr->easy) {
        return false;
    }

    if (um_ptr->m_ptr->mflag.has_not(MonsterTemporaryFlagType::VIEW)) {
        um_ptr->m_ptr->mflag.set(MonsterTemporaryFlagType::VIEW);
        if (um_ptr->do_disturb && (disturb_pets || um_ptr->m_ptr->is_hostile())) {
            disturb(player_ptr, true, true);
        }
    }

    return true;
}

/*!
 * @brief モンスターの各情報を更新する / This function updates the monster record of the given monster
 * @param m_idx 更新するモンスター情報のID
 * @param full プレイヤーとの距離更新を行うならばtrue
 */
void update_monster(PlayerType *player_ptr, MONSTER_IDX m_idx, bool full)
{
    um_type tmp_um;
    um_type *um_ptr = initialize_um_type(player_ptr, &tmp_um, m_idx, full);
    if (disturb_high) {
        monster_race *ap_r_ptr = &monraces_info[um_ptr->m_ptr->ap_r_idx];
        if (ap_r_ptr->r_tkills && ap_r_ptr->level >= player_ptr->lev) {
            um_ptr->do_disturb = true;
        }
    }

    if (um_ptr->m_ptr->mflag2.has(MonsterConstantFlagType::MARK)) {
        um_ptr->flag = true;
    }

    decide_sight_invisible_monster(player_ptr, um_ptr, m_idx);
    if (um_ptr->flag) {
        update_invisible_monster(player_ptr, um_ptr, m_idx);
    } else {
        update_visible_monster(player_ptr, um_ptr, m_idx);
    }

    if (update_clear_monster(player_ptr, um_ptr) || um_ptr->m_ptr->mflag.has_not(MonsterTemporaryFlagType::VIEW)) {
        return;
    }

    um_ptr->m_ptr->mflag.reset(MonsterTemporaryFlagType::VIEW);
    if (um_ptr->do_disturb && (disturb_pets || um_ptr->m_ptr->is_hostile())) {
        disturb(player_ptr, true, true);
    }
}

/*!
 * @param player_ptr プレイヤーへの参照ポインタ
 * @brief 単純に生存している全モンスターの更新処理を行う / This function simply updates all the (non-dead) monsters (see above).
 * @param full 距離更新を行うならtrue
 * @todo モンスターの感知状況しか更新していないように見える。関数名変更を検討する
 */
void update_monsters(PlayerType *player_ptr, bool full)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (MONSTER_IDX i = 1; i < floor_ptr->m_max; i++) {
        auto *m_ptr = &floor_ptr->m_list[i];
        if (!m_ptr->is_valid()) {
            continue;
        }

        update_monster(player_ptr, i, full);
    }
}

/*!
 * @brief SMART(適格に攻撃を行う)モンスターの学習状況を更新する / Learn about an "observed" resistance.
 * @param m_idx 更新を行う「モンスター情報ID
 * @param what 学習対象ID
 */
void update_smart_learn(PlayerType *player_ptr, MONSTER_IDX m_idx, int what)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    if (!smart_learn || (r_ptr->behavior_flags.has(MonsterBehaviorType::STUPID)) || ((r_ptr->behavior_flags.has_not(MonsterBehaviorType::SMART)) && (randint0(100) < 50))) {
        return;
    }

    switch (what) {
    case DRS_ACID:
        if (has_resist_acid(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::RES_ACID);
        }

        if (is_oppose_acid(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::OPP_ACID);
        }

        if (has_immune_acid(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::IMM_ACID);
        }

        break;
    case DRS_ELEC:
        if (has_resist_elec(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::RES_ELEC);
        }

        if (is_oppose_elec(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::OPP_ELEC);
        }

        if (has_immune_elec(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::IMM_ELEC);
        }

        break;
    case DRS_FIRE:
        if (has_resist_fire(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::RES_FIRE);
        }

        if (is_oppose_fire(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::OPP_FIRE);
        }

        if (has_immune_fire(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::IMM_FIRE);
        }

        break;
    case DRS_COLD:
        if (has_resist_cold(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::RES_COLD);
        }

        if (is_oppose_cold(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::OPP_COLD);
        }

        if (has_immune_cold(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::IMM_COLD);
        }

        break;
    case DRS_POIS:
        if (has_resist_pois(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::RES_POIS);
        }

        if (is_oppose_pois(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::OPP_POIS);
        }

        break;
    case DRS_NETH:
        if (has_resist_neth(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::RES_NETH);
        }

        break;
    case DRS_LITE:
        if (has_resist_lite(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::RES_LITE);
        }

        break;
    case DRS_DARK:
        if (has_resist_dark(player_ptr) || has_immune_dark(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::RES_DARK);
        }

        break;
    case DRS_FEAR:
        if (has_resist_fear(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::RES_FEAR);
        }

        break;
    case DRS_CONF:
        if (has_resist_conf(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::RES_CONF);
        }

        break;
    case DRS_CHAOS:
        if (has_resist_chaos(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::RES_CHAOS);
        }

        break;
    case DRS_DISEN:
        if (has_resist_disen(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::RES_DISEN);
        }

        break;
    case DRS_BLIND:
        if (has_resist_blind(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::RES_BLIND);
        }

        break;
    case DRS_NEXUS:
        if (has_resist_nexus(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::RES_NEXUS);
        }

        break;
    case DRS_SOUND:
        if (has_resist_sound(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::RES_SOUND);
        }

        break;
    case DRS_SHARD:
        if (has_resist_shard(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::RES_SHARD);
        }

        break;
    case DRS_FREE:
        if (player_ptr->free_act) {
            m_ptr->smart.set(MonsterSmartLearnType::IMM_FREE);
        }

        break;
    case DRS_MANA:
        if (!player_ptr->msp) {
            m_ptr->smart.set(MonsterSmartLearnType::IMM_MANA);
        }

        break;
    case DRS_REFLECT:
        if (has_reflect(player_ptr)) {
            m_ptr->smart.set(MonsterSmartLearnType::IMM_REFLECT);
        }

        break;
    default:
        break;
    }
}
