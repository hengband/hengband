#include "floor/floor-changer.h"
#include "action/travel-execution.h"
#include "dungeon/dungeon.h"
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
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags7.h"
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
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"
#include "world/world.h"

#include <algorithm>

/*!
 * @brief 階段移動先のフロアが生成できない時に簡単な行き止まりマップを作成する / Builds the dead end
 */
static void build_dead_end(PlayerType *player_ptr)
{
    clear_cave(player_ptr);
    player_ptr->x = player_ptr->y = 0;
    set_floor_and_wall(0);
    player_ptr->current_floor_ptr->height = SCREEN_HGT;
    player_ptr->current_floor_ptr->width = SCREEN_WID;
    for (POSITION y = 0; y < MAX_HGT; y++)
        for (POSITION x = 0; x < MAX_WID; x++)
            place_bold(player_ptr, y, x, GB_SOLID_PERM);

    player_ptr->y = player_ptr->current_floor_ptr->height / 2;
    player_ptr->x = player_ptr->current_floor_ptr->width / 2;
    place_bold(player_ptr, player_ptr->y, player_ptr->x, GB_FLOOR);
    wipe_generate_random_floor_flags(player_ptr->current_floor_ptr);
}

static MONSTER_IDX decide_pet_index(PlayerType *player_ptr, const int current_monster, POSITION *cy, POSITION *cx)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (current_monster == 0) {
        MONSTER_IDX m_idx = m_pop(floor_ptr);
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
            if (monster_can_enter(player_ptr, *cy, *cx, &r_info[party_mon[current_monster].r_idx], 0))
                break;
        }

        if (j != 0)
            break;
    }

    return (d == 6) ? 0 : m_pop(floor_ptr);
}

static void set_pet_params(PlayerType *player_ptr, monster_race **r_ptr, const int current_monster, MONSTER_IDX m_idx, const POSITION cy, const POSITION cx)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    player_ptr->current_floor_ptr->grid_array[cy][cx].m_idx = m_idx;
    m_ptr->r_idx = party_mon[current_monster].r_idx;
    *m_ptr = party_mon[current_monster];
    *r_ptr = real_r_ptr(m_ptr);
    m_ptr->fy = cy;
    m_ptr->fx = cx;
    m_ptr->current_floor_ptr = player_ptr->current_floor_ptr;
    m_ptr->ml = true;
    m_ptr->mtimed[MTIMED_CSLEEP] = 0;
    m_ptr->hold_o_idx_list.clear();
    m_ptr->target_y = 0;
    if ((*r_ptr)->behavior_flags.has(MonsterBehaviorType::PREVENT_SUDDEN_MAGIC) && !ironman_nightmare) {
        m_ptr->mflag.set(MonsterTemporaryFlagType::PREVENT_MAGIC);
    }
}

/*!
 * @brief 移動先のフロアに伴ったペットを配置する / Place preserved pet monsters on new floor
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void place_pet(PlayerType *player_ptr)
{
    int max_num = player_ptr->wild_mode ? 1 : MAX_PARTY_MON;
    for (int current_monster = 0; current_monster < max_num; current_monster++) {
        POSITION cy = 0;
        POSITION cx = 0;
        if (party_mon[current_monster].r_idx == 0)
            continue;

        MONSTER_IDX m_idx = decide_pet_index(player_ptr, current_monster, &cy, &cx);
        if (m_idx != 0) {
            monster_race *r_ptr;
            set_pet_params(player_ptr, &r_ptr, current_monster, m_idx, cy, cx);
            update_monster(player_ptr, m_idx, true);
            lite_spot(player_ptr, cy, cx);
            if (r_ptr->flags2 & RF2_MULTIPLY)
                player_ptr->current_floor_ptr->num_repro++;
        } else {
            auto *m_ptr = &party_mon[current_monster];
            auto *r_ptr = real_r_ptr(m_ptr);
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(player_ptr, m_name, m_ptr, 0);
            msg_format(_("%sとはぐれてしまった。", "You have lost sight of %s."), m_name);
            if (record_named_pet && m_ptr->nickname) {
                monster_desc(player_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
                exe_write_diary(player_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_LOST_SIGHT, m_name);
            }

            if (r_ptr->cur_num)
                r_ptr->cur_num--;
        }
    }

    std::fill(std::begin(party_mon), std::end(party_mon), monster_type{});
}

/*!
 * @brief ユニークモンスターやアーティファクトの所在フロアを更新する / Hack -- Update location of unique monsters and artifacts
 * @param cur_floor_id 現在のフロアID
 * @details
 * The r_ptr->floor_id and a_ptr->floor_id are not updated correctly\n
 * while new floor creation since dungeons may be re-created by\n
 * auto-scum option.\n
 */
