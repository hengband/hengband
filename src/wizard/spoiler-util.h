#pragma once

#include "system/angband.h"
#include "wizard/spoiler-table.h"

/* MAX_LINE_LEN specifies when a line should wrap. */
#define MAX_LINE_LEN 75

/* Given an array, determine how many elements are in the array */
#define N_ELEMENTS(a) (sizeof(a) / sizeof((a)[0]))

enum class SpoilerOutputResultType {
    SPOILER_OUTPUT_CANCEL,
    SPOILER_OUTPUT_SUCCESS,
    SPOILER_OUTPUT_FAIL_FOPEN,
    SPOILER_OUTPUT_FAIL_FCLOSE
};

/* A special type used just for deailing with pvals */
typedef struct pval_info_type {
    char pval_desc[12]; /* This will contain a string such as "+2", "-10", etc. */

    /* A list of various player traits affected by an object's pval such as stats, speed, stealth, etc. */
    concptr pval_affects[N_ELEMENTS(stat_flags_desc) - 1 + N_ELEMENTS(pval_flags1_desc) + 1];
} pval_info_type;

typedef struct obj_desc_list {
    char description[MAX_NLEN]; /* "The Longsword Dragonsmiter (6d4) (+20, +25)" */
    pval_info_type pval_info; /* Description of what is affected by an object's pval */
    concptr slays[N_ELEMENTS(slay_flags_desc) + 1]; /* A list of an object's slaying preferences */
    concptr brands[N_ELEMENTS(brand_flags_desc) + 1]; /* A list if an object's elemental brands */
    concptr immunities[N_ELEMENTS(immune_flags_desc) + 1]; /* A list of immunities granted by an object */
    concptr resistances[N_ELEMENTS(resist_flags_desc) + 1]; /* A list of resistances granted by an object */
    concptr vulnerables[N_ELEMENTS(vulnerable_flags_desc) + 1]; /* A list of resistances granted by an object */
    concptr sustains[N_ELEMENTS(sustain_flags_desc) - 1 + 1]; /* A list of stats sustained by an object */

    /* A list of various magical qualities an object may have */
    concptr misc_magic[N_ELEMENTS(misc_flags2_desc) + N_ELEMENTS(misc_flags3_desc) + 1 /* Permanent Light */
        + 1 /* TY curse */
        + 1 /* type of curse */
        + 1]; /* sentinel nullptr */

    char addition[80]; /* Additional ability or resistance */
    concptr activation; /* A string describing an artifact's activation */
    char misc_desc[80]; /* "Level 20, Rarity 30, 3.0 lbs, 20000 Gold" */
} obj_desc_list;

extern const char item_separator;
extern const char list_separator;
extern const int max_evolution_depth;
extern concptr spoiler_indent;
extern FILE *spoiler_file;

void spoiler_blanklines(int n);
void spoiler_underline(concptr str);
void spoil_out(concptr str);
