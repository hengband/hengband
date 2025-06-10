#include "spell-kind/earthquake.h"
#include "dungeon/quest.h"
#include "floor/floor-object.h"
#include "game-option/play-record-options.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "io/write-diary.h"
#include "mind/mind-ninja.h"
#include "monster-floor/monster-lite.h"
#include "monster-floor/monster-remover.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-update.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "player/player-status-flags.h"
#include "status/bad-status-setter.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "util/probability-table.h"
#include "view/display-messages.h"
#include <functional>
#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>
#include <span>

namespace {
std::vector<Pos2D> get_earthquake_area(FloorType &floor, const Pos2D &center, int radius)
{
    const auto is_eathquake_area = [&](const Pos2D &pos) {
        return floor.contains(pos) && Grid::calc_distance(center, pos) <= radius;
    };

    return Rect2D(center, Pos2DVec(radius, radius)) |
           ranges::views::filter(is_eathquake_area) |
           ranges::to<std::vector<Pos2D>>();
}

void reset_grid_info(FloorType &floor, std::span<const Pos2D> area)
{
    for (const auto &pos : area) {
        auto &grid = floor.get_grid(pos);
        grid.info &= ~(CAVE_ROOM | CAVE_ICKY | CAVE_UNSAFE);
        grid.info &= ~(CAVE_GLOW | CAVE_MARK | CAVE_KNOWN);
    }
}

std::vector<Pos2D> decide_collapse_positions(const FloorType &floor, std::span<const Pos2D> area, const Pos2D &center)
{
    const auto is_questor_position = [&](const auto &pos) {
        const auto &grid = floor.get_grid(pos);
        if (!grid.has_monster()) {
            return false;
        }

        const auto &monster = floor.m_list[grid.m_idx];
        return monster.get_monrace().misc_flags.has(MonsterMiscType::QUESTOR);
    };

    /// @note ranges::views::filterにevaluate_percentを渡しranges::toと組み合わせると
    /// 未定義動作となるのでevaluate_percentだけ単独で判定しpush_backで作成する
    /// (おそらくevalute_percentはposに対し結果が確定的でないのが原因)
    std::vector<Pos2D> pos_collapses;
    for (const auto &pos : area | ranges::views::filter([&](const auto &pos) { return pos != center; }) |
                               ranges::views::filter(std::not_fn(is_questor_position))) {
        if (evaluate_percent(15)) {
            pos_collapses.push_back(pos);
        }
    }
    return pos_collapses;
}

tl::optional<Pos2D> decide_player_dodge_posistion(PlayerType *player_ptr, std::span<const Pos2D> pos_collapses)
{
    const auto is_collapsed_pos = [&](const auto &pos) { return ranges::contains(pos_collapses, pos); };
    const auto pos_candidates =
        Direction::directions_8() |
        ranges::views::transform([&](const auto &d) { return player_ptr->get_neighbor(d); }) |
        ranges::views::filter([&](const auto &pos) { return player_ptr->current_floor_ptr->is_empty_at(pos) && (pos != player_ptr->get_position()); }) |
        ranges::views::filter(std::not_fn(is_collapsed_pos)) |
        ranges::to<std::vector<Pos2D>>();

    return pos_candidates.empty() ? tl::nullopt : tl::make_optional(rand_choice(pos_candidates));
}

std::string build_killer_on_earthquake(PlayerType *player_ptr, int m_idx)
{
    if (m_idx <= 0) {
        return _("地震", "an earthquake");
    }

    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    const auto m_name = monster_desc(player_ptr, monster, MD_WRONGDOER_NAME);
    return format(_("%sの起こした地震", "an earthquake caused by %s"), m_name.data());
}

void process_player_damage_undodged(PlayerType *player_ptr, int m_idx)
{
    const auto killer = build_killer_on_earthquake(player_ptr, m_idx);
    constexpr auto direct_hit_damage = 200;
    msg_print(_("あなたはひどい怪我を負った！", "You are severely crushed!"));
    /// FIXME: 避けた時はスタン値が増加するのに直撃時は増加していない。バグ？
    take_hit(player_ptr, DAMAGE_ATTACK, direct_hit_damage, killer);
}

void process_player_damage_dodged(PlayerType *player_ptr, int m_idx)
{
    constexpr std::array<std::pair<bool, std::string_view>, 3> candidates = { {
        { false, _("降り注ぐ岩をうまく避けた！", "You nimbly dodge the blast!") },
        { true, _("岩石があなたに直撃した!", "You are bashed by rubble!") },
        { true, _("あなたは床と壁との間に挟まれてしまった！", "You are crushed between the floor and ceiling!") },
    } };

    const auto &[is_damaged, msg] = rand_choice(candidates);
    msg_print(msg);
    if (!is_damaged) {
        return;
    }

    const auto killer = build_killer_on_earthquake(player_ptr, m_idx);
    BadStatusSetter(player_ptr).mod_stun(randnum1<short>(50));
    take_hit(player_ptr, DAMAGE_ATTACK, Dice::roll(10, 4), killer);
}

void process_hit_to_player(PlayerType *player_ptr, std::span<const Pos2D> pos_collapses, int m_idx)
{
    const auto has_hit = ranges::contains(pos_collapses, player_ptr->get_position());
    if (!has_hit || has_pass_wall(player_ptr) || has_kill_wall(player_ptr)) {
        return;
    }

    constexpr static auto msgs = {
        _("ダンジョンの壁が崩れた！", "The dungeon's ceiling collapses!"),
        _("ダンジョンの床が不自然にねじ曲がった！", "The dungeon's floor twists in an unnatural way!"),
        _("ダンジョンが揺れた！崩れた岩が頭に降ってきた！", "The dungeon quakes!  You are pummeled with debris!"),
    };
    msg_print(rand_choice(msgs));

    if (const auto pos_dodge = decide_player_dodge_posistion(player_ptr, pos_collapses)) {
        process_player_damage_dodged(player_ptr, m_idx);
        (void)move_player_effect(player_ptr, pos_dodge->y, pos_dodge->x, MPE_DONT_PICKUP);
        return;
    }
    process_player_damage_undodged(player_ptr, m_idx);
}

bool can_monster_dodge_to(const FloorType &floor, const Pos2D &p_pos, const Pos2D &pos, std::span<const Pos2D> collapsing_positions)
{
    const auto &grid = floor.get_grid(pos);
    auto can_dodge = floor.is_empty_at(pos) && (pos != p_pos);
    can_dodge &= !grid.is_rune_protection() && !grid.is_rune_explosion();
    can_dodge &= !grid.has(TerrainCharacteristics::PATTERN);
    can_dodge &= !ranges::contains(collapsing_positions, pos);
    return can_dodge;
}

tl::optional<Pos2D> decide_monster_dodge_position(const FloorType &floor, const Pos2D &p_pos, const MonsterEntity &monster, std::span<const Pos2D> pos_collapses)
{
    if (monster.get_monrace().behavior_flags.has(MonsterBehaviorType::NEVER_MOVE)) {
        return tl::nullopt;
    }

    const auto pos_candidates =
        Direction::directions_8() |
        ranges::views::transform([&](const auto &d) { return monster.get_position() + d.vec(); }) |
        ranges::views::filter([&](const auto &pos) { return can_monster_dodge_to(floor, p_pos, pos, pos_collapses); }) |
        ranges::to<std::vector<Pos2D>>();

    return pos_candidates.empty() ? tl::nullopt : tl::make_optional(rand_choice(pos_candidates));
}

void move_monster_to(PlayerType *player_ptr, MonsterEntity &monster, const Pos2D &pos_to)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto pos_from = monster.get_position();
    auto &grid_from = floor.get_grid(pos_from);
    auto &grid_to = floor.get_grid(pos_to);
    grid_to.m_idx = std::exchange(grid_from.m_idx, {});
    monster.set_position(pos_to);
    update_monster(player_ptr, grid_to.m_idx, true);
    lite_spot(player_ptr, pos_from);
    lite_spot(player_ptr, pos_to);
}