static void update_unique_artifact(floor_type *floor_ptr, int16_t cur_floor_id)
{
    for (int i = 1; i < floor_ptr->m_max; i++) {
        monster_race *r_ptr;
        auto *m_ptr = &floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr))
            continue;

        r_ptr = real_r_ptr(m_ptr);
        if ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL))
            r_ptr->floor_id = cur_floor_id;
    }

    for (int i = 1; i < floor_ptr->o_max; i++) {
        auto *o_ptr = &floor_ptr->o_list[i];
        if (!o_ptr->is_valid())
            continue;

        if (o_ptr->is_fixed_artifact())
            a_info[o_ptr->name1].floor_id = cur_floor_id;
    }
}

static void check_visited_floor(PlayerType *player_ptr, saved_floor_type *sf_ptr, bool *loaded)
{
    if ((sf_ptr->last_visit == 0) || !load_floor(player_ptr, sf_ptr, 0))
        return;

    *loaded = true;
    if ((player_ptr->change_floor_mode & CFM_NO_RETURN) == 0)
        return;

    auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x];
    if (feat_uses_special(g_ptr->feat))
        return;

    if (player_ptr->change_floor_mode & (CFM_DOWN | CFM_UP))
        g_ptr->feat = feat_ground_type[randint0(100)];

    g_ptr->special = 0;
}

static void update_floor_id(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    if (player_ptr->floor_id == 0) {
        if (player_ptr->change_floor_mode & CFM_UP)
            sf_ptr->lower_floor_id = 0;
        else if (player_ptr->change_floor_mode & CFM_DOWN)
            sf_ptr->upper_floor_id = 0;

        return;
    }

    saved_floor_type *cur_sf_ptr = get_sf_ptr(player_ptr->floor_id);
    if (player_ptr->change_floor_mode & CFM_UP) {
        if (cur_sf_ptr->upper_floor_id == new_floor_id)
            sf_ptr->lower_floor_id = player_ptr->floor_id;

        return;
    }

    if (((player_ptr->change_floor_mode & CFM_DOWN) != 0) && (cur_sf_ptr->lower_floor_id == new_floor_id))
        sf_ptr->upper_floor_id = player_ptr->floor_id;
}

static void reset_unique_by_floor_change(PlayerType *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (MONSTER_IDX i = 1; i < floor_ptr->m_max; i++) {
        monster_race *r_ptr;
        auto *m_ptr = &floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr))
            continue;

        if (!is_pet(m_ptr)) {
            m_ptr->hp = m_ptr->maxhp = m_ptr->max_maxhp;
            (void)set_monster_fast(player_ptr, i, 0);
            (void)set_monster_slow(player_ptr, i, 0);
            (void)set_monster_stunned(player_ptr, i, 0);
            (void)set_monster_confused(player_ptr, i, 0);
            (void)set_monster_monfear(player_ptr, i, 0);
            (void)set_monster_invulner(player_ptr, i, 0, false);
        }

        r_ptr = real_r_ptr(m_ptr);
        if (!(r_ptr->flags1 & RF1_UNIQUE) && !(r_ptr->flags7 & RF7_NAZGUL))
            continue;

        if (r_ptr->floor_id != new_floor_id)
            delete_monster_idx(player_ptr, i);
    }
}

static void new_floor_allocation(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    GAME_TURN tmp_last_visit = sf_ptr->last_visit;
    int alloc_chance = d_info[player_ptr->dungeon_idx].max_m_alloc_chance;
    while (tmp_last_visit > w_ptr->game_turn)
        tmp_last_visit -= TURNS_PER_TICK * TOWN_DAWN;

    GAME_TURN absence_ticks = (w_ptr->game_turn - tmp_last_visit) / TURNS_PER_TICK;
    reset_unique_by_floor_change(player_ptr);
    for (MONSTER_IDX i = 1; i < player_ptr->current_floor_ptr->o_max; i++) {
        auto *o_ptr = &player_ptr->current_floor_ptr->o_list[i];
        if (!o_ptr->is_valid() || !o_ptr->is_fixed_artifact())
            continue;

        if (a_info[o_ptr->name1].floor_id == new_floor_id)
            a_info[o_ptr->name1].cur_num = 1;
        else
            delete_object_idx(player_ptr, i);
    }

    (void)place_quest_monsters(player_ptr);
    GAME_TURN alloc_times = absence_ticks / alloc_chance;
    if (randint0(alloc_chance) < (absence_ticks % alloc_chance))
        alloc_times++;

    for (MONSTER_IDX i = 0; i < alloc_times; i++)
        (void)alloc_monster(player_ptr, 0, 0, summon_specific);
}

