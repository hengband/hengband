#include "floor/floor-leaver.h"
#include "cmd-building/cmd-building.h"
#include "floor/cave.h"
#include "floor/floor-events.h"
#include "floor/floor-mode-changer.h"
#include "floor/floor-save-util.h"
#include "floor/floor-save.h"
#include "floor/geometry.h"
#include "floor/line-of-sight.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "io/write-diary.h"
#include "mind/mind-ninja.h"
#include "monster-floor/monster-lite.h"
#include "monster-floor/monster-remover.h"
#include "monster-race/monster-race.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "pet/pet-util.h"
#include "save/floor-writer.h"
#include "spell-class/spells-mirror-master.h"
#include "system/artifact-type-definition.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

static void check_riding_preservation(PlayerType *player_ptr)
{
    if (!player_ptr->riding) {
        return;
    }

    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[player_ptr->riding];
    if (m_ptr->parent_m_idx) {
        player_ptr->riding = 0;
        player_ptr->pet_extra_flags &= ~(PF_TWO_HANDS);
        player_ptr->riding_ryoute = player_ptr->old_riding_ryoute = false;
    } else {
        party_mon[0] = *m_ptr;
        delete_monster_idx(player_ptr, player_ptr->riding);
    }
}

static bool check_pet_preservation_conditions(PlayerType *player_ptr, MonsterEntity *m_ptr)
{
    if (reinit_wilderness) {
        return false;
    }

    POSITION dis = distance(player_ptr->y, player_ptr->x, m_ptr->fy, m_ptr->fx);
    if (m_ptr->is_confused() || m_ptr->is_stunned() || m_ptr->is_asleep() || (m_ptr->parent_m_idx != 0)) {
        return true;
    }

    if (m_ptr->is_named() && ((player_has_los_bold(player_ptr, m_ptr->fy, m_ptr->fx) && projectable(player_ptr, player_ptr->y, player_ptr->x, m_ptr->fy, m_ptr->fx)) || (los(player_ptr, m_ptr->fy, m_ptr->fx, player_ptr->y, player_ptr->x) && projectable(player_ptr, m_ptr->fy, m_ptr->fx, player_ptr->y, player_ptr->x)))) {
        if (dis > 3) {
            return true;
        }
    } else if (dis > 1) {
        return true;
    }

    return false;
}

static void sweep_preserving_pet(PlayerType *player_ptr)
{
    if (player_ptr->wild_mode || player_ptr->current_floor_ptr->inside_arena || player_ptr->phase_out) {
        return;
    }

    for (MONSTER_IDX i = player_ptr->current_floor_ptr->m_max - 1, party_monster_num = 1; (i >= 1) && (party_monster_num < MAX_PARTY_MON); i--) {
        auto *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        if (!m_ptr->is_valid() || !m_ptr->is_pet() || (i == player_ptr->riding) || check_pet_preservation_conditions(player_ptr, m_ptr)) {
            continue;
        }

        party_mon[party_monster_num] = player_ptr->current_floor_ptr->m_list[i];
        party_monster_num++;
        delete_monster_idx(player_ptr, i);
    }
}

static void record_pet_diary(PlayerType *player_ptr)
{
    if (!record_named_pet) {
        return;
    }

    for (MONSTER_IDX i = player_ptr->current_floor_ptr->m_max - 1; i >= 1; i--) {
        auto *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        if (!m_ptr->is_valid() || !m_ptr->is_named_pet() || (player_ptr->riding == i)) {
            continue;
        }

        exe_write_diary(player_ptr, DiaryKind::NAMED_PET, RECORD_NAMED_PET_MOVED, monster_desc(player_ptr, m_ptr, MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE));
    }
}

/*!
 * @brief フロア移動時のペット保存処理 / Preserve_pets
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void preserve_pet(PlayerType *player_ptr)
{
    for (auto &mon : party_mon) {
        mon.r_idx = MonsterRace::empty_id();
    }

    check_riding_preservation(player_ptr);
    sweep_preserving_pet(player_ptr);
    record_pet_diary(player_ptr);
    for (MONSTER_IDX i = player_ptr->current_floor_ptr->m_max - 1; i >= 1; i--) {
        auto *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        const auto parent_r_idx = player_ptr->current_floor_ptr->m_list[m_ptr->parent_m_idx].r_idx;
        if ((m_ptr->parent_m_idx == 0) || MonsterRace(parent_r_idx).is_valid()) {
            continue;
        }

        if (is_seen(player_ptr, m_ptr)) {
            const auto m_name = monster_desc(player_ptr, m_ptr, 0);
            msg_format(_("%sは消え去った！", "%s^ disappears!"), m_name.data());
        }

        delete_monster_idx(player_ptr, i);
    }
}

/*!
 * @brief 新フロアに移動元フロアに繋がる階段を配置する / Virtually teleport onto the stairs that is connecting between two floors.
 * @param sf_ptr 移動元の保存フロア構造体参照ポインタ
 */
