#include "floor/floor-changer.h"
#include "action/travel-execution.h"
#include "dungeon/quest-monster-placer.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-generator.h"
#include "floor/floor-mode-changer.h"
#include "floor/floor-object.h"
#include "floor/floor-save-util.h"
#include "floor/floor-save.h"
#include "floor/floor-util.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "io/write-diary.h"
#include "load/floor-loader.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-summon.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "player-base/player-class.h"
#include "spell-kind/spells-floor.h"
#include "system/artifact-type-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"
#include "world/world.h"
#include <algorithm>

/*!
 * @brief 階段移動先のフロアが生成できない時に簡単な行き止まりマップを作成する / Builds the dead end
 */
static void build_dead_end(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    msg_print(_("階段は行き止まりだった。", "The staircases come to a dead end..."));
    clear_cave(player_ptr);
    player_ptr->x = player_ptr->y = 0;
    set_floor_and_wall(0);
    player_ptr->current_floor_ptr->height = SCREEN_HGT;
    player_ptr->current_floor_ptr->width = SCREEN_WID;
    for (POSITION y = 0; y < MAX_HGT; y++) {
        for (POSITION x = 0; x < MAX_WID; x++) {
            place_bold(player_ptr, y, x, GB_SOLID_PERM);
        }
    }

    player_ptr->y = player_ptr->current_floor_ptr->height / 2;
    player_ptr->x = player_ptr->current_floor_ptr->width / 2;
    place_bold(player_ptr, player_ptr->y, player_ptr->x, GB_FLOOR);
    wipe_generate_random_floor_flags(player_ptr->current_floor_ptr);
    const auto &fcms = FloorChangeModesStore::get_instace();
    if (fcms->has(FloorChangeMode::UP)) {
        sf_ptr->upper_floor_id = 0;
    } else if (fcms->has(FloorChangeMode::DOWN)) {
        sf_ptr->lower_floor_id = 0;
    }
}

static MONSTER_IDX decide_pet_index(PlayerType *player_ptr, const int current_monster, POSITION *cy, POSITION *cx)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (current_monster == 0) {
        const auto m_idx = floor.pop_empty_index_monster();
        player_ptr->riding = m_idx;
        if (m_idx) {
            *cy = player_ptr->y;
            *cx = player_ptr->x;
        }

        return m_idx;
    }

    POSITION d;
    for (d = 1; d < A_MAX; d++) {
        int j;
        for (j = 1000; j > 0; j--) {
            scatter(player_ptr, cy, cx, player_ptr->y, player_ptr->x, d, PROJECT_NONE);
            if (monster_can_enter(player_ptr, *cy, *cx, &party_mon[current_monster].get_monrace(), 0)) {
                break;
            }
        }

        if (j != 0) {
            break;
        }
    }

    return (d == 6) ? 0 : floor.pop_empty_index_monster();
}

static MonraceDefinition &set_pet_params(PlayerType *player_ptr, const int current_monster, MONSTER_IDX m_idx, const POSITION cy, const POSITION cx)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    player_ptr->current_floor_ptr->grid_array[cy][cx].m_idx = m_idx;
    m_ptr->r_idx = party_mon[current_monster].r_idx;
    *m_ptr = party_mon[current_monster].clone();
    m_ptr->fy = cy;
    m_ptr->fx = cx;
    m_ptr->current_floor_ptr = player_ptr->current_floor_ptr;
    m_ptr->ml = true;
    m_ptr->mtimed[MonsterTimedEffect::SLEEP] = 0;
    m_ptr->hold_o_idx_list.clear();
    m_ptr->target_y = 0;
    auto &r_ref = m_ptr->get_real_monrace();
    if (!ironman_nightmare) {
        m_ptr->mflag.set(MonsterTemporaryFlagType::PREVENT_MAGIC);
    }

    return r_ref;
}

