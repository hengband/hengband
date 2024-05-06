#include "room/rooms-pit-nest.h"
#include "floor/floor-generator.h"
#include "game-option/cheat-options.h"
#include "game-option/cheat-types.h"
#include "grid/door.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "room/door-definition.h"
#include "room/space-finder.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/probability-table.h"
#include "util/sort.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"
#include <array>
#include <optional>
#include <utility>
#include <vector>

namespace {
constexpr auto NUM_NEST_MON_TYPE = 64; //! nestの種別数.
constexpr auto TRAPPED_PIT_MONSTER_PLACE_MAX = 69; //! 開門トラップのモンスター数.

/*!
 * @brief 生成するNestの情報テーブル
 */
const std::vector<nest_pit_type> nest_types = {
    { _("クローン", "clone"), vault_aux_clone, vault_prep_clone, 5, 3 },
    { _("ゼリー", "jelly"), vault_aux_jelly, std::nullopt, 5, 6 },
    { _("シンボル(善)", "symbol good"), vault_aux_symbol_g, vault_prep_symbol, 25, 2 },
    { _("シンボル(悪)", "symbol evil"), vault_aux_symbol_e, vault_prep_symbol, 25, 2 },
    { _("ミミック", "mimic"), vault_aux_mimic, std::nullopt, 30, 4 },
    { _("狂気", "lovecraftian"), vault_aux_cthulhu, std::nullopt, 70, 2 },
    { _("犬小屋", "kennel"), vault_aux_kennel, std::nullopt, 45, 4 },
    { _("動物園", "animal"), vault_aux_animal, std::nullopt, 35, 5 },
    { _("教会", "chapel"), vault_aux_chapel_g, std::nullopt, 75, 4 },
    { _("アンデッド", "undead"), vault_aux_undead, std::nullopt, 75, 5 },
};

/*!
 * @brief ダンジョン毎に指定されたピット配列を基準にランダムなpit/nestタイプを決める
 * @param l_ptr 選択されたpit/nest情報を返す参照ポインタ
 * @param allow_flag_mask 生成が許されるpit/nestのビット配列
 * @return 選択されたpit/nestのID、選択失敗した場合-1を返す。
 */
int pick_vault_type(FloorType *floor_ptr, const std::vector<nest_pit_type> &l_ptr, BIT_FLAGS16 allow_flag_mask)
{
    ProbabilityTable<int> table;
    for (size_t i = 0; i < l_ptr.size(); i++) {
        const nest_pit_type *n_ptr = &l_ptr.at(i);

        if (n_ptr->level > floor_ptr->dun_level) {
            continue;
        }

        if (!(allow_flag_mask & (1UL << i))) {
            continue;
        }

        table.entry_item(i, n_ptr->chance * MAX_DEPTH / (std::min(floor_ptr->dun_level, MAX_DEPTH - 1) - n_ptr->level + 5));
    }

    return !table.empty() ? table.pick_one_at_random() : -1;
}

/*!
 * @brief デバッグ時に生成されたpit/nestの型を出力する処理
 * @param type pit/nestの型ID
 * @param nest TRUEならばnest、FALSEならばpit
 * @return デバッグ表示文字列の参照ポインタ
 * @details
 * Hack -- Get the string describing subtype of pit/nest
 * Determined in prepare function (some pit/nest only)
 */
std::string pit_subtype_string(int type, bool nest)
{
    if (nest) {
        switch (type) {
        case NEST_TYPE_CLONE:
            return std::string("(").append(monraces_info[vault_aux_race].name).append(1, ')');
        case NEST_TYPE_SYMBOL_GOOD:
        case NEST_TYPE_SYMBOL_EVIL:
            return std::string("(").append(1, vault_aux_char).append(1, ')');
        }

        return std::string();
    }

    /* Pits */
    switch (type) {
    case PIT_TYPE_SYMBOL_GOOD:
    case PIT_TYPE_SYMBOL_EVIL:
        return std::string("(").append(1, vault_aux_char).append(1, ')');
        break;
    case PIT_TYPE_DRAGON:
        if (vault_aux_dragon_mask4.has_all_of({ MonsterAbilityType::BR_ACID, MonsterAbilityType::BR_ELEC, MonsterAbilityType::BR_FIRE, MonsterAbilityType::BR_COLD, MonsterAbilityType::BR_POIS })) {
            return _("(万色)", "(multi-hued)");
        } else if (vault_aux_dragon_mask4.has(MonsterAbilityType::BR_ACID)) {
            return _("(酸)", "(acid)");
        } else if (vault_aux_dragon_mask4.has(MonsterAbilityType::BR_ELEC)) {
            return _("(稲妻)", "(lightning)");
        } else if (vault_aux_dragon_mask4.has(MonsterAbilityType::BR_FIRE)) {
            return _("(火炎)", "(fire)");
        } else if (vault_aux_dragon_mask4.has(MonsterAbilityType::BR_COLD)) {
            return _("(冷気)", "(frost)");
        } else if (vault_aux_dragon_mask4.has(MonsterAbilityType::BR_POIS)) {
            return _("(毒)", "(poison)");
        } else {
            return _("(未定義)", "(undefined)");
        }
        break;
    }

    return std::string();
}

/*
 *! @brief nestのモンスターリストをソートするための関数 /
 *  Comp function for sorting nest monster information
 *  @param u ソート処理対象配列ポインタ
 *  @param v 未使用
 *  @param a 比較対象参照ID1
 *  @param b 比較対象参照ID2
 *  TODO: to sort.c
 */
bool ang_sort_comp_nest_mon_info(PlayerType *player_ptr, vptr u, vptr v, int a, int b)
{
    /* Unused */
    (void)player_ptr;
    (void)v;

    nest_mon_info_type *nest_mon_info = (nest_mon_info_type *)u;
    auto w1 = nest_mon_info[a].r_idx;
    auto w2 = nest_mon_info[b].r_idx;
    MonsterRaceInfo *r1_ptr = &monraces_info[w1];
    MonsterRaceInfo *r2_ptr = &monraces_info[w2];
    int z1 = nest_mon_info[a].used;
    int z2 = nest_mon_info[b].used;

    if (z1 < z2) {
        return false;
    }
    if (z1 > z2) {
        return true;
    }

    if (r1_ptr->level < r2_ptr->level) {
        return true;
    }
    if (r1_ptr->level > r2_ptr->level) {
        return false;
    }

    if (r1_ptr->mexp < r2_ptr->mexp) {
        return true;
    }
    if (r1_ptr->mexp > r2_ptr->mexp) {
        return false;
    }

    return w1 <= w2;
}

/*!
 * @brief nestのモンスターリストをスワップするための関数 /
 * Swap function for sorting nest monster information
 * @param u スワップ処理対象配列ポインタ
 * @param v 未使用
 * @param a スワップ対象参照ID1
 * @param b スワップ対象参照ID2
 * TODO: to sort.c
 */
void ang_sort_swap_nest_mon_info(PlayerType *player_ptr, vptr u, vptr v, int a, int b)
{
    /* Unused */
    (void)player_ptr;
    (void)v;

    nest_mon_info_type *nest_mon_info = (nest_mon_info_type *)u;
    nest_mon_info_type holder = nest_mon_info[a];
    nest_mon_info[a] = nest_mon_info[b];
    nest_mon_info[b] = holder;
}

/*!
 * @brief Nestに格納するモンスターを選択する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param align アライメントが中立に設定されたモンスター実体 (その他の中身は空)
 * @return モンスター種族ID (見つからなかったらnullopt)
 * @details Nestにはそのフロアの通常レベルより11高いモンスターを中心に選ぶ
 */
std::optional<MonsterRaceId> select_nest_monrace_id(PlayerType *player_ptr, MonsterEntity &align)
{
    for (auto attempts = 100; attempts > 0; attempts--) {
        const auto monrace_id = get_mon_num(player_ptr, 0, player_ptr->current_floor_ptr->dun_level + 11, PM_NONE);
        const auto &monrace = monraces_info[monrace_id];
        if (monster_has_hostile_align(player_ptr, &align, 0, 0, &monrace)) {
            continue;
        }

        if (MonsterRace(monrace_id).is_valid()) {
            return monrace_id;
        }

        return std::nullopt;
    }

    return std::nullopt;
}
}

