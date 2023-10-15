/*!
 * @brief モンスターの移動に関する処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster-floor/monster-move.h"
#include "core/disturbance.h"
#include "core/speed-table.h"
#include "core/window-redrawer.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "io/files-util.h"
#include "monster-attack/monster-attack-processor.h"
#include "monster-floor/monster-object.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-flags8.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-processor-util.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "pet/pet-util.h"
#include "player/player-status-flags.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

static bool check_hp_for_terrain_destruction(TerrainType *f_ptr, MonsterEntity *m_ptr)
{
    auto can_destroy = f_ptr->flags.has_not(TerrainCharacteristics::GLASS);
    can_destroy |= monraces_info[m_ptr->r_idx].behavior_flags.has(MonsterBehaviorType::STUPID);
    can_destroy |= m_ptr->hp >= std::max(m_ptr->maxhp / 3, 200);
    return can_destroy;
}

/*!
 * @brief モンスターによる壁の透過・破壊を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param ny モンスターのY座標
 * @param nx モンスターのX座標
 * @param can_cross モンスターが地形を踏破できるならばTRUE
 * @return 透過も破壊もしなかった場合はFALSE、それ以外はTRUE
 */
static bool process_wall(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MonsterEntity *m_ptr, POSITION ny, POSITION nx, bool can_cross)
{
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[ny][nx];
    auto *f_ptr = &terrains_info[g_ptr->feat];
    if (player_bold(player_ptr, ny, nx)) {
        turn_flags_ptr->do_move = true;
        return true;
    }

    if (g_ptr->m_idx > 0) {
        turn_flags_ptr->do_move = true;
        return true;
    }

    using Mft = MonsterFeatureType;
    using Tc = TerrainCharacteristics;
    auto can_kill_wall = r_ptr->feature_flags.has(Mft::KILL_WALL);
    can_kill_wall &= can_cross ? f_ptr->flags.has_not(Tc::LOS) : !turn_flags_ptr->is_riding_mon;
    can_kill_wall &= f_ptr->flags.has(Tc::HURT_DISI);
    can_kill_wall &= f_ptr->flags.has_not(Tc::PERMANENT);
    can_kill_wall &= check_hp_for_terrain_destruction(f_ptr, m_ptr);
    if (can_kill_wall) {
        turn_flags_ptr->do_move = true;
        if (!can_cross) {
            turn_flags_ptr->must_alter_to_move = true;
        }

        turn_flags_ptr->did_kill_wall = true;
        return true;
    }

    if (!can_cross) {
        return false;
    }

    turn_flags_ptr->do_move = true;
    if ((r_ptr->feature_flags.has(Mft::PASS_WALL)) && (!turn_flags_ptr->is_riding_mon || has_pass_wall(player_ptr)) && f_ptr->flags.has(Tc::CAN_PASS)) {
        turn_flags_ptr->did_pass_wall = true;
    }

    return true;
}

/*!
 * @brief モンスターが普通のドアを開ける処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param ny モンスターのY座標
 * @param nx モンスターのX座標
 * @return ここではドアを開けず、ガラスのドアを開ける可能性があるならTRUE
 */
static bool bash_normal_door(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MonsterEntity *m_ptr, POSITION ny, POSITION nx)
{
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[ny][nx];
    auto *f_ptr = &terrains_info[g_ptr->feat];
    turn_flags_ptr->do_move = false;
    using Tc = TerrainCharacteristics;
    auto can_bash = r_ptr->behavior_flags.has_not(MonsterBehaviorType::OPEN_DOOR);
    can_bash |= f_ptr->flags.has_not(Tc::OPEN);
    can_bash |= m_ptr->is_pet() && ((player_ptr->pet_extra_flags & PF_OPEN_DOORS) == 0);
    if (can_bash) {
        return true;
    }

    if (f_ptr->power == 0) {
        turn_flags_ptr->did_open_door = true;
        turn_flags_ptr->do_turn = true;
        return false;
    }

    if (randint0(m_ptr->hp / 10) > f_ptr->power) {
        cave_alter_feat(player_ptr, ny, nx, Tc::DISARM);
        turn_flags_ptr->do_turn = true;
        return false;
    }

    return true;
}

/*!
 * @brief モンスターがガラスのドアを開ける処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param g_ptr グリッドへの参照ポインタ
 * @param f_ptr 地形への参照ポインタ
 */
