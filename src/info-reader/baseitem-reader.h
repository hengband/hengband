#pragma once

#include <nlohmann/json.hpp>
#include <string_view>

class BaseitemDefinition;

class BaseitemReader {
public:
    explicit BaseitemReader(nlohmann::json &baseitem_data);
    BaseitemReader(const BaseitemReader &) = delete;
    BaseitemReader(BaseitemReader &&) = delete;
    BaseitemReader &operator=(const BaseitemReader &) = delete;
    BaseitemReader &operator=(BaseitemReader &&) = delete;

    int read() const;

private:
    bool grab_one_baseitem_flag(BaseitemDefinition &baseitem, std::string_view what) const;
    int set_baseitem_symbol(BaseitemDefinition &baseitem) const;
    int set_baseitem_kind(BaseitemDefinition &baseitem) const;
    int set_baseitem_parameter_value(BaseitemDefinition &baseitem) const;
    int set_baseitem_allocations(BaseitemDefinition &baseitem) const;
    int set_baseitem_activate(BaseitemDefinition &baseitem) const;
    int set_baseitem_flags(BaseitemDefinition &baseitem) const;

    nlohmann::json &baseitem_data;
};
