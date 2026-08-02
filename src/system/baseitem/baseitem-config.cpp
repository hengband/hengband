#include "system/baseitem/baseitem-config.h"

BaseitemConfig::BaseitemConfig(const DisplaySymbol &ds)
    : symbol(ds)
{
}

bool BaseitemConfig::is_valid() const
{
    return (this->symbol.character != '\0') || (this->symbol.color > 0);
}

DisplaySymbol &BaseitemConfig::get_symbol()
{
    return this->symbol;
}

const DisplaySymbol &BaseitemConfig::get_symbol() const
{
    return this->symbol;
}

char BaseitemConfig::get_character() const
{
    return this->symbol.character;
}

uint8_t BaseitemConfig::get_color() const
{
    return this->symbol.color;
}

void BaseitemConfig::set_symbol(const DisplaySymbol &ds)
{
    this->symbol = ds;
}

void BaseitemConfig::update_character(char character)
{
    this->symbol.character = character;
}

void BaseitemConfig::update_color(uint8_t color)
{
    this->symbol.color = color;
}