/*!
 * @brief 移動先のフロアに伴ったペットを配置する / Place preserved pet monsters on new floor
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void place_pet(PlayerType *player_ptr)
{
    const auto max_num = AngbandWorld::get_instance().is_wild_mode() ? 1 : MAX_PARTY_MON;
    auto &floor = *player_ptr->current_floor_ptr;
    for (int current_monster = 0; current_monster < max_num; current_monster++) {
        POSITION cy = 0;
        POSITION cx = 0;
        if (!MonraceList::is_valid(party_mon[current_monster].r_idx)) {
            continue;
        }

        MONSTER_IDX m_idx = decide_pet_index(player_ptr, current_monster, &cy, &cx);
        if (m_idx != 0) {
            auto &r_ref = set_pet_params(player_ptr, current_monster, m_idx, cy, cx);
            update_monster(player_ptr, m_idx, true);
            lite_spot(player_ptr, cy, cx);
            if (r_ref.misc_flags.has(MonsterMiscType::MULTIPLY)) {
                floor.num_repro++;
            }
        } else {
            const auto &monster = party_mon[current_monster];
            auto &monrace = monster.get_real_monrace();
            msg_format(_("%sとはぐれてしまった。", "You have lost sight of %s."), monster_desc(player_ptr, &monster, 0).data());
            if (record_named_pet && monster.is_named()) {
                exe_write_diary(floor, DiaryKind::NAMED_PET, RECORD_NAMED_PET_LOST_SIGHT, monster_desc(player_ptr, &monster, MD_INDEF_VISIBLE));
            }

            if (monrace.has_entity()) {
                monrace.decrement_current_numbers();
            }
        }
    }

    for (auto &monster : party_mon) {
        monster.wipe();
    }
}

/*!
 * @brief ユニークモンスターやアーティファクトの所在フロアを更新する
 * @param floor_ptr 現在フロアへのポインタ
 * @param cur_floor_id 現在のフロアID
 * @details
 * The floor_id and floor_id are not updated correctly
 * while new floor creation since dungeons may be re-created by
 * auto-scum option.
 */
static void update_unique_artifact(FloorType *floor_ptr, int16_t cur_floor_id)
{
    for (int i = 1; i < floor_ptr->m_max; i++) {
        const auto &m_ref = floor_ptr->m_list[i];
        if (!m_ref.is_valid()) {
            continue;
        }

        auto &r_ref = m_ref.get_real_monrace();
        if (r_ref.kind_flags.has(MonsterKindType::UNIQUE) || (r_ref.population_flags.has(MonsterPopulationType::NAZGUL))) {
            r_ref.floor_id = cur_floor_id;
        }
    }

    for (int i = 1; i < floor_ptr->o_max; i++) {
        const auto &o_ref = floor_ptr->o_list[i];
        if (!o_ref.is_valid()) {
            continue;
        }

        if (o_ref.is_fixed_artifact()) {
            o_ref.get_fixed_artifact().floor_id = cur_floor_id;
        }
    }
}

static bool is_visited_floor(saved_floor_type *sf_ptr)
{
    return sf_ptr->last_visit != 0;
}

/*!
 * @brief フロア読込時にプレイヤー足元の地形に必要な情報を設定する
 * @param floor フロアへの参照
 * @param p_pos プレイヤーの現在位置
 */
static void set_player_grid(FloorType &floor, const Pos2D &p_pos)
{
    const auto &fcms = FloorChangeModesStore::get_instace();
    if (fcms->has_not(FloorChangeMode::NO_RETURN)) {
        return;
    }

    auto &grid = floor.get_grid(p_pos);
    if (feat_uses_special(grid.feat)) {
        return;
    }

    if (fcms->has_any_of({ FloorChangeMode::DOWN, FloorChangeMode::UP })) {
        grid.feat = rand_choice(feat_ground_type);
    }

    grid.special = 0;
}

static void update_floor_id(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    const auto &fcms = FloorChangeModesStore::get_instace();
    const auto is_up = fcms->has(FloorChangeMode::UP);
    const auto is_down = fcms->has(FloorChangeMode::DOWN);
    if (!player_ptr->in_saved_floor()) {
        if (is_up) {
            sf_ptr->lower_floor_id = 0;
        } else if (is_down) {
            sf_ptr->upper_floor_id = 0;
        }

        return;
    }

    saved_floor_type *cur_sf_ptr = get_sf_ptr(player_ptr->floor_id);
    if (is_up) {
        if (cur_sf_ptr->upper_floor_id == new_floor_id) {
            sf_ptr->lower_floor_id = player_ptr->floor_id;
        }

        return;
    }

    if (is_down && (cur_sf_ptr->lower_floor_id == new_floor_id)) {
        sf_ptr->upper_floor_id = player_ptr->floor_id;
    }
}

