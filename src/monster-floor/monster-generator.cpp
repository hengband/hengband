/*!
 * todo 後で再分割する
 * @brief モンスター生成処理
 * @date 2020/06/10
 * @author Hourier
 */

#include "monster-floor/monster-generator.h"
#include "effect/effect-characteristics.h"
#include "floor/cave.h"
#include "floor/floor-util.h"
#include "floor/geometry.h"
#include "game-option/cheat-options.h"
#include "game-option/cheat-types.h"
#include "monster-floor/one-monster-placer.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "monster/smart-learn-types.h"
#include "mspell/summon-checker.h"
#include "spell/summon-types.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"
#include <optional>

#define MON_SCAT_MAXD 10 /*!< mon_scatter()関数によるモンスター配置で許される中心からの最大距離 */

/*!
 * @brief モンスター1体を目標地点に可能な限り近い位置に生成する / improved version of scatter() for place monster
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param r_idx 生成モンスター種族
 * @param yp 結果生成位置y座標
 * @param xp 結果生成位置x座標
 * @param y 中心生成位置y座標
 * @param x 中心生成位置x座標
 * @param max_dist 生成位置の最大半径
 * @return 成功したらtrue
 *
 */
bool mon_scatter(PlayerType *player_ptr, MonsterRaceId r_idx, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION max_dist)
{
    int place_x[MON_SCAT_MAXD]{};
    int place_y[MON_SCAT_MAXD]{};
    int num[MON_SCAT_MAXD]{};

    if (max_dist >= MON_SCAT_MAXD) {
        return false;
    }

    int i;
    for (i = 0; i < MON_SCAT_MAXD; i++) {
        num[i] = 0;
    }

    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (POSITION nx = x - max_dist; nx <= x + max_dist; nx++) {
        for (POSITION ny = y - max_dist; ny <= y + max_dist; ny++) {
            if (!in_bounds(floor_ptr, ny, nx)) {
                continue;
            }
            if (!projectable(player_ptr, y, x, ny, nx)) {
                continue;
            }
            if (MonraceList::is_valid(r_idx)) {
                auto *r_ptr = &monraces_info[r_idx];
                if (!monster_can_enter(player_ptr, ny, nx, r_ptr, 0)) {
                    continue;
                }
            } else {
                if (!is_cave_empty_bold2(player_ptr, ny, nx)) {
                    continue;
                }
                if (pattern_tile(floor_ptr, ny, nx)) {
                    continue;
                }
            }

            i = distance(y, x, ny, nx);
            if (i > max_dist) {
                continue;
            }

            num[i]++;
            if (one_in_(num[i])) {
                place_x[i] = nx;
                place_y[i] = ny;
            }
        }
    }

    i = 0;
    while (i < MON_SCAT_MAXD && 0 == num[i]) {
        i++;
    }
    if (i >= MON_SCAT_MAXD) {
        return false;
    }

    *xp = place_x[i];
    *yp = place_y[i];

    return true;
}

/*!
 * @brief モンスターを増殖生成する / Let the given monster attempt to reproduce.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 増殖するモンスター情報ID
 * @param clone クローン・モンスター処理ならばtrue
 * @param mode 生成オプション
 * @return 生成に成功したらモンスターID、失敗したらstd::nullopt
 * @details
 * Note that "reproduction" REQUIRES empty space.
 */
std::optional<MONSTER_IDX> multiply_monster(PlayerType *player_ptr, MONSTER_IDX m_idx, bool clone, BIT_FLAGS mode)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    POSITION y, x;
    if (!mon_scatter(player_ptr, m_ptr->r_idx, &y, &x, m_ptr->fy, m_ptr->fx, 1)) {
        return std::nullopt;
    }

    if (m_ptr->mflag2.has(MonsterConstantFlagType::NOPET)) {
        mode |= PM_NO_PET;
    }

    const auto multiplied_m_idx = place_specific_monster(player_ptr, m_idx, y, x, m_ptr->r_idx, (mode | PM_NO_KAGE | PM_MULTIPLY));
    if (!multiplied_m_idx) {
        return std::nullopt;
    }

    if (clone || m_ptr->mflag2.has(MonsterConstantFlagType::CLONED)) {
        floor_ptr->m_list[*multiplied_m_idx].mflag2.set({ MonsterConstantFlagType::CLONED, MonsterConstantFlagType::NOPET });
    }

    return multiplied_m_idx;
}

