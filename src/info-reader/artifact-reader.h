#pragma once

#include "info-reader/definition-id-checker.h"
#include "info-reader/parse-error-types.h"
#include <nlohmann/json_fwd.hpp>
#include <tl/expected.hpp>

class ArtifactDefinition;

class ArtifactReader {
public:
    using Session = DefinitionIdChecker;
    static Session create_session(tl::optional<int> last_id = tl::nullopt);

    ArtifactReader(const nlohmann::json &art_data, Session &session);
    ArtifactReader(nlohmann::json &&, Session &) = delete;
    ArtifactReader(const ArtifactReader &) = delete;
    ArtifactReader(ArtifactReader &&) = delete;
    ArtifactReader &operator=(const ArtifactReader &) = delete;
    ArtifactReader &operator=(ArtifactReader &&) = delete;

    tl::expected<void, ParseErrorType> read() const;

private:
    tl::expected<void, ParseErrorType> set_art_baseitem(ArtifactDefinition &artifact) const;
    tl::expected<void, ParseErrorType> set_art_activate(ArtifactDefinition &artifact) const;
    tl::expected<void, ParseErrorType> set_art_flags(ArtifactDefinition &artifact) const;

    const nlohmann::json &art_data;
    Session &session;
};
