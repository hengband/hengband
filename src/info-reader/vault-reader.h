#pragma once

#include "info-reader/definition-id-checker.h"
#include "info-reader/record-diagnostic.h"
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <tl/optional.hpp>

class VaultReader : private RecordDiagnostic {
public:
    using RecordDiagnostic::error;
    using Session = DefinitionIdChecker;
    static Session create_session(tl::optional<int> last_id = tl::nullopt);

    VaultReader(const nlohmann::json &data, Session &session);
    VaultReader(nlohmann::json &&, Session &) = delete;
    VaultReader(const VaultReader &) = delete;
    VaultReader(VaultReader &&) = delete;
    VaultReader &operator=(const VaultReader &) = delete;
    VaultReader &operator=(VaultReader &&) = delete;

    tl::expected<void, ParseErrorType> read();

private:
    const nlohmann::json &data;
};
