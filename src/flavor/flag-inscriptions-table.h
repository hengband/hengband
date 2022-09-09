#pragma once

#include <optional>
#include <vector>

#include "system/angband.h"

#define MAX_GAME_INSCRIPTIONS 10

enum tr_type : int32_t;

/*! オブジェクトの特性表示記号テーブルの構造体 / Structs and tables for Auto Inscription for flags */
struct flag_insc_table {
#ifdef JP
    flag_insc_table(concptr japanese, concptr english, tr_type flag, std::optional<tr_type> except_flag = std::nullopt)
        : japanese(japanese)
        , english(english)
        , flag(flag)
        , except_flag(except_flag)
    {
    }
#else
    flag_insc_table(concptr english, tr_type flag, std::optional<tr_type> except_flag = std::nullopt)
        : english(english)
        , flag(flag)
        , except_flag(except_flag)
    {
    }
#endif

#ifdef JP
    concptr japanese;
#endif
    concptr english;
    tr_type flag;
    std::optional<tr_type> except_flag;
};

extern const concptr game_inscriptions[MAX_GAME_INSCRIPTIONS];

extern std::vector<flag_insc_table> flag_insc_plus;
extern std::vector<flag_insc_table> flag_insc_immune;
extern std::vector<flag_insc_table> flag_insc_vuln;
extern std::vector<flag_insc_table> flag_insc_resistance;
extern std::vector<flag_insc_table> flag_insc_misc;
extern std::vector<flag_insc_table> flag_insc_aura;
extern std::vector<flag_insc_table> flag_insc_brand;
extern std::vector<flag_insc_table> flag_insc_kill;
extern std::vector<flag_insc_table> flag_insc_slay;
extern std::vector<flag_insc_table> flag_insc_esp1;
extern std::vector<flag_insc_table> flag_insc_esp2;
extern std::vector<flag_insc_table> flag_insc_sust;
