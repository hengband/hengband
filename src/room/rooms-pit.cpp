#include "room/rooms-pit.h"
#include "game-option/cheat-options.h"
#include "game-option/cheat-types.h"
#include "grid/door.h"
#include "grid/grid.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-util.h"
#include "room/pit-nest-util.h"
#include "room/space-finder.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"
#include <algorithm>

namespace {
constexpr auto NUM_PIT_MONRACES = 16; //!< pit内に存在する最大モンスター種族数.
constexpr Pos2DVec PIT_SIZE(4, 11);

/*!
 * @brief 生成するPitの情報テーブル
 */
const std::map<PitKind, nest_pit_type> pit_types = {
    { PitKind::ORC, { _("オーク", "orc"), MonraceHook::ORC, PitNestHook::NONE, 5, 6 } },
    { PitKind::TROLL, { _("トロル", "troll"), MonraceHook::TROLL, PitNestHook::NONE, 20, 6 } },
    { PitKind::GIANT, { _("巨人", "giant"), MonraceHook::GIANT, PitNestHook::NONE, 50, 6 } },
    { PitKind::HORROR, { _("狂気", "lovecraftian"), MonraceHook::HORROR, PitNestHook::NONE, 80, 2 } },
    { PitKind::SYMBOL_GOOD, { _("シンボル(善)", "symbol good"), MonraceHook::GOOD, PitNestHook::SYMBOL, 70, 1 } },
    { PitKind::SYMBOL_EVIL, { _("シンボル(悪)", "symbol evil"), MonraceHook::EVIL, PitNestHook::SYMBOL, 70, 1 } },
    { PitKind::CHAPEL, { _("教会", "chapel"), MonraceHook::CHAPEL, PitNestHook::NONE, 65, 2 } },
    { PitKind::DRAGON, { _("ドラゴン", "dragon"), MonraceHook::DRAGON, PitNestHook::DRAGON, 70, 6 } },
    { PitKind::DEMON, { _("デーモン", "demon"), MonraceHook::DEMON, PitNestHook::NONE, 80, 6 } },
    { PitKind::DARK_ELF, { _("ダークエルフ", "dark elf"), MonraceHook::DARK_ELF, PitNestHook::NONE, 45, 4 } },
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

std::optional<std::array<MonraceId, NUM_PIT_MONRACES>> pick_pit_monraces(PlayerType *player_ptr, MonsterEntity &align, int boost = 0)
{
    std::array<MonraceId, NUM_PIT_MONRACES> whats{};
    const auto &monraces = MonraceList::get_instance();
    for (auto &what : whats) {
        const auto monrace_id = select_pit_nest_monrace_id(player_ptr, align, boost);
        if (!monrace_id) {
            return std::nullopt;
        }

        const auto &monrace = monraces.get_monrace(*monrace_id);
        if (monrace.kind_flags.has(MonsterKindType::EVIL)) {
            align.sub_align |= SUB_ALIGN_EVIL;
        }

        if (monrace.kind_flags.has(MonsterKindType::GOOD)) {
            align.sub_align |= SUB_ALIGN_GOOD;
        }

        what = *monrace_id;
    }

    return whats;
}

void place_pit_outer(PlayerType *player_ptr, const Pos2D &center)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const Rect2D rectangle(center, PIT_SIZE);
    for (auto y = rectangle.top_left.y - 1; y <= rectangle.bottom_right.y + 1; y++) {
        for (auto x = rectangle.top_left.x - 1; x <= rectangle.bottom_right.x + 1; x++) {
            auto &grid = floor.get_grid({ y, x });
            place_grid(player_ptr, grid, GB_FLOOR);
            grid.add_info(CAVE_ROOM);
        }
    }

    for (auto y = rectangle.top_left.y - 1; y <= rectangle.bottom_right.y + 1; y++) {
        place_grid(player_ptr, floor.get_grid({ y, rectangle.top_left.x - 1 }), GB_OUTER);
        place_grid(player_ptr, floor.get_grid({ y, rectangle.bottom_right.x + 1 }), GB_OUTER);
    }

    for (auto x = rectangle.top_left.x - 1; x <= rectangle.bottom_right.x + 1; x++) {
        place_grid(player_ptr, floor.get_grid({ rectangle.top_left.y - 1, x }), GB_OUTER);
        place_grid(player_ptr, floor.get_grid({ rectangle.bottom_right.y + 1, x }), GB_OUTER);
    }
}

void place_pit_inner(PlayerType *player_ptr, const Pos2D &center)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto rectangle = Rect2D(center, PIT_SIZE).resized(-2);
    for (auto y = rectangle.top_left.y - 1; y <= rectangle.bottom_right.y + 1; y++) {
        place_grid(player_ptr, floor.get_grid({ y, rectangle.top_left.x - 1 }), GB_INNER);
        place_grid(player_ptr, floor.get_grid({ y, rectangle.bottom_right.x + 1 }), GB_INNER);
    }