static void reset_unique_by_floor_change(PlayerType *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (MONSTER_IDX i = 1; i < floor_ptr->m_max; i++) {
        auto *m_ptr = &floor_ptr->m_list[i];
        if (!m_ptr->is_valid()) {
            continue;
        }

        if (!m_ptr->is_pet()) {
            m_ptr->hp = m_ptr->maxhp = m_ptr->max_maxhp;
            (void)set_monster_fast(player_ptr, i, 0);
            (void)set_monster_slow(player_ptr, i, 0);
            (void)set_monster_stunned(player_ptr, i, 0);
            (void)set_monster_confused(player_ptr, i, 0);
            (void)set_monster_monfear(player_ptr, i, 0);
            (void)set_monster_invulner(player_ptr, i, 0, false);
        }

        const auto &r_ref = m_ptr->get_real_monrace();
        if (r_ref.kind_flags.has_not(MonsterKindType::UNIQUE) && r_ref.population_flags.has_not(MonsterPopulationType::NAZGUL)) {
            continue;
        }

        if (r_ref.floor_id != new_floor_id) {
            delete_monster_idx(player_ptr, i);
        }
    }
}

static void allocate_loaded_floor(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    GAME_TURN tmp_last_visit = sf_ptr->last_visit;
    const auto &floor = *player_ptr->current_floor_ptr;
    auto alloc_chance = floor.get_dungeon_definition().max_m_alloc_chance;
    const auto &world = AngbandWorld::get_instance();
    while (tmp_last_visit > world.game_turn) {
        tmp_last_visit -= TURNS_PER_TICK * TOWN_DAWN;
    }

    GAME_TURN absence_ticks = (world.game_turn - tmp_last_visit) / TURNS_PER_TICK;
    reset_unique_by_floor_change(player_ptr);
    for (MONSTER_IDX i = 1; i < floor.o_max; i++) {
        const auto *o_ptr = &floor.o_list[i];
        if (!o_ptr->is_valid() || !o_ptr->is_fixed_artifact()) {
            continue;
        }

        auto &artifact = o_ptr->get_fixed_artifact();
        if (artifact.floor_id == new_floor_id) {
            artifact.is_generated = true;
        } else {
            delete_object_idx(player_ptr, i);
        }
    }

    (void)place_quest_monsters(player_ptr);
    GAME_TURN alloc_times = absence_ticks / alloc_chance;
    if (randint0(alloc_chance) < (absence_ticks % alloc_chance)) {
        alloc_times++;
    }

    for (MONSTER_IDX i = 0; i < alloc_times; i++) {
        (void)alloc_monster(player_ptr, 0, 0, summon_specific);
    }
}

/*!
 * @brief プレイヤー足元に階段を設置する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void set_stairs(PlayerType *player_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &grid = floor.grid_array[player_ptr->y][player_ptr->x];
    const auto &fcms = FloorChangeModesStore::get_instace();
    const auto &dungeon = floor.get_dungeon_definition();
    const auto &terrains = TerrainList::get_instance();
    if (fcms->has(FloorChangeMode::UP) && !inside_quest(floor.get_quest_id())) {
        grid.feat = fcms->has(FloorChangeMode::SHAFT) ? dungeon.convert_terrain_id(feat_down_stair, TerrainCharacteristics::SHAFT) : feat_down_stair;
    } else if (fcms->has(FloorChangeMode::DOWN) && !ironman_downward) {
        const auto terrain_up_stair = terrains.get_terrain_id(TerrainTag::UP_STAIR);
        const auto terrain_id = fcms->has(FloorChangeMode::SHAFT) ? dungeon.convert_terrain_id(terrain_up_stair, TerrainCharacteristics::SHAFT) : terrain_up_stair;
        grid.set_terrain_id(terrain_id);
    }

    grid.mimic = 0;
    grid.special = player_ptr->floor_id;
}

/*!
 * @brief 保存済フロア読込不可時の新規フロア生成を行う。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @params sf_ptr 保存済フロアへの参照ポインタ
 */
