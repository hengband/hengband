#include "room/rooms-pit.h"
#include "game-option/cheat-options.h"
#include "game-option/cheat-types.h"
#include "grid/door.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "room/door-definition.h"
#include "room/pit-nest-util.h"
#include "room/space-finder.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"

namespace {
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

class TrappedMonster {
public:
    TrappedMonster(const Pos2D &pos, int strength)
        : pos(pos)
        , strength(strength)
    {
    }

    Pos2D pos;
    int strength;
};

// clang-format off
/*!
 * @brief 開門トラップのモンスター配置テーブル
 * @details
 * 中央からの相対座標(X,Y)、モンスターの強さ
 */
const std::vector<TrappedMonster> place_table_trapped_pit = {
    { { -2, -9 }, 0 }, { { -2, -8 }, 0 }, { { -3, -7 }, 0 }, { { -3, -6 }, 0 }, { { +2, -9 }, 0 }, { { +2, -8 }, 0 }, { { +3, -7 }, 0 }, { { +3, -6 }, 0 },
    { { -2, +9 }, 0 }, { { -2, +8 }, 0 }, { { -3, +7 }, 0 }, { { -3, +6 }, 0 }, { { +2, +9 }, 0 }, { { +2, +8 }, 0 }, { { +3, +7 }, 0 }, { { +3, +6 }, 0 },
    { { -2, -7 }, 1 }, { { -3, -5 }, 1 }, { { -3, -4 }, 1 }, { { -2, +7 }, 1 }, { { -3, +5 }, 1 }, { { -3, +4 }, 1 },
    { { +2, -7 }, 1 }, { { +3, -5 }, 1 }, { { +3, -4 }, 1 }, { { +2, +7 }, 1 }, { { +3, +5 }, 1 }, { { +3, +4 }, 1 },
    { { -2, -6 }, 2 }, { { -2, -5 }, 2 }, { { -3, -3 }, 2 }, { { -2, +6 }, 2 }, { { -2, +5 }, 2 }, { { -3, +3 }, 2 },
    { { +2, -6 }, 2 }, { { +2, -5 }, 2 }, { { +3, -3 }, 2 }, { { +2, +6 }, 2 }, { { +2, +5 }, 2 }, { { +3, +3 }, 2 },
    { { -2, -4 }, 3 }, { { -3, -2 }, 3 }, { { -2, +4 }, 3 }, { { -3, +2 }, 3 },
    { { +2, -4 }, 3 }, { { +3, -2 }, 3 }, { { +2, +4 }, 3 }, { { +3, +2 }, 3 },
    { { -2, -3 }, 4 }, { { -3, -1 }, 4 }, { { +2, -3 }, 4 }, { { +3, -1 }, 4 },
    { { -2, +3 }, 4 }, { { -3, +1 }, 4 }, { { +2, +3 }, 4 }, { { +3, +1 }, 4 },
    { { -2, -2 }, 5 }, { { -3, 0 }, 5 }, { { -2, +2 }, 5 }, { { +2, -2 }, 5 }, { { +3, 0 }, 5 }, { { +2, +2 }, 5 },
    { { -2, -1 }, 6 }, { { -2, +1 }, 6 }, { { +2, -1 }, 6 }, { { +2, +1 }, 6 },
    { { -2, 0 }, 7 }, { { +2, 0 }, 7 },
};
// clang-format on
}

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
    const auto cur_pit_type = pick_vault_type(pit_types, floor.get_dungeon_definition().pit, floor.dun_level);

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
    int cur_pit_type = pick_vault_type(pit_types, floor_ptr->get_dungeon_definition().pit, floor_ptr->dun_level);
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

    const Pos2DVec vec(yval, xval);
    for (const auto &trapped_monster : place_table_trapped_pit) {
        const auto pos = trapped_monster.pos + vec;
        place_specific_monster(player_ptr, 0, pos.y, pos.x, what[trapped_monster.strength], PM_NO_KAGE);
    }

    return true;
}