/*!
 * @brief タイプ5の部屋…nestを生成する / Type 5 -- Monster nests
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * A monster nest is a "big" room, with an "inner" room, containing\n
 * a "collection" of monsters of a given type strewn about the room.\n
 *\n
 * The monsters are chosen from a set of 64 randomly selected monster\n
 * races, to allow the nest creation to fail instead of having "holes".\n
 *\n
 * Note the use of the "get_mon_num_prep()" function, and the special\n
 * "get_mon_num_hook()" restriction function, to prepare the "monster\n
 * allocation table" in such a way as to optimize the selection of\n
 * "appropriate" non-unique monsters for the nest.\n
 *\n
 * Note that the "get_mon_num()" function may (rarely) fail, in which\n
 * case the nest will be empty.\n
 *\n
 * Note that "monster nests" will never contain "unique" monsters.\n
 */
bool build_type5(PlayerType *player_ptr, dun_data_type *dd_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto cur_nest_type = pick_vault_type(&floor, nest_types, floor.get_dungeon_definition().nest);
    if (cur_nest_type < 0) {
        return false;
    }

    const auto n_ptr = &nest_types[cur_nest_type];
    if (n_ptr->prep_func) {
        (*n_ptr->prep_func)(player_ptr);
    }

    get_mon_num_prep(player_ptr, n_ptr->hook_func, nullptr);
    MonsterEntity align;
    align.sub_align = SUB_ALIGN_NEUTRAL;

    /* Pick some monster types */
    std::array<nest_mon_info_type, NUM_NEST_MON_TYPE> nest_mon_info_list{};
    for (auto &nest_mon_info : nest_mon_info_list) {
        const auto r_idx = select_nest_monrace_id(player_ptr, align);
        if (!r_idx) {
            return false;
        }

        const auto *r_ptr = &monraces_info[*r_idx];

        /* Note the alignment */
        if (r_ptr->kind_flags.has(MonsterKindType::EVIL)) {
            align.sub_align |= SUB_ALIGN_EVIL;
        }

        if (r_ptr->kind_flags.has(MonsterKindType::GOOD)) {
            align.sub_align |= SUB_ALIGN_GOOD;
        }

        nest_mon_info.r_idx = *r_idx;
        nest_mon_info.used = false;
    }

    /* Find and reserve some space in the dungeon.  Get center of room. */
    int xval;
    int yval;
    if (!find_space(player_ptr, dd_ptr, &yval, &xval, 11, 25)) {
        return false;
    }

    /* Large room */
    auto y1 = yval - 4;
    auto y2 = yval + 4;
    auto x1 = xval - 11;
    auto x2 = xval + 11;

    /* Place the floor area */
    for (auto y = y1 - 1; y <= y2 + 1; y++) {
        for (auto x = x1 - 1; x <= x2 + 1; x++) {
            auto &grid = floor.get_grid({ y, x });
            place_grid(player_ptr, &grid, GB_FLOOR);
            grid.info |= (CAVE_ROOM);
        }
    }

    /* Place the outer walls */
    for (auto y = y1 - 1; y <= y2 + 1; y++) {
        place_grid(player_ptr, &floor.get_grid({ y, x1 - 1 }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ y, x2 + 1 }), GB_OUTER);
    }

    for (auto x = x1 - 1; x <= x2 + 1; x++) {
        place_grid(player_ptr, &floor.get_grid({ y1 - 1, x }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ y2 + 1, x }), GB_OUTER);
    }

    /* Advance to the center room */
    y1 = y1 + 2;
    y2 = y2 - 2;
    x1 = x1 + 2;
    x2 = x2 - 2;

    /* The inner walls */
    for (auto y = y1 - 1; y <= y2 + 1; y++) {
        place_grid(player_ptr, &floor.get_grid({ y, x1 - 1 }), GB_INNER);
        place_grid(player_ptr, &floor.get_grid({ y, x2 + 1 }), GB_INNER);
    }

    for (auto x = x1 - 1; x <= x2 + 1; x++) {
        place_grid(player_ptr, &floor.get_grid({ y1 - 1, x }), GB_INNER);
        place_grid(player_ptr, &floor.get_grid({ y2 + 1, x }), GB_INNER);
    }

    for (auto y = y1; y <= y2; y++) {
        for (auto x = x1; x <= x2; x++) {
            floor.get_grid({ y, x }).add_info(CAVE_ICKY);
        }
    }

    /* Place a secret door */
    switch (randint1(4)) {
    case 1:
        place_secret_door(player_ptr, y1 - 1, xval, DOOR_DEFAULT);
        break;
    case 2:
        place_secret_door(player_ptr, y2 + 1, xval, DOOR_DEFAULT);
        break;
    case 3:
        place_secret_door(player_ptr, yval, x1 - 1, DOOR_DEFAULT);
        break;
    case 4:
        place_secret_door(player_ptr, yval, x2 + 1, DOOR_DEFAULT);
        break;
    }

    constexpr auto fmt_nest = _("モンスター部屋(nest)(%s%s)を生成します。", "Monster nest (%s%s)");
    msg_format_wizard(player_ptr, CHEAT_DUNGEON, fmt_nest, n_ptr->name.data(), pit_subtype_string(cur_nest_type, true).data());

    /* Place some monsters */
    for (auto y = yval - 2; y <= yval + 2; y++) {
        for (auto x = xval - 9; x <= xval + 9; x++) {
            auto &nest_mon_info = rand_choice(nest_mon_info_list);

            /* Place that "random" monster (no groups) */
            (void)place_specific_monster(player_ptr, 0, y, x, nest_mon_info.r_idx, 0L);
            nest_mon_info.used = true;
        }
    }

    if (cheat_room) {
        ang_sort(player_ptr, nest_mon_info_list.data(), nullptr, NUM_NEST_MON_TYPE, ang_sort_comp_nest_mon_info, ang_sort_swap_nest_mon_info);

        /* Dump the entries (prevent multi-printing) */
        for (auto i = 0; i < NUM_NEST_MON_TYPE; i++) {
            if (!nest_mon_info_list[i].used) {
                break;
            }
            for (; i < NUM_NEST_MON_TYPE - 1; i++) {
                if (nest_mon_info_list[i].r_idx != nest_mon_info_list[i + 1].r_idx) {
                    break;
                }

                if (!nest_mon_info_list[i + 1].used) {
                    break;
                }
            }

            constexpr auto fmt_nest_num = _("Nest構成モンスターNo.%d: %s", "Nest monster No.%d: %s");
            msg_format_wizard(player_ptr, CHEAT_DUNGEON, fmt_nest_num, i, monraces_info[nest_mon_info_list[i].r_idx].name.data());
        }
    }

    return true;
}

