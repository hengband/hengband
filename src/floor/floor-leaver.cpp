#include "floor/floor-leaver.h"
#include "cmd-building/cmd-building.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/floor-events.h"
#include "floor/floor-mode-changer.h"
#include "floor/floor-save-util.h"
#include "floor/floor-save.h"
#include "floor/geometry.h"
#include "floor/line-of-sight.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "io/write-diary.h"
#include "mind/mind-mirror-master.h"
#include "mind/mind-ninja.h"
#include "monster-floor/monster-lite.h"
#include "monster-floor/monster-remover.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "pet/pet-util.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "player-status/player-energy.h"
#include "save/floor-writer.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

static void check_riding_preservation(PlayerType *player_ptr)
{
    if (!player_ptr->riding)
        return;

    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[player_ptr->riding];
    if (m_ptr->parent_m_idx) {
        player_ptr->riding = 0;
        player_ptr->pet_extra_flags &= ~(PF_TWO_HANDS);
        player_ptr->riding_ryoute = player_ptr->old_riding_ryoute = false;
    } else {
        party_mon[0] = *m_ptr;
        delete_monster_idx(player_ptr, player_ptr->riding);
    }
}

static bool check_pet_preservation_conditions(PlayerType *player_ptr, monster_type *m_ptr)
{
    if (reinit_wilderness)
        return false;

    POSITION dis = distance(player_ptr->y, player_ptr->x, m_ptr->fy, m_ptr->fx);
    if (monster_confused_remaining(m_ptr) || monster_stunned_remaining(m_ptr) || monster_csleep_remaining(m_ptr) || (m_ptr->parent_m_idx != 0))
        return true;

    if (m_ptr->nickname
        && ((player_has_los_bold(player_ptr, m_ptr->fy, m_ptr->fx) && projectable(player_ptr, player_ptr->y, player_ptr->x, m_ptr->fy, m_ptr->fx))
            || (los(player_ptr, m_ptr->fy, m_ptr->fx, player_ptr->y, player_ptr->x)
                && projectable(player_ptr, m_ptr->fy, m_ptr->fx, player_ptr->y, player_ptr->x)))) {
        if (dis > 3)
            return true;
    } else if (dis > 1)
        return true;

    return false;
}

static void sweep_preserving_pet(PlayerType *player_ptr)
{
    if (player_ptr->wild_mode || player_ptr->current_floor_ptr->inside_arena || player_ptr->phase_out)
        return;

    for (MONSTER_IDX i = player_ptr->current_floor_ptr->m_max - 1, party_monster_num = 1; (i >= 1) && (party_monster_num < MAX_PARTY_MON); i--) {
        monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr) || !is_pet(m_ptr) || (i == player_ptr->riding) || check_pet_preservation_conditions(player_ptr, m_ptr))
            continue;

        party_mon[party_monster_num] = player_ptr->current_floor_ptr->m_list[i];
        party_monster_num++;
        delete_monster_idx(player_ptr, i);
    }
}

static void record_pet_diary(PlayerType *player_ptr)
{
    if (!record_named_pet)
        return;

    for (MONSTER_IDX i = player_ptr->current_floor_ptr->m_max - 1; i >= 1; i--) {
        monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        GAME_TEXT m_name[MAX_NLEN];
        if (!monster_is_valid(m_ptr) || !is_pet(m_ptr) || !m_ptr->nickname || (player_ptr->riding == i))
            continue;

        monster_desc(player_ptr, m_name, m_ptr, MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE);
        exe_write_diary(player_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_MOVED, m_name);
    }
}

/*!
 * @brief フロア移動時のペット保存処理 / Preserve_pets
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void preserve_pet(PlayerType *player_ptr)
{
    for (MONSTER_IDX party_monster_num = 0; party_monster_num < MAX_PARTY_MON; party_monster_num++)
        party_mon[party_monster_num].r_idx = 0;

    check_riding_preservation(player_ptr);
    sweep_preserving_pet(player_ptr);
    record_pet_diary(player_ptr);
    for (MONSTER_IDX i = player_ptr->current_floor_ptr->m_max - 1; i >= 1; i--) {
        monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        if ((m_ptr->parent_m_idx == 0) || (player_ptr->current_floor_ptr->m_list[m_ptr->parent_m_idx].r_idx != 0))
            continue;

        if (is_seen(player_ptr, m_ptr)) {
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(player_ptr, m_name, m_ptr, 0);
            msg_format(_("%sは消え去った！", "%^s disappears!"), m_name);
        }

        delete_monster_idx(player_ptr, i);
    }
}

/*!
 * @brief 新フロアに移動元フロアに繋がる階段を配置する / Virtually teleport onto the stairs that is connecting between two floors.
 * @param sf_ptr 移動元の保存フロア構造体参照ポインタ
 */
