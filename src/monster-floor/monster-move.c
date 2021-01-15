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
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "game-option/disturbance-options.h"
#include "grid/feature.h"
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
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "pet/pet-util.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "player/player-status-flags.h"

static bool check_hp_for_feat_destruction(feature_type *f_ptr, monster_type *m_ptr)
{
    return !has_flag(f_ptr->flags, FF_GLASS) || (r_info[m_ptr->r_idx].flags2 & RF2_STUPID) || (m_ptr->hp >= MAX(m_ptr->maxhp / 3, 200));
}

/*!
 * @brief モンスターによる壁の透過・破壊を行う
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param ny モンスターのY座標
 * @param nx モンスターのX座標
 * @param can_cross モンスターが地形を踏破できるならばTRUE
 * @return 透過も破壊もしなかった場合はFALSE、それ以外はTRUE
 */
static bool process_wall(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx, bool can_cross)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    grid_type *g_ptr;
    g_ptr = &target_ptr->current_floor_ptr->grid_array[ny][nx];
    feature_type *f_ptr;
    f_ptr = &f_info[g_ptr->feat];
    if (player_bold(target_ptr, ny, nx)) {
        turn_flags_ptr->do_move = TRUE;
        return TRUE;
    }

    if (g_ptr->m_idx > 0) {
        turn_flags_ptr->do_move = TRUE;
        return TRUE;
    }

    if (((r_ptr->flags2 & RF2_KILL_WALL) != 0) && (can_cross ? !has_flag(f_ptr->flags, FF_LOS) : !turn_flags_ptr->is_riding_mon)
        && has_flag(f_ptr->flags, FF_HURT_DISI) && !has_flag(f_ptr->flags, FF_PERMANENT) && check_hp_for_feat_destruction(f_ptr, m_ptr)) {
        turn_flags_ptr->do_move = TRUE;
        if (!can_cross)
            turn_flags_ptr->must_alter_to_move = TRUE;

        turn_flags_ptr->did_kill_wall = TRUE;
        return TRUE;
    }

    if (!can_cross)
        return FALSE;

    turn_flags_ptr->do_move = TRUE;
    if (((r_ptr->flags2 & RF2_PASS_WALL) != 0) && (!turn_flags_ptr->is_riding_mon || has_pass_wall(target_ptr))
        && has_flag(f_ptr->flags, FF_CAN_PASS)) {
        turn_flags_ptr->did_pass_wall = TRUE;
    }

    return TRUE;
}

/*!
 * @brief モンスターが普通のドアを開ける処理
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param ny モンスターのY座標
 * @param nx モンスターのX座標
 * @return ここではドアを開けず、ガラスのドアを開ける可能性があるならTRUE
 */
static bool bash_normal_door(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    grid_type *g_ptr;
    g_ptr = &target_ptr->current_floor_ptr->grid_array[ny][nx];
    feature_type *f_ptr;
    f_ptr = &f_info[g_ptr->feat];
    turn_flags_ptr->do_move = FALSE;
    if (((r_ptr->flags2 & RF2_OPEN_DOOR) == 0) || !has_flag(f_ptr->flags, FF_OPEN) || (is_pet(m_ptr) && ((target_ptr->pet_extra_flags & PF_OPEN_DOORS) == 0)))
        return TRUE;

    if (f_ptr->power == 0) {
        turn_flags_ptr->did_open_door = TRUE;
        turn_flags_ptr->do_turn = TRUE;
        return FALSE;
    }

    if (randint0(m_ptr->hp / 10) > f_ptr->power) {
        cave_alter_feat(target_ptr, ny, nx, FF_DISARM);
        turn_flags_ptr->do_turn = TRUE;
        return FALSE;
    }

    return TRUE;
}

/*!
 * @brief モンスターがガラスのドアを開ける処理
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param g_ptr グリッドへの参照ポインタ
 * @param f_ptr 地形への参照ポインタ
 * @return なし
 */
static void bash_glass_door(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, feature_type *f_ptr, bool may_bash)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if (!may_bash || ((r_ptr->flags2 & RF2_BASH_DOOR) == 0) || !has_flag(f_ptr->flags, FF_BASH)
        || (is_pet(m_ptr) && ((target_ptr->pet_extra_flags & PF_OPEN_DOORS) == 0)))
        return;

    if (!check_hp_for_feat_destruction(f_ptr, m_ptr) || (randint0(m_ptr->hp / 10) <= f_ptr->power))
        return;

    if (has_flag(f_ptr->flags, FF_GLASS))
        msg_print(_("ガラスが砕ける音がした！", "You hear glass breaking!"));
    else
        msg_print(_("ドアを叩き開ける音がした！", "You hear a door burst open!"));

    if (disturb_minor)
        disturb(target_ptr, FALSE, FALSE);

    turn_flags_ptr->did_bash_door = TRUE;
    turn_flags_ptr->do_move = TRUE;
    turn_flags_ptr->must_alter_to_move = TRUE;
}

