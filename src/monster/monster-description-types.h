#pragma once

enum monsetr_description_type {
    MD_NONE = 0x00000000,
    MD_OBJECTIVE = 0x00000001, /* Objective (or Reflexive) */
    MD_POSSESSIVE = 0x00000002, /* Possessive (or Reflexive) */
    MD_INDEF_HIDDEN = 0x00000004, /* Use indefinites for hidden monsters ("something") */
    MD_INDEF_VISIBLE = 0x00000008, /* Use indefinites for visible monsters ("a kobold") */
    MD_PRON_HIDDEN = 0x00000010, /* Pronominalize hidden monsters */
    MD_PRON_VISIBLE = 0x00000020, /* Pronominalize visible monsters */
    MD_ASSUME_HIDDEN = 0x00000040, /* Assume the monster is hidden */
    MD_ASSUME_VISIBLE = 0x00000080, /* Assume the monster is visible */
    MD_TRUE_NAME = 0x00000100, /* Chameleon's true name */
    MD_IGNORE_HALLU = 0x00000200, /* Ignore hallucination, and penetrate shape change */
    MD_NO_OWNER = 0x00000400, /* Do not indicate the pet's owner as in "your" */
};

#define MD_WRONGDOER_NAME (MD_IGNORE_HALLU | MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE) /* 加害明記向け */
