#include "monster-race/monster-race-hook.h"
#include "dungeon/quest.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/race-ability-mask.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-misc-flags.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "player/player-status.h"
#include "room/pit-nest-util.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "system/services/dungeon-monrace-service.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include <set>

PitNestFilter PitNestFilter::instance{};

PitNestFilter &PitNestFilter::get_instance()
{
    return instance;
}

MonraceId PitNestFilter::get_monrace_id() const
{
    return this->monrace_id;
}

char PitNestFilter::get_monrace_symbol() const
{
    return this->monrace_symbol;
}

const EnumClassFlagGroup<MonsterAbilityType> &PitNestFilter::get_dragon_breaths() const
{
    return this->dragon_breaths;
}

/*!
 * @brief デバッグ時に生成されたpitの型を出力する処理
 * @param type pitの型ID
 * @return デバッグ表示文字列
 */
std::string PitNestFilter::pit_subtype(PitKind type) const
{
    switch (type) {
    case PitKind::SYMBOL_GOOD:
    case PitKind::SYMBOL_EVIL:
        return std::string("(").append(1, this->monrace_symbol).append(1, ')');
    case PitKind::DRAGON:
        if (this->dragon_breaths.has_all_of({ MonsterAbilityType::BR_ACID, MonsterAbilityType::BR_ELEC, MonsterAbilityType::BR_FIRE, MonsterAbilityType::BR_COLD, MonsterAbilityType::BR_POIS })) {
            return _("(万色)", "(multi-hued)");
        }

        if (this->dragon_breaths.has(MonsterAbilityType::BR_ACID)) {
            return _("(酸)", "(acid)");
        }

        if (this->dragon_breaths.has(MonsterAbilityType::BR_ELEC)) {
            return _("(稲妻)", "(lightning)");
        }

        if (this->dragon_breaths.has(MonsterAbilityType::BR_FIRE)) {
            return _("(火炎)", "(fire)");
        }

        if (this->dragon_breaths.has(MonsterAbilityType::BR_COLD)) {
            return _("(冷気)", "(frost)");
        }

        if (this->dragon_breaths.has(MonsterAbilityType::BR_POIS)) {
            return _("(毒)", "(poison)");
        }

        return _("(未定義)", "(undefined)"); // @todo 本来は例外を飛ばすべき.
    default:
        return "";
    }
}

/*!
 * @brief デバッグ時に生成されたnestの型を出力する処理
 * @param type nestの型ID
 * @return デバッグ表示文字列
 */
std::string PitNestFilter::nest_subtype(NestKind type) const
{
    switch (type) {
    case NestKind::CLONE: {
        const auto &monrace = MonraceList::get_instance().get_monrace(this->monrace_id);
        std::stringstream ss;
        ss << '(' << monrace.name << ')';
        return ss.str();
    }
    case NestKind::SYMBOL_GOOD:
    case NestKind::SYMBOL_EVIL:
        return std::string("(").append(1, this->monrace_symbol).append(1, ')');
    default:
        return "";
    }
}

void PitNestFilter::set_monrace_id(MonraceId id)
{
    this->monrace_id = id;
}

void PitNestFilter::set_monrace_symbol(char symbol)
{
    this->monrace_symbol = symbol;
}

/*!
 * @brief pit/nestの基準となるドラゴンの種類を決める
 */
void PitNestFilter::set_dragon_breaths()
{
    this->dragon_breaths.clear();
    constexpr static auto element_breaths = {
        MonsterAbilityType::BR_ACID, /* Black */
        MonsterAbilityType::BR_ELEC, /* Blue */
        MonsterAbilityType::BR_FIRE, /* Red */
        MonsterAbilityType::BR_COLD, /* White */
        MonsterAbilityType::BR_POIS, /* Green */
    };

    if (one_in_(6)) {
        this->dragon_breaths.set(element_breaths);
        return;
    }

    this->dragon_breaths.set(rand_choice(element_breaths));
}

/*!
 * @brief モンスターが単一クローンnestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (clone)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_clone(PlayerType *player_ptr, MonraceId r_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    return r_idx == PitNestFilter::get_instance().get_monrace_id();
}

/*!
 * @brief モンスターが邪悪属性シンボルクローンnestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (symbol clone)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_symbol_e(PlayerType *player_ptr, MonraceId r_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    if (monrace.behavior_flags.has(MonsterBehaviorType::KILL_BODY) && monrace.behavior_flags.has_not(MonsterBehaviorType::NEVER_BLOW)) {
        return false;
    }

    if (monrace.kind_flags.has(MonsterKindType::GOOD)) {
        return false;
    }

    if (monrace.symbol_definition.character != PitNestFilter::get_instance().get_monrace_symbol()) {
        return false;
    }

    return true;
}

/*!
 * @brief モンスターが善良属性シンボルクローンnestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (symbol clone)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_symbol_g(PlayerType *player_ptr, MonraceId r_idx)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(r_idx);
    const auto &floor = *player_ptr->current_floor_ptr;
    auto is_valid = !floor.is_underground() || DungeonMonraceService::is_suitable_for_dungeon(floor.dungeon_id, r_idx);
    is_valid &= monrace.is_suitable_for_special_room();
    if (!is_valid) {
        return false;
    }

    if (monrace.behavior_flags.has(MonsterBehaviorType::KILL_BODY) && monrace.behavior_flags.has_not(MonsterBehaviorType::NEVER_BLOW)) {
        return false;
    }

    if (monrace.kind_flags.has(MonsterKindType::EVIL)) {
        return false;
    }

    if (monrace.symbol_definition.character != PitNestFilter::get_instance().get_monrace_symbol()) {
        return false;
    }

    return true;
}
