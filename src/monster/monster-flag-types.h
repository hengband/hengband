#pragma once

typedef enum monster_flags_type {
	MFLAG_VIEW = 0x01, /* Monster is in line of sight */
    MFLAG_LOS = 0x02, /* Monster is marked for project_all_los(caster_ptr, ) */
    MFLAG_XXX2 = 0x04, /* (unused) */
    MFLAG_ETF = 0x08, /* Monster is entering the field. */
    MFLAG_BORN = 0x10, /* Monster is still being born */
    MFLAG_PREVENT_MAGIC = 0x20, /* Monster is still being no-magic */
} monster_flags_type;

typedef enum monster_flags2_type {
    MFLAG2_KAGE = 0x01, /* Monster is kage */
    MFLAG2_NOPET = 0x02, /* Cannot make monster pet */
    MFLAG2_NOGENO = 0x04, /* Cannot genocide */
    MFLAG2_CHAMELEON = 0x08, /* Monster is chameleon */
    MFLAG2_NOFLOW = 0x10, /* Monster is in no_flow_by_smell mode */
    MFLAG2_SHOW = 0x20, /* Monster is recently memorized */
    MFLAG2_MARK = 0x40, /* Monster is currently memorized */
} monster_flags2_type;