/*!
 * @brief モンスターによるドアの開放・破壊を行う
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param ny モンスターのY座標
 * @param nx モンスターのX座標
 * @return モンスターが死亡した場合のみFALSE
 */
static bool process_door(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    grid_type *g_ptr;
    g_ptr = &target_ptr->current_floor_ptr->grid_array[ny][nx];
    if (!is_closed_door(target_ptr, g_ptr->feat))
        return TRUE;

    feature_type *f_ptr;
    f_ptr = &f_info[g_ptr->feat];
    bool may_bash = bash_normal_door(target_ptr, turn_flags_ptr, m_ptr, ny, nx);
    bash_glass_door(target_ptr, turn_flags_ptr, m_ptr, f_ptr, may_bash);

    if (!turn_flags_ptr->did_open_door && !turn_flags_ptr->did_bash_door)
        return TRUE;

    if (turn_flags_ptr->did_bash_door
        && ((randint0(100) < 50) || (feat_state(target_ptr, g_ptr->feat, FF_OPEN) == g_ptr->feat) || has_flag(f_ptr->flags, FF_GLASS))) {
        cave_alter_feat(target_ptr, ny, nx, FF_BASH);
        if (!monster_is_valid(m_ptr)) {
            target_ptr->update |= (PU_FLOW);
            target_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
            if (is_original_ap_and_seen(target_ptr, m_ptr))
                r_ptr->r_flags2 |= (RF2_BASH_DOOR);

            return FALSE;
        }
    } else {
        cave_alter_feat(target_ptr, ny, nx, FF_OPEN);
    }

    f_ptr = &f_info[g_ptr->feat];
    turn_flags_ptr->do_view = TRUE;
    return TRUE;
}

/*!
 * @brief 守りのルーンによるモンスターの移動制限を処理する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param ny モンスターのY座標
 * @param nx モンスターのX座標
 * @return ルーンのある/なし
 */