static void bash_glass_door(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MonsterEntity *m_ptr, TerrainType *f_ptr, bool may_bash)
{
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    auto can_bash = may_bash;
    can_bash &= r_ptr->behavior_flags.has(MonsterBehaviorType::BASH_DOOR);
    can_bash &= f_ptr->flags.has(TerrainCharacteristics::BASH);
    can_bash &= !m_ptr->is_pet() || any_bits(player_ptr->pet_extra_flags, PF_OPEN_DOORS);
    if (!can_bash) {
        return;
    }

    if (!check_hp_for_terrain_destruction(f_ptr, m_ptr) || (randint0(m_ptr->hp / 10) <= f_ptr->power)) {
        return;
    }

    if (f_ptr->flags.has(TerrainCharacteristics::GLASS)) {
        msg_print(_("ガラスが砕ける音がした！", "You hear glass breaking!"));
    } else {
        msg_print(_("ドアを叩き開ける音がした！", "You hear a door burst open!"));
    }

    if (disturb_minor) {
        disturb(player_ptr, false, false);
    }

    turn_flags_ptr->did_bash_door = true;
    turn_flags_ptr->do_move = true;
    turn_flags_ptr->must_alter_to_move = true;
}

/*!
 * @brief モンスターによるドアの開放・破壊を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param ny モンスターのY座標
 * @param nx モンスターのX座標
 * @return モンスターが死亡した場合のみFALSE
 */
static bool process_door(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MonsterEntity *m_ptr, POSITION ny, POSITION nx)
{
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    const auto &g_ref = player_ptr->current_floor_ptr->grid_array[ny][nx];
    if (!is_closed_door(player_ptr, g_ref.feat)) {
        return true;
    }

    auto *terrain_ptr = &terrains_info[g_ref.feat];
    auto may_bash = bash_normal_door(player_ptr, turn_flags_ptr, m_ptr, ny, nx);
    bash_glass_door(player_ptr, turn_flags_ptr, m_ptr, terrain_ptr, may_bash);
    if (!turn_flags_ptr->did_open_door && !turn_flags_ptr->did_bash_door) {
        return true;
    }

    const auto is_open = feat_state(player_ptr->current_floor_ptr, g_ref.feat, TerrainCharacteristics::OPEN) == g_ref.feat;
    if (turn_flags_ptr->did_bash_door && ((randint0(100) < 50) || is_open || terrain_ptr->flags.has(TerrainCharacteristics::GLASS))) {
        cave_alter_feat(player_ptr, ny, nx, TerrainCharacteristics::BASH);
        if (!m_ptr->is_valid()) {
            auto &rfu = RedrawingFlagsUpdater::get_instance();
            rfu.set_flag(StatusRecalculatingFlag::FLOW);
            static constexpr auto flags = {
                SubWindowRedrawingFlag::OVERHEAD,
                SubWindowRedrawingFlag::DUNGEON,
            };
            rfu.set_flags(flags);
            if (is_original_ap_and_seen(player_ptr, m_ptr)) {
                r_ptr->r_behavior_flags.set(MonsterBehaviorType::BASH_DOOR);
            }

            return false;
        }
    } else {
        cave_alter_feat(player_ptr, ny, nx, TerrainCharacteristics::OPEN);
    }

    turn_flags_ptr->do_view = true;
    return true;
}

/*!
 * @brief 守りのルーンによるモンスターの移動制限を処理する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param ny モンスターのY座標
 * @param nx モンスターのX座標
 * @return ルーンに侵入できるか否か
 */
static bool process_protection_rune(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MonsterEntity *m_ptr, POSITION ny, POSITION nx)
{
    auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[ny][nx];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    auto can_enter = turn_flags_ptr->do_move;
    can_enter &= g_ptr->is_rune_protection();
    can_enter &= (r_ptr->behavior_flags.has_not(MonsterBehaviorType::NEVER_BLOW)) || !player_bold(player_ptr, ny, nx);
    if (!can_enter) {
        return false;
    }

    turn_flags_ptr->do_move = false;
    if (m_ptr->is_pet() || (randint1(BREAK_RUNE_PROTECTION) >= r_ptr->level)) {
        return true;
    }

    if (g_ptr->is_mark()) {
        msg_print(_("守りのルーンが壊れた！", "The rune of protection is broken!"));
    }

    g_ptr->info &= ~(CAVE_MARK);
    g_ptr->info &= ~(CAVE_OBJECT);
    g_ptr->mimic = 0;
    turn_flags_ptr->do_move = true;
    note_spot(player_ptr, ny, nx);
    return true;
}

