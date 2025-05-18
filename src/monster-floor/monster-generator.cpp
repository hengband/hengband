/*!
 * todo 後で再分割する
 * @brief モンスター生成処理
 * @date 2020/06/10
 * @author Hourier
 */

#include "monster-floor/monster-generator.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-util.h"
#include "game-option/cheat-options.h"
#include "game-option/cheat-types.h"
#include "monster-floor/one-monster-placer.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "spell/summon-types.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"

/*!
 * @brief モンスター1体を目標地点に可能な限り近い位置に生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monracde_id 生成モンスター種族
 * @param pos 中心生成位置座標
 * @param max_distance 生成位置の最大半径
 * @return 生成成功ならば結果生成位置座標、失敗ならばnullopt
 *
 */
std::optional<Pos2D> mon_scatter(PlayerType *player_ptr, MonraceId monrace_id, const Pos2D &pos, int max_distance)
{
    constexpr auto max_distance_permitted = 10;
    std::vector<Pos2D> places;
    for (auto i = 0; i < max_distance_permitted; i++) {
        places.emplace_back(0, 0);
    }

    std::vector<int> numbers(max_distance_permitted);
    if (max_distance >= max_distance_permitted) {
        return std::nullopt;
    }

    const auto p_pos = player_ptr->get_position();
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monraces = MonraceList::get_instance();
    auto dist = 0;
    for (auto nx = pos.x - max_distance; nx <= pos.x + max_distance; nx++) {
        for (auto ny = pos.y - max_distance; ny <= pos.y + max_distance; ny++) {
            const Pos2D pos_neighbor(ny, nx);
            if (!floor.contains(pos_neighbor)) {
                continue;
            }

            if (!projectable(floor, p_pos, pos, pos_neighbor)) {
                continue;
            }

            if (MonraceList::is_valid(monrace_id)) {
                const auto &monrace = monraces.get_monrace(monrace_id);
                if (!monster_can_enter(player_ptr, pos_neighbor.y, pos_neighbor.x, monrace, 0)) {
                    continue;
                }
            } else {
                if (!floor.can_generate_monster_at(pos_neighbor) || (p_pos == pos_neighbor)) {
                    continue;
                }

                if (floor.has_terrain_characteristics(pos_neighbor, TerrainCharacteristics::PATTERN)) {
                    continue;
                }
            }

            dist = Grid::calc_distance(pos, pos_neighbor);
            if (dist > max_distance) {
                continue;
            }

            numbers[dist]++;
            if (one_in_(numbers[dist])) {
                places[dist] = pos_neighbor;
            }
        }
    }

    auto i = 0;
    while ((i < max_distance_permitted) && (numbers[i] == 0)) {
        i++;
    }

    if (i >= max_distance_permitted) {
        return std::nullopt;
    }

    return places[i];
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
    auto &floor = *player_ptr->current_floor_ptr;
    auto &monster = floor.m_list[m_idx];
    const auto pos = mon_scatter(player_ptr, monster.r_idx, monster.get_position(), 1);
    if (!pos) {
        return std::nullopt;
    }

    if (monster.mflag2.has(MonsterConstantFlagType::NOPET)) {
        mode |= PM_NO_PET;
    }

    const auto multiplied_m_idx = place_specific_monster(player_ptr, pos->y, pos->x, monster.r_idx, (mode | PM_NO_KAGE | PM_MULTIPLY), m_idx);
    if (!multiplied_m_idx) {
        return std::nullopt;
    }

    if (clone || monster.mflag2.has(MonsterConstantFlagType::CLONED)) {
        floor.m_list[*multiplied_m_idx].mflag2.set({ MonsterConstantFlagType::CLONED, MonsterConstantFlagType::NOPET });
    }

    return multiplied_m_idx;
}

/*!
 * @brief モンスターを目標地点に集団生成する
 * @param pos_center 中心生成位置
 * @param monrace_id 生成モンスター種族
 * @param mode 生成オプション
 * @param summoner_m_idx モンスターの召喚による場合、召喚主のモンスターID
 */
