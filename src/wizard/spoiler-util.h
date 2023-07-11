#pragma once

#include "object-enchant/tr-flags.h"
#include "system/angband.h"
#include "wizard/spoiler-table.h"
#include <string>
#include <string_view>
#include <vector>

/* MAX_LINE_LEN specifies when a line should wrap. */
#define MAX_LINE_LEN 75

/* Given an array, determine how many elements are in the array */
#define N_ELEMENTS(a) (sizeof(a) / sizeof((a)[0]))

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
    char description[MAX_NLEN]{}; /* "The Longsword Dragonsmiter (6d4) (+20, +25)" */
    ParameterValueInfo pval_info{}; /* Description of what is affected by an object's pval */
    concptr slays[N_ELEMENTS(slay_flags_desc) + 1]{}; /* A list of an object's slaying preferences */
    concptr brands[N_ELEMENTS(brand_flags_desc) + 1]{}; /* A list if an object's elemental brands */
    concptr immunities[N_ELEMENTS(immune_flags_desc) + 1]{}; /* A list of immunities granted by an object */
    concptr resistances[N_ELEMENTS(resist_flags_desc) + 1]{}; /* A list of resistances granted by an object */
    concptr vulnerables[N_ELEMENTS(vulnerable_flags_desc) + 1]{}; /* A list of resistances granted by an object */
    concptr sustains[N_ELEMENTS(sustain_flags_desc) - 1 + 1]{}; /* A list of stats sustained by an object */
    std::vector<std::string> misc_magic{}; // その他の特性 (呪い、光源範囲等)

    char addition[80] = ""; /* Additional ability or resistance */
    concptr activation = ""; /* A string describing an artifact's activation */
    char misc_desc[80] = ""; /* "Level 20, Rarity 30, 3.0 lbs, 20000 Gold" */
};

extern const char item_separator;
extern const char list_separator;
extern const int max_evolution_depth;
extern concptr spoiler_indent;
extern FILE *spoiler_file;

struct flag_desc;
std::vector<std::string> extract_spoiler_flags(const TrFlags &art_flags, const std::vector<flag_desc> &definitions);
void spoiler_blanklines(int n);
void spoiler_underline(concptr str);
void spoil_out(std::string_view sv, bool flush_buffer = false);
