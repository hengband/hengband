#pragma once 

/*
 * Information about "vault generation"
 */

typedef struct vault_type vault_type;

struct vault_type
{
	STR_OFFSET name;	/* Name (offset) */
	STR_OFFSET text;	/* Text (offset) */

	ROOM_IDX typ;		/* Vault type */
	PROB rat;			/* Vault rating (unused) */
	POSITION hgt;		/* Vault height */
	POSITION wid;		/* Vault width */
};

extern vault_type *v_info;
extern char *v_name;
extern char *v_text;

extern VAULT_IDX max_v_idx;

extern bool build_type7(void);
extern bool build_type8(void);
extern bool build_type10(void);
extern bool build_type17(void);