/*!
 * @brief モンスターを目標地点に集団生成する / Attempt to place a "group" of monsters around the given location
 * @param src_idx 召喚主のモンスター情報ID
 * @param y 中心生成位置y座標
 * @param x 中心生成位置x座標
 * @param r_idx 生成モンスター種族
 * @param mode 生成オプション
 * @param summoner_m_idx モンスターの召喚による場合、召喚主のモンスターID
 * @return 成功したらtrue
 */
static bool place_monster_group(PlayerType *player_ptr, MONSTER_IDX src_idx, POSITION y, POSITION x, MonsterRaceId r_idx, BIT_FLAGS mode, std::optional<MONSTER_IDX> summoner_m_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    auto total = randint1(10);

    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto extra = 0;
    if (r_ptr->level > floor_ptr->dun_level) {
        extra = r_ptr->level - floor_ptr->dun_level;
        extra = 0 - randint1(extra);
    } else if (r_ptr->level < floor_ptr->dun_level) {
        extra = floor_ptr->dun_level - r_ptr->level;
        extra = randint1(extra);
    }

    if (extra > 9) {
        extra = 9;
    }

    total += extra;

    if (total < 1) {
        total = 1;
    }

    constexpr auto max_monsters_count = 32;
    if (total > max_monsters_count) {
        total = max_monsters_count;
    }

    auto hack_n = 1;
    POSITION hack_x[max_monsters_count]{};
    hack_x[0] = x;
    POSITION hack_y[max_monsters_count]{};
    hack_y[0] = y;

    for (auto n = 0; (n < hack_n) && (hack_n < total); n++) {
        POSITION hx = hack_x[n];
        POSITION hy = hack_y[n];
        for (int i = 0; (i < 8) && (hack_n < total); i++) {
            POSITION mx, my;
            scatter(player_ptr, &my, &mx, hy, hx, 4, PROJECT_NONE);
            if (!is_cave_empty_bold2(player_ptr, my, mx)) {
                continue;
            }

            if (place_monster_one(player_ptr, src_idx, my, mx, r_idx, mode, summoner_m_idx)) {
                hack_y[hack_n] = my;
                hack_x[hack_n] = mx;
                hack_n++;
            }
        }
    }

    return true;
}

/*!
 * @brief モンスター種族が護衛となれるかどうかをチェックする
 * @param monrace_id チェックするモンスターの種族ID
 * @param escorted_monrace_id 護衛されるモンスターの種族ID
 * @param escorted_m_idx 護衛されるモンスターのモンスターID
 * @return 護衛にできるならばtrue
 */
static bool place_monster_can_escort(PlayerType *player_ptr, MonsterRaceId monrace_id, MonsterRaceId escorted_monrace_id, MONSTER_IDX escorted_m_idx)
{
    auto *r_ptr = &monraces_info[escorted_monrace_id];
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[escorted_m_idx];
    MonsterRaceInfo *z_ptr = &monraces_info[monrace_id];

    if (mon_hook_dungeon(player_ptr, escorted_monrace_id) != mon_hook_dungeon(player_ptr, monrace_id)) {
        return false;
    }

    if (z_ptr->symbol_definition.character != r_ptr->symbol_definition.character) {
        return false;
    }

    if (z_ptr->level > r_ptr->level) {
        return false;
    }

    if (z_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        return false;
    }

    if (escorted_monrace_id == monrace_id) {
        return false;
    }

    if (monster_has_hostile_align(player_ptr, m_ptr, 0, 0, z_ptr)) {
        return false;
    }

    if (r_ptr->behavior_flags.has(MonsterBehaviorType::FRIENDLY)) {
        if (monster_has_hostile_align(player_ptr, nullptr, 1, -1, z_ptr)) {
            return false;
        }
    }

    if (r_ptr->misc_flags.has(MonsterMiscType::CHAMELEON) && z_ptr->misc_flags.has_not(MonsterMiscType::CHAMELEON)) {
        return false;
    }

    return true;
}

