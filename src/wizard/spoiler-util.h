#pragma once

#include "object-enchant/tr-flags.h"
#include "system/angband.h"
#include "wizard/spoiler-table.h"
#include <string>
#include <string_view>
#include <vector>

/* MAX_LINE_LEN specifies when a line should wrap. */
#define MAX_LINE_LEN 75

enum class SpoilerOutputResultType {
    CANCELED,
    SUCCESSFUL,
    FILE_OPEN_FAILED,
    FILE_CLOSE_FAILED,
};

class ItemEntity;
class ParameterValueInfo {
public:
    ParameterValueInfo() = default;

    std::string pval_desc = ""; /* This will contain a string such as "+2", "-10", etc. */

    /* A list of various player traits affected by an object's pval such as stats, speed, stealth, etc. */
    std::vector<std::string> pval_affects{};

    void analyze(const ItemEntity &item);
};

struct obj_desc_list {
    std::string description = ""; /* "The Longsword Dragonsmiter (6d4) (+20, +25)" */
    ParameterValueInfo pval_info{}; /* Description of what is affected by an object's pval */
    std::vector<std::string> slays{}; /* A list of an object's slaying preferences */
    std::vector<std::string> brands{}; /* A list if an object's elemental brands */
    std::vector<std::string> immunities{}; /* A list of immunities granted by an object */
    std::vector<std::string> resistances{}; /* A list of resistances granted by an object */
    std::vector<std::string> vulnerabilities{}; /* A list of resistances granted by an object */
    std::vector<std::string> sustenances{}; /* A list of stats sustained by an object */
    std::vector<std::string> misc_magic{}; // その他の特性 (呪い、光源範囲等)

    std::string addition = ""; /* Additional ability or resistance */
    std::string activation = ""; /* A string describing an artifact's activation */
    std::string misc_desc = ""; /* "Level 20, Rarity 30, 3.0 lbs, 20000 Gold" */
};

extern const char item_separator;
extern const char list_separator;
extern const int max_evolution_depth;
extern const std::string spoiler_indent;
extern FILE *spoiler_file;

struct flag_desc;
std::vector<std::string> extract_spoiler_flags(const TrFlags &art_flags, const std::vector<flag_desc> &definitions);
void spoiler_blanklines(int n);
void spoiler_underline(std::string_view str);
void spoil_out(std::string_view sv, bool flush_buffer = false);
