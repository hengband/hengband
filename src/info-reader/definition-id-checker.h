#pragma once

#include "info-reader/parse-error-types.h"
#include "info-reader/range.h"
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <tl/optional.hpp>
#include <unordered_set>

struct IdCheckFailure {
    ParseErrorType code;
    std::string reason;
};

enum class DefinitionIdOrder {
    SEQUENTIAL,
    STRICTLY_INCREASING,
    UNIQUE,
};

class DefinitionIdChecker {
public:
    DefinitionIdChecker(DefinitionIdOrder order, Range bounds, std::string_view label, tl::optional<int> last_id = tl::nullopt);
    tl::expected<int, IdCheckFailure> read(const nlohmann::json &id_json);
    tl::expected<int, ParseErrorType> read_and_report(const nlohmann::json &id_json);
    tl::expected<int, IdCheckFailure> check(int id);
    tl::expected<int, ParseErrorType> check_and_report(int id);
    void commit();
    tl::optional<int> last_id() const;

private:
    std::string range_error_reason(std::string_view id_text) const;

    DefinitionIdOrder order;
    Range bounds;
    std::string label;
    tl::optional<int> last;
    tl::optional<int> pending;
    std::unordered_set<int> defined;
};