static void locate_connected_stairs(PlayerType *player_ptr, FloorType *floor_ptr, saved_floor_type *sf_ptr, BIT_FLAGS floor_mode)
{
    POSITION sx = 0;
    POSITION sy = 0;
    POSITION x_table[20];
    POSITION y_table[20];
    int num = 0;
    for (POSITION y = 0; y < floor_ptr->height; y++) {
        for (POSITION x = 0; x < floor_ptr->width; x++) {
            auto *g_ptr = &floor_ptr->grid_array[y][x];
            auto *f_ptr = &terrains_info[g_ptr->feat];
            bool ok = false;
            if (floor_mode & CFM_UP) {
                if (f_ptr->flags.has_all_of({ TerrainCharacteristics::LESS, TerrainCharacteristics::STAIRS }) && f_ptr->flags.has_not(TerrainCharacteristics::SPECIAL)) {
                    ok = true;
                    if (g_ptr->special && g_ptr->special == sf_ptr->upper_floor_id) {
                        sx = x;
                        sy = y;
                    }
                }
            } else if (floor_mode & CFM_DOWN) {
                if (f_ptr->flags.has_all_of({ TerrainCharacteristics::MORE, TerrainCharacteristics::STAIRS }) && f_ptr->flags.has_not(TerrainCharacteristics::SPECIAL)) {
                    ok = true;
                    if (g_ptr->special && g_ptr->special == sf_ptr->lower_floor_id) {
                        sx = x;
                        sy = y;
                    }
                }
            } else {
                if (f_ptr->flags.has(TerrainCharacteristics::BLDG)) {
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
        if (!feat_uses_special(floor_ptr->grid_array[player_ptr->y][player_ptr->x].feat)) {
            floor_ptr->grid_array[player_ptr->y][player_ptr->x].special = 0;
        }

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
    auto *floor_ptr = player_ptr->current_floor_ptr;
    MONSTER_IDX m_idx = floor_ptr->grid_array[oy][ox].m_idx;
    if (m_idx == 0) {
        return;
    }

    while (true) {
        MonsterEntity *m_ptr;
        POSITION ny = rand_spread(oy, dis);
        POSITION nx = rand_spread(ox, dis);
        tries++;
        if (tries > 10000) {
            return;
        }

        if (tries > 20 * dis * dis) {
            dis++;
        }

        if (!in_bounds(floor_ptr, ny, nx) || !is_cave_empty_bold(player_ptr, ny, nx) || floor_ptr->grid_array[ny][nx].is_rune_protection() || floor_ptr->grid_array[ny][nx].is_rune_explosion() || pattern_tile(floor_ptr, ny, nx)) {
            continue;
        }

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
    auto quest_r_idx = MonsterRace::empty_id();
    const auto &quest_list = QuestList::get_instance();
    const auto &floor = *player_ptr->current_floor_ptr;
    for (const auto &[q_idx, quest] : quest_list) {
        auto quest_relating_monster = (quest.status == QuestStatusType::TAKEN);
        quest_relating_monster &= ((quest.type == QuestKindType::KILL_LEVEL) || (quest.type == QuestKindType::RANDOM));
        quest_relating_monster &= (quest.level == floor.dun_level);
        quest_relating_monster &= (floor.dungeon_idx == quest.dungeon);
        quest_relating_monster &= !(quest.flags & QUEST_FLAG_PRESET);
        if (quest_relating_monster) {
            quest_r_idx = quest.r_idx;
        }
    }

    for (DUNGEON_IDX i = 1; i < floor.m_max; i++) {
        auto *m_ptr = &floor.m_list[i];
        if (!m_ptr->is_valid() || (quest_r_idx != m_ptr->r_idx)) {
            continue;
        }

        const auto &r_ref = m_ptr->get_real_r_ref();
        if (r_ref.kind_flags.has(MonsterKindType::UNIQUE) || (r_ref.population_flags.has(MonsterPopulationType::NAZGUL))) {
            continue;
        }

        delete_monster_idx(player_ptr, i);
    }

    for (DUNGEON_IDX i = 0; i < INVEN_PACK; i++) {
        auto *o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->is_valid()) {
            continue;
        }

        if (o_ptr->is_fixed_artifact()) {
            o_ptr->get_fixed_artifact().floor_id = 0;
        }
    }
}

static void set_grid_by_leaving_floor(PlayerType *player_ptr, grid_type **g_ptr)
{
    if ((player_ptr->change_floor_mode & CFM_SAVE_FLOORS) == 0) {
        return;
    }

    *g_ptr = &player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x];
    auto *f_ptr = &terrains_info[(*g_ptr)->feat];
    if ((*g_ptr)->special && f_ptr->flags.has_not(TerrainCharacteristics::SPECIAL) && get_sf_ptr((*g_ptr)->special)) {
        new_floor_id = (*g_ptr)->special;
    }

    if (f_ptr->flags.has_all_of({ TerrainCharacteristics::STAIRS, TerrainCharacteristics::SHAFT })) {
        prepare_change_floor_mode(player_ptr, CFM_SHAFT);
    }
}

static void jump_floors(PlayerType *player_ptr)
{
    const auto mode = player_ptr->change_floor_mode;
    if (none_bits(mode, CFM_DOWN | CFM_UP)) {
        return;
    }

    auto move_num = 0;
    if (any_bits(mode, CFM_DOWN)) {
        move_num = 1;
    } else if (any_bits(mode, CFM_UP)) {
        move_num = -1;
    }

    if (any_bits(mode, CFM_SHAFT)) {
        move_num *= 2;
    }

    auto &floor = *player_ptr->current_floor_ptr;
    const auto &dungeon = dungeons_info[floor.dungeon_idx];
    if (any_bits(mode, CFM_DOWN)) {
        if (!floor.is_in_dungeon()) {
            move_num = dungeon.mindepth;
        }
    } else if (any_bits(mode, CFM_UP)) {
        if (floor.dun_level + move_num < dungeon.mindepth) {
            move_num = -floor.dun_level;
        }
    }

    floor.dun_level += move_num;
}

static void exit_to_wilderness(PlayerType *player_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (floor.is_in_dungeon() || (floor.dungeon_idx == 0)) {
        return;
    }

    player_ptr->leaving_dungeon = true;
    if (!vanilla_town && !lite_town) {
        const auto &dungeon = dungeons_info[floor.dungeon_idx];
        player_ptr->wilderness_y = dungeon.dy;
        player_ptr->wilderness_x = dungeon.dx;
    }

    player_ptr->recall_dungeon = floor.dungeon_idx;
    player_ptr->word_recall = 0;
    floor.dungeon_idx = 0;
    player_ptr->change_floor_mode &= ~CFM_SAVE_FLOORS; // TODO
}

static void kill_saved_floors(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    if (!(player_ptr->change_floor_mode & CFM_SAVE_FLOORS)) {
        for (DUNGEON_IDX i = 0; i < MAX_SAVED_FLOORS; i++) {
            kill_saved_floor(player_ptr, &saved_floors[i]);
        }

        latest_visit_mark = 1;
        return;
    }
    if (player_ptr->change_floor_mode & CFM_NO_RETURN) {
        kill_saved_floor(player_ptr, sf_ptr);
    }
}

static void refresh_new_floor_id(PlayerType *player_ptr, grid_type *g_ptr)
{
    if (new_floor_id != 0) {
        return;
    }

    new_floor_id = get_new_floor_id(player_ptr);
    if ((g_ptr != nullptr) && !feat_uses_special(g_ptr->feat)) {
        g_ptr->special = new_floor_id;
    }
}

static void update_upper_lower_or_floor_id(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    if ((player_ptr->change_floor_mode & CFM_RAND_CONNECT) == 0) {
        return;
    }

    if (player_ptr->change_floor_mode & CFM_UP) {
        sf_ptr->upper_floor_id = new_floor_id;
    } else if (player_ptr->change_floor_mode & CFM_DOWN) {
        sf_ptr->lower_floor_id = new_floor_id;
    }
}

static void exe_leave_floor(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    grid_type *g_ptr = nullptr;
    set_grid_by_leaving_floor(player_ptr, &g_ptr);
    jump_floors(player_ptr);
    exit_to_wilderness(player_ptr);
    kill_saved_floors(player_ptr, sf_ptr);
    if (player_ptr->floor_id == 0) {
        return;
    }

    refresh_new_floor_id(player_ptr, g_ptr);
    update_upper_lower_or_floor_id(player_ptr, sf_ptr);
    if (((player_ptr->change_floor_mode & CFM_SAVE_FLOORS) == 0) || ((player_ptr->change_floor_mode & CFM_NO_RETURN) != 0)) {
        return;
    }

    get_out_monster(player_ptr);
    sf_ptr->last_visit = w_ptr->game_turn;
    forget_lite(player_ptr->current_floor_ptr);
    forget_view(player_ptr->current_floor_ptr);
    clear_mon_lite(player_ptr->current_floor_ptr);
    if (save_floor(player_ptr, sf_ptr, 0)) {
        return;
    }

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
    SpellsMirrorMaster(player_ptr).remove_all_mirrors(false);
    set_superstealth(player_ptr, false);

    new_floor_id = 0;

    preserve_info(player_ptr);
    saved_floor_type *sf_ptr = get_sf_ptr(player_ptr->floor_id);
    if (player_ptr->change_floor_mode & CFM_RAND_CONNECT) {
        locate_connected_stairs(player_ptr, player_ptr->current_floor_ptr, sf_ptr, player_ptr->change_floor_mode);
    }

    exe_leave_floor(player_ptr, sf_ptr);
}
