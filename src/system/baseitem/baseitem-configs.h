#pragma once

#include "util/abstract-vector-wrapper.h"
#include <cstdint>

class DisplaySymbol;
class BaseitemConfig;
class BaseitemConfigs : public util::AbstractVectorWrapper<BaseitemConfig> {
public:
    BaseitemConfigs(const BaseitemConfigs &) = delete;
    BaseitemConfigs &operator=(BaseitemConfigs &) = delete;
    BaseitemConfigs(const BaseitemConfigs &&) = delete;
    BaseitemConfigs &operator=(BaseitemConfigs &&) = delete;
    ~BaseitemConfigs() override;

    static BaseitemConfigs &get_instance();
    void emplace_back(const DisplaySymbol &ds);

    const BaseitemConfig &get_config(short bi_id) const;
    BaseitemConfig &get_config(short bi_id);
    const BaseitemConfig &pick_one_at_random() const;
    char get_character(short bi_id) const;
    uint8_t get_color(short bi_id) const;
    void set_config(short bi_id, const DisplaySymbol &ds);

private:
    BaseitemConfigs() = default;

    static BaseitemConfigs instance;

    std::vector<BaseitemConfig> configs;

    void validate(short bi_id) const;

    std::vector<BaseitemConfig> &get_inner_container() override
    {
        return this->configs;
    }
};