static bool process_protection_rune(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx)
{
    grid_type *g_ptr;
    g_ptr = &target_ptr->current_floor_ptr->grid_array[ny][nx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if (!turn_flags_ptr->do_move || !is_glyph_grid(g_ptr) || (((r_ptr->flags1 & RF1_NEVER_BLOW) != 0) && player_bold(target_ptr, ny, nx)))
        return FALSE;

    turn_flags_ptr->do_move = FALSE;
    if (is_pet(m_ptr) || (randint1(BREAK_GLYPH) >= r_ptr->level))
        return TRUE;

    if (g_ptr->info & CAVE_MARK) {
        msg_print(_("守りのルーンが壊れた！", "The rune of protection is broken!"));
    }

    g_ptr->info &= ~(CAVE_MARK);
    g_ptr->info &= ~(CAVE_OBJECT);
    g_ptr->mimic = 0;
    turn_flags_ptr->do_move = TRUE;
    note_spot(target_ptr, ny, nx);
    return TRUE;
}

/*!
 * @brief 爆発のルーンを処理する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param ny モンスターのY座標
 * @param nx モンスターのX座標
 * @return モンスターが死亡した場合のみFALSE
 */
static bool process_explosive_rune(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx)
{
    grid_type *g_ptr;
    g_ptr = &target_ptr->current_floor_ptr->grid_array[ny][nx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if (!turn_flags_ptr->do_move || !is_explosive_rune_grid(g_ptr) || (((r_ptr->flags1 & RF1_NEVER_BLOW) != 0) && player_bold(target_ptr, ny, nx)))
        return TRUE;

    turn_flags_ptr->do_move = FALSE;
    if (is_pet(m_ptr))
        return TRUE;

    if (randint1(BREAK_MINOR_GLYPH) > r_ptr->level) {
        if (g_ptr->info & CAVE_MARK) {
            msg_print(_("ルーンが爆発した！", "The rune explodes!"));
            BIT_FLAGS project_flags = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI;
            project(target_ptr, 0, 2, ny, nx, 2 * (target_ptr->lev + damroll(7, 7)), GF_MANA, project_flags, -1);
        }
    } else {
        msg_print(_("爆発のルーンは解除された。", "An explosive rune was disarmed."));
    }

    g_ptr->info &= ~(CAVE_MARK);
    g_ptr->info &= ~(CAVE_OBJECT);
    g_ptr->mimic = 0;

    note_spot(target_ptr, ny, nx);
    lite_spot(target_ptr, ny, nx);

    if (!monster_is_valid(m_ptr))
        return FALSE;

    turn_flags_ptr->do_move = TRUE;
    return TRUE;
}

/*!
 * @brief モンスターが壁を掘った後続処理を実行する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @param ny モンスターのY座標
 * @param nx モンスターのX座標
 * @return モンスターが死亡した場合のみFALSE
 */
static bool process_post_dig_wall(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr, POSITION ny, POSITION nx)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    grid_type *g_ptr;
    g_ptr = &target_ptr->current_floor_ptr->grid_array[ny][nx];
    feature_type *f_ptr;
    f_ptr = &f_info[g_ptr->feat];
    if (!turn_flags_ptr->did_kill_wall || !turn_flags_ptr->do_move)
        return TRUE;

    if (one_in_(GRINDNOISE)) {
        if (has_flag(f_ptr->flags, FF_GLASS))
            msg_print(_("何かの砕ける音が聞こえる。", "There is a crashing sound."));
        else
            msg_print(_("ギシギシいう音が聞こえる。", "There is a grinding sound."));
    }

    cave_alter_feat(target_ptr, ny, nx, FF_HURT_DISI);

    if (!monster_is_valid(m_ptr)) {
        target_ptr->update |= (PU_FLOW);
        target_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
        if (is_original_ap_and_seen(target_ptr, m_ptr))
            r_ptr->r_flags2 |= (RF2_KILL_WALL);

        return FALSE;
    }

    f_ptr = &f_info[g_ptr->feat];
    turn_flags_ptr->do_view = TRUE;
    turn_flags_ptr->do_turn = TRUE;
    return TRUE;
}

/*!
 * todo 少し長いが、これといってブロックとしてまとまった部分もないので暫定でこのままとする
 * @brief モンスターの移動に関するメインルーチン
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param mm モンスターの移動方向
 * @param oy 移動前の、モンスターのY座標
 * @param ox 移動前の、モンスターのX座標
 * @param count 移動回数 (のはず todo)
 * @return 移動が阻害される何か (ドア等)があったらFALSE
 */
bool process_monster_movement(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, DIRECTION *mm, POSITION oy, POSITION ox, int *count)
{
    for (int i = 0; mm[i]; i++) {
        int d = mm[i];
        if (d == 5)
            d = ddd[randint0(8)];

        POSITION ny = oy + ddy[d];
        POSITION nx = ox + ddx[d];
        if (!in_bounds2(target_ptr->current_floor_ptr, ny, nx))
            continue;

        grid_type *g_ptr;
        g_ptr = &target_ptr->current_floor_ptr->grid_array[ny][nx];
        monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
        bool can_cross = monster_can_cross_terrain(target_ptr, g_ptr->feat, r_ptr, turn_flags_ptr->is_riding_mon ? CEM_RIDING : 0);

        /* Non stupid, non confused pets avoid attacking player */
        if (is_pet(m_ptr)
            && !(r_ptr->r_flags2 & RF2_STUPID)
            && !m_ptr->mtimed[MTIMED_CONFUSED]
            && player_bold(target_ptr, ny, nx)) {
            continue;
        }

        if (!process_wall(target_ptr, turn_flags_ptr, m_ptr, ny, nx, can_cross)) {
            if (!process_door(target_ptr, turn_flags_ptr, m_ptr, ny, nx))
                return FALSE;
        }

        if (!process_protection_rune(target_ptr, turn_flags_ptr, m_ptr, ny, nx)) {
            if (!process_explosive_rune(target_ptr, turn_flags_ptr, m_ptr, ny, nx))
                return FALSE;
        }

        exe_monster_attack_to_player(target_ptr, turn_flags_ptr, m_idx, ny, nx);
        if (process_monster_attack_to_monster(target_ptr, turn_flags_ptr, m_idx, g_ptr, can_cross))
            return FALSE;

        if (turn_flags_ptr->is_riding_mon) {
            if (!target_ptr->riding_ryoute && !monster_fear_remaining(&target_ptr->current_floor_ptr->m_list[target_ptr->riding]))
                turn_flags_ptr->do_move = FALSE;
        }

        if (!process_post_dig_wall(target_ptr, turn_flags_ptr, m_ptr, ny, nx))
            return FALSE;

        if (turn_flags_ptr->must_alter_to_move && (r_ptr->flags7 & RF7_AQUATIC)) {
            if (!monster_can_cross_terrain(target_ptr, g_ptr->feat, r_ptr, turn_flags_ptr->is_riding_mon ? CEM_RIDING : 0))
                turn_flags_ptr->do_move = FALSE;
        }

        if (turn_flags_ptr->do_move && !can_cross && !turn_flags_ptr->did_kill_wall && !turn_flags_ptr->did_bash_door)
            turn_flags_ptr->do_move = FALSE;

        if (turn_flags_ptr->do_move && (r_ptr->flags1 & RF1_NEVER_MOVE)) {
            if (is_original_ap_and_seen(target_ptr, m_ptr))
                r_ptr->r_flags1 |= (RF1_NEVER_MOVE);

            turn_flags_ptr->do_move = FALSE;
        }

        if (!turn_flags_ptr->do_move) {
            if (turn_flags_ptr->do_turn)
                break;

            continue;
        }

        turn_flags_ptr->do_turn = TRUE;
        feature_type *f_ptr;
        f_ptr = &f_info[g_ptr->feat];
        if (has_flag(f_ptr->flags, FF_TREE) && ((r_ptr->flags7 & RF7_CAN_FLY) == 0) && ((r_ptr->flags8 & RF8_WILD_WOOD) == 0))
            m_ptr->energy_need += ENERGY_NEED();

        if (!update_riding_monster(target_ptr, turn_flags_ptr, m_idx, oy, ox, ny, nx))
            break;

        monster_race *ap_r_ptr = &r_info[m_ptr->ap_r_idx];
        if (m_ptr->ml
            && (disturb_move || (disturb_near && (m_ptr->mflag & MFLAG_VIEW) && projectable(target_ptr, target_ptr->y, target_ptr->x, m_ptr->fy, m_ptr->fx))
                || (disturb_high && ap_r_ptr->r_tkills && ap_r_ptr->level >= target_ptr->lev))) {
            if (is_hostile(m_ptr))
                disturb(target_ptr, FALSE, TRUE);
        }

        bool is_takable_or_killable = g_ptr->o_idx > 0;
        is_takable_or_killable &= (r_ptr->flags2 & (RF2_TAKE_ITEM | RF2_KILL_ITEM)) != 0;

        bool is_pickup_items = (target_ptr->pet_extra_flags & PF_PICKUP_ITEMS) != 0;
        is_pickup_items &= (r_ptr->flags2 & RF2_TAKE_ITEM) != 0;

        is_takable_or_killable &= !is_pet(m_ptr) || is_pickup_items;
        if (!is_takable_or_killable) {
            if (turn_flags_ptr->do_turn)
                break;

            continue;
        }

        update_object_by_monster_movement(target_ptr, turn_flags_ptr, m_idx, ny, nx);
        if (turn_flags_ptr->do_turn)
            break;

        (*count)++;
    }

    return TRUE;
}

/*!
 * @brief モンスターを喋らせたり足音を立てたりする
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param oy モンスターが元々いたY座標
 * @param ox モンスターが元々いたX座標
 * @param aware モンスターがプレーヤーに気付いているならばTRUE、超隠密状態ならばFALSE
 * @return なし
 */
void process_speak_sound(player_type *target_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox, bool aware)
{
    if (target_ptr->phase_out)
        return;

    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *ap_r_ptr = &r_info[m_ptr->ap_r_idx];
    if (m_ptr->ap_r_idx == MON_CYBER && one_in_(CYBERNOISE) && !m_ptr->ml && (m_ptr->cdis <= MAX_SIGHT)) {
        if (disturb_minor)
            disturb(target_ptr, FALSE, FALSE);
        msg_print(_("重厚な足音が聞こえた。", "You hear heavy steps."));
    }

    if (((ap_r_ptr->flags2 & RF2_CAN_SPEAK) == 0) || !aware || !one_in_(SPEAK_CHANCE) || !player_has_los_bold(target_ptr, oy, ox)
        || !projectable(target_ptr, oy, ox, target_ptr->y, target_ptr->x))
        return;

    GAME_TEXT m_name[MAX_NLEN];
    char monmessage[1024];
    concptr filename;

    if (m_ptr->ml)
        monster_desc(target_ptr, m_name, m_ptr, 0);
    else
        strcpy(m_name, _("それ", "It"));

    if (monster_fear_remaining(m_ptr))
        filename = _("monfear_j.txt", "monfear.txt");
    else if (is_pet(m_ptr))
        filename = _("monpet_j.txt", "monpet.txt");
    else if (is_friendly(m_ptr))
        filename = _("monfrien_j.txt", "monfrien.txt");
    else
        filename = _("monspeak_j.txt", "monspeak.txt");

    if (get_rnd_line(filename, m_ptr->ap_r_idx, monmessage) == 0) {
        msg_format(_("%^s%s", "%^s %s"), m_name, monmessage);
    }
}

/*!
 * @brief モンスターの目標地点をセットする / Set the target of counter attack
 * @param m_ptr モンスターの参照ポインタ
 * @param y 目標y座標
 * @param x 目標x座標
 * @return なし
 */
void set_target(monster_type *m_ptr, POSITION y, POSITION x)
{
    m_ptr->target_y = y;
    m_ptr->target_x = x;
}

/*!
 * @brief モンスターの目標地点をリセットする / Reset the target of counter attack
 * @param m_ptr モンスターの参照ポインタ
 * @return なし
 */
void reset_target(monster_type *m_ptr) { set_target(m_ptr, 0, 0); }
