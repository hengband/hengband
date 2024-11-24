#include "room/rooms-nest.h"
#include "game-option/cheat-options.h"
#include "game-option/cheat-types.h"
#include "grid/door.h"
#include "grid/grid.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
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
#include "util/probability-table.h"
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
const std::map<NestKind, nest_pit_type> nest_types = {
    { NestKind::CLONE, { _("クローン", "clone"), vault_aux_clone, vault_prep_clone, 5, 3 } },
    { NestKind::JELLY, { _("ゼリー", "jelly"), vault_aux_jelly, std::nullopt, 5, 6 } },
    { NestKind::SYMBOL_GOOD, { _("シンボル(善)", "symbol good"), vault_aux_symbol_g, vault_prep_symbol, 25, 2 } },
    { NestKind::SYMBOL_EVIL, { _("シンボル(悪)", "symbol evil"), vault_aux_symbol_e, vault_prep_symbol, 25, 2 } },
    { NestKind::MIMIC, { _("ミミック", "mimic"), vault_aux_mimic, std::nullopt, 30, 4 } },
    { NestKind::LOVECRAFTIAN, { _("狂気", "lovecraftian"), vault_aux_cthulhu, std::nullopt, 70, 2 } },
    { NestKind::KENNEL, { _("犬小屋", "kennel"), vault_aux_kennel, std::nullopt, 45, 4 } },
    { NestKind::ANIMAL, { _("動物園", "animal"), vault_aux_animal, std::nullopt, 35, 5 } },
    { NestKind::CHAPEL, { _("教会", "chapel"), vault_aux_chapel_g, std::nullopt, 75, 4 } },
    { NestKind::UNDEAD, { _("アンデッド", "undead"), vault_aux_undead, std::nullopt, 75, 5 } },
};

std::optional<std::array<NestMonsterInfo, NUM_NEST_MON_TYPE>> pick_nest_monraces(PlayerType *player_ptr, MonsterEntity &align)
{
    std::array<NestMonsterInfo, NUM_NEST_MON_TYPE> nest_mon_info_list{};
    for (auto &nest_mon_info : nest_mon_info_list) {
        const auto monrace_id = select_pit_nest_monrace_id(player_ptr, align, 11);
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

        nest_mon_info.monrace_id = *monrace_id;
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

void place_monsters_in_nest(PlayerType *player_ptr, const Pos2D &center, std::array<NestMonsterInfo, NUM_NEST_MON_TYPE> &nest_mon_info_list)
{
    Rect2D(center, Vector2D(2, 9)).each_area([player_ptr, &nest_mon_info_list](const Pos2D &pos) {
        auto &nest_mon_info = rand_choice(nest_mon_info_list);
        (void)place_specific_monster(player_ptr, pos.y, pos.x, nest_mon_info.monrace_id, 0L);
        nest_mon_info.used = true;
    });
}

void output_debug_nest(PlayerType *player_ptr, std::array<NestMonsterInfo, NUM_NEST_MON_TYPE> &nest_mon_info_list)
{
    if (!cheat_room) {
        return;
    }

    std::stable_sort(nest_mon_info_list.begin(), nest_mon_info_list.end(),
        [](const auto &x, const auto &y) { return x.order_nest(y); });
    for (auto i = 0; i < NUM_NEST_MON_TYPE; i++) {
        const auto &nest_mon_info = nest_mon_info_list[i];
        const auto &next_nest_mon_info = nest_mon_info_list[i + 1];
        if (!nest_mon_info.used) {
            return;
        }

        for (; i < NUM_NEST_MON_TYPE - 1; i++) {
            if (nest_mon_info.monrace_id != next_nest_mon_info.monrace_id) {
                break;
            }

            if (!next_nest_mon_info.used) {
                break;
            }
        }

        if (i == NUM_NEST_MON_TYPE) {
            return;
        }

        constexpr auto fmt_nest_num = _("Nest構成モンスターNo.%d: %s", "Nest monster No.%d: %s");
        msg_format_wizard(player_ptr, CHEAT_DUNGEON, fmt_nest_num, i, nest_mon_info.get_monrace().name.data());
    }
}
}

bool NestMonsterInfo::order_nest(const NestMonsterInfo &other) const
{
    if (this->used && !other.used) {
        return true;
    }

    if (!this->used && other.used) {
        return false;
    }

    const auto &monrace1 = this->get_monrace();
    const auto &monrace2 = other.get_monrace();
    const auto order_level = monrace2.order_level(monrace1);
    if (order_level) {
        return *order_level;
    }

    if (monrace1.mexp < monrace2.mexp) {
        return true;
    }

    if (monrace1.mexp > monrace2.mexp) {
        return false;
    }

    return this->monrace_id < other.monrace_id;
}

const MonraceDefinition &NestMonsterInfo::get_monrace() const
{
    return MonraceList::get_instance().get_monrace(this->monrace_id);
}

/*!
 * @brief タイプ5の部屋…nestを生成する / Type 5 -- Monster nests
 * @param player_ptr プレイヤーへの参照ポインタ
 */
bool build_type5(PlayerType *player_ptr, DungeonData *dd_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto nest_type = pick_nest_type(floor, nest_types);
    if (!nest_type) {
        return false;
    }

    const auto &nest = nest_types.at(*nest_type);
    if (nest.prep_func) {
        (*nest.prep_func)(player_ptr);
    }

    get_mon_num_prep(player_ptr, nest.hook_func, nullptr);
    MonsterEntity align;
    align.sub_align = SUB_ALIGN_NEUTRAL;

    auto nest_mon_info_list = pick_nest_monraces(player_ptr, align);
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
    msg_format_wizard(player_ptr, CHEAT_DUNGEON, fmt_nest, nest.name.data(), nest_subtype_string(*nest_type).data());
    place_monsters_in_nest(player_ptr, center, *nest_mon_info_list);
    output_debug_nest(player_ptr, *nest_mon_info_list);
    return true;
}
