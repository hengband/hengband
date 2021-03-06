#pragma once

enum trg_type {
	TRG_INSTA_ART = 0x00000001L, /* Item must be an artifact */
    TRG_QUESTITEM = 0x00000002L, /* quest level item -KMW- */
    TRG_XTRA_POWER = 0x00000004L, /* Extra power */
    TRG_ONE_SUSTAIN = 0x00000008L, /* One sustain */
    TRG_XTRA_RES_OR_POWER = 0x00000010L, /* Extra resistance or power */
    TRG_XTRA_H_RES = 0x00000020L, /* Extra high resistance */
    TRG_XTRA_E_RES = 0x00000040L, /* Extra element resistance */
    TRG_XTRA_L_RES = 0x00000080L, /* Extra lordly resistance */
    TRG_XTRA_D_RES = 0x00000100L, /* Extra dragon resistance */
    TRG_XTRA_RES = 0x00000200L, /* Extra resistance */
    TRG_CURSED = 0x00000400L, /* Item is Cursed */
    TRG_HEAVY_CURSE = 0x00000800L, /* Item is Heavily Cursed */
    TRG_PERMA_CURSE = 0x00001000L, /* Item is Perma Cursed */
    TRG_RANDOM_CURSE0 = 0x00002000L, /* Item is Random Cursed */
    TRG_RANDOM_CURSE1 = 0x00004000L, /* Item is Random Cursed */
    TRG_RANDOM_CURSE2 = 0x00008000L, /* Item is Random Cursed */
    TRG_XTRA_DICE = 0x00010000L, /* Extra dice */
    TRG_POWERFUL = 0x00020000L, /* Item has good value even if Cursed */
};
