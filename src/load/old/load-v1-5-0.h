#pragma once

#include "system/angband.h"
#include "system/monster-entity.h"

// TODO: 更に分割する可能性が中程度あるのでヘッダに置いておく
enum old_monster_resistance_type {
    RF3_IM_ACID = 0x00010000, /* Resist acid a lot */
    RF3_IM_ELEC = 0x00020000, /* Resist elec a lot */
    RF3_IM_FIRE = 0x00040000, /* Resist fire a lot */
    RF3_IM_COLD = 0x00080000, /* Resist cold a lot */
    RF3_IM_POIS = 0x00100000, /* Resist poison a lot */
    RF3_RES_TELE = 0x00200000, /* Resist teleportation */
    RF3_RES_NETH = 0x00400000, /* Resist nether a lot */
    RF3_RES_WATE = 0x00800000, /* Resist water */
    RF3_RES_PLAS = 0x01000000, /* Resist plasma */
    RF3_RES_NEXU = 0x02000000, /* Resist nexus */
    RF3_RES_DISE = 0x04000000, /* Resist disenchantment */
    RF3_RES_ALL = 0x08000000, /* Resist all */
};

enum old_monster_breath_type {
    RF4_BR_LITE = 0x00004000, /* Breathe Lite */
    RF4_BR_DARK = 0x00008000, /* Breathe Dark */
    RF4_BR_CONF = 0x00010000, /* Breathe Confusion */
    RF4_BR_SOUN = 0x00020000, /* Breathe Sound */
    RF4_BR_CHAO = 0x00040000, /* Breathe Chaos */
    RF4_BR_TIME = 0x00200000, /* Breathe Time */
    RF4_BR_INER = 0x00400000, /* Breathe Inertia */
    RF4_BR_GRAV = 0x00800000, /* Breathe Gravity */
    RF4_BR_SHAR = 0x01000000, /* Breathe Shards */
    RF4_BR_WALL = 0x04000000, /* Breathe Force */
};

extern const int OLD_QUEST_WATER_CAVE;
extern const int QUEST_OLD_CASTLE;
extern const int QUEST_ROYAL_CRYPT;

class ItemEntity;
class PlayerType;
void rd_item_old(ItemEntity *o_ptr);
void rd_monster_old(PlayerType *player_ptr, MonsterEntity *m_ptr);
void set_old_lore(MonsterRaceInfo *r_ptr, BIT_FLAGS f4, const MonsterRaceId r_idx);
errr rd_dungeon_old(PlayerType *player_ptr);
