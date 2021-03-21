#pragma once

// clang-format off
enum class TRG {
	INSTA_ART         =  0, /* Item must be an artifact */
    QUESTITEM         =  1, /* quest level item -KMW- */
    XTRA_POWER        =  2, /* Extra power */
    ONE_SUSTAIN       =  3, /* One sustain */
    XTRA_RES_OR_POWER =  4, /* Extra resistance or power */
    XTRA_H_RES        =  5, /* Extra high resistance */
    XTRA_E_RES        =  6, /* Extra element resistance */
    XTRA_L_RES        =  7, /* Extra lordly resistance */
    XTRA_D_RES        =  8, /* Extra dragon resistance */
    XTRA_RES          =  9, /* Extra resistance */
    CURSED            = 10, /* Item is Cursed */
    HEAVY_CURSE       = 11, /* Item is Heavily Cursed */
    PERMA_CURSE       = 12, /* Item is Perma Cursed */
    RANDOM_CURSE0     = 13, /* Item is Random Cursed */
    RANDOM_CURSE1     = 14, /* Item is Random Cursed */
    RANDOM_CURSE2     = 15, /* Item is Random Cursed */
    XTRA_DICE         = 16, /* Extra dice */
    POWERFUL          = 17, /* Item has good value even if Cursed */
    MAX,
};
// clang-format on
