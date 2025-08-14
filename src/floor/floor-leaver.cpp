#include "floor/floor-leaver.h"
#include "dungeon/quest.h"
#include "floor/floor-events.h"
#include "floor/floor-mode-changer.h"
#include "floor/floor-save-util.h"
#include "floor/floor-save.h"
#include "floor/line-of-sight.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "inventory/inventory-slot-types.h"
#include "io/write-diary.h"
#include "mind/mind-ninja.h"
#include "monster-floor/monster-lite.h"
#include "monster-floor/monster-remover.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "pet/pet-util.h"
#include "save/floor-writer.h"
#include "spell-class/spells-mirror-master.h"
#include "system/artifact-type-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/floor-info.h"
#include "system/floor/wilderness-grid.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "target/projection-path-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

static void check_riding_preservation(PlayerType *player_ptr)
{
    if (!player_ptr->riding) {
        return;
    }

    const auto &monster = player_ptr->current_floor_ptr->m_list[player_ptr->riding];
    if (monster.has_parent()) {
        player_ptr->ride_monster(0);
        player_ptr->pet_extra_flags &= ~(PF_TWO_HANDS);
        player_ptr->riding_ryoute = player_ptr->old_riding_ryoute = false;
    } else {
        party_mon[0] = monster.clone();
        delete_monster_idx(player_ptr, player_ptr->riding);
    }
}

static bool check_pet_preservation_conditions(PlayerType *player_ptr, const MonsterEntity &monster)
{
    if (WildernessGrids::get_instance().should_reinitialize()) {
        return false;
    }

    const auto p_pos = player_ptr->get_position();
    const auto m_pos = monster.get_position();
    const auto dis = Grid::calc_distance(p_pos, m_pos);
    if (monster.is_confused() || monster.is_stunned() || monster.is_asleep() || monster.has_parent()) {
        return true;
    }

    const auto should_preserve = monster.is_named();
    const auto &floor = *player_ptr->current_floor_ptr;
    auto sight_from_player = floor.has_los_at(m_pos);
    sight_from_player &= projectable(floor, p_pos, m_pos);
    auto sight_from_monster = los(floor, m_pos, p_pos);
    sight_from_monster &= projectable(floor, m_pos, p_pos);
    if (should_preserve && (sight_from_player || sight_from_monster)) {
        return dis > 3;
    }

    return dis > 1;
}

static void sweep_preserving_pet(PlayerType *player_ptr)
{
    if (AngbandWorld::get_instance().is_wild_mode() || player_ptr->current_floor_ptr->inside_arena || AngbandSystem::get_instance().is_phase_out()) {
        return;
    }

    for (MONSTER_IDX i = player_ptr->current_floor_ptr->m_max - 1, party_monster_num = 1; (i >= 1) && (party_monster_num < MAX_PARTY_MON); i--) {
        const auto &monster = player_ptr->current_floor_ptr->m_list[i];
        if (!monster.is_valid() || !monster.is_pet() || monster.is_riding() || check_pet_preservation_conditions(player_ptr, monster)) {
            continue;
        }

        party_mon[party_monster_num] = player_ptr->current_floor_ptr->m_list[i].clone();
        party_monster_num++;
        delete_monster_idx(player_ptr, i);
    }
}

