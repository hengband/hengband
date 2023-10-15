#include "spell-kind/earthquake.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "game-option/play-record-options.h"
#include "game-option/text-display-options.h"
#include "grid/feature-flag-types.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/stair.h"
#include "io/write-diary.h"
#include "mind/mind-ninja.h"
#include "monster-floor/monster-lite.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
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
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain-type-definition.h"
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
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if ((floor_ptr->is_in_quest() && QuestType::is_fixed(floor_ptr->quest_number)) || !floor_ptr->dun_level) {
        return false;
    }

    if (r > 12) {
        r = 12;
    }

    bool map[32][32];
    for (POSITION y = 0; y < 32; y++) {
        for (POSITION x = 0; x < 32; x++) {
            map[y][x] = false;
        }
    }

    int damage = 0;
    bool hurt = false;
    for (POSITION dy = -r; dy <= r; dy++) {
        for (POSITION dx = -r; dx <= r; dx++) {
            POSITION yy = cy + dy;
            POSITION xx = cx + dx;

            if (!in_bounds(floor_ptr, yy, xx)) {
                continue;
            }

            if (distance(cy, cx, yy, xx) > r) {
                continue;
            }

            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[yy][xx];
            g_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY | CAVE_UNSAFE);
            g_ptr->info &= ~(CAVE_GLOW | CAVE_MARK | CAVE_KNOWN);
            if (!dx && !dy) {
                continue;
            }

            if (randint0(100) < 85) {
                continue;
            }

            map[16 + yy - cy][16 + xx - cx] = true;
            if (player_bold(player_ptr, yy, xx)) {
                hurt = true;
            }
        }
    }

    int sn = 0;
    POSITION sy = 0, sx = 0;
    if (hurt && !has_pass_wall(player_ptr) && !has_kill_wall(player_ptr)) {
        for (DIRECTION i = 0; i < 8; i++) {
            POSITION y = player_ptr->y + ddy_ddd[i];
            POSITION x = player_ptr->x + ddx_ddd[i];
            if (!is_cave_empty_bold(player_ptr, y, x)) {
                continue;
            }

            if (map[16 + y - cy][16 + x - cx]) {
                continue;
            }

            if (floor_ptr->grid_array[y][x].m_idx) {
                continue;
            }

            sn++;
            if (randint0(sn) > 0) {
                continue;
            }

            sy = y;
            sx = x;
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
                damage = damroll(10, 4);
                BadStatusSetter(player_ptr).mod_stun(randint1(50));
            }

            (void)move_player_effect(player_ptr, sy, sx, MPE_DONT_PICKUP);
        }

        map[16 + player_ptr->y - cy][16 + player_ptr->x - cx] = false;
        if (damage) {
            std::string killer;

            if (m_idx) {
                auto *m_ptr = &floor_ptr->m_list[m_idx];
                const auto m_name = monster_desc(player_ptr, m_ptr, MD_WRONGDOER_NAME);
                killer = format(_("%sの起こした地震", "an earthquake caused by %s"), m_name.data());
            } else {
                killer = _("地震", "an earthquake");
            }

            take_hit(player_ptr, DAMAGE_ATTACK, damage, killer);
        }
    }

    for (POSITION dy = -r; dy <= r; dy++) {
        for (POSITION dx = -r; dx <= r; dx++) {
            POSITION yy = cy + dy;
            POSITION xx = cx + dx;
            if (!map[16 + yy - cy][16 + xx - cx]) {
                continue;
            }

            grid_type *gg_ptr;
            gg_ptr = &floor_ptr->grid_array[yy][xx];
            if (gg_ptr->m_idx == player_ptr->riding) {
                continue;
            }

            if (!gg_ptr->m_idx) {
                continue;
            }

            auto *m_ptr = &floor_ptr->m_list[gg_ptr->m_idx];
            auto *r_ptr = &m_ptr->get_monrace();
            if (r_ptr->flags1 & RF1_QUESTOR) {
                map[16 + yy - cy][16 + xx - cx] = false;
                continue;
            }

            if (r_ptr->feature_flags.has(MonsterFeatureType::KILL_WALL) || r_ptr->feature_flags.has(MonsterFeatureType::PASS_WALL)) {
                continue;
            }

            sn = 0;
            if (r_ptr->behavior_flags.has_not(MonsterBehaviorType::NEVER_MOVE)) {
                for (DIRECTION i = 0; i < 8; i++) {
                    POSITION y = yy + ddy_ddd[i];
                    POSITION x = xx + ddx_ddd[i];
                    if (!is_cave_empty_bold(player_ptr, y, x)) {
                        continue;
                    }

                    auto *g_ptr = &floor_ptr->grid_array[y][x];
                    if (g_ptr->is_rune_protection()) {
                        continue;
                    }

                    if (g_ptr->is_rune_explosion()) {
                        continue;
                    }

                    if (pattern_tile(floor_ptr, y, x)) {
                        continue;
                    }

                    if (map[16 + y - cy][16 + x - cx]) {
                        continue;
                    }

                    if (floor_ptr->grid_array[y][x].m_idx) {
                        continue;
                    }

                    if (player_bold(player_ptr, y, x)) {
                        continue;
                    }

                    sn++;

                    if (randint0(sn) > 0) {
                        continue;
                    }

                    sy = y;
                    sx = x;
                }
            }

            const auto m_name = monster_desc(player_ptr, m_ptr, 0);
            if (!ignore_unview || is_seen(player_ptr, m_ptr)) {
                msg_format(_("%s^は苦痛で泣きわめいた！", "%s^ wails out in pain!"), m_name.data());
            }

            damage = (sn ? damroll(4, 8) : (m_ptr->hp + 1));
            (void)set_monster_csleep(player_ptr, gg_ptr->m_idx, 0);
            m_ptr->hp -= damage;
            if (m_ptr->hp < 0) {
                if (!ignore_unview || is_seen(player_ptr, m_ptr)) {
                    msg_format(_("%s^は岩石に埋もれてしまった！", "%s^ is embedded in the rock!"), m_name.data());
                }

                if (gg_ptr->m_idx) {
                    const auto &m_ref = floor_ptr->m_list[gg_ptr->m_idx];
                    if (record_named_pet && m_ref.is_named_pet()) {
                        const auto m2_name = monster_desc(player_ptr, m_ptr, MD_INDEF_VISIBLE);
                        exe_write_diary(player_ptr, DiaryKind::NAMED_PET, RECORD_NAMED_PET_EARTHQUAKE, m2_name);
                    }
                }

                delete_monster(player_ptr, yy, xx);
                sn = 0;
            }

            if (sn == 0) {
                continue;
            }

            IDX m_idx_aux = floor_ptr->grid_array[yy][xx].m_idx;
            floor_ptr->grid_array[yy][xx].m_idx = 0;
            floor_ptr->grid_array[sy][sx].m_idx = m_idx_aux;
            m_ptr->fy = sy;
            m_ptr->fx = sx;
            update_monster(player_ptr, m_idx_aux, true);
            lite_spot(player_ptr, yy, xx);
            lite_spot(player_ptr, sy, sx);
        }
    }

    clear_mon_lite(floor_ptr);
    for (POSITION dy = -r; dy <= r; dy++) {
        for (POSITION dx = -r; dx <= r; dx++) {
            POSITION yy = cy + dy;
            POSITION xx = cx + dx;
            if (!map[16 + yy - cy][16 + xx - cx]) {
                continue;
            }

            if (!cave_valid_bold(floor_ptr, yy, xx)) {
                continue;
            }

            delete_all_items_from_floor(player_ptr, yy, xx);
            int t = cave_has_flag_bold(floor_ptr, yy, xx, TerrainCharacteristics::PROJECT) ? randint0(100) : 200;
            if (t < 20) {
                cave_set_feat(player_ptr, yy, xx, feat_granite);
                continue;
            }

            if (t < 70) {
                cave_set_feat(player_ptr, yy, xx, feat_quartz_vein);
                continue;
            }

            if (t < 100) {
                cave_set_feat(player_ptr, yy, xx, feat_magma_vein);
                continue;
            }

            cave_set_feat(player_ptr, yy, xx, rand_choice(feat_ground_type));
        }
    }

    for (POSITION dy = -r; dy <= r; dy++) {
        for (POSITION dx = -r; dx <= r; dx++) {
            POSITION yy = cy + dy;
            POSITION xx = cx + dx;
            if (!in_bounds(floor_ptr, yy, xx)) {
                continue;
            }

            if (distance(cy, cx, yy, xx) > r) {
                continue;
            }

            auto *g_ptr = &floor_ptr->grid_array[yy][xx];
            if (g_ptr->is_mirror()) {
                g_ptr->info |= CAVE_GLOW;
                continue;
            }

            if (floor_ptr->get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
                continue;
            }

            grid_type *cc_ptr;
            for (DIRECTION ii = 0; ii < 9; ii++) {
                POSITION yyy = yy + ddy_ddd[ii];
                POSITION xxx = xx + ddx_ddd[ii];
                if (!in_bounds2(floor_ptr, yyy, xxx)) {
                    continue;
                }
                cc_ptr = &floor_ptr->grid_array[yyy][xxx];
                if (terrains_info[cc_ptr->get_feat_mimic()].flags.has(TerrainCharacteristics::GLOW)) {
                    g_ptr->info |= CAVE_GLOW;
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
    if (floor_ptr->grid_array[player_ptr->y][player_ptr->x].info & CAVE_GLOW) {
        set_superstealth(player_ptr, false);
    }

    return true;
}
