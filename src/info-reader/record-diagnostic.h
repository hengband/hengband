#pragma once

#include "info-reader/definition-id-checker.h"
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tl/expected.hpp>

struct RecordReadError {
    std::string id;
    std::string path;
    std::string reason;
};

class RecordDiagnostic {
public:
    explicit RecordDiagnostic(DefinitionIdChecker &session);
    RecordDiagnostic(const RecordDiagnostic &) = delete;
    RecordDiagnostic(RecordDiagnostic &&) = delete;
    RecordDiagnostic &operator=(const RecordDiagnostic &) = delete;
    RecordDiagnostic &operator=(RecordDiagnostic &&) = delete;

    void begin_read(const nlohmann::json &data);
    const std::optional<RecordReadError> &error() const;
    tl::expected<void, ParseErrorType> fail(ParseErrorType code, std::string_view path, std::string_view reason);
    tl::expected<void, ParseErrorType> check_keys(const nlohmann::json &data, std::string_view path, std::span<const std::string_view> keys);
    tl::expected<void, ParseErrorType> read_integer(const nlohmann::json &data, int &value, int minimum, int maximum, std::string_view path);

protected:
    DefinitionIdChecker &session;

private:
    std::string record_id;
    std::optional<RecordReadError> diagnostic;
};