static void check_dead_end(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    if (sf_ptr->last_visit == 0) {
        generate_floor(player_ptr);
        return;
    }

    msg_print(_("階段は行き止まりだった。", "The staircases come to a dead end..."));
    build_dead_end(player_ptr);
    if (player_ptr->change_floor_mode & CFM_UP)
        sf_ptr->upper_floor_id = 0;
    else if (player_ptr->change_floor_mode & CFM_DOWN)
        sf_ptr->lower_floor_id = 0;
}

static void update_new_floor_feature(PlayerType *player_ptr, saved_floor_type *sf_ptr, const bool loaded)
{
    if (loaded) {
        new_floor_allocation(player_ptr, sf_ptr);
        return;
    }

    check_dead_end(player_ptr, sf_ptr);
    sf_ptr->last_visit = w_ptr->game_turn;
    sf_ptr->dun_level = player_ptr->current_floor_ptr->dun_level;
    if ((player_ptr->change_floor_mode & CFM_NO_RETURN) != 0)
        return;

    auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x];
    if ((player_ptr->change_floor_mode & CFM_UP) && !inside_quest(quest_number(player_ptr, player_ptr->current_floor_ptr->dun_level)))
        g_ptr->feat = (player_ptr->change_floor_mode & CFM_SHAFT) ? feat_state(player_ptr->current_floor_ptr, feat_down_stair, FloorFeatureType::SHAFT) : feat_down_stair;
    else if ((player_ptr->change_floor_mode & CFM_DOWN) && !ironman_downward)
        g_ptr->feat = (player_ptr->change_floor_mode & CFM_SHAFT) ? feat_state(player_ptr->current_floor_ptr, feat_up_stair, FloorFeatureType::SHAFT) : feat_up_stair;

    g_ptr->mimic = 0;
    g_ptr->special = player_ptr->floor_id;
}

static void cut_off_the_upstair(PlayerType *player_ptr)
{
    if (player_ptr->change_floor_mode & CFM_RAND_PLACE) {
        (void)new_player_spot(player_ptr);
        return;
    }

    if (((player_ptr->change_floor_mode & CFM_NO_RETURN) == 0) || ((player_ptr->change_floor_mode & (CFM_DOWN | CFM_UP)) == 0))
        return;

    if (!player_ptr->blind)
        msg_print(_("突然階段が塞がれてしまった。", "Suddenly the stairs is blocked!"));
    else
        msg_print(_("ゴトゴトと何か音がした。", "You hear some noises."));
}

static void update_floor(PlayerType *player_ptr)
{
    if (!(player_ptr->change_floor_mode & CFM_SAVE_FLOORS) && !(player_ptr->change_floor_mode & CFM_FIRST_FLOOR)) {
        generate_floor(player_ptr);
        new_floor_id = 0;
        return;
    }

    if (new_floor_id == 0)
        new_floor_id = get_new_floor_id(player_ptr);

    saved_floor_type *sf_ptr;
    bool loaded = false;
    sf_ptr = get_sf_ptr(new_floor_id);
    check_visited_floor(player_ptr, sf_ptr, &loaded);
    update_floor_id(player_ptr, sf_ptr);
    update_new_floor_feature(player_ptr, sf_ptr, loaded);
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
    w_ptr->character_dungeon = false;
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
    w_ptr->character_dungeon = true;
    if (player_ptr->ppersonality == PERSONALITY_MUNCHKIN)
        wiz_lite(player_ptr, PlayerClass(player_ptr).equals(PlayerClassType::NINJA));

    player_ptr->current_floor_ptr->generated_turn = w_ptr->game_turn;
    player_ptr->feeling_turn = player_ptr->current_floor_ptr->generated_turn;
    player_ptr->feeling = 0;
    player_ptr->change_floor_mode = 0L;
    select_floor_music(player_ptr);
    player_ptr->change_floor_mode = 0;
}