    for (auto x = rectangle.top_left.x - 1; x <= rectangle.bottom_right.x + 1; x++) {
        place_grid(player_ptr, floor.get_grid({ rectangle.top_left.y - 1, x }), GB_INNER);
        place_grid(player_ptr, floor.get_grid({ rectangle.bottom_right.y + 1, x }), GB_INNER);
    }

    for (auto y = rectangle.top_left.y; y <= rectangle.bottom_right.y; y++) {
        for (auto x = rectangle.top_left.x; x <= rectangle.bottom_right.x; x++) {
            floor.get_grid({ y, x }).add_info(CAVE_ICKY);
        }
    }

    switch (randint1(4)) {
    case 1:
        place_secret_door(player_ptr, { rectangle.top_left.y - 1, center.x });
        break;
    case 2:
        place_secret_door(player_ptr, { rectangle.bottom_right.y + 1, center.x });
        break;
    case 3:
        place_secret_door(player_ptr, { center.y, rectangle.top_left.x - 1 });
        break;
    case 4:
        place_secret_door(player_ptr, { center.y, rectangle.bottom_right.x + 1 });
        break;
    }
}
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
bool build_type6(PlayerType *player_ptr, DungeonData *dd_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto pit_type = pick_pit_type(floor, pit_types);
    if (!pit_type) {
        return false;
    }

    const auto &pit = pit_types.at(*pit_type);
    pit.prepare_filter(player_ptr);
    get_mon_num_prep_enum(player_ptr, pit.monrace_hook);
    MonsterEntity align;
    align.sub_align = SUB_ALIGN_NEUTRAL;

    auto whats = pick_pit_monraces(player_ptr, align, 11);
    if (!whats) {
        return false;
    }

    const auto center = find_space(player_ptr, dd_ptr, 11, 25);
    if (!center) {
        return false;
    }

    place_pit_outer(player_ptr, *center);
    place_pit_inner(player_ptr, *center);
    const auto &monraces = MonraceList::get_instance();
    std::stable_sort(whats->begin(), whats->end(), [&monraces](const auto monrace_id1, const auto monrace_id2) {
        return monraces.order_level(monrace_id2, monrace_id1);
    });
    constexpr auto fmt_generate = _("モンスター部屋(pit)(%s%s)を生成します。", "Monster pit (%s%s)");
    const auto pit_subtype = PitNestFilter::get_instance().pit_subtype(*pit_type);
    msg_format_wizard(player_ptr, CHEAT_DUNGEON, fmt_generate, pit.name.data(), pit_subtype.data());

    for (auto i = 0; i < NUM_PIT_MONRACES / 2; i++) {
        (*whats)[i] = (*whats)[i * 2];
        constexpr auto fmt_pit_num = _("Pit構成モンスター選択No.%d:%s", "Pit Monster Select No.%d:%s");
        const auto &monrace = monraces.get_monrace((*whats)[i]);
        msg_format_wizard(player_ptr, CHEAT_DUNGEON, fmt_pit_num, i, monrace.name.data());
    }

    /* Top and bottom rows */
    for (auto x = center->x - 9; x <= center->x + 9; x++) {
        place_specific_monster(player_ptr, center->y - 2, x, (*whats)[0], PM_NO_KAGE);
        place_specific_monster(player_ptr, center->y + 2, x, (*whats)[0], PM_NO_KAGE);
    }

    /* Middle columns */
    for (auto y = center->y - 1; y <= center->y + 1; y++) {
        place_specific_monster(player_ptr, y, center->x - 9, (*whats)[0], PM_NO_KAGE);
        place_specific_monster(player_ptr, y, center->x + 9, (*whats)[0], PM_NO_KAGE);

        place_specific_monster(player_ptr, y, center->x - 8, (*whats)[1], PM_NO_KAGE);
        place_specific_monster(player_ptr, y, center->x + 8, (*whats)[1], PM_NO_KAGE);

        place_specific_monster(player_ptr, y, center->x - 7, (*whats)[1], PM_NO_KAGE);
        place_specific_monster(player_ptr, y, center->x + 7, (*whats)[1], PM_NO_KAGE);

        place_specific_monster(player_ptr, y, center->x - 6, (*whats)[2], PM_NO_KAGE);
        place_specific_monster(player_ptr, y, center->x + 6, (*whats)[2], PM_NO_KAGE);

        place_specific_monster(player_ptr, y, center->x - 5, (*whats)[2], PM_NO_KAGE);
        place_specific_monster(player_ptr, y, center->x + 5, (*whats)[2], PM_NO_KAGE);

        place_specific_monster(player_ptr, y, center->x - 4, (*whats)[3], PM_NO_KAGE);
        place_specific_monster(player_ptr, y, center->x + 4, (*whats)[3], PM_NO_KAGE);

        place_specific_monster(player_ptr, y, center->x - 3, (*whats)[3], PM_NO_KAGE);
        place_specific_monster(player_ptr, y, center->x + 3, (*whats)[3], PM_NO_KAGE);

        place_specific_monster(player_ptr, y, center->x - 2, (*whats)[4], PM_NO_KAGE);
        place_specific_monster(player_ptr, y, center->x + 2, (*whats)[4], PM_NO_KAGE);
    }

    /* Above/Below the center monster */
    for (auto x = center->x - 1; x <= center->x + 1; x++) {
        place_specific_monster(player_ptr, center->y + 1, x, (*whats)[5], PM_NO_KAGE);
        place_specific_monster(player_ptr, center->y - 1, x, (*whats)[5], PM_NO_KAGE);
    }

    /* Next to the center monster */
    place_specific_monster(player_ptr, center->y, center->x + 1, (*whats)[6], PM_NO_KAGE);
    place_specific_monster(player_ptr, center->y, center->x - 1, (*whats)[6], PM_NO_KAGE);

    /* Center monster */
    place_specific_monster(player_ptr, center->y, center->x, (*whats)[7], PM_NO_KAGE);

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
bool build_type13(PlayerType *player_ptr, DungeonData *dd_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto pit_type = pick_pit_type(floor, pit_types);

    /* Only in Angband */
    if (floor.dungeon_id != DungeonId::ANGBAND) {
        return false;
    }

    /* No type available */
    if (!pit_type) {
        return false;
    }

    const auto &pit = pit_types.at(*pit_type);
    pit.prepare_filter(player_ptr);
    get_mon_num_prep_enum(player_ptr, pit.monrace_hook, MonraceHookTerrain::TRAPPED_PIT);
    MonsterEntity align;
    align.sub_align = SUB_ALIGN_NEUTRAL;
    auto whats = pick_pit_monraces(player_ptr, align);
    if (!whats) {
        return false;
    }

    const auto center = find_space(player_ptr, dd_ptr, 13, 25);
    if (!center) {
        return false;
    }

    constexpr Pos2DVec vec_rectangle(5, 11);
    const Rect2D rectangle(*center, vec_rectangle);

    /* Fill with inner walls */
    for (auto y = rectangle.top_left.y - 1; y <= rectangle.bottom_right.y + 1; y++) {
        for (auto x = rectangle.top_left.x - 1; x <= rectangle.bottom_right.x + 1; x++) {
            auto &grid = floor.get_grid({ y, x });
            place_grid(player_ptr, grid, GB_INNER);
            grid.add_info(CAVE_ROOM);
        }
    }

    /* Place the floor area 1 */
    for (auto x = rectangle.top_left.x + 3; x <= rectangle.bottom_right.x - 3; x++) {
        auto &grid_top = floor.get_grid({ center->y - 2, x });
        place_grid(player_ptr, grid_top, GB_FLOOR);
        grid_top.add_info(CAVE_ICKY);

        auto &grid_bottom = floor.get_grid({ center->y + 2, x });
        place_grid(player_ptr, grid_bottom, GB_FLOOR);
        grid_bottom.add_info(CAVE_ICKY);
    }

    /* Place the floor area 2 */
    for (auto x = rectangle.top_left.x + 5; x <= rectangle.bottom_right.x - 5; x++) {
        auto &grid_left = floor.get_grid({ center->y - 3, x });
        place_grid(player_ptr, grid_left, GB_FLOOR);
        grid_left.add_info(CAVE_ICKY);

        auto &grid_right = floor.get_grid({ center->y + 3, x });
        place_grid(player_ptr, grid_right, GB_FLOOR);
        grid_right.add_info(CAVE_ICKY);
    }

    /* Corridor */
    for (auto x = rectangle.top_left.x; x <= rectangle.bottom_right.x; x++) {
        place_grid(player_ptr, floor.get_grid({ center->y, x }), GB_FLOOR);
        place_grid(player_ptr, floor.get_grid({ rectangle.top_left.y, x }), GB_FLOOR);
        place_grid(player_ptr, floor.get_grid({ rectangle.bottom_right.y, x }), GB_FLOOR);
    }

    /* Place the outer walls */
    for (auto y = rectangle.top_left.y - 1; y <= rectangle.bottom_right.y + 1; y++) {
        place_grid(player_ptr, floor.get_grid({ y, rectangle.top_left.x - 1 }), GB_OUTER);
        place_grid(player_ptr, floor.get_grid({ y, rectangle.bottom_right.x + 1 }), GB_OUTER);
    }

    for (auto x = rectangle.top_left.x - 1; x <= rectangle.bottom_right.x + 1; x++) {
        place_grid(player_ptr, floor.get_grid({ rectangle.top_left.y - 1, x }), GB_OUTER);
        place_grid(player_ptr, floor.get_grid({ rectangle.bottom_right.y + 1, x }), GB_OUTER);
    }

    /* Random corridor */
    if (one_in_(2)) {
        for (auto y = rectangle.top_left.y; y <= center->y; y++) {
            place_bold(player_ptr, y, rectangle.bottom_right.x, GB_FLOOR);
            place_bold(player_ptr, y, rectangle.top_left.x - 1, GB_SOLID);
        }
        for (auto y = center->y; y <= rectangle.bottom_right.y + 1; y++) {
            place_bold(player_ptr, y, rectangle.top_left.x, GB_FLOOR);
            place_bold(player_ptr, y, rectangle.bottom_right.x + 1, GB_SOLID);
        }
    } else {
        for (auto y = center->y; y <= rectangle.bottom_right.y + 1; y++) {
            place_bold(player_ptr, y, rectangle.top_left.x, GB_FLOOR);
            place_bold(player_ptr, y, rectangle.bottom_right.x + 1, GB_SOLID);
        }
        for (auto y = rectangle.top_left.y; y <= center->y; y++) {
            place_bold(player_ptr, y, rectangle.bottom_right.x, GB_FLOOR);
            place_bold(player_ptr, y, rectangle.top_left.x - 1, GB_SOLID);
        }
    }

    /* Place the wall open trap */
    auto &grid = floor.get_grid(*center);
    grid.mimic = grid.feat;
    grid.set_terrain_id(TerrainTag::TRAP_OPEN);
    const auto &monraces = MonraceList::get_instance();
    std::stable_sort(whats->begin(), whats->end(), [&monraces](const auto monrace_id1, const auto monrace_id2) {
        return monraces.order_level(monrace_id2, monrace_id1);
    });
    constexpr auto fmt = _("%s%sの罠ピットが生成されました。", "Trapped monster pit (%s%s)");
    const auto pit_subtype = PitNestFilter::get_instance().pit_subtype(*pit_type);
    msg_format_wizard(player_ptr, CHEAT_DUNGEON, fmt, pit.name.data(), pit_subtype.data());

    for (auto i = 0; i < NUM_PIT_MONRACES / 2; i++) {
        (*whats)[i] = (*whats)[i * 2];
        if (cheat_hear) {
            msg_print(monraces.get_monrace((*whats)[i]).name);
        }
    }

    const Pos2DVec vec(center->y, center->x);
    for (const auto &trapped_monster : place_table_trapped_pit) {
        const auto trapped_pos = trapped_monster.pos + vec;
        place_specific_monster(player_ptr, trapped_pos.y, trapped_pos.x, (*whats)[trapped_monster.strength], PM_NO_KAGE);
    }

    return true;
}
