/*
 * @brief マクロ設定実装
 * @author Hourier
 * @date 2024/02/19
 */

#include "io/macro-configurations-store.h"

size_t max_macrotrigger = 0;
std::optional<std::string> macro_template;
std::optional<std::string> macro_modifier_chr;
std::vector<std::string> macro_modifier_names = std::vector<std::string>(MAX_MACRO_MOD);
std::vector<std::string> macro_trigger_names = std::vector<std::string>(MAX_MACRO_TRIG);
std::map<ShiftStatus, std::vector<std::string>> macro_trigger_keycodes = {
    { ShiftStatus::OFF, std::vector<std::string>(MAX_MACRO_TRIG) },
    { ShiftStatus::ON, std::vector<std::string>(MAX_MACRO_TRIG) },
};