bool process_monster_damage(PlayerType *player_ptr, MonsterEntity &monster, bool has_dodged)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &grid = floor.get_grid(monster.get_position());
    const auto damage = (has_dodged ? Dice::roll(4, 8) : (monster.hp + 1));
    (void)set_monster_csleep(player_ptr, grid.m_idx, 0);
    monster.hp -= damage;
    if (monster.hp >= 0) {
        return false;
    }

    if (!ignore_unview || is_seen(player_ptr, monster)) {
        const auto m_name = monster_desc(player_ptr, monster, 0);
        msg_format(_("%s^は岩石に埋もれてしまった！", "%s^ is embedded in the rock!"), m_name.data());
    }

    if (record_named_pet && monster.is_named_pet()) {
        const auto m2_name = monster_desc(player_ptr, monster, MD_INDEF_VISIBLE);
        exe_write_diary(floor, DiaryKind::NAMED_PET, RECORD_NAMED_PET_EARTHQUAKE, m2_name);
    }

    delete_monster_idx(player_ptr, grid.m_idx);
    return true;
}

void process_hit_to_monster(PlayerType *player_ptr, MonsterEntity &monster, std::span<const Pos2D> pos_collapses)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto pos_dodge = decide_monster_dodge_position(floor, player_ptr->get_position(), monster, pos_collapses);
    const auto m_name = monster_desc(player_ptr, monster, 0);
    if (!ignore_unview || is_seen(player_ptr, monster)) {
        msg_format(_("%s^は苦痛で泣きわめいた！", "%s^ wails out in pain!"), m_name.data());
    }

    if (process_monster_damage(player_ptr, monster, pos_dodge.has_value())) {
        return;
    }

    if (pos_dodge) {
        move_monster_to(player_ptr, monster, *pos_dodge);
    }
}

