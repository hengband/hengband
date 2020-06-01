#pragma once

/*
 * Magic realms
 */
#define REALM_NONE         0
#define REALM_LIFE         1
#define REALM_SORCERY      2
#define REALM_NATURE       3
#define REALM_CHAOS        4
#define REALM_DEATH        5
#define REALM_TRUMP        6
#define REALM_ARCANE       7
#define REALM_CRAFT        8
#define REALM_DAEMON       9
#define REALM_CRUSADE      10
#define MAX_MAGIC          10
#define MIN_TECHNIC        16
#define REALM_MUSIC        16
#define REALM_HISSATSU     17
#define REALM_HEX          18
#define MAX_REALM          18

#define VALID_REALM        (MAX_REALM + MAX_MAGIC - MIN_TECHNIC + 1)
#define NUM_TECHNIC        (MAX_REALM - MIN_TECHNIC + 1)

#define is_magic(A) ((((A) > REALM_NONE) && ((A) < MAX_MAGIC + 1)) ? TRUE : FALSE)
#define tval2realm(A) ((A) - TV_LIFE_BOOK + 1)
#define technic2magic(A)      (is_magic(A) ? (A) : (A) - MIN_TECHNIC + 1 + MAX_MAGIC)
#define is_good_realm(REALM)   ((REALM) == REALM_LIFE || (REALM) == REALM_CRUSADE)

extern const concptr realm_names[];
#ifdef JP
extern const concptr E_realm_names[];
#endif