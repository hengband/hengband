#include "room/rooms-pit-nest.h"
#include "game-option/cheat-options.h"
#include "game-option/cheat-types.h"
#include "grid/door.h"
#include "grid/grid.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "room/door-definition.h"
#include "room/space-finder.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "util/probability-table.h"
#include "util/sort.h"
#include "wizard/wizard-messages.h"
#include <array>
#include <optional>
#include <utility>
#include <vector>

namespace {
constexpr auto NUM_NEST_MON_TYPE = 64; //! nestの種別数.

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

std::optional<std::array<nest_mon_info_type, NUM_NEST_MON_TYPE>> pick_nest_monster(PlayerType *player_ptr, MonsterEntity &align)
{
    std::array<nest_mon_info_type, NUM_NEST_MON_TYPE> nest_mon_info_list{};
    for (auto &nest_mon_info : nest_mon_info_list) {
        const auto monrace_id = select_nest_monrace_id(player_ptr, align);
        if (!monrace_id) {
            return std::nullopt;
        }

        const auto &monrace = monraces_info[*monrace_id];
        if (monrace.kind_flags.has(MonsterKindType::EVIL)) {
            align.sub_align |= SUB_ALIGN_EVIL;
        }

        if (monrace.kind_flags.has(MonsterKindType::GOOD)) {
            align.sub_align |= SUB_ALIGN_GOOD;
        }

        nest_mon_info.r_idx = *monrace_id;
    }

    return nest_mon_info_list;
}

Rect2D generate_large_room(PlayerType *player_ptr, const Pos2D &center)
{
    auto &floor = *player_ptr->current_floor_ptr;
    constexpr Vector2D vec(4, 11);
    const Rect2D rectangle(center, vec);
    rectangle.resized(1).each_area([player_ptr, &floor](const Pos2D &pos) {
        auto &grid = floor.get_grid(pos);
        place_grid(player_ptr, &grid, GB_FLOOR);
        grid.add_info(CAVE_ROOM);
    });

    rectangle.resized(1).each_edge([player_ptr, &floor](const Pos2D &pos) {
        place_grid(player_ptr, &floor.get_grid(pos), GB_OUTER);
    });

    return rectangle;
}

void generate_inner_room(PlayerType *player_ptr, const Pos2D &center, Rect2D &rectangle)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto inner_rectangle = rectangle.resized(-2);

    inner_rectangle.resized(1).each_edge([player_ptr, &floor](const Pos2D &pos) {
        place_grid(player_ptr, &floor.get_grid(pos), GB_INNER);
    });

    inner_rectangle.each_area([&floor](const Pos2D &pos) {
        floor.get_grid(pos).add_info(CAVE_ICKY);
    });

    /* Place a secret door */
    switch (randint1(4)) {
    case 1:
        place_secret_door(player_ptr, inner_rectangle.top_left.y - 1, center.x, DOOR_DEFAULT);
        break;
    case 2:
        place_secret_door(player_ptr, inner_rectangle.bottom_right.y + 1, center.x, DOOR_DEFAULT);
        break;
    case 3:
        place_secret_door(player_ptr, center.y, inner_rectangle.top_left.x - 1, DOOR_DEFAULT);
        break;
    case 4:
        place_secret_door(player_ptr, center.y, inner_rectangle.bottom_right.x + 1, DOOR_DEFAULT);
        break;
    }
}

void place_monsters_in_nest(PlayerType *player_ptr, const Pos2D &center, std::array<nest_mon_info_type, NUM_NEST_MON_TYPE> &nest_mon_info_list)
{
    Rect2D(center, Vector2D(2, 9)).each_area([player_ptr, &nest_mon_info_list](const Pos2D &pos) {
        auto &nest_mon_info = rand_choice(nest_mon_info_list);
        (void)place_specific_monster(player_ptr, 0, pos.y, pos.x, nest_mon_info.r_idx, 0L);
        nest_mon_info.used = true;
    });
}

void output_debug_nest(PlayerType *player_ptr, std::array<nest_mon_info_type, NUM_NEST_MON_TYPE> &nest_mon_info_list)
{
    if (!cheat_room) {
        return;
    }

    ang_sort(player_ptr, nest_mon_info_list.data(), nullptr, NUM_NEST_MON_TYPE, ang_sort_comp_nest_mon_info, ang_sort_swap_nest_mon_info);
    for (auto i = 0; i < NUM_NEST_MON_TYPE; i++) {
        if (!nest_mon_info_list[i].used) {
            return;
        }

        for (; i < NUM_NEST_MON_TYPE - 1; i++) {
            if (nest_mon_info_list[i].r_idx != nest_mon_info_list[i + 1].r_idx) {
                break;
            }

            if (!nest_mon_info_list[i + 1].used) {
                break;
            }
        }

        if (i == NUM_NEST_MON_TYPE) {
            return;
        }

        constexpr auto fmt_nest_num = _("Nest構成モンスターNo.%d: %s", "Nest monster No.%d: %s");
        msg_format_wizard(player_ptr, CHEAT_DUNGEON, fmt_nest_num, i, monraces_info[nest_mon_info_list[i].r_idx].name.data());
    }
}
}

/*!
 * @brief ダンジョン毎に指定されたピット配列を基準にランダムなpit/nestタイプを決める
 * @param l_ptr 選択されたpit/nest情報を返す参照ポインタ
 * @param allow_flag_mask 生成が許されるpit/nestのビット配列
 * @return 選択されたpit/nestのID、選択失敗した場合-1を返す。
 */
int pick_vault_type(const std::vector<nest_pit_type> &np_types, uint16_t allow_flag_mask, int dun_level)
{
    ProbabilityTable<int> table;
    for (size_t i = 0; i < np_types.size(); i++) {
        const nest_pit_type *n_ptr = &np_types.at(i);

        if (n_ptr->level > dun_level) {
            continue;
        }

        if (!(allow_flag_mask & (1UL << i))) {
            continue;
        }

        table.entry_item(i, n_ptr->chance * MAX_DEPTH / (std::min(dun_level, MAX_DEPTH - 1) - n_ptr->level + 5));
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
    const auto cur_nest_type = pick_vault_type(nest_types, floor.get_dungeon_definition().nest, floor.dun_level);
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

    auto nest_mon_info_list = pick_nest_monster(player_ptr, align);
    if (!nest_mon_info_list) {
        return false;
    }

    /* Find and reserve some space in the dungeon.  Get center of room. */
    int xval;
    int yval;
    if (!find_space(player_ptr, dd_ptr, &yval, &xval, 11, 25)) {
        return false;
    }

    const Pos2D center(yval, xval);
    auto rectangle = generate_large_room(player_ptr, center);
    generate_inner_room(player_ptr, center, rectangle);

    constexpr auto fmt_nest = _("モンスター部屋(nest)(%s%s)を生成します。", "Monster nest (%s%s)");
    msg_format_wizard(player_ptr, CHEAT_DUNGEON, fmt_nest, n_ptr->name.data(), pit_subtype_string(cur_nest_type, true).data());
    place_monsters_in_nest(player_ptr, center, *nest_mon_info_list);
    output_debug_nest(player_ptr, *nest_mon_info_list);
    return true;
}