/*!
 * @brief 特定モンスターを生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param src_idx 召喚主のモンスター情報ID
 * @param y 生成地点y座標
 * @param x 生成地点x座標
 * @param r_idx 生成するモンスターの種族ID
 * @param mode 生成オプション
 * @param summoner_m_idx モンスターの召喚による場合、召喚主のモンスターID
 * @return 生成に成功したらモンスターID、失敗したらstd::nullopt
 * @details 護衛も一緒に生成する
 */
std::optional<MONSTER_IDX> place_specific_monster(PlayerType *player_ptr, MONSTER_IDX src_idx, POSITION y, POSITION x, MonsterRaceId r_idx, BIT_FLAGS mode, std::optional<MONSTER_IDX> summoner_m_idx)
{
    auto *r_ptr = &monraces_info[r_idx];

    if (!(mode & PM_NO_KAGE) && one_in_(333)) {
        mode |= PM_KAGE;
    }

    const auto m_idx = place_monster_one(player_ptr, src_idx, y, x, r_idx, mode, summoner_m_idx);
    if (!m_idx) {
        return std::nullopt;
    }
    if (!(mode & PM_ALLOW_GROUP)) {
        return m_idx;
    }

    /* Reinforcement */
    for (const auto &[reinforce_monrace_id, num_dice] : r_ptr->reinforces) {
        if (!MonraceList::is_valid(reinforce_monrace_id)) {
            continue;
        }
        auto n = num_dice.roll();
        for (int j = 0; j < n; j++) {
            POSITION nx, ny, d;
            const POSITION scatter_min = 7;
            const POSITION scatter_max = 40;
            for (d = scatter_min; d <= scatter_max; d++) {
                scatter(player_ptr, &ny, &nx, y, x, d, PROJECT_NONE);
                if (place_monster_one(player_ptr, *m_idx, ny, nx, reinforce_monrace_id, mode, summoner_m_idx)) {
                    break;
                }
            }
            if (d > scatter_max) {
                msg_format_wizard(player_ptr, CHEAT_MONSTER, _("護衛の指定生成に失敗しました。", "Failed fixed escorts."));
            }
        }
    }

    if (r_ptr->misc_flags.has(MonsterMiscType::HAS_FRIENDS)) {
        (void)place_monster_group(player_ptr, src_idx, y, x, r_idx, mode, summoner_m_idx);
    }

    if (r_ptr->misc_flags.has_not(MonsterMiscType::ESCORT)) {
        return m_idx;
    }

    for (int i = 0; i < 32; i++) {
        POSITION nx, ny, d = 3;
        scatter(player_ptr, &ny, &nx, y, x, d, PROJECT_NONE);
        if (!is_cave_empty_bold2(player_ptr, ny, nx)) {
            continue;
        }

        auto hook = [place_monster_monrace_id = r_idx, place_monster_m_idx = *m_idx](PlayerType *player_ptr, MonsterRaceId escort_monrace_id) {
            return place_monster_can_escort(player_ptr, escort_monrace_id, place_monster_monrace_id, place_monster_m_idx);
        };
        get_mon_num_prep(player_ptr, std::move(hook), get_monster_hook2(player_ptr, ny, nx));
        const auto monrace_id = get_mon_num(player_ptr, 0, r_ptr->level, 0);
        if (!MonraceList::is_valid(monrace_id)) {
            break;
        }

        (void)place_monster_one(player_ptr, *m_idx, ny, nx, monrace_id, mode, summoner_m_idx);
        if (monraces_info[monrace_id].misc_flags.has(MonsterMiscType::HAS_FRIENDS) || r_ptr->misc_flags.has(MonsterMiscType::MORE_ESCORT)) {
            (void)place_monster_group(player_ptr, *m_idx, ny, nx, monrace_id, mode, summoner_m_idx);
        }
    }

    return m_idx;
}
/*!
 * @brief フロア相当のモンスターを1体生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 生成地点y座標
 * @param x 生成地点x座標
 * @param mode 生成オプション
 * @return 生成に成功したらモンスターID、失敗したらstd::nullopt
 */