/*!
 * @brief 生成するPitの情報テーブル
 */
const std::vector<nest_pit_type> pit_types = {
    { _("オーク", "orc"), vault_aux_orc, std::nullopt, 5, 6 },
    { _("トロル", "troll"), vault_aux_troll, std::nullopt, 20, 6 },
    { _("巨人", "giant"), vault_aux_giant, std::nullopt, 50, 6 },
    { _("狂気", "lovecraftian"), vault_aux_cthulhu, std::nullopt, 80, 2 },
    { _("シンボル(善)", "symbol good"), vault_aux_symbol_g, vault_prep_symbol, 70, 1 },
    { _("シンボル(悪)", "symbol evil"), vault_aux_symbol_e, vault_prep_symbol, 70, 1 },
    { _("教会", "chapel"), vault_aux_chapel_g, std::nullopt, 65, 2 },
    { _("ドラゴン", "dragon"), vault_aux_dragon, vault_prep_dragon, 70, 6 },
    { _("デーモン", "demon"), vault_aux_demon, std::nullopt, 80, 6 },
    { _("ダークエルフ", "dark elf"), vault_aux_dark_elf, std::nullopt, 45, 4 },
};

/*!
 * @brief タイプ6の部屋…pitを生成する / Type 6 -- Monster pits
 * @details
 * A monster pit is a "big" room, with an "inner" room, containing\n
 * a "collection" of monsters of a given type organized in the room.\n
 *\n
 * The inside room in a monster pit appears as shown below, where the\n
 * actual monsters in each location depend on the type of the pit\n
 *\n
 *   XXXXXXXXXXXXXXXXXXXXX\n
 *   X0000000000000000000X\n
 *   X0112233455543322110X\n
 *   X0112233467643322110X\n
 *   X0112233455543322110X\n
 *   X0000000000000000000X\n
 *   XXXXXXXXXXXXXXXXXXXXX\n
 *\n
 * Note that the monsters in the pit are now chosen by using "get_mon_num()"\n
 * to request 16 "appropriate" monsters, sorting them by level, and using\n
 * the "even" entries in this sorted list for the contents of the pit.\n
 *\n
 * Hack -- all of the "dragons" in a "dragon" pit must be the same "color",\n
 * which is handled by requiring a specific "breath" attack for all of the\n
 * dragons.  This may include "multi-hued" breath.  Note that "wyrms" may\n
 * be present in many of the dragon pits, if they have the proper breath.\n
 *\n
 * Note the use of the "get_mon_num_prep()" function, and the special\n
 * "get_mon_num_hook()" restriction function, to prepare the "monster\n
 * allocation table" in such a way as to optimize the selection of\n
 * "appropriate" non-unique monsters for the pit.\n
 *\n
 * Note that the "get_mon_num()" function may (rarely) fail, in which case\n
 * the pit will be empty.\n
 *\n
 * Note that "monster pits" will never contain "unique" monsters.\n
 */
