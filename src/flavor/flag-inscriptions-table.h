#pragma once

#include "system/angband.h"

#define MAX_GAME_INSCRIPTIONS 10

#define MAX_INSCRIPTIONS_PLUS 14
#define MAX_INSCRIPTIONS_IMMUNE 5
#define MAX_INSCRIPTIONS_RESISTANCE 17
#define MAX_INSCRIPTIONS_MISC 26
#define MAX_INSCRIPTIONS_AURA 6
#define MAX_INSCRIPTIONS_BRAND 11
#define MAX_INSCRIPTIONS_KILL 10
#define MAX_INSCRIPTIONS_SLAY 10
#define MAX_INSCRIPTIONS_ESP_1 6
#define MAX_INSCRIPTIONS_ESP_2 9
#define MAX_INSCRIPTIONS_SUSTAINER 7

/*! オブジェクトの特性表示記号テーブルの構造体 / Structs and tables for Auto Inscription for flags */
typedef struct flag_insc_table {
#ifdef JP
    concptr japanese;
#endif
    concptr english;
    int flag;
    int except_flag;
} flag_insc_table;

extern const concptr game_inscriptions[MAX_GAME_INSCRIPTIONS];

extern flag_insc_table flag_insc_plus[MAX_INSCRIPTIONS_PLUS];
extern flag_insc_table flag_insc_immune[MAX_INSCRIPTIONS_IMMUNE];
extern flag_insc_table flag_insc_resistance[MAX_INSCRIPTIONS_RESISTANCE];
extern flag_insc_table flag_insc_misc[MAX_INSCRIPTIONS_MISC];
extern flag_insc_table flag_insc_aura[MAX_INSCRIPTIONS_AURA];
extern flag_insc_table flag_insc_brand[MAX_INSCRIPTIONS_BRAND];
extern flag_insc_table flag_insc_kill[MAX_INSCRIPTIONS_KILL];
extern flag_insc_table flag_insc_slay[MAX_INSCRIPTIONS_SLAY];
extern flag_insc_table flag_insc_esp1[MAX_INSCRIPTIONS_ESP_1];
extern flag_insc_table flag_insc_esp2[MAX_INSCRIPTIONS_ESP_2];
extern flag_insc_table flag_insc_sust[MAX_INSCRIPTIONS_SUSTAINER];