static void place_monster_group(PlayerType *player_ptr, const Pos2D &pos_center, MonraceId monrace_id, BIT_FLAGS mode, std::optional<MONSTER_IDX> summoner_m_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto floor_level = floor.dun_level;
    auto extra = 0;
    if (monrace.level > floor_level) {
        extra = monrace.level - floor_level;
        extra = 0 - randint1(extra);
    } else if (monrace.level < floor_level) {
        extra = floor_level - monrace.level;
        extra = randint1(extra);
    }

    if (extra > 9) {
        extra = 9;
    }

    auto total_int = randint1(10) + extra;
    if (total_int < 1) {
        total_int = 1;
    }

    constexpr auto max_monsters_count = 32;
    if (total_int > max_monsters_count) {
        total_int = max_monsters_count;
    }

    const size_t total_size = total_int;
    std::vector<Pos2D> positions;
    positions.push_back(pos_center);
    const auto p_pos = player_ptr->get_position();
    for (size_t n = 0; (n < positions.size()) && (positions.size() < total_size); n++) {
        for (auto i = 0; (i < 8) && (positions.size() < total_size); i++) {
            //!< @details 要素数が変わると参照がダングリング状態になるので毎回取得する必要がある.
            const auto &pos_neighbor = positions.at(n);
            const auto pos = scatter(player_ptr, pos_neighbor, 4, PROJECT_NONE);
            if (!floor.can_generate_monster_at(pos) || (p_pos == pos)) {
                continue;
            }

            if (place_monster_one(player_ptr, pos.y, pos.x, monrace_id, mode, summoner_m_idx)) {
                positions.push_back(pos);
            }
        }
    }
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
std::optional<MONSTER_IDX> place_specific_monster(PlayerType *player_ptr, POSITION y, POSITION x, MonraceId r_idx, BIT_FLAGS mode, std::optional<MONSTER_IDX> summoner_m_idx)
{
    const Pos2D pos(y, x);
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    if (!(mode & PM_NO_KAGE) && one_in_(333)) {
        mode |= PM_KAGE;
    }

    const auto m_idx = place_monster_one(player_ptr, y, x, r_idx, mode, summoner_m_idx);
    if (!m_idx) {
        return std::nullopt;
    }
    if (!(mode & PM_ALLOW_GROUP)) {
        return m_idx;
    }

    /* Reinforcement */
    for (const auto &reinforce : monrace.get_reinforces()) {
        if (!reinforce.is_valid()) {
            continue;
        }

        const auto n = reinforce.roll_dice();
        for (int j = 0; j < n; j++) {
            constexpr auto scatter_min = 7;
            constexpr auto scatter_max = 40;
            int d;
            for (d = scatter_min; d <= scatter_max; d++) {
                const auto pos_neighbor = scatter(player_ptr, pos, d, PROJECT_NONE);
                if (place_monster_one(player_ptr, pos_neighbor.y, pos_neighbor.x, reinforce.get_monrace_id(), mode, *m_idx)) {
                    break;
                }
            }
            if (d > scatter_max) {
                msg_format_wizard(player_ptr, CHEAT_MONSTER, _("護衛の指定生成に失敗しました。", "Failed fixed escorts."));
            }
        }
    }

    if (monrace.misc_flags.has(MonsterMiscType::HAS_FRIENDS)) {
        place_monster_group(player_ptr, pos, r_idx, mode, summoner_m_idx);
    }

    if (monrace.misc_flags.has_not(MonsterMiscType::ESCORT)) {
        return m_idx;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    for (auto i = 0; i < 32; i++) {
        constexpr auto d = 3;
        const auto pos_neighbor = scatter(player_ptr, pos, d, PROJECT_NONE);
        if (!floor.can_generate_monster_at(pos_neighbor) || (p_pos == pos_neighbor)) {
            continue;
        }

        get_mon_num_prep_escort(player_ptr, r_idx, *m_idx, player_ptr->current_floor_ptr->get_monrace_hook_terrain_at(pos_neighbor));
        const auto monrace_id = get_mon_num(player_ptr, 0, monrace.level, 0);
        if (!MonraceList::is_valid(monrace_id)) {
            break;
        }

        (void)place_monster_one(player_ptr, pos_neighbor.y, pos_neighbor.x, monrace_id, mode, *m_idx);
        if (monrace.misc_flags.has(MonsterMiscType::HAS_FRIENDS) || monrace.misc_flags.has(MonsterMiscType::MORE_ESCORT)) {
            place_monster_group(player_ptr, pos_neighbor, monrace_id, mode, *m_idx);
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
    const Pos2D pos(y, x);
    const auto &floor = *player_ptr->current_floor_ptr;
    get_mon_num_prep_enum(player_ptr, floor.get_monrace_hook(), floor.get_monrace_hook_terrain_at(pos));
    const auto &monraces = MonraceList::get_instance();
    MonraceId monrace_id;
    do {
        monrace_id = get_mon_num(player_ptr, 0, floor.monster_level, PM_NONE);
    } while ((mode & PM_NO_QUEST) && monraces.get_monrace(monrace_id).misc_flags.has(MonsterMiscType::NO_QUEST));
    if (!MonraceList::is_valid(monrace_id)) {
        return std::nullopt;
    }

    auto try_become_jural = one_in_(5) || !floor.is_underground();
    const auto &monrace = monraces.get_monrace(monrace_id);
    try_become_jural &= monrace.kind_flags.has_not(MonsterKindType::UNIQUE);
    try_become_jural &= monrace.symbol_char_is_any_of("hkoptuyAHLOPTUVY");
    if (try_become_jural) {
        mode |= PM_JURAL;
    }

    return place_specific_monster(player_ptr, y, x, monrace_id, mode);
}

static std::optional<MonraceId> select_horde_leader_r_idx(PlayerType *player_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monraces = MonraceList::get_instance();
    for (auto attempts = 1000; attempts > 0; --attempts) {
        const auto monrace_id = get_mon_num(player_ptr, 0, floor.monster_level, PM_NONE);
        if (!MonraceList::is_valid(monrace_id)) {
            return std::nullopt;
        }

        if (monraces.get_monrace(monrace_id).kind_flags.has(MonsterKindType::UNIQUE)) {
            continue;
        }

        if (monrace_id == MonraceId::HAGURE) {
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
    Pos2D pos(y, x);
    const auto &floor = *player_ptr->current_floor_ptr;
    get_mon_num_prep_enum(player_ptr, floor.get_monrace_hook(), floor.get_monrace_hook_terrain_at(pos));
    const auto monrace_id = select_horde_leader_r_idx(player_ptr);
    if (!monrace_id) {
        return false;
    }

    for (auto attempts = 1000;; --attempts) {
        if (attempts <= 0) {
            return false;
        }

        if (place_specific_monster(player_ptr, y, x, *monrace_id, 0L)) {
            break;
        }
    }

    const auto m_idx = floor.get_grid(pos).m_idx;
    const auto &monentity = floor.m_list[m_idx];
    for (auto attempts = randint1(10) + 5; attempts > 0; attempts--) {
        const auto pos_scat = scatter(player_ptr, pos, 5, PROJECT_NONE);
        (void)(*summon_specific)(player_ptr, pos_scat.y, pos_scat.x, floor.dun_level + 5, SUMMON_KIN, PM_ALLOW_GROUP, m_idx);
        pos = pos_scat;
    }

    if (!cheat_hear) {
        return true;
    }

    const auto &monraces = MonraceList::get_instance();
    const auto is_chameleon = monentity.mflag2.has(MonsterConstantFlagType::CHAMELEON);
    const auto &monrace = is_chameleon ? monentity.get_monrace() : monraces.get_monrace(*monrace_id);
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
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &dungeon = floor.get_dungeon_definition();
    if (!dungeon.has_guardian()) {
        return def_val;
    }

    const auto &monrace = dungeon.get_guardian();
    auto is_guardian_applicable = dungeon.maxdepth == floor.dun_level;
    is_guardian_applicable &= monrace.cur_num < monrace.max_num;
    if (!is_guardian_applicable) {
        return def_val;
    }

    const auto p_pos = player_ptr->get_position();
    auto try_count = 4000;
    while (try_count > 0) {
        const auto pos = Pos2D(randint1(floor.height - 4), randint1(floor.width - 4)) + Pos2DVec(2, 2);
        if (!floor.can_generate_monster_at(pos) || (p_pos == pos)) {
            try_count++;
            continue;
        }

        if (!monster_can_cross_terrain(player_ptr, floor.get_grid(pos).feat, monrace, 0)) {
            try_count++;
            continue;
        }

        if (place_specific_monster(player_ptr, pos.y, pos.x, dungeon.final_guardian, (PM_ALLOW_GROUP | PM_NO_KAGE | PM_NO_PET))) {
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

    const auto p_pos = player_ptr->get_position();
    auto &floor = *player_ptr->current_floor_ptr;
    Pos2D pos(0, 0);
    auto attempts_left = 10000;
    while (attempts_left--) {
        pos.y = randint0(floor.height);
        pos.x = randint0(floor.width);
        if (floor.is_underground()) {
            if (!floor.can_generate_monster_at(pos) || (p_pos == pos)) {
                continue;
            }
        } else {
            if (!floor.is_empty_at(pos) || (pos == p_pos)) {
                continue;
            }
        }

        const auto dist = Grid::calc_distance(pos, p_pos);
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

    if (randint1(5000) <= floor.dun_level) {
        return alloc_horde(player_ptr, pos.y, pos.x, summon_specific);
    }

    return place_random_monster(player_ptr, pos.y, pos.x, (mode | PM_ALLOW_GROUP)).has_value();
}
