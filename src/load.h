#pragma once

/*
 * Flags for wr_item()/rd_item()
 */
#define SAVE_ITEM_PVAL         0x00000001
#define SAVE_ITEM_DISCOUNT     0x00000002
#define SAVE_ITEM_NUMBER       0x00000004
#define SAVE_ITEM_NAME1        0x00000008
#define SAVE_ITEM_NAME2        0x00000010
#define SAVE_ITEM_TIMEOUT      0x00000020
#define SAVE_ITEM_TO_H         0x00000040
#define SAVE_ITEM_TO_D         0x00000080
#define SAVE_ITEM_TO_A         0x00000100
#define SAVE_ITEM_AC           0x00000200
#define SAVE_ITEM_DD           0x00000400
#define SAVE_ITEM_DS           0x00000800
#define SAVE_ITEM_IDENT        0x00001000
#define SAVE_ITEM_MARKED       0x00002000
#define SAVE_ITEM_ART_FLAGS0   0x00004000
#define SAVE_ITEM_ART_FLAGS1   0x00008000
#define SAVE_ITEM_ART_FLAGS2   0x00010000
#define SAVE_ITEM_ART_FLAGS3   0x00020000
#define SAVE_ITEM_CURSE_FLAGS  0x00040000
#define SAVE_ITEM_HELD_M_IDX   0x00080000
#define SAVE_ITEM_XTRA1        0x00100000
#define SAVE_ITEM_XTRA2        0x00200000
#define SAVE_ITEM_XTRA3        0x00400000
#define SAVE_ITEM_XTRA4        0x00800000
#define SAVE_ITEM_XTRA5        0x01000000
#define SAVE_ITEM_FEELING      0x02000000
#define SAVE_ITEM_INSCRIPTION  0x04000000
#define SAVE_ITEM_ART_NAME     0x08000000
#define SAVE_ITEM_ART_FLAGS4   0x10000000

 /*
  * Flags for wr_monster()/rd_monster()
  */
#define SAVE_MON_AP_R_IDX     0x00000001
#define SAVE_MON_SUB_ALIGN    0x00000002
#define SAVE_MON_CSLEEP       0x00000004
#define SAVE_MON_FAST         0x00000008
#define SAVE_MON_SLOW         0x00000010
#define SAVE_MON_STUNNED      0x00000020
#define SAVE_MON_CONFUSED     0x00000040
#define SAVE_MON_MONFEAR      0x00000080
#define SAVE_MON_TARGET_Y     0x00000100
#define SAVE_MON_TARGET_X     0x00000200
#define SAVE_MON_INVULNER     0x00000400
#define SAVE_MON_SMART        0x00000800
#define SAVE_MON_EXP          0x00001000
#define SAVE_MON_MFLAG2       0x00002000
#define SAVE_MON_NICKNAME     0x00004000
#define SAVE_MON_PARENT       0x00008000

/* load.c */
extern errr rd_savefile_new(player_type *player_ptr);
extern bool load_floor(player_type *player_ptr, saved_floor_type *sf_ptr, BIT_FLAGS mode);
