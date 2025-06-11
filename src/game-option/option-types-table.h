#pragma once

#include <cstdint>
#include <string>
#include <tl/optional.hpp>
#include <vector>

enum class GameOptionPage : int;
class GameOption {
public:
    GameOption(bool *value, bool norm, uint8_t set, uint8_t bits, std::string &&text, std::string &&description, const tl::optional<GameOptionPage> &page = tl::nullopt);
    bool *value;
    bool default_value;
    uint8_t flag_position;
    uint8_t offset;
    std::string text;
    std::string description;
    tl::optional<GameOptionPage> page;
};

extern const std::vector<GameOption> option_info;
extern const std::vector<GameOption> cheat_info;
extern const std::vector<GameOption> autosave_info;
