#pragma once
/*!
 * @file sound-definitions-table.h
 * @brief 設定ファイル用の効果音名定義ヘッダ
 */

#include "system/angband.h"

enum sound_type {
    SOUND_HIT = 1,
    SOUND_MISS = 2,
    SOUND_FLEE = 3,
    SOUND_DROP = 4,
    SOUND_KILL = 5,
    SOUND_LEVEL = 6,
    SOUND_DEATH = 7,
    SOUND_STUDY = 8,
    SOUND_TELEPORT = 9,
    SOUND_SHOOT = 10,
    SOUND_QUAFF = 11,
    SOUND_ZAP = 12,
    SOUND_WALK = 13, /*!< unused */
    SOUND_TPOTHER = 14,
    SOUND_HITWALL = 15,
    SOUND_EAT = 16,
    SOUND_STORE1 = 17,
    SOUND_STORE2 = 18,
    SOUND_STORE3 = 19,
    SOUND_STORE4 = 20,
    SOUND_DIG = 21,
    SOUND_OPENDOOR = 22,
    SOUND_SHUTDOOR = 23,
    SOUND_TPLEVEL = 24,
    SOUND_SCROLL = 25,
    SOUND_BUY = 26,
    SOUND_SELL = 27,
    SOUND_WARN = 28,
    SOUND_ROCKET = 29, /*!< (unused) Somebody's shooting rockets */
    SOUND_N_KILL = 30, /*!< The player kills a non-living/undead monster */
    SOUND_U_KILL = 31, /*!< (unused) The player kills a unique*/
    SOUND_QUEST = 32, /*!< (unused) The player has just completed a quest */
    SOUND_HEAL = 33, /*!< (unused) The player was healed a little bit */
    SOUND_X_HEAL = 34, /*!< (unused) The player was healed full health */
    SOUND_BITE = 35, /*!< A monster bites you */
    SOUND_CLAW = 36, /*!< A monster claws you */
    SOUND_M_SPELL = 37, /*!< (unused) A monster casts a miscellaneous spell */
    SOUND_SUMMON = 38, /*!< A monster casts a summoning spell  */
    SOUND_BREATH = 39, /*!< A monster breathes */
    SOUND_BALL = 40, /*!< (unused) A monster casts a ball / bolt spell */
    SOUND_M_HEAL = 41, /*!< (unused) A monster heals itself somehow */
    SOUND_ATK_SPELL = 42, /*!< (unused) A monster casts a misc. offensive spell */
    SOUND_EVIL = 43, /*!< Something nasty has just happened! */
    SOUND_TOUCH = 44, /*!< A monster touches you */
    SOUND_STING = 45, /*!< A monster stings you */
    SOUND_CRUSH = 46, /*!< A monster crushes / envelopes you */
    SOUND_SLIME = 47, /*!< A monster drools/spits/etc on you */
    SOUND_WAIL = 48, /*!< A monster wails */
    SOUND_WINNER = 49, /*!< (unused) Just won the game! */
    SOUND_FIRE = 50, /*!< (unused) An item was burned  */
    SOUND_ACID = 51, /*!< (unused) An item was destroyed by acid */
    SOUND_ELEC = 52, /*!< (unused) An item was destroyed by electricity */
    SOUND_COLD = 53, /*!< (unused) An item was shattered */
    SOUND_ILLEGAL = 54, /*!< Illegal command attempted */
    SOUND_FAIL = 55, /*!< Fail to get a spell off / activate an item */
    SOUND_WAKEUP = 56, /*!< (unused) A monster wakes up */
    SOUND_INVULN = 57, /*!< (unused) Invulnerability! */
    SOUND_FALL = 58, /*!< Falling through a trapdoor... */
    SOUND_PAIN = 59, /*!< A monster is in pain! */
    SOUND_DESTITEM = 60, /*!< An item was destroyed by misc. means */
    SOUND_MOAN = 61, /*!< A monster makes a moan/beg/insult attack */
    SOUND_SHOW = 62, /*!< A monster makes a "show" attack */
    SOUND_UNUSED = 63, /*!< (unused) (no sound for gaze attacks) */
    SOUND_EXPLODE = 64, /*!< Something (or somebody) explodes */
    SOUND_GLASS = 65, /*!< A glass feature was crashed */
    SOUND_REFLECT = 66, /*!< A bolt was reflected */
    SOUND_HUNGRY = 67, /*!< getting hungry */
    SOUND_WEAK = 68, /*!< getting weak from hunger */
    SOUND_FAINT = 69, /*!< getting faint from hunger */
    SOUND_GOOD_HIT = 70, /*!< critical hit - good */
    SOUND_GREAT_HIT = 71, /*!< critical hit - great */
    SOUND_SUPERB_HIT = 72, /*!< critical hit - superb */
    SOUND_STAR_GREAT_HIT = 73, /*!< critical hit - *great* */
    SOUND_STAR_SUPERB_HIT = 74, /*!< critical hit - *superb* */
    SOUND_GOUGE_HIT = 75, /*!< vorpal hit - gouge */
    SOUND_MAIM_HIT = 76, /*!< vorpal hit - maim */
    SOUND_CARVE_HIT = 77, /*!< vorpal hit - carve */
    SOUND_CLEAVE_HIT = 78, /*!< vorpal hit - cleave */
    SOUND_SMITE_HIT = 79, /*!< vorpal hit - smite */
    SOUND_EVISCERATE_HIT = 80, /*!< vorpal hit - eviscerate */
    SOUND_SHRED_HIT = 81, /*!< vorpal hit - shred */
    SOUND_MAX = 82, /*!< 効果音定義の最大数 / Maximum numbers of sound effect */
};

extern const concptr angband_sound_name[SOUND_MAX];
