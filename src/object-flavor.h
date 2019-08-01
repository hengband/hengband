#pragma once

#include "h-type.h"
#include "object.h"

/* flavor.c */
extern void get_table_name_aux(char *out_string);
extern void get_table_name(char *out_string);
extern void get_table_sindarin_aux(char *out_string);
extern void get_table_sindarin(char *out_string);
extern void flavor_init(void);
extern char *object_desc_kosuu(char *t, object_type *o_ptr);
extern void object_desc(char *buf, object_type *o_ptr, BIT_FLAGS mode);
extern void strip_name(char *buf, KIND_OBJECT_IDX k_idx);

extern const concptr game_inscriptions[];

/*
 * Bit flags for object_desc()
 */
#define OD_NAME_ONLY        0x00000001  /* Omit values, pval, inscription */
#define OD_NAME_AND_ENCHANT 0x00000002  /* Omit pval, inscription */
#define OD_OMIT_INSCRIPTION 0x00000004  /* Omit inscription */
#define OD_OMIT_PREFIX      0x00000008  /* Omit numeric prefix */
#define OD_NO_PLURAL        0x00000010  /* Don't use plural */
#define OD_STORE            0x00000020  /* Assume to be aware and known */
#define OD_NO_FLAVOR        0x00000040  /* Allow to hidden flavor */
#define OD_FORCE_FLAVOR     0x00000080  /* Get un-shuffled flavor name */