/*!
 * @brief 爆発のルーンを処理する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param ny モンスターのY座標
 * @param nx モンスターのX座標
 * @return モンスターが死亡した場合のみFALSE
 */
static bool process_explosive_rune(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MonsterEntity *m_ptr, POSITION ny, POSITION nx)
{
    auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[ny][nx];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    auto should_explode = turn_flags_ptr->do_move;
    should_explode &= g_ptr->is_rune_explosion();
    should_explode &= (r_ptr->behavior_flags.has_not(MonsterBehaviorType::NEVER_BLOW)) || !player_bold(player_ptr, ny, nx);
    if (!should_explode) {
        return true;
    }

    turn_flags_ptr->do_move = false;
    if (m_ptr->is_pet()) {
        return true;
    }

    if (randint1(BREAK_RUNE_EXPLOSION) > r_ptr->level) {
        if (g_ptr->info & CAVE_MARK) {
            msg_print(_("ルーンが爆発した！", "The rune explodes!"));
            BIT_FLAGS project_flags = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI;
            project(player_ptr, 0, 2, ny, nx, 2 * (player_ptr->lev + damroll(7, 7)), AttributeType::MANA, project_flags);
        }
    } else {
        msg_print(_("爆発のルーンは解除された。", "An explosive rune was disarmed."));
    }

    g_ptr->info &= ~(CAVE_MARK);
    g_ptr->info &= ~(CAVE_OBJECT);
    g_ptr->mimic = 0;

    note_spot(player_ptr, ny, nx);
    lite_spot(player_ptr, ny, nx);

    if (!m_ptr->is_valid()) {
        return false;
    }

    turn_flags_ptr->do_move = true;
    return true;
}

/*!
 * @brief モンスターが壁を掘った後続処理を実行する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param ny モンスターのY座標
 * @param nx モンスターのX座標
 * @return モンスターが死亡した場合のみFALSE
 */
static bool process_post_dig_wall(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MonsterEntity *m_ptr, POSITION ny, POSITION nx)
{
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    grid_type *g_ptr;
    g_ptr = &player_ptr->current_floor_ptr->grid_array[ny][nx];
    TerrainType *f_ptr;
    f_ptr = &terrains_info[g_ptr->feat];
    if (!turn_flags_ptr->did_kill_wall || !turn_flags_ptr->do_move) {
        return true;
    }

    constexpr auto chance_sound = 20;
    if (one_in_(chance_sound)) {
        if (f_ptr->flags.has(TerrainCharacteristics::GLASS)) {
            msg_print(_("何かの砕ける音が聞こえる。", "There is a crashing sound."));
        } else {
            msg_print(_("ギシギシいう音が聞こえる。", "There is a grinding sound."));
        }
    }

    cave_alter_feat(player_ptr, ny, nx, TerrainCharacteristics::HURT_DISI);

    if (!m_ptr->is_valid()) {
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(StatusRecalculatingFlag::FLOW);
        static constexpr auto flags = {
            SubWindowRedrawingFlag::OVERHEAD,
            SubWindowRedrawingFlag::DUNGEON,
        };
        rfu.set_flags(flags);
        if (is_original_ap_and_seen(player_ptr, m_ptr)) {
            r_ptr->r_feature_flags.set(MonsterFeatureType::KILL_WALL);
        }

        return false;
    }

    f_ptr = &terrains_info[g_ptr->feat];
    turn_flags_ptr->do_view = true;
    turn_flags_ptr->do_turn = true;
    return true;
}

/*!
 * @brief モンスターの移動に関するメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param mm モンスターの移動方向
 * @param oy 移動前の、モンスターのY座標
 * @param ox 移動前の、モンスターのX座標
 * @param count 移動回数 (のはず todo)
 * @return 移動が阻害される何か (ドア等)があったらFALSE
 * @todo 少し長いが、これといってブロックとしてまとまった部分もないので暫定でこのままとする
 */