static void locate_connected_stairs(PlayerType *player_ptr, floor_type *floor_ptr, saved_floor_type *sf_ptr, BIT_FLAGS floor_mode)
{
    POSITION sx = 0;
    POSITION sy = 0;
    POSITION x_table[20];
    POSITION y_table[20];
    int num = 0;
    for (POSITION y = 0; y < floor_ptr->height; y++) {
        for (POSITION x = 0; x < floor_ptr->width; x++) {
            grid_type *g_ptr = &floor_ptr->grid_array[y][x];
            feature_type *f_ptr = &f_info[g_ptr->feat];
            bool ok = false;
            if (floor_mode & CFM_UP) {
                if (f_ptr->flags.has_all_of({FloorFeatureType::LESS, FloorFeatureType::STAIRS}) && f_ptr->flags.has_not(FloorFeatureType::SPECIAL)) {
                    ok = true;
                    if (g_ptr->special && g_ptr->special == sf_ptr->upper_floor_id) {
                        sx = x;
                        sy = y;
                    }
                }
            } else if (floor_mode & CFM_DOWN) {
                if (f_ptr->flags.has_all_of({FloorFeatureType::MORE, FloorFeatureType::STAIRS}) && f_ptr->flags.has_not(FloorFeatureType::SPECIAL)) {
                    ok = true;
                    if (g_ptr->special && g_ptr->special == sf_ptr->lower_floor_id) {
                        sx = x;
                        sy = y;
                    }
                }
            } else {
                if (f_ptr->flags.has(FloorFeatureType::BLDG)) {
                    ok = true;
                }
            }

            if (ok && (num < 20)) {
                x_table[num] = x;
                y_table[num] = y;
                num++;
            }
        }
    }

    if (sx) {
        player_ptr->y = sy;
        player_ptr->x = sx;
        return;
    }

    if (num == 0) {
        prepare_change_floor_mode(player_ptr, CFM_RAND_PLACE | CFM_NO_RETURN);
        if (!feat_uses_special(floor_ptr->grid_array[player_ptr->y][player_ptr->x].feat))
            floor_ptr->grid_array[player_ptr->y][player_ptr->x].special = 0;

        return;
    }

    int i = randint0(num);
    player_ptr->y = y_table[i];
    player_ptr->x = x_table[i];
}

/*!
 * @brief フロア移動時、プレイヤーの移動先モンスターが既にいた場合ランダムな近隣に移動させる / When a monster is at a place where player will return,
 */
static void get_out_monster(PlayerType *player_ptr)
{
    int tries = 0;
    POSITION dis = 1;
    POSITION oy = player_ptr->y;
    POSITION ox = player_ptr->x;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    MONSTER_IDX m_idx = floor_ptr->grid_array[oy][ox].m_idx;
    if (m_idx == 0)
        return;

    while (true) {
        monster_type *m_ptr;
        POSITION ny = rand_spread(oy, dis);
        POSITION nx = rand_spread(ox, dis);
        tries++;
        if (tries > 10000)
            return;

        if (tries > 20 * dis * dis)
            dis++;

        if (!in_bounds(floor_ptr, ny, nx) || !is_cave_empty_bold(player_ptr, ny, nx) || floor_ptr->grid_array[ny][nx].is_rune_protection()
            || floor_ptr->grid_array[ny][nx].is_rune_explosion() || pattern_tile(floor_ptr, ny, nx))
            continue;

        m_ptr = &floor_ptr->m_list[m_idx];
        floor_ptr->grid_array[oy][ox].m_idx = 0;
        floor_ptr->grid_array[ny][nx].m_idx = m_idx;
        m_ptr->fy = ny;
        m_ptr->fx = nx;
        return;
    }
}