void process_hit_to_monsters(PlayerType *player_ptr, std::span<const Pos2D> pos_collapses)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto has_monster = [&](const auto &pos) { return floor.get_grid(pos).has_monster(); };
    for (const auto &pos : pos_collapses | ranges::views::filter(has_monster)) {
        auto &grid = floor.get_grid(pos);
        auto &monster = floor.m_list[grid.m_idx];
        if (monster.is_riding()) {
            continue;
        }

        const auto &monrace = monster.get_monrace();
        if (monrace.feature_flags.has_any_of({ MonsterFeatureType::KILL_WALL, MonsterFeatureType::PASS_WALL })) {
            continue;
        }

        process_hit_to_monster(player_ptr, monster, pos_collapses);
    }
}

void destruct_earthquake_area(PlayerType *player_ptr, std::span<const Pos2D> pos_collapses)
{
    auto &floor = *player_ptr->current_floor_ptr;
    clear_mon_lite(floor);
    const auto &dungeon = floor.get_dungeon_definition();
    const auto is_changeable = [&](const auto &pos) { return floor.is_grid_changeable(pos); };

    ProbabilityTable<TerrainTag> pt;
    pt.entry_item(TerrainTag::GRANITE_WALL, 20);
    pt.entry_item(TerrainTag::QUARTZ_VEIN, 50);
    pt.entry_item(TerrainTag::MAGMA_VEIN, 30);

    for (const auto &pos : pos_collapses | ranges::views::filter(is_changeable)) {
        delete_all_items_from_floor(player_ptr, pos);

        if (floor.has_terrain_characteristics(pos, TerrainCharacteristics::PROJECTION)) {
            set_terrain_id_to_grid(player_ptr, pos, pt.pick_one_at_random());
        } else {
            set_terrain_id_to_grid(player_ptr, pos, dungeon.select_floor_terrain_id());
        }
    }
}

void glow_earthquake_area(auto &floor, std::span<const Pos2D> area)
{
    for (const auto &pos : area) {
        auto &grid = floor.get_grid(pos);
        if (grid.is_mirror()) {
            grid.info |= CAVE_GLOW;
        }
    }

    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        return;
    }

    for (const auto &pos : area) {
        auto &grid = floor.get_grid(pos);
        for (const auto &d : Direction::directions()) {
            const auto pos_neighbor = pos + d.vec();
            if (!floor.contains(pos_neighbor, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
                continue;
            }

            const auto &grid_neighbor = floor.get_grid(pos_neighbor);
            if (grid_neighbor.get_terrain(TerrainKind::MIMIC).flags.has(TerrainCharacteristics::GLOW)) {
                grid.info |= CAVE_GLOW;
                break;
            }
        }
    }
}

void set_redrawing_flags()
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::UN_VIEW,
        StatusRecalculatingFlag::UN_LITE,
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::FLOW,
        StatusRecalculatingFlag::MONSTER_LITE,
        StatusRecalculatingFlag::MONSTER_STATUSES,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::HEALTH,
        MainWindowRedrawingFlag::UHEALTH,
        MainWindowRedrawingFlag::MAP,
    };
    rfu.set_flags(flags_mwrf);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    rfu.set_flags(flags_swrf);
}
} // namespace

/*!
 * @brief 地震処理
 * Induce an "earthquake" of the given radius at the given location.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param center 中心座標
 * @param radius 効果半径
 * @param m_idx 地震を起こしたモンスターID(0ならばプレイヤー)
 * @return 効力があった場合TRUEを返す
 * @note 効果半径は15に制限される。
 * 現状効果半径が15より大きく設定されているのは自然の脅威による地震(半径 20+(レベル/2)、でかすぎ)のみ。
 */
bool earthquake(PlayerType *player_ptr, const Pos2D &center, int radius, MONSTER_IDX m_idx)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if ((floor.is_in_quest() && QuestType::is_fixed(floor.quest_number)) || !floor.is_underground()) {
        return false;
    }

    if (radius > 15) {
        radius = 15;
    }

    const auto earthquake_area = get_earthquake_area(floor, center, radius);
    reset_grid_info(floor, earthquake_area);

    auto pos_collapses = decide_collapse_positions(floor, earthquake_area, center);
    process_hit_to_player(player_ptr, pos_collapses, m_idx);
    process_hit_to_monsters(player_ptr, pos_collapses);

    // プレイヤーが避けられなかった場合でも壁と重ならないようにする
    ranges::remove(pos_collapses, player_ptr->get_position());

    destruct_earthquake_area(player_ptr, pos_collapses);
    glow_earthquake_area(floor, earthquake_area);

    set_redrawing_flags();

    if (floor.get_grid(player_ptr->get_position()).info & CAVE_GLOW) {
        set_superstealth(player_ptr, false);
    }

    return true;
}
