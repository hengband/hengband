#include "info-reader/town-preferences-reader.h"
#include "info-reader/general-parser.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/legend-ego-validator.h"
#include "info-reader/legend-object-trap-validator.h"
#include "info-reader/quest-reader.h"
#include "info-reader/random-grid-effect-types.h"
#include "locale/language-switcher.h"
#include "system/dungeon/quest-fixed-map.h"
#include "view/display-messages.h"
#include <cstdint>
#include <limits>
#include <nlohmann/json.hpp>
#include <utility>

TownPreferencesReader::TownPreferencesReader(const nlohmann::json &data)
    : data(data)
{
}

tl::expected<void, ParseErrorType> TownPreferencesReader::read(TownPreferencesLegend &legend, TownPreferencesCellParser parse_cell) const
{
    if (const auto result = info_validate_version(this->data, 1); !result) {
        return result;
    }
    if (!this->data.contains("legend") || !this->data["legend"].is_object() || this->data["legend"].empty()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    TownPreferencesLegend parsed;
    for (const auto &[symbol, cell_data] : this->data["legend"].items()) {
        const auto parsed_symbol = parse_town_map_symbol(symbol);
        if (!parsed_symbol) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        if (!cell_data.is_object() ||
            !cell_data.contains("terrain") || !cell_data["terrain"].is_string() || !cell_data.contains("caveInfo") || !cell_data["caveInfo"].is_array()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        for (const auto &flag : cell_data["caveInfo"]) {
            if (!flag.is_string()) {
                return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
            }
        }
        int special = 0;
        if (cell_data.contains("special") &&
            !info_set_integer(cell_data["special"], special, true, Range(std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max()))) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }

        QuestLegendCell cell;
        std::string reason;
        // 共通パーサーが先に拒否する配置衝突も、町の凡例の位置とともに報告する。
        const auto parse_result = parse_cell(cell_data, cell, reason);
        if (!parse_result) {
            if (!reason.empty()) {
                msg_print(_("町の配置指定が不正です: legend['{}'].trap: {}", "Invalid town placement: legend['{}'].trap: {}"), symbol, reason);
            }
            return parse_result;
        }
        const auto random_trap = (cell.grid.random & RANDOM_TRAP) != 0;
        if (has_legend_object_trap_conflict(cell.grid, cell.object_is_quest_reward, cell.grid.trap != 0 && !random_trap, random_trap)) {
            msg_print(_("町の配置指定が不正です: legend['{}'].trap: {}", "Invalid town placement: legend['{}'].trap: {}"), symbol, describe_legend_object_trap_conflict());
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        if (const auto result = validate_legend_ego(cell.grid, cell.object_is_quest_reward); !result) {
            constexpr auto fmt = _("町のエゴ指定が不正です: legend['{}'].{}: {}", "Invalid town ego: legend['{}'].{}: {}");
            msg_print(fmt, symbol, describe_legend_ego_spec(cell.grid), describe_legend_ego_error(result.error()));
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        parsed.emplace_back(static_cast<unsigned char>(*parsed_symbol), cell.grid);
    }

    legend = std::move(parsed);
    return {};
}