std::optional<MONSTER_IDX> place_random_monster(PlayerType *player_ptr, POSITION y, POSITION x, BIT_FLAGS mode)
{
    get_mon_num_prep(player_ptr, get_monster_hook(player_ptr), get_monster_hook2(player_ptr, y, x));
    const auto &floor = *player_ptr->current_floor_ptr;
    MonsterRaceId monrace_id;
    do {
        monrace_id = get_mon_num(player_ptr, 0, floor.monster_level, PM_NONE);
    } while ((mode & PM_NO_QUEST) && monraces_info[monrace_id].misc_flags.has(MonsterMiscType::NO_QUEST));
    if (!MonraceList::is_valid(monrace_id)) {
        return std::nullopt;
    }

    auto try_become_jural = one_in_(5) || !floor.is_in_underground();
    try_become_jural &= monraces_info[monrace_id].kind_flags.has_not(MonsterKindType::UNIQUE);
    try_become_jural &= monraces_info[monrace_id].symbol_char_is_any_of("hkoptuyAHLOPTUVY");
    if (try_become_jural) {
        mode |= PM_JURAL;
    }

    return place_specific_monster(player_ptr, 0, y, x, monrace_id, mode);
}

static std::optional<MonsterRaceId> select_horde_leader_r_idx(PlayerType *player_ptr)
{
    const auto *floor_ptr = player_ptr->current_floor_ptr;

    for (auto attempts = 1000; attempts > 0; --attempts) {
        const auto monrace_id = get_mon_num(player_ptr, 0, floor_ptr->monster_level, PM_NONE);
        if (!MonraceList::is_valid(monrace_id)) {
            return std::nullopt;
        }

        if (monraces_info[monrace_id].kind_flags.has(MonsterKindType::UNIQUE)) {
            continue;
        }

        if (monrace_id == MonsterRaceId::HAGURE) {
            continue;
        }

        return monrace_id;
    }

    return std::nullopt;
}

/*!
 * @brief 指定地点に1種類のモンスター種族による群れを生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 生成地点y座標
 * @param x 生成地点x座標
 * @return 生成に成功したらtrue
 */
