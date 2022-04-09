/*!
 * @brief モンスターの移動に関する処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster-floor/monster-move.h"
#include "core/disturbance.h"
#include "core/player-update-types.h"
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
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

static bool check_hp_for_feat_destruction(feature_type *f_ptr, monster_type *m_ptr)
{
    return f_ptr->flags.has_not(FloorFeatureType::GLASS) || r_info[m_ptr->r_idx].behavior_flags.has(MonsterBehaviorType::STUPID) || (m_ptr->hp >= std::max(m_ptr->maxhp / 3, 200));
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
static bool process_wall(PlayerType *player_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx, bool can_cross)
{
    auto *r_ptr = &r_info[m_ptr->r_idx];
    grid_type *g_ptr;
    g_ptr = &player_ptr->current_floor_ptr->grid_array[ny][nx];
    feature_type *f_ptr;
    f_ptr = &f_info[g_ptr->feat];
    if (player_bold(player_ptr, ny, nx)) {
        turn_flags_ptr->do_move = true;
        return true;
    }

    if (g_ptr->m_idx > 0) {
        turn_flags_ptr->do_move = true;
        return true;
    }

    if (r_ptr->feature_flags.has(MonsterFeatureType::KILL_WALL) && (can_cross ? f_ptr->flags.has_not(FloorFeatureType::LOS) : !turn_flags_ptr->is_riding_mon) && f_ptr->flags.has(FloorFeatureType::HURT_DISI) && f_ptr->flags.has_not(FloorFeatureType::PERMANENT) && check_hp_for_feat_destruction(f_ptr, m_ptr)) {
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
    if ((r_ptr->feature_flags.has(MonsterFeatureType::PASS_WALL)) && (!turn_flags_ptr->is_riding_mon || has_pass_wall(player_ptr)) && f_ptr->flags.has(FloorFeatureType::CAN_PASS)) {
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
static bool bash_normal_door(PlayerType *player_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx)
{
    auto *r_ptr = &r_info[m_ptr->r_idx];
    grid_type *g_ptr;
    g_ptr = &player_ptr->current_floor_ptr->grid_array[ny][nx];
    feature_type *f_ptr;
    f_ptr = &f_info[g_ptr->feat];
    turn_flags_ptr->do_move = false;
    if ((r_ptr->behavior_flags.has_not(MonsterBehaviorType::OPEN_DOOR)) || f_ptr->flags.has_not(FloorFeatureType::OPEN) || (is_pet(m_ptr) && ((player_ptr->pet_extra_flags & PF_OPEN_DOORS) == 0))) {
        return true;
    }

    if (f_ptr->power == 0) {
        turn_flags_ptr->did_open_door = true;
        turn_flags_ptr->do_turn = true;
        return false;
    }

    if (randint0(m_ptr->hp / 10) > f_ptr->power) {
        cave_alter_feat(player_ptr, ny, nx, FloorFeatureType::DISARM);
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
static void bash_glass_door(PlayerType *player_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, feature_type *f_ptr, bool may_bash)
{
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (!may_bash || (r_ptr->behavior_flags.has_not(MonsterBehaviorType::BASH_DOOR)) || f_ptr->flags.has_not(FloorFeatureType::BASH) || (is_pet(m_ptr) && ((player_ptr->pet_extra_flags & PF_OPEN_DOORS) == 0))) {
        return;
    }

    if (!check_hp_for_feat_destruction(f_ptr, m_ptr) || (randint0(m_ptr->hp / 10) <= f_ptr->power)) {
        return;
    }

    if (f_ptr->flags.has(FloorFeatureType::GLASS)) {
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
static bool process_door(PlayerType *player_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx)
{
    auto *r_ptr = &r_info[m_ptr->r_idx];
    grid_type *g_ptr;
    g_ptr = &player_ptr->current_floor_ptr->grid_array[ny][nx];
    if (!is_closed_door(player_ptr, g_ptr->feat)) {
        return true;
    }

    feature_type *f_ptr;
    f_ptr = &f_info[g_ptr->feat];
    bool may_bash = bash_normal_door(player_ptr, turn_flags_ptr, m_ptr, ny, nx);
    bash_glass_door(player_ptr, turn_flags_ptr, m_ptr, f_ptr, may_bash);

    if (!turn_flags_ptr->did_open_door && !turn_flags_ptr->did_bash_door) {
        return true;
    }

    if (turn_flags_ptr->did_bash_door && ((randint0(100) < 50) || (feat_state(player_ptr->current_floor_ptr, g_ptr->feat, FloorFeatureType::OPEN) == g_ptr->feat) || f_ptr->flags.has(FloorFeatureType::GLASS))) {
        cave_alter_feat(player_ptr, ny, nx, FloorFeatureType::BASH);
        if (!monster_is_valid(m_ptr)) {
            player_ptr->update |= (PU_FLOW);
            player_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
            if (is_original_ap_and_seen(player_ptr, m_ptr)) {
                r_ptr->r_behavior_flags.set(MonsterBehaviorType::BASH_DOOR);
            }

            return false;
        }
    } else {
        cave_alter_feat(player_ptr, ny, nx, FloorFeatureType::OPEN);
    }

    f_ptr = &f_info[g_ptr->feat];
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
 * @return ルーンのある/なし
 */