bool process_monster_movement(PlayerType *player_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, DIRECTION *mm, POSITION oy, POSITION ox, int *count)
{
    for (int i = 0; mm[i]; i++) {
        int d = mm[i];
        if (d == 5) {
            d = ddd[randint0(8)];
        }

        POSITION ny = oy + ddy[d];
        POSITION nx = ox + ddx[d];
        if (!in_bounds2(player_ptr->current_floor_ptr, ny, nx)) {
            continue;
        }

        grid_type *g_ptr;
        g_ptr = &player_ptr->current_floor_ptr->grid_array[ny][nx];
        auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
        auto *r_ptr = &monraces_info[m_ptr->r_idx];
        bool can_cross = monster_can_cross_terrain(player_ptr, g_ptr->feat, r_ptr, turn_flags_ptr->is_riding_mon ? CEM_RIDING : 0);

        if (!process_wall(player_ptr, turn_flags_ptr, m_ptr, ny, nx, can_cross)) {
            if (!process_door(player_ptr, turn_flags_ptr, m_ptr, ny, nx)) {
                return false;
            }
        }

        if (!process_protection_rune(player_ptr, turn_flags_ptr, m_ptr, ny, nx)) {
            if (!process_explosive_rune(player_ptr, turn_flags_ptr, m_ptr, ny, nx)) {
                return false;
            }
        }

        exe_monster_attack_to_player(player_ptr, turn_flags_ptr, m_idx, ny, nx);
        if (process_monster_attack_to_monster(player_ptr, turn_flags_ptr, m_idx, g_ptr, can_cross)) {
            return false;
        }

        if (turn_flags_ptr->is_riding_mon) {
            const auto &m_ref = player_ptr->current_floor_ptr->m_list[player_ptr->riding];
            if (!player_ptr->riding_ryoute && !m_ref.is_fearful()) {
                turn_flags_ptr->do_move = false;
            }
        }

        if (!process_post_dig_wall(player_ptr, turn_flags_ptr, m_ptr, ny, nx)) {
            return false;
        }

        if (turn_flags_ptr->must_alter_to_move && r_ptr->feature_flags.has(MonsterFeatureType::AQUATIC)) {
            if (!monster_can_cross_terrain(player_ptr, g_ptr->feat, r_ptr, turn_flags_ptr->is_riding_mon ? CEM_RIDING : 0)) {
                turn_flags_ptr->do_move = false;
            }
        }

        if (turn_flags_ptr->do_move && !can_cross && !turn_flags_ptr->did_kill_wall && !turn_flags_ptr->did_bash_door) {
            turn_flags_ptr->do_move = false;
        }

        if (turn_flags_ptr->do_move && r_ptr->behavior_flags.has(MonsterBehaviorType::NEVER_MOVE)) {
            if (is_original_ap_and_seen(player_ptr, m_ptr)) {
                r_ptr->r_behavior_flags.set(MonsterBehaviorType::NEVER_MOVE);
            }

            turn_flags_ptr->do_move = false;
        }

        if (!turn_flags_ptr->do_move) {
            if (turn_flags_ptr->do_turn) {
                break;
            }

            continue;
        }

        turn_flags_ptr->do_turn = true;
        const auto &terrain_ref = terrains_info[g_ptr->feat];
        auto can_recover_energy = terrain_ref.flags.has(TerrainCharacteristics::TREE);
        can_recover_energy &= r_ptr->feature_flags.has_not(MonsterFeatureType::CAN_FLY);
        can_recover_energy &= r_ptr->wilderness_flags.has_not(MonsterWildernessType::WILD_WOOD);
        if (can_recover_energy) {
            m_ptr->energy_need += ENERGY_NEED();
        }

        if (!update_riding_monster(player_ptr, turn_flags_ptr, m_idx, oy, ox, ny, nx)) {
            break;
        }

        const auto &ap_r_ref = monraces_info[m_ptr->ap_r_idx];
        const auto is_projectable = projectable(player_ptr, player_ptr->y, player_ptr->x, m_ptr->fy, m_ptr->fx);
        const auto can_see = disturb_near && m_ptr->mflag.has(MonsterTemporaryFlagType::VIEW) && is_projectable;
        const auto is_high_level = disturb_high && (ap_r_ref.r_tkills > 0) && (ap_r_ref.level >= player_ptr->lev);
        if (m_ptr->ml && (disturb_move || can_see || is_high_level)) {
            if (m_ptr->is_hostile()) {
                disturb(player_ptr, false, true);
            }
        }

        bool is_takable_or_killable = !g_ptr->o_idx_list.empty();
        is_takable_or_killable &= r_ptr->behavior_flags.has_any_of({ MonsterBehaviorType::TAKE_ITEM, MonsterBehaviorType::KILL_ITEM });

        bool is_pickup_items = (player_ptr->pet_extra_flags & PF_PICKUP_ITEMS) != 0;
        is_pickup_items &= r_ptr->behavior_flags.has(MonsterBehaviorType::TAKE_ITEM);

        is_takable_or_killable &= !m_ptr->is_pet() || is_pickup_items;
        if (!is_takable_or_killable) {
            if (turn_flags_ptr->do_turn) {
                break;
            }

            continue;
        }

        update_object_by_monster_movement(player_ptr, turn_flags_ptr, m_idx, ny, nx);
        if (turn_flags_ptr->do_turn) {
            break;
        }

        (*count)++;
    }

    return true;
}

