#pragma once

enum object_description_type {
    OD_NAME_ONLY = 0x00000001, /* Omit values, pval, inscription */
    OD_NAME_AND_ENCHANT = 0x00000002, /* Omit pval, inscription */
    OD_OMIT_INSCRIPTION = 0x00000004, /* Omit inscription */
    OD_OMIT_PREFIX = 0x00000008, /* Omit numeric prefix */
    OD_NO_PLURAL = 0x00000010, /* Don't use plural */
    OD_STORE = 0x00000020, /* Assume to be aware and known */
    OD_NO_FLAVOR = 0x00000040, /* Allow to hidden flavor */
    OD_FORCE_FLAVOR = 0x00000080, /* Get un-shuffled flavor name */
    OD_BASE_NAME = 0x00000100, /* Use kind name without artifact, ego, or smithing details */
    OD_DEBUG = 0x10000000, /* Print for DEBUG */
};
