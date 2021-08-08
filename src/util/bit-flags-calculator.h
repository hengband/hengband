#pragma once

#define reset_bits(FLAG, INDEX) ((FLAG) &= ~(INDEX))
#define set_bits(FLAG, INDEX) ((FLAG) |= (INDEX))
#define any_bits(FLAG, INDEX) (((FLAG) & (INDEX)) != 0)
#define all_bits(FLAG, INDEX) (((FLAG) & (INDEX)) == (INDEX))
#define none_bits(FLAG, INDEX) (((FLAG) & (INDEX)) == 0)
#define match_bits(FLAG, INDEX, MATCH) (((FLAG) & (INDEX)) == (MATCH))
#define has_flag(ARRAY, INDEX) !!((ARRAY)[(INDEX) / 32] & (1UL << ((INDEX) % 32)))
#define add_flag(ARRAY, INDEX) ((ARRAY)[(INDEX) / 32] |= (1UL << ((INDEX) % 32)))
#define remove_flag(ARRAY, INDEX) ((ARRAY)[(INDEX) / 32] &= ~(1UL << ((INDEX) % 32)))
#define is_pval_flag(INDEX) ((TR_STR <= (INDEX) && (INDEX) <= TR_MAGIC_MASTERY) || (TR_STEALTH <= (INDEX) && (INDEX) <= TR_BLOWS))
#define has_pval_flags(ARRAY) !!((ARRAY)[0] & (0x00003f7f))
