#pragma once

#include "view/display-symbol.h"

class BaseitemConfig {
public:
    BaseitemConfig(const DisplaySymbol &ds);
    BaseitemConfig(const BaseitemConfig &) = delete;
    BaseitemConfig &operator=(const BaseitemConfig &) = delete;
    BaseitemConfig(BaseitemConfig &&) = default;
    BaseitemConfig &operator=(BaseitemConfig &&) = delete;

    bool is_valid() const;
    DisplaySymbol &get_symbol();
    const DisplaySymbol &get_symbol() const;
    char get_character() const;
    uint8_t get_color() const;
    void set_symbol(const DisplaySymbol &ds);
    void update_character(char character);
    void update_color(uint8_t color);

private:
    DisplaySymbol symbol{};
};