bool alloc_horde(PlayerType *player_ptr, POSITION y, POSITION x, summon_specific_pf summon_specific)
{
    get_mon_num_prep(player_ptr, get_monster_hook(player_ptr), get_monster_hook2(player_ptr, y, x));

    const auto r_idx = select_horde_leader_r_idx(player_ptr);
    if (!r_idx) {
        return false;
    }

    for (auto attempts = 1000;; --attempts) {
        if (attempts <= 0) {
            return false;
        }

        if (place_specific_monster(player_ptr, 0, y, x, *r_idx, 0L)) {
            break;
        }
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto m_idx = floor.get_grid({ y, x }).m_idx;
    const auto &monentity = floor.m_list[m_idx];

    POSITION cy = y;
    POSITION cx = x;
    for (auto attempts = randint1(10) + 5; attempts > 0; attempts--) {
        scatter(player_ptr, &cy, &cx, y, x, 5, PROJECT_NONE);
        (void)(*summon_specific)(player_ptr, cy, cx, floor.dun_level + 5, SUMMON_KIN, PM_ALLOW_GROUP, m_idx);
        y = cy;
        x = cx;
    }

    if (!cheat_hear) {
        return true;
    }

    const auto &monrace = monentity.mflag2.has(MonsterConstantFlagType::CHAMELEON) ? monentity.get_monrace() : monraces_info[*r_idx];
    msg_format(_("モンスターの大群(%c)", "Monster horde (%c)."), monrace.symbol_definition.character);
    return true;
}

/*!
 * @brief ダンジョンの主生成を試みる / Put the Guardian
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param def_val 現在の主の生成状態
 * @return 生成に成功したらtrue
 */
bool alloc_guardian(PlayerType *player_ptr, bool def_val)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    const auto &dungeon = floor_ptr->get_dungeon_definition();
    if (!dungeon.has_guardian()) {
        return def_val;
    }

    const auto &monrace = dungeon.get_guardian();
    auto is_guardian_applicable = dungeon.maxdepth == floor_ptr->dun_level;
    is_guardian_applicable &= monrace.cur_num < monrace.max_num;
    if (!is_guardian_applicable) {
        return def_val;
    }

    auto try_count = 4000;
    while (try_count > 0) {
        const auto pos = Pos2D(randint1(floor_ptr->height - 4), randint1(floor_ptr->width - 4)) + Pos2DVec(2, 2);
        if (!is_cave_empty_bold2(player_ptr, pos.y, pos.x)) {
            try_count++;
            continue;
        }

        if (!monster_can_cross_terrain(player_ptr, floor_ptr->get_grid(pos).feat, &monrace, 0)) {
            try_count++;
            continue;
        }

        if (place_specific_monster(player_ptr, 0, pos.y, pos.x, dungeon.final_guardian, (PM_ALLOW_GROUP | PM_NO_KAGE | PM_NO_PET))) {
            return true;
        }

        try_count--;
    }

    return false;
}

/*!
 * @brief ダンジョンの初期配置モンスターを生成1回生成する / Attempt to allocate a random monster in the dungeon.
 * @param dis プレイヤーから離れるべき最小距離
 * @param mode 生成オプション
 * @param summon_specific 特定モンスター種別を生成するための関数ポインタ
 * @param max_dis プレイヤーから離れるべき最大距離 (デバッグ用)
 * @return 生成に成功したらtrue
 */
bool alloc_monster(PlayerType *player_ptr, int min_dis, BIT_FLAGS mode, summon_specific_pf summon_specific, int max_dis)
{
    if (alloc_guardian(player_ptr, false)) {
        return true;
    }

    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto y = 0;
    auto x = 0;
    auto attempts_left = 10000;
    while (attempts_left--) {
        y = randint0(floor_ptr->height);
        x = randint0(floor_ptr->width);

        if (floor_ptr->dun_level) {
            if (!is_cave_empty_bold2(player_ptr, y, x)) {
                continue;
            }
        } else {
            if (!is_cave_empty_bold(player_ptr, y, x)) {
                continue;
            }
        }

        const auto dist = distance(y, x, player_ptr->y, player_ptr->x);
        if ((min_dis < dist) && (dist <= max_dis)) {
            break;
        }
    }

    if (!attempts_left) {
        if (cheat_xtra || cheat_hear) {
            msg_print(_("警告！新たなモンスターを配置できません。小さい階ですか？", "Warning! Could not allocate a new monster. Small level?"));
        }

        return false;
    }

    if (randint1(5000) <= floor_ptr->dun_level) {
        if (alloc_horde(player_ptr, y, x, summon_specific)) {
            return true;
        }
    } else {
        if (place_random_monster(player_ptr, y, x, (mode | PM_ALLOW_GROUP))) {
            return true;
        }
    }

    return false;
}
