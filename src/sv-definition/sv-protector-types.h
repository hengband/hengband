#pragma once

/* The "sval" codes for TV_SHIELD */
typedef enum sv_shield_type {
	SV_SMALL_LEATHER_SHIELD = 2,
    SV_SMALL_METAL_SHIELD = 3,
    SV_LARGE_LEATHER_SHIELD = 4,
    SV_LARGE_METAL_SHIELD = 5,
    SV_DRAGON_SHIELD = 6,
    SV_KNIGHT_SHIELD = 7,
    SV_MIRROR_SHIELD = 10,
    SV_YATA_MIRROR = 50,
} sv_shield_type;

/* The "sval" codes for TV_HELM */
typedef enum sv_helm_type {
    SV_HARD_LEATHER_CAP = 2,
    SV_METAL_CAP = 3,
    SV_JINGASA = 4, /* 4 */
    SV_IRON_HELM = 5,
    SV_STEEL_HELM = 6,
    SV_DRAGON_HELM = 7,
    SV_KABUTO = 8, /* 7 */
} sv_helm_type;

/* The "sval" codes for TV_CROWN */
typedef enum sv_crown_type {
    SV_IRON_CROWN = 10,
    SV_GOLDEN_CROWN = 11,
    SV_JEWELED_CROWN = 12,
    SV_CHAOS = 50,
} sv_crown_type;

/* The "sval" codes for TV_BOOTS */
typedef enum sv_boots_type {
    SV_PAIR_OF_SOFT_LEATHER_BOOTS = 2,
    SV_PAIR_OF_HARD_LEATHER_BOOTS = 3,
    SV_PAIR_OF_DRAGON_GREAVE = 4,
    SV_PAIR_OF_METAL_SHOD_BOOTS = 6,
} sv_boots_type;

/* The "sval" codes for TV_CLOAK */
typedef enum sv_cloak_type {
    SV_CLOAK = 1,
    SV_ELVEN_CLOAK = 2,
    SV_FUR_CLOAK = 3,
    SV_ETHEREAL_CLOAK = 5,
    SV_SHADOW_CLOAK = 6,
} sv_cloak_type;

/* The "sval" codes for TV_GLOVES */
typedef enum sv_gloves_type {
    SV_SET_OF_LEATHER_GLOVES = 1,
    SV_SET_OF_GAUNTLETS = 2,
    SV_SET_OF_DRAGON_GLOVES = 3,
    SV_SET_OF_CESTI = 5,
} sv_gloves_type;
