#pragma once

#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "system/angband.h"

#include <vector>

#define MAX_STAT_FLAGS_DESCRIPTION 6
#define MAX_PVAL_FLAGS_DESCRIPTION 7
#define MAX_SLAY_FLAGS_DESCRIPTION 20
#define MAX_BRAND_FLAGS_DESCRIPTION 12
#define MAX_RESISTANCE_FLAGS_DESCRIPTION 19
#define MAX_VULNERABLE_FLAGS_DESCRIPTION 6
#define MAX_IMMUNITY_FLAGS_DESCRIPTION 4
#define MAX_SUSTAINER_FLAGS_DESCRIPTION 6
#define MAX_MISC2_FLAGS_DESCRIPTION 4
#define MAX_MISC3_FLAGS_DESCRIPTION 37

/* A tval grouper */
struct grouper {
    std::vector<ItemKindType> tval_set;
    concptr name;
};

/*
 * Pair together a constant flag with a textual description.
 * Note that it sometimes more efficient to actually make an array
 * of textual names, where entry 'N' is assumed to be paired with
 * the flag whose value is "1UL << N", but that requires hard-coding.
 */
typedef struct flag_desc {
    tr_type flag;
    concptr desc;
} flag_desc;

extern const std::vector<grouper> group_item_list;
extern const std::vector<grouper> group_artifact_list;
extern flag_desc stat_flags_desc[MAX_STAT_FLAGS_DESCRIPTION];
extern flag_desc pval_flags1_desc[MAX_PVAL_FLAGS_DESCRIPTION];
extern flag_desc slay_flags_desc[MAX_SLAY_FLAGS_DESCRIPTION];
extern flag_desc brand_flags_desc[MAX_BRAND_FLAGS_DESCRIPTION];
extern const flag_desc resist_flags_desc[MAX_RESISTANCE_FLAGS_DESCRIPTION];
extern const flag_desc vulnerable_flags_desc[MAX_VULNERABLE_FLAGS_DESCRIPTION];
extern const flag_desc immune_flags_desc[MAX_IMMUNITY_FLAGS_DESCRIPTION];
extern const flag_desc sustain_flags_desc[MAX_SUSTAINER_FLAGS_DESCRIPTION];
extern const flag_desc misc_flags2_desc[MAX_MISC2_FLAGS_DESCRIPTION];
extern const flag_desc misc_flags3_desc[MAX_MISC3_FLAGS_DESCRIPTION];