static bool process_protection_rune(PlayerType *player_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx)
{
    grid_type *g_ptr;
    g_ptr = &player_ptr->current_floor_ptr->grid_array[ny][nx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (!turn_flags_ptr->do_move || !g_ptr->is_rune_protection() || ((r_ptr->behavior_flags.has(MonsterBehaviorType::NEVER_BLOW)) && player_bold(player_ptr, ny, nx))) {
        return false;
    }

    turn_flags_ptr->do_move = false;
    if (is_pet(m_ptr) || (randint1(BREAK_RUNE_PROTECTION) >= r_ptr->level)) {
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
static bool process_explosive_rune(PlayerType *player_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx)
{
    grid_type *g_ptr;
    g_ptr = &player_ptr->current_floor_ptr->grid_array[ny][nx];
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (!turn_flags_ptr->do_move || !g_ptr->is_rune_explosion() || ((r_ptr->behavior_flags.has(MonsterBehaviorType::NEVER_BLOW)) && player_bold(player_ptr, ny, nx))) {
        return true;
    }

    turn_flags_ptr->do_move = false;
    if (is_pet(m_ptr)) {
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

    if (!monster_is_valid(m_ptr)) {
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
static bool process_post_dig_wall(PlayerType *player_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx)
{
    auto *r_ptr = &r_info[m_ptr->r_idx];
    grid_type *g_ptr;
    g_ptr = &player_ptr->current_floor_ptr->grid_array[ny][nx];
    feature_type *f_ptr;
    f_ptr = &f_info[g_ptr->feat];
    if (!turn_flags_ptr->did_kill_wall || !turn_flags_ptr->do_move) {
        return true;
    }

    if (one_in_(GRINDNOISE)) {
        if (f_ptr->flags.has(FloorFeatureType::GLASS)) {
            msg_print(_("何かの砕ける音が聞こえる。", "There is a crashing sound."));
        } else {
            msg_print(_("ギシギシいう音が聞こえる。", "There is a grinding sound."));
        }
    }

    cave_alter_feat(player_ptr, ny, nx, FloorFeatureType::HURT_DISI);

    if (!monster_is_valid(m_ptr)) {
        player_ptr->update |= (PU_FLOW);
        player_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
        if (is_original_ap_and_seen(player_ptr, m_ptr)) {
            r_ptr->r_feature_flags.set(MonsterFeatureType::KILL_WALL);
        }

        return false;
    }

    f_ptr = &f_info[g_ptr->feat];
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
        auto *r_ptr = &r_info[m_ptr->r_idx];
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
            if (!player_ptr->riding_ryoute && !monster_fear_remaining(&player_ptr->current_floor_ptr->m_list[player_ptr->riding])) {
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
        feature_type *f_ptr;
        f_ptr = &f_info[g_ptr->feat];
        if (f_ptr->flags.has(FloorFeatureType::TREE) && r_ptr->feature_flags.has_not(MonsterFeatureType::CAN_FLY) && (r_ptr->wilderness_flags.has_not(MonsterWildernessType::WILD_WOOD))) {
            m_ptr->energy_need += ENERGY_NEED();
        }

        if (!update_riding_monster(player_ptr, turn_flags_ptr, m_idx, oy, ox, ny, nx)) {
            break;
        }

        monster_race *ap_r_ptr = &r_info[m_ptr->ap_r_idx];
        if (m_ptr->ml && (disturb_move || (disturb_near && m_ptr->mflag.has(MonsterTemporaryFlagType::VIEW) && projectable(player_ptr, player_ptr->y, player_ptr->x, m_ptr->fy, m_ptr->fx)) || (disturb_high && ap_r_ptr->r_tkills && ap_r_ptr->level >= player_ptr->lev))) {
            if (is_hostile(m_ptr)) {
                disturb(player_ptr, false, true);
            }
        }

        bool is_takable_or_killable = !g_ptr->o_idx_list.empty();
        is_takable_or_killable &= r_ptr->behavior_flags.has_any_of({ MonsterBehaviorType::TAKE_ITEM, MonsterBehaviorType::KILL_ITEM });

        bool is_pickup_items = (player_ptr->pet_extra_flags & PF_PICKUP_ITEMS) != 0;
        is_pickup_items &= r_ptr->behavior_flags.has(MonsterBehaviorType::TAKE_ITEM);

        is_takable_or_killable &= !is_pet(m_ptr) || is_pickup_items;
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
    monster_race *ap_r_ptr = &r_info[m_ptr->ap_r_idx];
    if (m_ptr->ap_r_idx == MonsterRaceId::CYBER && one_in_(CYBERNOISE) && !m_ptr->ml && (m_ptr->cdis <= MAX_SIGHT)) {
        if (disturb_minor) {
            disturb(player_ptr, false, false);
        }
        msg_print(_("重厚な足音が聞こえた。", "You hear heavy steps."));
    }

    if (((ap_r_ptr->flags2 & RF2_CAN_SPEAK) == 0) || !aware || !one_in_(SPEAK_CHANCE) || !player_has_los_bold(player_ptr, oy, ox) || !projectable(player_ptr, oy, ox, player_ptr->y, player_ptr->x)) {
        return;
    }

    GAME_TEXT m_name[MAX_NLEN];
    char monmessage[1024];
    concptr filename;

    if (m_ptr->ml) {
        monster_desc(player_ptr, m_name, m_ptr, 0);
    } else {
        strcpy(m_name, _("それ", "It"));
    }

    if (monster_fear_remaining(m_ptr)) {
        filename = _("monfear_j.txt", "monfear.txt");
    } else if (is_pet(m_ptr)) {
        filename = _("monpet_j.txt", "monpet.txt");
    } else if (is_friendly(m_ptr)) {
        filename = _("monfrien_j.txt", "monfrien.txt");
    } else {
        filename = _("monspeak_j.txt", "monspeak.txt");
    }

    if (get_rnd_line(filename, enum2i(m_ptr->ap_r_idx), monmessage) == 0) {
        msg_format(_("%^s%s", "%^s %s"), m_name, monmessage);
    }
}

/*!
 * @brief モンスターの目標地点をセットする / Set the target of counter attack
 * @param m_ptr モンスターの参照ポインタ
 * @param y 目標y座標
 * @param x 目標x座標
 */
void set_target(monster_type *m_ptr, POSITION y, POSITION x)
{
    m_ptr->target_y = y;
    m_ptr->target_x = x;
}

/*!
 * @brief モンスターの目標地点をリセットする / Reset the target of counter attack
 * @param m_ptr モンスターの参照ポインタ
 */
void reset_target(monster_type *m_ptr)
{
    set_target(m_ptr, 0, 0);
}
