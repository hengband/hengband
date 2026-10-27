#pragma once

#include "info-reader/definition-id-checker.h"
#include "info-reader/parse-error-types.h"
#include <nlohmann/json_fwd.hpp>
#include <string_view>
#include <tl/expected.hpp>

class BaseitemDefinition;

class BaseitemReader {
public:
    using Session = DefinitionIdChecker;
    static Session create_session(tl::optional<int> last_id = tl::nullopt);

    BaseitemReader(const nlohmann::json &baseitem_data, Session &session);
    BaseitemReader(nlohmann::json &&, Session &) = delete;
    BaseitemReader(const BaseitemReader &) = delete;
    BaseitemReader(BaseitemReader &&) = delete;
    BaseitemReader &operator=(const BaseitemReader &) = delete;
    BaseitemReader &operator=(BaseitemReader &&) = delete;

    tl::expected<void, ParseErrorType> read() const;

private:
    bool grab_one_baseitem_flag(BaseitemDefinition &baseitem, std::string_view what) const;
    tl::expected<void, ParseErrorType> set_baseitem_symbol(BaseitemDefinition &baseitem) const;
    tl::expected<void, ParseErrorType> set_baseitem_kind(BaseitemDefinition &baseitem) const;
    tl::expected<void, ParseErrorType> set_baseitem_parameter_value(BaseitemDefinition &baseitem) const;
    tl::expected<void, ParseErrorType> set_baseitem_allocations(BaseitemDefinition &baseitem) const;
    tl::expected<void, ParseErrorType> set_baseitem_activate(BaseitemDefinition &baseitem) const;
    tl::expected<void, ParseErrorType> set_baseitem_flags(BaseitemDefinition &baseitem) const;
    tl::expected<void, ParseErrorType> set_baseitem_natural_generation(BaseitemDefinition &baseitem) const;

    const nlohmann::json &baseitem_data;
    Session &session;
};
