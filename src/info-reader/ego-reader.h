#pragma once

#include <nlohmann/json_fwd.hpp>

class EgoItemDefinition;
struct ego_generate_type;

class EgoReader {
public:
    explicit EgoReader(const nlohmann::json &ego_data);
    EgoReader(nlohmann::json &&) = delete;
    EgoReader(const EgoReader &) = delete;
    EgoReader(EgoReader &&) = delete;
    EgoReader &operator=(const EgoReader &) = delete;
    EgoReader &operator=(EgoReader &&) = delete;

    int read() const;

private:
    bool grab_one_flag(EgoItemDefinition &ego, const nlohmann::json &flag) const;
    bool grab_one_extra_flag(ego_generate_type &extra, const nlohmann::json &flag) const;
    int set_flags(EgoItemDefinition &ego) const;
    int set_extra_flags(EgoItemDefinition &ego) const;
    int set_activation(EgoItemDefinition &ego) const;

    const nlohmann::json &ego_data;
};
