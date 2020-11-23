#pragma once 

#include "system/angband.h"

typedef struct vault_type {
	STR_OFFSET name;	/* Name (offset) */
	STR_OFFSET text;	/* Text (offset) */

	ROOM_IDX typ;		/* Vault type */
	PROB rat;			/* Vault rating (unused) */
	POSITION hgt;		/* Vault height */
	POSITION wid;		/* Vault width */
} vault_type;

extern vault_type *v_info;
extern char *v_name;
extern char *v_text;
extern VAULT_IDX max_v_idx;

typedef struct dun_data_type dun_data_type;
bool build_type7(player_type *player_ptr, dun_data_type *dd_ptr);
bool build_type8(player_type *player_ptr, dun_data_type *dd_ptr);
bool build_type10(player_type *player_ptr, dun_data_type *dd_ptr);
bool build_type17(player_type *player_ptr, dun_data_type *dd_ptr);
