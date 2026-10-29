#include "info-reader/definition-id-checker.h"
#include "info-reader/json-reader-util.h"
#include "locale/language-switcher.h"
#include "view/display-messages.h"
#include <cstdint>
#include <fmt/format.h>
#include <stdexcept>

DefinitionIdChecker::DefinitionIdChecker(DefinitionIdOrder order, Range bounds, std::string_view label, tl::optional<int> last_id)
    : order(order)
    , bounds(bounds)
    , label(label)
    , last(last_id)
{
}

/*! @brief ID を整数として読み、範囲と順序を確かめる */
tl::expected<int, IdCheckFailure> DefinitionIdChecker::read(const nlohmann::json &id_json)
{
    this->pending.reset();
    int id = 0;
    if (const auto result = info_set_integer(id_json, id, true, this->bounds); !result) {
        std::string reason;
        if (result.error() == ParseErrorType::TOO_FEW_ARGUMENTS) {
            reason = fmt::format(_("{}の ID がありません", "Missing {} ID"), this->label);
        } else if (result.error() == ParseErrorType::INVALID_TYPE) {
            reason = fmt::format(_("{}の ID は整数ではありません", "{} ID is not an integer"), this->label);
        } else {
            reason = fmt::format(_("{}の ID '{}' は {}～{} の範囲外です", "{} ID '{}' is outside the range {} to {}"), this->label, id_json.dump(), this->bounds.first, this->bounds.second);
        }
        return tl::unexpected(IdCheckFailure{ result.error(), std::move(reason) });
    }
    return this->check(id);
}

/*!
 * @brief ID を読み込み、失敗した理由を表示する
 * @param id_json ID の JSON 値
 * @return 成功時は ID、失敗時は解析エラーコード
 */
tl::expected<int, ParseErrorType> DefinitionIdChecker::read_and_report(const nlohmann::json &id_json)
{
    const auto result = this->read(id_json);
    if (!result) {
        msg_print(result.error().reason);
        return tl::unexpected(result.error().code);
    }
    return *result;
}

/*! @brief 整数 ID の範囲と順序を確かめ、確定待ちの値を保持する */
tl::expected<int, IdCheckFailure> DefinitionIdChecker::check(int id)
{
    this->pending.reset();
    if (id < this->bounds.first || id > this->bounds.second) {
        return tl::unexpected(IdCheckFailure{ ParseErrorType::INVALID_VALUE,
            fmt::format(_("{}の ID '{}' は {}～{} の範囲外です", "{} ID '{}' is outside the range {} to {}"), this->label, id, this->bounds.first, this->bounds.second) });
    }
    std::string reason;
    switch (this->order) {
    case DefinitionIdOrder::SEQUENTIAL:
        if (this->last) {
            const auto next_id = static_cast<int64_t>(*this->last) + 1;
            if (id != next_id) {
                reason = fmt::format(_("{}の ID '{}' は直前の ID '{}' の次（{}）ではありません", "{} ID '{}' does not follow previous ID '{}' (expected {})"), this->label, id, *this->last, next_id);
            }
        } else if (id != this->bounds.first) {
            reason = fmt::format(_("{}の最初の ID は {} です", "The first {} ID must be {}"), this->label, this->bounds.first);
        }
        break;
    case DefinitionIdOrder::STRICTLY_INCREASING:
        if (this->last && id <= *this->last) {
            reason = fmt::format(_("{}の ID '{}' は直前の ID '{}' より大きくありません", "{} ID '{}' is not greater than previous ID '{}'"), this->label, id, *this->last);
        }
        break;
    case DefinitionIdOrder::UNIQUE:
        if (this->defined.contains(id)) {
            reason = fmt::format(_("{}の ID '{}' は重複しています", "{} ID '{}' is duplicated"), this->label, id);
        }
        break;
    }
    if (!reason.empty()) {
        return tl::unexpected(IdCheckFailure{ ParseErrorType::NON_SEQUENTIAL_RECORDS, std::move(reason) });
    }
    this->pending = id;
    return id;
}

/*! @brief 記録全体の読み込みに成功した ID を確定する */
void DefinitionIdChecker::commit()
{
    if (!this->pending) {
        throw std::logic_error("No definition ID to commit");
    }
    if (this->order == DefinitionIdOrder::UNIQUE) {
        this->defined.insert(*this->pending);
    }
    this->last = this->pending;
    this->pending.reset();
}

/*! @brief 最後に確定した ID を返す */
tl::optional<int> DefinitionIdChecker::last_id() const
{
    return this->last;
}
