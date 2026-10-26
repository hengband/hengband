#pragma once

#include "info-reader/json-reader-util.h"
#include <string>
#include <string_view>
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
    tl::expected<int, IdCheckFailure> check(int id);
    void commit();
    tl::optional<int> last_id() const;

private:
    DefinitionIdOrder order;
    Range bounds;
    std::string label;
    tl::optional<int> last;
    tl::optional<int> pending;
    std::unordered_set<int> defined;
};