bool build_type6(PlayerType *player_ptr, dun_data_type *dd_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto cur_pit_type = pick_vault_type(&floor, pit_types, floor.get_dungeon_definition().pit);

    /* No type available */
    if (cur_pit_type < 0) {
        return false;
    }

    const auto *n_ptr = &pit_types[cur_pit_type];

    /* Process a preparation function if necessary */
    if (n_ptr->prep_func) {
        (*(n_ptr->prep_func))(player_ptr);
    }

    get_mon_num_prep(player_ptr, n_ptr->hook_func, nullptr);
    MonsterEntity align;
    align.sub_align = SUB_ALIGN_NEUTRAL;

    /* Pick some monster types */
    std::array<MonsterRaceId, 16> whats{};
    for (auto &what : whats) {
        auto r_idx = MonsterRace::empty_id();
        auto attempts = 100;
        MonsterRaceInfo *r_ptr = nullptr;
        while (attempts--) {
            r_idx = get_mon_num(player_ptr, 0, floor.dun_level + 11, PM_NONE);
            r_ptr = &monraces_info[r_idx];
            if (monster_has_hostile_align(player_ptr, &align, 0, 0, r_ptr)) {
                continue;
            }

            break;
        }

        if (!MonsterRace(r_idx).is_valid() || (attempts == 0)) {
            return false;
        }

        if (r_ptr->kind_flags.has(MonsterKindType::EVIL)) {
            align.sub_align |= SUB_ALIGN_EVIL;
        }

        if (r_ptr->kind_flags.has(MonsterKindType::GOOD)) {
            align.sub_align |= SUB_ALIGN_GOOD;
        }

        what = r_idx;
    }

    int yval;
    int xval;
    if (!find_space(player_ptr, dd_ptr, &yval, &xval, 11, 25)) {
        return false;
    }

    /* Large room */
    auto y1 = yval - 4;
    auto y2 = yval + 4;
    auto x1 = xval - 11;
    auto x2 = xval + 11;

    /* Place the floor area */
    Grid *g_ptr;
    for (auto y = y1 - 1; y <= y2 + 1; y++) {
        for (auto x = x1 - 1; x <= x2 + 1; x++) {
            g_ptr = &floor.grid_array[y][x];
            place_grid(player_ptr, g_ptr, GB_FLOOR);
            g_ptr->info |= (CAVE_ROOM);
        }
    }

    /* Place the outer walls */
    for (auto y = y1 - 1; y <= y2 + 1; y++) {
        g_ptr = &floor.grid_array[y][x1 - 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr = &floor.grid_array[y][x2 + 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
    }
    for (auto x = x1 - 1; x <= x2 + 1; x++) {
        g_ptr = &floor.grid_array[y1 - 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr = &floor.grid_array[y2 + 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
    }

    /* Advance to the center room */
    y1 = y1 + 2;
    y2 = y2 - 2;
    x1 = x1 + 2;
    x2 = x2 - 2;

    /* The inner walls */
    for (auto y = y1 - 1; y <= y2 + 1; y++) {
        g_ptr = &floor.grid_array[y][x1 - 1];
        place_grid(player_ptr, g_ptr, GB_INNER);
        g_ptr = &floor.grid_array[y][x2 + 1];
        place_grid(player_ptr, g_ptr, GB_INNER);
    }
    for (auto x = x1 - 1; x <= x2 + 1; x++) {
        g_ptr = &floor.grid_array[y1 - 1][x];
        place_grid(player_ptr, g_ptr, GB_INNER);
        g_ptr = &floor.grid_array[y2 + 1][x];
        place_grid(player_ptr, g_ptr, GB_INNER);
    }
    for (auto y = y1; y <= y2; y++) {
        for (auto x = x1; x <= x2; x++) {
            floor.grid_array[y][x].add_info(CAVE_ICKY);
        }
    }

    /* Place a secret door */
    switch (randint1(4)) {
    case 1:
        place_secret_door(player_ptr, y1 - 1, xval, DOOR_DEFAULT);
        break;
    case 2:
        place_secret_door(player_ptr, y2 + 1, xval, DOOR_DEFAULT);
        break;
    case 3:
        place_secret_door(player_ptr, yval, x1 - 1, DOOR_DEFAULT);
        break;
    case 4:
        place_secret_door(player_ptr, yval, x2 + 1, DOOR_DEFAULT);
        break;
    }

    /* Sort the entries */
    for (auto i = 0; i < 16 - 1; i++) {
        /* Sort the entries */
        for (auto j = 0; j < 16 - 1; j++) {
            int i1 = j;
            int i2 = j + 1;

            int p1 = monraces_info[whats[i1]].level;
            int p2 = monraces_info[whats[i2]].level;

            /* Bubble */
            if (p1 > p2) {
                std::swap(whats[i1], whats[i2]);
            }
        }
    }

    constexpr auto fmt_generate = _("モンスター部屋(pit)(%s%s)を生成します。", "Monster pit (%s%s)");
    msg_format_wizard(
        player_ptr, CHEAT_DUNGEON, fmt_generate, n_ptr->name.data(), pit_subtype_string(cur_pit_type, false).data());

    /* Select the entries */
    for (auto i = 0; i < 8; i++) {
        /* Every other entry */
        whats[i] = whats[i * 2];
        constexpr auto fmt_pit_num = _("Pit構成モンスター選択No.%d:%s", "Pit Monster Select No.%d:%s");
        msg_format_wizard(player_ptr, CHEAT_DUNGEON, fmt_pit_num, i, monraces_info[whats[i]].name.data());
    }

    /* Top and bottom rows */
    for (auto x = xval - 9; x <= xval + 9; x++) {
        place_specific_monster(player_ptr, 0, yval - 2, x, whats[0], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, yval + 2, x, whats[0], PM_NO_KAGE);
    }

    /* Middle columns */
    for (auto y = yval - 1; y <= yval + 1; y++) {
        place_specific_monster(player_ptr, 0, y, xval - 9, whats[0], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, y, xval + 9, whats[0], PM_NO_KAGE);

        place_specific_monster(player_ptr, 0, y, xval - 8, whats[1], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, y, xval + 8, whats[1], PM_NO_KAGE);

        place_specific_monster(player_ptr, 0, y, xval - 7, whats[1], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, y, xval + 7, whats[1], PM_NO_KAGE);

        place_specific_monster(player_ptr, 0, y, xval - 6, whats[2], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, y, xval + 6, whats[2], PM_NO_KAGE);

        place_specific_monster(player_ptr, 0, y, xval - 5, whats[2], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, y, xval + 5, whats[2], PM_NO_KAGE);

        place_specific_monster(player_ptr, 0, y, xval - 4, whats[3], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, y, xval + 4, whats[3], PM_NO_KAGE);

        place_specific_monster(player_ptr, 0, y, xval - 3, whats[3], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, y, xval + 3, whats[3], PM_NO_KAGE);

        place_specific_monster(player_ptr, 0, y, xval - 2, whats[4], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, y, xval + 2, whats[4], PM_NO_KAGE);
    }

    /* Above/Below the center monster */
    for (auto x = xval - 1; x <= xval + 1; x++) {
        place_specific_monster(player_ptr, 0, yval + 1, x, whats[5], PM_NO_KAGE);
        place_specific_monster(player_ptr, 0, yval - 1, x, whats[5], PM_NO_KAGE);
    }

    /* Next to the center monster */
    place_specific_monster(player_ptr, 0, yval, xval + 1, whats[6], PM_NO_KAGE);
    place_specific_monster(player_ptr, 0, yval, xval - 1, whats[6], PM_NO_KAGE);

    /* Center monster */
    place_specific_monster(player_ptr, 0, yval, xval, whats[7], PM_NO_KAGE);

    return true;
}

// clang-format off
/*!
 * @brief 開門トラップのモンスター配置テーブル
 * @details
 * 中央からの相対座標(X,Y)、モンスターの強さ
 */
const int place_table_trapped_pit[TRAPPED_PIT_MONSTER_PLACE_MAX][3] = {
    { -2, -9, 0 }, { -2, -8, 0 }, { -3, -7, 0 }, { -3, -6, 0 }, { +2, -9, 0 }, { +2, -8, 0 }, { +3, -7, 0 }, { +3, -6, 0 },
    { -2, +9, 0 }, { -2, +8, 0 }, { -3, +7, 0 }, { -3, +6, 0 }, { +2, +9, 0 }, { +2, +8, 0 }, { +3, +7, 0 }, { +3, +6, 0 },
    { -2, -7, 1 }, { -3, -5, 1 }, { -3, -4, 1 }, { -2, +7, 1 }, { -3, +5, 1 }, { -3, +4, 1 },
    { +2, -7, 1 }, { +3, -5, 1 }, { +3, -4, 1 }, { +2, +7, 1 }, { +3, +5, 1 }, { +3, +4, 1 },
    { -2, -6, 2 }, { -2, -5, 2 }, { -3, -3, 2 }, { -2, +6, 2 }, { -2, +5, 2 }, { -3, +3, 2 },
    { +2, -6, 2 }, { +2, -5, 2 }, { +3, -3, 2 }, { +2, +6, 2 }, { +2, +5, 2 }, { +3, +3, 2 },
    { -2, -4, 3 }, { -3, -2, 3 }, { -2, +4, 3 }, { -3, +2, 3 },
    { +2, -4, 3 }, { +3, -2, 3 }, { +2, +4, 3 }, { +3, +2, 3 },
    { -2, -3, 4 }, { -3, -1, 4 }, { +2, -3, 4 }, { +3, -1, 4 },
    { -2, +3, 4 }, { -3, +1, 4 }, { +2, +3, 4 }, { +3, +1, 4 },
    { -2, -2, 5 }, { -3, 0, 5 }, { -2, +2, 5 }, { +2, -2, 5 }, { +3, 0, 5 }, { +2, +2, 5 },
    { -2, -1, 6 }, { -2, +1, 6 }, { +2, -1, 6 }, { +2, +1, 6 },
    { -2, 0, 7 }, { +2, 0, 7 },
    { 0, 0, -1 } };
// clang-format on

/*!
 * @brief 開門トラップに配置するモンスターの条件フィルタ
 * @detai;
 * 穴を掘るモンスター、壁を抜けるモンスターは却下
 */
static bool vault_aux_trapped_pit(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    /* Unused */
    (void)player_ptr;

    auto *r_ptr = &monraces_info[r_idx];

    if (!vault_monster_okay(player_ptr, r_idx)) {
        return false;
    }

    /* No wall passing monster */
    if (r_ptr->feature_flags.has_any_of({ MonsterFeatureType::PASS_WALL, MonsterFeatureType::KILL_WALL })) {
        return false;
    }

    return true;
}

/*!
 * @brief タイプ13の部屋…開門トラップpitの生成 / Type 13 -- Trapped monster pits
 * @details
 * A trapped monster pit is a "big" room with a straight corridor in\n
 * which wall opening traps are placed, and with two "inner" rooms\n
 * containing a "collection" of monsters of a given type organized in\n
 * the room.\n
 *\n
 * The trapped monster pit appears as shown below, where the actual\n
 * monsters in each location depend on the type of the pit\n
 *\n
 *  XXXXXXXXXXXXXXXXXXXXXXXXX\n
 *  X                       X\n
 *  XXXXXXXXXXXXXXXXXXXXXXX X\n
 *  XXXXX001123454321100XXX X\n
 *  XXX0012234567654322100X X\n
 *  XXXXXXXXXXXXXXXXXXXXXXX X\n
 *  X           ^           X\n
 *  X XXXXXXXXXXXXXXXXXXXXXXX\n
 *  X X0012234567654322100XXX\n
 *  X XXX001123454321100XXXXX\n
 *  X XXXXXXXXXXXXXXXXXXXXXXX\n
 *  X                       X\n
 *  XXXXXXXXXXXXXXXXXXXXXXXXX\n
 *\n
 * Note that the monsters in the pit are now chosen by using "get_mon_num()"\n
 * to request 16 "appropriate" monsters, sorting them by level, and using\n
 * the "even" entries in this sorted list for the contents of the pit.\n
 *\n
 * Hack -- all of the "dragons" in a "dragon" pit must be the same "color",\n
 * which is handled by requiring a specific "breath" attack for all of the\n
 * dragons.  This may include "multi-hued" breath.  Note that "wyrms" may\n
 * be present in many of the dragon pits, if they have the proper breath.\n
 *\n
 * Note the use of the "get_mon_num_prep()" function, and the special\n
 * "get_mon_num_hook()" restriction function, to prepare the "monster\n
 * allocation table" in such a way as to optimize the selection of\n
 * "appropriate" non-unique monsters for the pit.\n
 *\n
 * Note that the "get_mon_num()" function may (rarely) fail, in which case\n
 * the pit will be empty.\n
 *\n
 * Note that "monster pits" will never contain "unique" monsters.\n
 */
bool build_type13(PlayerType *player_ptr, dun_data_type *dd_ptr)
{
    POSITION y, x, y1, x1, y2, x2, xval, yval;
    int i, j;

    MonsterRaceId what[16];

    MonsterEntity align;

    Grid *g_ptr;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    int cur_pit_type = pick_vault_type(floor_ptr, pit_types, floor_ptr->get_dungeon_definition().pit);
    const nest_pit_type *n_ptr;

    /* Only in Angband */
    if (floor_ptr->dungeon_idx != DUNGEON_ANGBAND) {
        return false;
    }

    /* No type available */
    if (cur_pit_type < 0) {
        return false;
    }

    n_ptr = &pit_types[cur_pit_type];

    /* Process a preparation function if necessary */
    if (n_ptr->prep_func) {
        (*(n_ptr->prep_func))(player_ptr);
    }
    get_mon_num_prep(player_ptr, n_ptr->hook_func, vault_aux_trapped_pit);

    align.sub_align = SUB_ALIGN_NEUTRAL;

    /* Pick some monster types */
    for (i = 0; i < 16; i++) {
        auto r_idx = MonsterRace::empty_id();
        int attempts = 100;
        MonsterRaceInfo *r_ptr = nullptr;

        while (attempts--) {
            /* Get a (hard) monster type */
            r_idx = get_mon_num(player_ptr, 0, floor_ptr->dun_level + 0, PM_NONE);
            r_ptr = &monraces_info[r_idx];

            /* Decline incorrect alignment */
            if (monster_has_hostile_align(player_ptr, &align, 0, 0, r_ptr)) {
                continue;
            }

            /* Accept this monster */
            break;
        }

        /* Notice failure */
        if (!MonsterRace(r_idx).is_valid() || !attempts) {
            return false;
        }

        /* Note the alignment */
        if (r_ptr->kind_flags.has(MonsterKindType::EVIL)) {
            align.sub_align |= SUB_ALIGN_EVIL;
        }
        if (r_ptr->kind_flags.has(MonsterKindType::GOOD)) {
            align.sub_align |= SUB_ALIGN_GOOD;
        }

        what[i] = r_idx;
    }

    /* Find and reserve some space in the dungeon.  Get center of room. */
    if (!find_space(player_ptr, dd_ptr, &yval, &xval, 13, 25)) {
        return false;
    }

    /* Large room */
    y1 = yval - 5;
    y2 = yval + 5;
    x1 = xval - 11;
    x2 = xval + 11;

    /* Fill with inner walls */
    for (y = y1 - 1; y <= y2 + 1; y++) {
        for (x = x1 - 1; x <= x2 + 1; x++) {
            g_ptr = &floor_ptr->grid_array[y][x];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr->info |= (CAVE_ROOM);
        }
    }

    /* Place the floor area 1 */
    for (x = x1 + 3; x <= x2 - 3; x++) {
        g_ptr = &floor_ptr->grid_array[yval - 2][x];
        place_grid(player_ptr, g_ptr, GB_FLOOR);
        floor_ptr->grid_array[yval - 2][x].add_info(CAVE_ICKY);

        g_ptr = &floor_ptr->grid_array[yval + 2][x];
        place_grid(player_ptr, g_ptr, GB_FLOOR);
        floor_ptr->grid_array[yval + 2][x].add_info(CAVE_ICKY);
    }

    /* Place the floor area 2 */
    for (x = x1 + 5; x <= x2 - 5; x++) {
        g_ptr = &floor_ptr->grid_array[yval - 3][x];
        place_grid(player_ptr, g_ptr, GB_FLOOR);
        floor_ptr->grid_array[yval - 3][x].add_info(CAVE_ICKY);

        g_ptr = &floor_ptr->grid_array[yval + 3][x];
        place_grid(player_ptr, g_ptr, GB_FLOOR);
        floor_ptr->grid_array[yval + 3][x].add_info(CAVE_ICKY);
    }

    /* Corridor */
    for (x = x1; x <= x2; x++) {
        g_ptr = &floor_ptr->grid_array[yval][x];
        place_grid(player_ptr, g_ptr, GB_FLOOR);
        g_ptr = &floor_ptr->grid_array[y1][x];
        place_grid(player_ptr, g_ptr, GB_FLOOR);
        g_ptr = &floor_ptr->grid_array[y2][x];
        place_grid(player_ptr, g_ptr, GB_FLOOR);
    }

    /* Place the outer walls */
    for (y = y1 - 1; y <= y2 + 1; y++) {
        g_ptr = &floor_ptr->grid_array[y][x1 - 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr = &floor_ptr->grid_array[y][x2 + 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
    }
    for (x = x1 - 1; x <= x2 + 1; x++) {
        g_ptr = &floor_ptr->grid_array[y1 - 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr = &floor_ptr->grid_array[y2 + 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
    }

    /* Random corridor */
    if (one_in_(2)) {
        for (y = y1; y <= yval; y++) {
            place_bold(player_ptr, y, x2, GB_FLOOR);
            place_bold(player_ptr, y, x1 - 1, GB_SOLID);
        }
        for (y = yval; y <= y2 + 1; y++) {
            place_bold(player_ptr, y, x1, GB_FLOOR);
            place_bold(player_ptr, y, x2 + 1, GB_SOLID);
        }
    } else {
        for (y = yval; y <= y2 + 1; y++) {
            place_bold(player_ptr, y, x1, GB_FLOOR);
            place_bold(player_ptr, y, x2 + 1, GB_SOLID);
        }
        for (y = y1; y <= yval; y++) {
            place_bold(player_ptr, y, x2, GB_FLOOR);
            place_bold(player_ptr, y, x1 - 1, GB_SOLID);
        }
    }

    /* Place the wall open trap */
    floor_ptr->grid_array[yval][xval].mimic = floor_ptr->grid_array[yval][xval].feat;
    floor_ptr->grid_array[yval][xval].feat = feat_trap_open;

    /* Sort the entries */
    for (i = 0; i < 16 - 1; i++) {
        /* Sort the entries */
        for (j = 0; j < 16 - 1; j++) {
            int i1 = j;
            int i2 = j + 1;

            int p1 = monraces_info[what[i1]].level;
            int p2 = monraces_info[what[i2]].level;

            /* Bubble */
            if (p1 > p2) {
                MonsterRaceId tmp = what[i1];
                what[i1] = what[i2];
                what[i2] = tmp;
            }
        }
    }

    constexpr auto fmt = _("%s%sの罠ピットが生成されました。", "Trapped monster pit (%s%s)");
    msg_format_wizard(player_ptr, CHEAT_DUNGEON, fmt, n_ptr->name.data(), pit_subtype_string(cur_pit_type, false).data());

    /* Select the entries */
    for (i = 0; i < 8; i++) {
        /* Every other entry */
        what[i] = what[i * 2];

        if (cheat_hear) {
            msg_print(monraces_info[what[i]].name);
        }
    }

    for (i = 0; place_table_trapped_pit[i][2] >= 0; i++) {
        y = yval + place_table_trapped_pit[i][0];
        x = xval + place_table_trapped_pit[i][1];
        place_specific_monster(player_ptr, 0, y, x, what[place_table_trapped_pit[i][2]], PM_NO_KAGE);
    }

    return true;
}
