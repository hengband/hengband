#include "spell-kind/earthquake.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "game-option/play-record-options.h"
#include "game-option/text-display-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "io/write-diary.h"
#include "mind/mind-ninja.h"
#include "monster-floor/monster-lite.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-update.h"
#include "monster/smart-learn-types.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "status/bad-status-setter.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 地震処理
 * Induce an "earthquake" of the given radius at the given location.
 * @param player_ptrプレイヤーへの参照ポインタ
 * @param cy 中心Y座標
 * @param cx 中心X座標
 * @param r 効果半径
 * @param m_idx 地震を起こしたモンスターID(0ならばプレイヤー)
 * @return 効力があった場合TRUEを返す
 */
bool earthquake(PlayerType *player_ptr, POSITION cy, POSITION cx, POSITION r, MONSTER_IDX m_idx)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if ((floor.is_in_quest() && QuestType::is_fixed(floor.quest_number)) || !floor.is_underground()) {
        return false;
    }

    if (r > 12) {
        r = 12;
    }

    bool map[32][32]{};
    for (auto y = 0; y < 32; y++) {
        for (auto x = 0; x < 32; x++) {
            map[y][x] = false;
        }
    }

    auto damage = 0;
    auto hurt = false;
    for (auto dy = -r; dy <= r; dy++) {
        for (auto dx = -r; dx <= r; dx++) {
            const Pos2D pos(cy + dy, cx + dx);
            if (!in_bounds(&floor, pos.y, pos.x)) {
                continue;
            }

            if (distance(cy, cx, pos.y, pos.x) > r) {
                continue;
            }

            auto &grid = floor.get_grid(pos);
            grid.info &= ~(CAVE_ROOM | CAVE_ICKY | CAVE_UNSAFE);
            grid.info &= ~(CAVE_GLOW | CAVE_MARK | CAVE_KNOWN);
            if (!dx && !dy) {
                continue;
            }

            if (evaluate_percent(85)) {
                continue;
            }

            map[16 + pos.y - cy][16 + pos.x - cx] = true;
            if (player_ptr->is_located_at(pos)) {
                hurt = true;
            }
        }
    }

    auto sn = 0;
    Pos2D p_pos_new(0, 0); // 落石を避けた後のプレイヤー座標
    if (hurt && !has_pass_wall(player_ptr) && !has_kill_wall(player_ptr)) {
        for (auto i = 0; i < 8; i++) {
            auto y = player_ptr->y + ddy_ddd[i];
            auto x = player_ptr->x + ddx_ddd[i];
            if (!is_cave_empty_bold(player_ptr, y, x)) {
                continue;
            }

            if (map[16 + y - cy][16 + x - cx]) {
                continue;
            }

            if (floor.grid_array[y][x].has_monster()) {
                continue;
            }

            sn++;
            if (randint0(sn) > 0) {
                continue;
            }

            p_pos_new = { y, x };
        }

        constexpr static auto msgs = {
            _("ダンジョンの壁が崩れた！", "The dungeon's ceiling collapses!"),
            _("ダンジョンの床が不自然にねじ曲がった！", "The dungeon's floor twists in an unnatural way!"),
            _("ダンジョンが揺れた！崩れた岩が頭に降ってきた！", "The dungeon quakes!  You are pummeled with debris!"),
        };
        msg_print(rand_choice(msgs));

        if (!sn) {
            msg_print(_("あなたはひどい怪我を負った！", "You are severely crushed!"));
            damage = 200;
        } else {
            constexpr std::array<std::pair<bool, std::string_view>, 3> candidates = { {
                { false, _("降り注ぐ岩をうまく避けた！", "You nimbly dodge the blast!") },
                { true, _("岩石があなたに直撃した!", "You are bashed by rubble!") },
                { true, _("あなたは床と壁との間に挟まれてしまった！", "You are crushed between the floor and ceiling!") },
            } };

            const auto &[is_damaged, msg] = rand_choice(candidates);

            msg_print(msg);
            if (is_damaged) {
                damage = Dice::roll(10, 4);
                BadStatusSetter(player_ptr).mod_stun(randnum1<short>(50));
            }

            (void)move_player_effect(player_ptr, p_pos_new.y, p_pos_new.x, MPE_DONT_PICKUP);
        }

        map[16 + player_ptr->y - cy][16 + player_ptr->x - cx] = false;
        if (damage) {
            std::string killer;

            if (m_idx) {
                auto *m_ptr = &floor.m_list[m_idx];
                const auto m_name = monster_desc(player_ptr, m_ptr, MD_WRONGDOER_NAME);
                killer = format(_("%sの起こした地震", "an earthquake caused by %s"), m_name.data());
            } else {
                killer = _("地震", "an earthquake");
            }

            take_hit(player_ptr, DAMAGE_ATTACK, damage, killer);
        }
    }

    for (auto dy = -r; dy <= r; dy++) {
        for (auto dx = -r; dx <= r; dx++) {
            const Pos2D pos(cy + dy, cx + dx);
            if (!map[16 + pos.y - cy][16 + pos.x - cx]) {
                continue;
            }

            auto &grid = floor.get_grid(pos);

            if (!grid.has_monster()) {
                continue;
            }

            auto *m_ptr = &floor.m_list[grid.m_idx];

            if (m_ptr->is_riding()) {
                continue;
            }

            auto *r_ptr = &m_ptr->get_monrace();
            if (r_ptr->misc_flags.has(MonsterMiscType::QUESTOR)) {
                map[16 + pos.y - cy][16 + pos.x - cx] = false;
                continue;
            }

            if (r_ptr->feature_flags.has(MonsterFeatureType::KILL_WALL) || r_ptr->feature_flags.has(MonsterFeatureType::PASS_WALL)) {
                continue;
            }

            sn = 0;
            if (r_ptr->behavior_flags.has_not(MonsterBehaviorType::NEVER_MOVE)) {
                for (DIRECTION i = 0; i < 8; i++) {
                    const Pos2D pos_neighbor(pos.y + ddy_ddd[i], pos.x + ddx_ddd[i]);
                    if (!is_cave_empty_bold(player_ptr, pos_neighbor.y, pos_neighbor.x)) {
                        continue;
                    }

                    const auto &grid_neighbor = floor.get_grid(pos_neighbor);
                    if (grid_neighbor.is_rune_protection()) {
                        continue;
                    }

                    if (grid_neighbor.is_rune_explosion()) {
                        continue;
                    }

                    if (pattern_tile(&floor, pos_neighbor.y, pos_neighbor.x)) {
                        continue;
                    }

                    if (map[16 + pos_neighbor.y - cy][16 + pos_neighbor.x - cx]) {
                        continue;
                    }

                    if (grid_neighbor.has_monster()) {
                        continue;
                    }

                    if (player_ptr->is_located_at(pos)) {
                        continue;
                    }

                    sn++;

                    if (randint0(sn) > 0) {
                        continue;
                    }

                    p_pos_new = pos_neighbor;
                }
            }

            const auto m_name = monster_desc(player_ptr, m_ptr, 0);
            if (!ignore_unview || is_seen(player_ptr, m_ptr)) {
                msg_format(_("%s^は苦痛で泣きわめいた！", "%s^ wails out in pain!"), m_name.data());
            }

            damage = (sn ? Dice::roll(4, 8) : (m_ptr->hp + 1));
            (void)set_monster_csleep(player_ptr, grid.m_idx, 0);
            m_ptr->hp -= damage;
            if (m_ptr->hp < 0) {
                if (!ignore_unview || is_seen(player_ptr, m_ptr)) {
                    msg_format(_("%s^は岩石に埋もれてしまった！", "%s^ is embedded in the rock!"), m_name.data());
                }

                if (grid.has_monster()) {
                    const auto &monster = floor.m_list[grid.m_idx];
                    if (record_named_pet && monster.is_named_pet()) {
                        const auto m2_name = monster_desc(player_ptr, m_ptr, MD_INDEF_VISIBLE);
                        exe_write_diary(floor, DiaryKind::NAMED_PET, RECORD_NAMED_PET_EARTHQUAKE, m2_name);
                    }
                }

                delete_monster(player_ptr, pos.y, pos.x);
                sn = 0;
            }

            if (sn == 0) {
                continue;
            }

            const auto m_idx_aux = grid.m_idx;
            grid.m_idx = 0;
            floor.get_grid(p_pos_new).m_idx = m_idx_aux;
            m_ptr->fy = p_pos_new.y;
            m_ptr->fx = p_pos_new.x;
            update_monster(player_ptr, m_idx_aux, true);
            lite_spot(player_ptr, pos.y, pos.x);
            lite_spot(player_ptr, p_pos_new.y, p_pos_new.x);
        }
    }

    clear_mon_lite(&floor);
    for (auto dy = -r; dy <= r; dy++) {
        for (auto dx = -r; dx <= r; dx++) {
            auto yy = cy + dy;
            auto xx = cx + dx;
            if (!map[16 + yy - cy][16 + xx - cx]) {
                continue;
            }

            const Pos2D pos(yy, xx);
            if (!floor.is_grid_changeable(pos)) {
                continue;
            }

            delete_all_items_from_floor(player_ptr, pos.y, pos.x);
            const auto t = floor.has_terrain_characteristics(pos, TerrainCharacteristics::PROJECT) ? randint0(100) : 200;
            if (t < 20) {
                cave_set_feat(player_ptr, pos.y, pos.x, feat_granite);
                continue;
            }

            if (t < 70) {
                cave_set_feat(player_ptr, pos.y, pos.x, feat_quartz_vein);
                continue;
            }

            if (t < 100) {
                cave_set_feat(player_ptr, pos.y, pos.x, feat_magma_vein);
                continue;
            }

            cave_set_feat(player_ptr, pos.y, pos.x, rand_choice(feat_ground_type));
        }
    }

    for (auto dy = -r; dy <= r; dy++) {
        for (auto dx = -r; dx <= r; dx++) {
            const Pos2D pos(cy + dy, cx + dx);
            if (!in_bounds(&floor, pos.y, pos.x)) {
                continue;
            }

            if (distance(cy, cx, pos.y, pos.x) > r) {
                continue;
            }

            auto &grid = floor.get_grid(pos);
            if (grid.is_mirror()) {
                grid.info |= CAVE_GLOW;
                continue;
            }

            if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
                continue;
            }

            for (auto j = 0; j < 9; j++) {
                const Pos2D pos_neighbor(pos.y + ddy_ddd[j], pos.x + ddx_ddd[j]);
                if (!in_bounds2(&floor, pos_neighbor.y, pos_neighbor.x)) {
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
    if (floor.grid_array[player_ptr->y][player_ptr->x].info & CAVE_GLOW) {
        set_superstealth(player_ptr, false);
    }

    return true;
}
