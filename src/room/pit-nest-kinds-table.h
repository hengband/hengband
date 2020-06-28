#pragma once

#include "system/angband.h"

#define NUM_NEST_MON_TYPE 64 /*!<nestの種別数 */

/*! nestのID定義 /  Nest types code */
#define NEST_TYPE_CLONE        0
#define NEST_TYPE_JELLY        1
#define NEST_TYPE_SYMBOL_GOOD  2
#define NEST_TYPE_SYMBOL_EVIL  3
#define NEST_TYPE_MIMIC        4
#define NEST_TYPE_LOVECRAFTIAN 5
#define NEST_TYPE_KENNEL       6
#define NEST_TYPE_ANIMAL       7
#define NEST_TYPE_CHAPEL       8
#define NEST_TYPE_UNDEAD       9

/*! pitのID定義 / Pit types code */
#define PIT_TYPE_ORC           0
#define PIT_TYPE_TROLL         1
#define PIT_TYPE_GIANT         2
#define PIT_TYPE_LOVECRAFTIAN  3
#define PIT_TYPE_SYMBOL_GOOD   4
#define PIT_TYPE_SYMBOL_EVIL   5
#define PIT_TYPE_CHAPEL        6
#define PIT_TYPE_DRAGON        7
#define PIT_TYPE_DEMON         8
#define PIT_TYPE_DARK_ELF      9

#define MAX_PIT_NEST_KINDS 11
#define MAX_MONSTER_PLACE 69

/*! pit/nest型情報の構造体定義 */
typedef struct vault_aux_type
{
	concptr name;
    bool (*hook_func)(player_type *player_ptr, MONRACE_IDX r_idx);
	void (*prep_func)(player_type *player_ptr);
	DEPTH level;
	int chance;
} vault_aux_type;

extern vault_aux_type nest_types[MAX_PIT_NEST_KINDS];
extern vault_aux_type pit_types[MAX_PIT_NEST_KINDS];

extern const int placing[MAX_MONSTER_PLACE][3];
