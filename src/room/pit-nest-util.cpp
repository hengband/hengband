#include "room/pit-nest-util.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"
#include "util/probability-table.h"

/*!
 * @brief ダンジョン毎に指定されたピット配列を基準にランダムなnestタイプを決める
 * @param floor フロアへの参照
 * @param nest_types nest定義のマップ
 * @return 選択されたnestのID、選択失敗した場合nullopt.
 */
std::optional<NestKind> pick_nest_type(const FloorType &floor, const std::map<NestKind, nest_pit_type> &nest_types)
{
    ProbabilityTable<NestKind> table;
    for (const auto &[nest_kind, nest] : nest_types) {
        if (nest.level > floor.dun_level) {
            continue;
        }

        if (none_bits(floor.get_dungeon_definition().nest, (1UL << enum2i(nest_kind)))) {
            continue;
        }

        table.entry_item(nest_kind, nest.chance * MAX_DEPTH / (std::min(floor.dun_level, MAX_DEPTH - 1) - nest.level + 5));
    }

    if (table.empty()) {
        return std::nullopt;
    }

    return table.pick_one_at_random();
}

/*!
 * @brief ダンジョン毎に指定されたピット配列を基準にランダムなpitタイプを決める
 * @param floor フロアへの参照
 * @param pit_types pit定義のマップ
 * @return 選択されたpitのID、選択失敗した場合nullopt.
 */
std::optional<PitKind> pick_pit_type(const FloorType &floor, const std::map<PitKind, nest_pit_type> &pit_types)
{
    ProbabilityTable<PitKind> table;
    for (const auto &[pit_kind, pit] : pit_types) {
        if (pit.level > floor.dun_level) {
            continue;
        }

        if (none_bits(floor.get_dungeon_definition().pit, (1UL << enum2i(pit_kind)))) {
            continue;
        }

        table.entry_item(pit_kind, pit.chance * MAX_DEPTH / (std::min(floor.dun_level, MAX_DEPTH - 1) - pit.level + 5));
    }

    if (table.empty()) {
        return std::nullopt;
    }

    return table.pick_one_at_random();
}

/*!
 * @brief Pit/Nestに格納するモンスターを選択する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param align アライメントが中立に設定されたモンスター実体 (その他の中身は空)
 * @param boost 選択基準となるフロアの増分
 * @return モンスター種族ID (見つからなかったらnullopt)
 * @details Nestにはそのフロアの通常レベルより11高いモンスターを中心に選ぶ
 */
std::optional<MonraceId> select_pit_nest_monrace_id(PlayerType *player_ptr, MonsterEntity &align, int boost)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monraces = MonraceList::get_instance();
    for (auto attempts = 100; attempts > 0; attempts--) {
        const auto monrace_id = get_mon_num(player_ptr, 0, floor.dun_level + boost, PM_NONE);
        const auto &monrace = monraces.get_monrace(monrace_id);
        if (monster_has_hostile_align(player_ptr, &align, 0, 0, &monrace)) {
            continue;
        }

        if (MonraceList::is_valid(monrace_id)) {
            return monrace_id;
        }

        return std::nullopt;
    }

    return std::nullopt;
}

/*!
 * @brief デバッグ時に生成されたpitの型を出力する処理
 * @param type pitの型ID
 * @return デバッグ表示文字列
 */
std::string pit_subtype_string(PitKind type)
{
    switch (type) {
    case PitKind::SYMBOL_GOOD:
    case PitKind::SYMBOL_EVIL:
        return std::string("(").append(1, vault_aux_char).append(1, ')');
    case PitKind::DRAGON:
        if (vault_aux_dragon_mask4.has_all_of({ MonsterAbilityType::BR_ACID, MonsterAbilityType::BR_ELEC, MonsterAbilityType::BR_FIRE, MonsterAbilityType::BR_COLD, MonsterAbilityType::BR_POIS })) {
            return _("(万色)", "(multi-hued)");
        }

        if (vault_aux_dragon_mask4.has(MonsterAbilityType::BR_ACID)) {
            return _("(酸)", "(acid)");
        }

        if (vault_aux_dragon_mask4.has(MonsterAbilityType::BR_ELEC)) {
            return _("(稲妻)", "(lightning)");
        }

        if (vault_aux_dragon_mask4.has(MonsterAbilityType::BR_FIRE)) {
            return _("(火炎)", "(fire)");
        }

        if (vault_aux_dragon_mask4.has(MonsterAbilityType::BR_COLD)) {
            return _("(冷気)", "(frost)");
        }

        if (vault_aux_dragon_mask4.has(MonsterAbilityType::BR_POIS)) {
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
std::string nest_subtype_string(NestKind type)
{
    switch (type) {
    case NestKind::CLONE: {
        const auto &monrace = MonraceList::get_instance().get_monrace(vault_aux_race);
        std::stringstream ss;
        ss << '(' << monrace.name << ')';
        return ss.str();
    }
    case NestKind::SYMBOL_GOOD:
    case NestKind::SYMBOL_EVIL:
        return std::string("(").append(1, vault_aux_char).append(1, ')');
    default:
        return "";
    }
}