static void record_pet_diary(PlayerType *player_ptr)
{
    if (!record_named_pet) {
        return;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    for (MONSTER_IDX i = floor.m_max - 1; i >= 1; i--) {
        const auto &monster = floor.m_list[i];
        if (!monster.is_valid() || !monster.is_named_pet() || monster.is_riding()) {
            continue;
        }

        exe_write_diary(floor, DiaryKind::NAMED_PET, RECORD_NAMED_PET_MOVED, monster_desc(player_ptr, monster, MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE));
    }
}

/*!
 * @brief フロア移動時のペット保存処理 / Preserve_pets
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void preserve_pet(PlayerType *player_ptr)
{
    for (auto &mon : party_mon) {
        mon.r_idx = MonraceList::empty_id();
    }

    check_riding_preservation(player_ptr);
    sweep_preserving_pet(player_ptr);
    record_pet_diary(player_ptr);
    for (MONSTER_IDX i = player_ptr->current_floor_ptr->m_max - 1; i >= 1; i--) {
        const auto &monster = player_ptr->current_floor_ptr->m_list[i];
        const auto parent_r_idx = player_ptr->current_floor_ptr->m_list[monster.parent_m_idx].r_idx;
        if (!monster.has_parent() || MonraceList::is_valid(parent_r_idx)) {
            continue;
        }

        if (is_seen(player_ptr, monster)) {
            const auto m_name = monster_desc(player_ptr, monster, 0);
            msg_format(_("%sは消え去った！", "%s^ disappears!"), m_name.data());
        }

        delete_monster_idx(player_ptr, i);
    }
}

/*!
 * @brief 新フロアに移動元フロアに繋がる階段を配置する / Virtually teleport onto the stairs that is connecting between two floors.
 * @param sf_ptr 移動元の保存フロア構造体参照ポインタ
 */
static void locate_connected_stairs(PlayerType *player_ptr, FloorType &floor, saved_floor_type *sf_ptr)
{
    auto sx = 0;
    auto sy = 0;
    int x_table[20]{};
    int y_table[20]{};
    auto num = 0;
    const auto &fcms = FloorChangeModesStore::get_instace();
    for (const auto &pos : floor.get_area()) {
        const auto &grid = floor.get_grid(pos);
        const auto &terrain = grid.get_terrain();
        auto ok = false;
        if (fcms->has(FloorChangeMode::UP)) {
            if (terrain.flags.has_all_of({ TerrainCharacteristics::LESS, TerrainCharacteristics::STAIRS }) && terrain.flags.has_not(TerrainCharacteristics::SPECIAL)) {
                ok = true;
                if (grid.special && grid.special == sf_ptr->upper_floor_id) {
                    sx = pos.x;
                    sy = pos.y;
                }
            }
        } else if (fcms->has(FloorChangeMode::DOWN)) {
            if (terrain.flags.has_all_of({ TerrainCharacteristics::MORE, TerrainCharacteristics::STAIRS }) && terrain.flags.has_not(TerrainCharacteristics::SPECIAL)) {
                ok = true;
                if (grid.special && grid.special == sf_ptr->lower_floor_id) {
                    sx = pos.x;
                    sy = pos.y;
                }
            }
        } else {
            if (terrain.flags.has(TerrainCharacteristics::BLDG)) {
                ok = true;
            }
        }

        if (ok && (num < 20)) {
            x_table[num] = pos.x;
            y_table[num] = pos.y;
            num++;
        }
    }

    if (sx) {
        player_ptr->y = sy;
        player_ptr->x = sx;
        return;
    }

    if (num == 0) {
        FloorChangeModesStore::get_instace()->set({ FloorChangeMode::RANDOM_PLACE, FloorChangeMode::NO_RETURN });
        auto &grid = floor.get_grid(player_ptr->get_position());
        if (!grid.has_special_terrain()) {
            grid.special = 0;
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
    auto tries = 0;
    auto dis = 1;
    const auto p_pos = player_ptr->get_position();
    auto &floor = *player_ptr->current_floor_ptr;
    auto &p_grid = floor.get_grid(p_pos);
    const auto m_idx = p_grid.m_idx;
    if (m_idx == 0) {
        return;
    }

    while (true) {
        auto y = rand_spread(p_pos.y, dis);
        auto x = rand_spread(p_pos.x, dis);
        const Pos2D pos(y, x); //!< @details 乱数引数の評価順を固定.
        tries++;
        if (tries > 10000) {
            return;
        }

        if (tries > 20 * dis * dis) {
            dis++;
        }

        auto &grid = floor.get_grid(pos);
        if (!floor.contains(pos) || !floor.is_empty_at(pos) || (pos == p_pos) || grid.is_rune_protection() || grid.is_rune_explosion() || grid.has(TerrainCharacteristics::PATTERN)) {
            continue;
        }

        p_grid.m_idx = 0;
        grid.m_idx = m_idx;
        auto &monster = floor.m_list[m_idx];
        monster.set_position(pos);
        return;
    }
}

/*!
 * @brief クエスト・フロア内のモンスター・インベントリ情報を保存する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void preserve_info(PlayerType *player_ptr)
{
    auto quest_monrace_id = MonraceList::empty_id();
    const auto &quests = QuestList::get_instance();
    const auto &floor = *player_ptr->current_floor_ptr;
    for (const auto &[quest_id, quest] : quests) {
        auto quest_relating_monster = (quest.status == QuestStatusType::TAKEN);
        quest_relating_monster &= ((quest.type == QuestKindType::KILL_LEVEL) || (quest.type == QuestKindType::RANDOM));
        quest_relating_monster &= (quest.level == floor.dun_level);
        quest_relating_monster &= (floor.dungeon_id == quest.dungeon);
        quest_relating_monster &= !(quest.flags & QUEST_FLAG_PRESET);
        if (quest_relating_monster) {
            quest_monrace_id = quest.r_idx;
        }
    }

    for (short i = 1; i < floor.m_max; i++) {
        const auto &monster = floor.m_list[i];
        if (!monster.is_valid() || (quest_monrace_id != monster.r_idx)) {
            continue;
        }

        const auto &r_ref = monster.get_real_monrace();
        if (r_ref.kind_flags.has(MonsterKindType::UNIQUE) || (r_ref.population_flags.has(MonsterPopulationType::NAZGUL))) {
            continue;
        }

        delete_monster_idx(player_ptr, i);
    }

    for (short i = 0; i < INVEN_PACK; i++) {
        auto *o_ptr = player_ptr->inventory[i].get();
        if (!o_ptr->is_valid()) {
            continue;
        }

        if (o_ptr->is_fixed_artifact()) {
            o_ptr->get_fixed_artifact().floor_id = 0;
        }
    }
}

static Grid *set_grid_by_leaving_floor(FloorType &floor, const Pos2D &p_pos)
{
    if (FloorChangeModesStore::get_instace()->has_not(FloorChangeMode::SAVE_FLOORS)) {
        return nullptr;
    }

    auto &grid = floor.get_grid(p_pos);
    const auto &terrain = grid.get_terrain();
    if (grid.special && terrain.flags.has_not(TerrainCharacteristics::SPECIAL) && get_sf_ptr(grid.special)) {
        new_floor_id = grid.special;
    }

    if (terrain.flags.has_all_of({ TerrainCharacteristics::STAIRS, TerrainCharacteristics::SHAFT })) {
        FloorChangeModesStore::get_instace()->set(FloorChangeMode::SHAFT);
    }

    return &grid;
}

static void jump_floors(FloorType &floor)
{
    const auto &fcms = FloorChangeModesStore::get_instace();
    if (fcms->has_none_of({ FloorChangeMode::DOWN, FloorChangeMode::UP })) {
        return;
    }

    auto move_num = 0;
    if (fcms->has(FloorChangeMode::DOWN)) {
        move_num = 1;
    } else if (fcms->has(FloorChangeMode::UP)) {
        move_num = -1;
    }

    if (fcms->has(FloorChangeMode::SHAFT)) {
        move_num *= 2;
    }

    const auto &dungeon = floor.get_dungeon_definition();
    if (fcms->has(FloorChangeMode::DOWN)) {
        if (!floor.is_underground()) {
            move_num = dungeon.mindepth;
        }
    } else if (fcms->has(FloorChangeMode::UP)) {
        if (floor.dun_level + move_num < dungeon.mindepth) {
            move_num = -floor.dun_level;
        }
    }

    floor.dun_level += move_num;
}

//!< @details SAVE_FLOORS フラグを抜く箇所に「TODO」のコメントがあった、何をするかは書いておらず詳細不明
static void exit_to_wilderness(PlayerType *player_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (floor.is_underground() || (floor.dungeon_id == DungeonId::WILDERNESS)) {
        return;
    }

    floor.leave_dungeon(true);
    if (!vanilla_town && !lite_town) {
        const auto &dungeon = floor.get_dungeon_definition();
        WildernessGrids::get_instance().set_player_position(dungeon.get_position());
    }

    player_ptr->recall_dungeon = floor.dungeon_id;
    player_ptr->word_recall = 0;
    floor.reset_dungeon_index();
    FloorChangeModesStore::get_instace()->reset(FloorChangeMode::SAVE_FLOORS);
}

static void kill_saved_floors(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    const auto &fcms = FloorChangeModesStore::get_instace();
    if (fcms->has_not(FloorChangeMode::SAVE_FLOORS)) {
        for (auto i = 0; i < MAX_SAVED_FLOORS; i++) {
            kill_saved_floor(player_ptr, &saved_floors[i]);
        }

        latest_visit_mark = 1;
        return;
    }
    if (fcms->has(FloorChangeMode::NO_RETURN)) {
        kill_saved_floor(player_ptr, sf_ptr);
    }
}

static void refresh_new_floor_id(PlayerType *player_ptr, Grid *grid_ptr)
{
    if (new_floor_id != 0) {
        return;
    }

    new_floor_id = get_unused_floor_id(player_ptr);
    if ((grid_ptr != nullptr) && !grid_ptr->has_special_terrain()) {
        grid_ptr->special = new_floor_id;
    }
}

static void update_upper_lower_or_floor_id(saved_floor_type *sf_ptr)
{
    const auto &fcms = FloorChangeModesStore::get_instace();
    if (fcms->has_not(FloorChangeMode::RANDOM_CONNECT)) {
        return;
    }

    if (fcms->has(FloorChangeMode::UP)) {
        sf_ptr->upper_floor_id = new_floor_id;
    } else if (fcms->has(FloorChangeMode::DOWN)) {
        sf_ptr->lower_floor_id = new_floor_id;
    }
}

static void exe_leave_floor(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    auto *grid_ptr = set_grid_by_leaving_floor(*player_ptr->current_floor_ptr, player_ptr->get_position());
    jump_floors(*player_ptr->current_floor_ptr);
    exit_to_wilderness(player_ptr);
    kill_saved_floors(player_ptr, sf_ptr);
    if (!player_ptr->in_saved_floor()) {
        return;
    }

    refresh_new_floor_id(player_ptr, grid_ptr);
    update_upper_lower_or_floor_id(sf_ptr);
    auto &fcms = FloorChangeModesStore::get_instace();
    if (fcms->has_not(FloorChangeMode::SAVE_FLOORS) || fcms->has(FloorChangeMode::NO_RETURN)) {
        return;
    }

    get_out_monster(player_ptr);
    sf_ptr->last_visit = AngbandWorld::get_instance().game_turn;
    forget_lite(*player_ptr->current_floor_ptr);
    forget_view(*player_ptr->current_floor_ptr);
    clear_mon_lite(*player_ptr->current_floor_ptr);
    if (save_floor(player_ptr, sf_ptr, 0)) {
        return;
    }

    fcms->set(FloorChangeMode::NO_RETURN);
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
    if (FloorChangeModesStore::get_instace()->has(FloorChangeMode::RANDOM_CONNECT)) {
        locate_connected_stairs(player_ptr, *player_ptr->current_floor_ptr, sf_ptr);
    }

    exe_leave_floor(player_ptr, sf_ptr);
}
