#pragma once

#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "system/angband.h"
#include <vector>

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
struct flag_desc {
    tr_type flag;
    concptr desc;
};

extern const std::vector<grouper> group_item_list;
extern const std::vector<grouper> group_artifact_list;
extern const std::vector<flag_desc> stat_flags_desc;
extern const std::vector<flag_desc> pval_flags1_desc;
extern const std::vector<flag_desc> slay_flags_desc;
extern const std::vector<flag_desc> brand_flags_desc;
extern const std::vector<flag_desc> resist_flags_desc;
extern const std::vector<flag_desc> vulnerable_flags_desc;
extern const std::vector<flag_desc> immune_flags_desc;
extern const std::vector<flag_desc> sustain_flags_desc;
extern const std::vector<flag_desc> misc_flags2_desc;
extern const std::vector<flag_desc> misc_flags3_desc;