/*!
 * @brief クエスト・フロア内のモンスター・インベントリ情報を保存する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void preserve_info(PlayerType *player_ptr)
{
    MONRACE_IDX quest_r_idx = 0;
    for (DUNGEON_IDX i = 0; i < max_q_idx; i++) {
        if ((quest[i].status == QuestStatusType::TAKEN) && ((quest[i].type == QuestKindType::KILL_LEVEL) || (quest[i].type == QuestKindType::RANDOM))
            && (quest[i].level == player_ptr->current_floor_ptr->dun_level) && (player_ptr->dungeon_idx == quest[i].dungeon)
            && !(quest[i].flags & QUEST_FLAG_PRESET)) {
            quest_r_idx = quest[i].r_idx;
        }
    }

    for (DUNGEON_IDX i = 1; i < player_ptr->current_floor_ptr->m_max; i++) {
        monster_race *r_ptr;
        monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr) || (quest_r_idx != m_ptr->r_idx))
            continue;

        r_ptr = real_r_ptr(m_ptr);
        if ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL))
            continue;

        delete_monster_idx(player_ptr, i);
    }

    for (DUNGEON_IDX i = 0; i < INVEN_PACK; i++) {
        object_type *o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->is_valid())
            continue;

        if (o_ptr->is_fixed_artifact())
            a_info[o_ptr->name1].floor_id = 0;
    }
}

static void set_grid_by_leaving_floor(PlayerType *player_ptr, grid_type **g_ptr)
{
    if ((player_ptr->change_floor_mode & CFM_SAVE_FLOORS) == 0)
        return;

    *g_ptr = &player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x];
    feature_type *f_ptr =  &f_info[(*g_ptr)->feat];
    if ((*g_ptr)->special && f_ptr->flags.has_not(FloorFeatureType::SPECIAL) && get_sf_ptr((*g_ptr)->special))
        new_floor_id = (*g_ptr)->special;

    if (f_ptr->flags.has_all_of({FloorFeatureType::STAIRS, FloorFeatureType::SHAFT}))
        prepare_change_floor_mode(player_ptr, CFM_SHAFT);
}

static void jump_floors(PlayerType *player_ptr)
{
    if (none_bits(player_ptr->change_floor_mode, CFM_DOWN | CFM_UP)) {
        return;
    }

    auto move_num = 0;
    if (any_bits(player_ptr->change_floor_mode, CFM_DOWN)) {
        move_num = 1;
    } else if (any_bits(player_ptr->change_floor_mode, CFM_UP)) {
        move_num = -1;
    }

    if (any_bits(player_ptr->change_floor_mode, CFM_SHAFT)) {
        move_num *= 2;
    }

    if (any_bits(player_ptr->change_floor_mode, CFM_DOWN)) {
        if (!is_in_dungeon(player_ptr)) {
            move_num = d_info[player_ptr->dungeon_idx].mindepth;
        }
    } else if (any_bits(player_ptr->change_floor_mode, CFM_UP)) {
        if (player_ptr->current_floor_ptr->dun_level + move_num < d_info[player_ptr->dungeon_idx].mindepth) {
            move_num = -player_ptr->current_floor_ptr->dun_level;
        }
    }

    player_ptr->current_floor_ptr->dun_level += move_num;
}

static void exit_to_wilderness(PlayerType *player_ptr)
{
    if (is_in_dungeon(player_ptr) || (player_ptr->dungeon_idx == 0))
        return;

    player_ptr->leaving_dungeon = true;
    if (!vanilla_town && !lite_town) {
        player_ptr->wilderness_y = d_info[player_ptr->dungeon_idx].dy;
        player_ptr->wilderness_x = d_info[player_ptr->dungeon_idx].dx;
    }

    player_ptr->recall_dungeon = player_ptr->dungeon_idx;
    player_ptr->word_recall = 0;
    player_ptr->dungeon_idx = 0;
    player_ptr->change_floor_mode &= ~CFM_SAVE_FLOORS; // TODO
}

static void kill_saved_floors(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    if (!(player_ptr->change_floor_mode & CFM_SAVE_FLOORS)) {
        for (DUNGEON_IDX i = 0; i < MAX_SAVED_FLOORS; i++)
            kill_saved_floor(player_ptr, &saved_floors[i]);

        latest_visit_mark = 1;
        return;
    }
    
    if (player_ptr->change_floor_mode & CFM_NO_RETURN)
        kill_saved_floor(player_ptr, sf_ptr);
}

static void refresh_new_floor_id(PlayerType *player_ptr, grid_type *g_ptr)
{
    if (new_floor_id != 0)
        return;

    new_floor_id = get_new_floor_id(player_ptr);
    if ((g_ptr != nullptr) && !feat_uses_special(g_ptr->feat))
        g_ptr->special = new_floor_id;
}

static void update_upper_lower_or_floor_id(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    if ((player_ptr->change_floor_mode & CFM_RAND_CONNECT) == 0)
        return;

    if (player_ptr->change_floor_mode & CFM_UP)
        sf_ptr->upper_floor_id = new_floor_id;
    else if (player_ptr->change_floor_mode & CFM_DOWN)
        sf_ptr->lower_floor_id = new_floor_id;
}

static void exe_leave_floor(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    grid_type *g_ptr = nullptr;
    set_grid_by_leaving_floor(player_ptr, &g_ptr);
    jump_floors(player_ptr);
    exit_to_wilderness(player_ptr);
    kill_saved_floors(player_ptr, sf_ptr);
    if (player_ptr->floor_id == 0)
        return;

    refresh_new_floor_id(player_ptr, g_ptr);
    update_upper_lower_or_floor_id(player_ptr, sf_ptr);
    if (((player_ptr->change_floor_mode & CFM_SAVE_FLOORS) == 0) || ((player_ptr->change_floor_mode & CFM_NO_RETURN) != 0))
        return;

    get_out_monster(player_ptr);
    sf_ptr->last_visit = w_ptr->game_turn;
    forget_lite(player_ptr->current_floor_ptr);
    forget_view(player_ptr->current_floor_ptr);
    clear_mon_lite(player_ptr->current_floor_ptr);
    if (save_floor(player_ptr, sf_ptr, 0))
        return;

    prepare_change_floor_mode(player_ptr, CFM_NO_RETURN);
    kill_saved_floor(player_ptr, get_sf_ptr(player_ptr->floor_id));
}

/*!
 * @brief 現在のフロアを離れるに伴って行なわれる保存処理
 * / Maintain quest monsters, mark next floor_id at stairs, save current floor, and prepare to enter next floor.
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void leave_floor(PlayerType *player_ptr)
{
    preserve_pet(player_ptr);
    remove_all_mirrors(player_ptr, false);
    set_superstealth(player_ptr, false);

    new_floor_id = 0;

    preserve_info(player_ptr);
    saved_floor_type *sf_ptr = get_sf_ptr(player_ptr->floor_id);
    if (player_ptr->change_floor_mode & CFM_RAND_CONNECT)
        locate_connected_stairs(player_ptr, player_ptr->current_floor_ptr, sf_ptr, player_ptr->change_floor_mode);

    exe_leave_floor(player_ptr, sf_ptr);
}

/*!
 * @brief 任意のダンジョン及び階層に飛ぶ
 * Go to any level
 */
void jump_floor(PlayerType *player_ptr, DUNGEON_IDX dun_idx, DEPTH depth)
{
    player_ptr->dungeon_idx = dun_idx;
    player_ptr->current_floor_ptr->dun_level = depth;
    prepare_change_floor_mode(player_ptr, CFM_RAND_PLACE);
    if (!is_in_dungeon(player_ptr))
        player_ptr->dungeon_idx = 0;

    player_ptr->current_floor_ptr->inside_arena = false;
    player_ptr->wild_mode = false;
    leave_quest_check(player_ptr);
    if (record_stair)
        exe_write_diary(player_ptr, DIARY_WIZ_TELE, 0, nullptr);

    player_ptr->current_floor_ptr->inside_quest = 0;
    PlayerEnergy(player_ptr).reset_player_turn();
    player_ptr->energy_need = 0;
    prepare_change_floor_mode(player_ptr, CFM_FIRST_FLOOR);
    player_ptr->leaving = true;
}