static void generate_new_floor(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    if (!is_visited_floor(sf_ptr)) {
        generate_floor(player_ptr);
    } else {
        build_dead_end(player_ptr, sf_ptr);
    }

    sf_ptr->last_visit = AngbandWorld::get_instance().game_turn;
    auto &floor = *player_ptr->current_floor_ptr;
    sf_ptr->dun_level = floor.dun_level;
    if (FloorChangeModesStore::get_instace()->has(FloorChangeMode::NO_RETURN)) {
        return;
    }

    set_stairs(player_ptr);
}

static void cut_off_the_upstair(PlayerType *player_ptr)
{
    const auto &fcms = FloorChangeModesStore::get_instace();
    if (fcms->has(FloorChangeMode::RANDOM_PLACE)) {
        (void)new_player_spot(player_ptr);
        return;
    }

    if (fcms->has_not(FloorChangeMode::NO_RETURN) || fcms->has_none_of({ FloorChangeMode::DOWN, FloorChangeMode::UP })) {
        return;
    }

    if (!player_ptr->effects()->blindness().is_blind()) {
        msg_print(_("突然階段が塞がれてしまった。", "Suddenly the stairs is blocked!"));
    } else {
        msg_print(_("ゴトゴトと何か音がした。", "You hear some noises."));
    }
}

static void update_floor(PlayerType *player_ptr)
{
    const auto &fcms = FloorChangeModesStore::get_instace();
    if (fcms->has_none_of({ FloorChangeMode::SAVE_FLOORS, FloorChangeMode::FIRST_FLOOR })) {
        generate_floor(player_ptr);
        new_floor_id = 0;
        return;
    }

    if (new_floor_id == 0) {
        new_floor_id = get_unused_floor_id(player_ptr);
    }

    saved_floor_type *sf_ptr;
    sf_ptr = get_sf_ptr(new_floor_id);
    const bool loaded = is_visited_floor(sf_ptr) && load_floor(player_ptr, sf_ptr, 0);
    set_player_grid(*player_ptr->current_floor_ptr, player_ptr->get_position());
    update_floor_id(player_ptr, sf_ptr);

    if (loaded) {
        allocate_loaded_floor(player_ptr, sf_ptr);
    } else {
        generate_new_floor(player_ptr, sf_ptr);
    }

    cut_off_the_upstair(player_ptr);
    sf_ptr->visit_mark = latest_visit_mark++;
}

/*!
 * @brief フロアの切り替え処理 / Enter new floor.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * If the floor is an old saved floor, it will be\n
 * restored from the temporary file.  If the floor is new one, new floor\n
 * will be generated.\n
 */
void change_floor(PlayerType *player_ptr)
{
    auto &world = AngbandWorld::get_instance();
    world.character_dungeon = false;
    player_ptr->dtrap = false;
    panel_row_min = 0;
    panel_row_max = 0;
    panel_col_min = 0;
    panel_col_max = 0;
    player_ptr->ambush_flag = false;
    update_floor(player_ptr);
    place_pet(player_ptr);
    forget_travel_flow(player_ptr->current_floor_ptr);
    update_unique_artifact(player_ptr->current_floor_ptr, new_floor_id);
    player_ptr->floor_id = new_floor_id;
    world.character_dungeon = true;
    if (player_ptr->ppersonality == PERSONALITY_MUNCHKIN) {
        wiz_lite(player_ptr, PlayerClass(player_ptr).equals(PlayerClassType::NINJA));
    }

    player_ptr->current_floor_ptr->generated_turn = world.game_turn;
    player_ptr->feeling_turn = player_ptr->current_floor_ptr->generated_turn;
    player_ptr->feeling = 0;
    auto &fcms = FloorChangeModesStore::get_instace();
    fcms->clear();
    select_floor_music(player_ptr);
    fcms->clear();
}
