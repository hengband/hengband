#pragma once

#include <tl/optional.hpp>
#include <vector>

#include "locale/localized-string.h"
#include "system/angband.h"

#define MAX_GAME_INSCRIPTIONS 10

enum tr_type : int32_t;

/*! オブジェクトの特性表示記号テーブルの構造体 / Structs and tables for Auto Inscription for flags */
struct flag_insc_table {
    flag_insc_table(LocalizedString &&inscription, tr_type flag, const tl::optional<tr_type> &except_flag = tl::nullopt)
        : inscription(std::move(inscription))
        , flag(flag)
        , except_flag(except_flag)
    {
    }

    LocalizedString inscription;
    tr_type flag;
    tl::optional<tr_type> except_flag;
};

extern const concptr game_inscriptions[MAX_GAME_INSCRIPTIONS];

extern const std::vector<flag_insc_table> flag_insc_plus;
extern const std::vector<flag_insc_table> flag_insc_immune;
extern const std::vector<flag_insc_table> flag_insc_vuln;
extern const std::vector<flag_insc_table> flag_insc_resistance;
extern const std::vector<flag_insc_table> flag_insc_misc;
extern const std::vector<flag_insc_table> flag_insc_aura;
extern const std::vector<flag_insc_table> flag_insc_brand;
extern const std::vector<flag_insc_table> flag_insc_kill;
extern const std::vector<flag_insc_table> flag_insc_slay;
extern const std::vector<flag_insc_table> flag_insc_esp1;
extern const std::vector<flag_insc_table> flag_insc_esp2;
extern const std::vector<flag_insc_table> flag_insc_sust;
