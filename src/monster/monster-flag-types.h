#pragma once

enum class MonsterTemporaryFlagType {
    VIEW = 0, /* Monster is in line of sight */
    LOS = 1, /* Monster is marked for project_all_los(player_ptr, ) */
    ESP = 2, /* Monster is being sensed by ESP */
    ETF = 3, /* Monster is entering the field. */
    BORN = 4, /* Monster is still being born */
    PREVENT_MAGIC = 5, /* Monster is still being no-magic */
    SANITY_BLAST = 6, /* Monster gives sanity blast effects to player */
    MAX,
};

enum class MonsterConstantFlagType {
    KAGE = 0, /* Monster is kage */
    NOPET = 1, /* Cannot make monster pet */
    NOGENO = 2, /* Cannot genocide */
    CHAMELEON = 3, /* Monster is chameleon */
    NOFLOW = 4, /* Monster is in no_flow_by_smell mode */
    SHOW = 5, /* Monster is recently memorized */
    MARK = 6, /* Monster is currently memorized */
    FRIENDLY = 7, /*!< 友好的である / Friendly */
    PET = 8, /*!< ペットである / Pet */
    CLONED = 9, /*!< クローンである / Cloned */
    MAX,
};