static bool can_speak(const MonsterRaceInfo &ap_r_ref, MonsterSpeakType mon_speak_type)
{
    const auto can_speak_all = ap_r_ref.speak_flags.has(MonsterSpeakType::SPEAK_ALL);
    const auto can_speak_specific = ap_r_ref.speak_flags.has(mon_speak_type);
    return can_speak_all || can_speak_specific;
}

static std::string_view get_speak_filename(MonsterEntity *m_ptr)
{
    const auto &ap_r_ref = monraces_info[m_ptr->ap_r_idx];
    if (m_ptr->is_fearful() && can_speak(ap_r_ref, MonsterSpeakType::SPEAK_FEAR)) {
        return _("monfear_j.txt", "monfear.txt");
    }

    constexpr auto monspeak_txt(_("monspeak_j.txt", "monspeak.txt"));
    if (m_ptr->is_pet() && can_speak(ap_r_ref, MonsterSpeakType::SPEAK_BATTLE)) {
        return monspeak_txt;
    }

    if (m_ptr->is_friendly() && can_speak(ap_r_ref, MonsterSpeakType::SPEAK_FRIEND)) {
        return _("monfrien_j.txt", "monfrien.txt");
    }

    if (can_speak(ap_r_ref, MonsterSpeakType::SPEAK_BATTLE)) {
        return monspeak_txt;
    }

    return "";
}

/*!
 * @brief モンスターを喋らせたり足音を立てたりする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param oy モンスターが元々いたY座標
 * @param ox モンスターが元々いたX座標
 * @param aware モンスターがプレイヤーに気付いているならばTRUE、超隠密状態ならばFALSE
 */
void process_speak_sound(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox, bool aware)
{
    if (player_ptr->phase_out) {
        return;
    }

    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    constexpr auto chance_noise = 20;
    if (m_ptr->ap_r_idx == MonsterRaceId::CYBER && one_in_(chance_noise) && !m_ptr->ml && (m_ptr->cdis <= MAX_PLAYER_SIGHT)) {
        if (disturb_minor) {
            disturb(player_ptr, false, false);
        }
        msg_print(_("重厚な足音が聞こえた。", "You hear heavy steps."));
    }

    auto can_speak = monraces_info[m_ptr->ap_r_idx].speak_flags.any();
    constexpr auto chance_speak = 8;
    if (!can_speak || !aware || !one_in_(chance_speak) || !player_has_los_bold(player_ptr, oy, ox) || !projectable(player_ptr, oy, ox, player_ptr->y, player_ptr->x)) {
        return;
    }

    const auto m_name = m_ptr->ml ? monster_desc(player_ptr, m_ptr, 0) : std::string(_("それ", "It"));
    auto filename = get_speak_filename(m_ptr);
    if (filename.empty()) {
        return;
    }

    const auto monmessage = get_random_line(filename.data(), enum2i(m_ptr->ap_r_idx));
    if (monmessage.has_value()) {
        msg_format(_("%s^%s", "%s^ %s"), m_name.data(), monmessage->data());
    }
}

/*!
 * @brief モンスターの目標地点をセットする / Set the target of counter attack
 * @param m_ptr モンスターの参照ポインタ
 * @param y 目標y座標
 * @param x 目標x座標
 */
void set_target(MonsterEntity *m_ptr, POSITION y, POSITION x)
{
    m_ptr->target_y = y;
    m_ptr->target_x = x;
}

/*!
 * @brief モンスターの目標地点をリセットする / Reset the target of counter attack
 * @param m_ptr モンスターの参照ポインタ
 */
void reset_target(MonsterEntity *m_ptr)
{
    set_target(m_ptr, 0, 0);
}
