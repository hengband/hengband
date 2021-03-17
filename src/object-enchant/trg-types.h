#pragma once

// clang-format off
enum class TRG {
	INSTA_ART         =  1, /* Item must be an artifact */
    QUESTITEM         =  2, /* quest level item -KMW- */
    XTRA_POWER        =  3, /* Extra power */
    ONE_SUSTAIN       =  4, /* One sustain */
    XTRA_RES_OR_POWER =  5, /* Extra resistance or power */
    XTRA_H_RES        =  6, /* Extra high resistance */
    XTRA_E_RES        =  7, /* Extra element resistance */
    XTRA_L_RES        =  8, /* Extra lordly resistance */
    XTRA_D_RES        =  9, /* Extra dragon resistance */
    XTRA_RES          = 10, /* Extra resistance */
    CURSED            = 11, /* Item is Cursed */
    HEAVY_CURSE       = 12, /* Item is Heavily Cursed */
    PERMA_CURSE       = 13, /* Item is Perma Cursed */
    RANDOM_CURSE0     = 14, /* Item is Random Cursed */
    RANDOM_CURSE1     = 15, /* Item is Random Cursed */
    RANDOM_CURSE2     = 16, /* Item is Random Cursed */
    XTRA_DICE         = 17, /* Extra dice */
    POWERFUL          = 18, /* Item has good value even if Cursed */
    MAX,
};
// clang-format on
