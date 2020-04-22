#pragma once

#define SOUND_HIT        1
#define SOUND_MISS       2
#define SOUND_FLEE       3
#define SOUND_DROP       4
#define SOUND_KILL       5
#define SOUND_LEVEL      6
#define SOUND_DEATH      7
#define SOUND_STUDY      8
#define SOUND_TELEPORT   9
#define SOUND_SHOOT     10
#define SOUND_QUAFF     11
#define SOUND_ZAP       12
#define SOUND_WALK      13
#define SOUND_TPOTHER   14
#define SOUND_HITWALL   15
#define SOUND_EAT       16
#define SOUND_STORE1    17
#define SOUND_STORE2    18
#define SOUND_STORE3    19
#define SOUND_STORE4    20
#define SOUND_DIG       21
#define SOUND_OPENDOOR  22
#define SOUND_SHUTDOOR  23
#define SOUND_TPLEVEL   24
#define SOUND_SCROLL	25
#define SOUND_BUY	    26
#define SOUND_SELL	    27
#define SOUND_WARN	    28
#define SOUND_ROCKET    29 /*!< Somebody's shooting rockets */
#define SOUND_N_KILL    30 /*!< The player kills a non-living/undead monster */
#define SOUND_U_KILL    31 /*!< The player kills a unique */
#define SOUND_QUEST     32 /*!< The player has just completed a quest */
#define SOUND_HEAL      33 /*!< The player was healed a little bit */
#define SOUND_X_HEAL    34 /*!< The player was healed full health */
#define SOUND_BITE      35 /*!< A monster bites you */
#define SOUND_CLAW      36 /*!< A monster claws you */
#define SOUND_M_SPELL   37 /*!< A monster casts a miscellaneous spell */
#define SOUND_SUMMON    38 /*!< A monster casts a summoning spell  */
#define SOUND_BREATH    39 /*!< A monster breathes */
#define SOUND_BALL      40 /*!< A monster casts a ball / bolt spell */
#define SOUND_M_HEAL    41 /*!< A monster heals itself somehow */
#define SOUND_ATK_SPELL 42 /*!< A monster casts a misc. offensive spell */
#define SOUND_EVIL      43 /*!< Something nasty has just happened! */
#define SOUND_TOUCH     44 /*!< A monster touches you */
#define SOUND_STING     45 /*!< A monster stings you */
#define SOUND_CRUSH     46 /*!< A monster crushes / envelopes you */
#define SOUND_SLIME     47 /*!< A monster drools/spits/etc on you */
#define SOUND_WAIL      48 /*!< A monster wails */
#define SOUND_WINNER    49 /*!< Just won the game! */
#define SOUND_FIRE      50 /*!< An item was burned  */
#define SOUND_ACID      51 /*!< An item was destroyed by acid */
#define SOUND_ELEC      52 /*!< An item was destroyed by electricity */
#define SOUND_COLD      53 /*!< An item was shattered */
#define SOUND_ILLEGAL   54 /*!< Illegal command attempted */
#define SOUND_FAIL      55 /*!< Fail to get a spell off / activate an item */
#define SOUND_WAKEUP    56 /*!< A monster wakes up */
#define SOUND_INVULN    57 /*!< Invulnerability! */
#define SOUND_FALL      58 /*!< Falling through a trapdoor... */
#define SOUND_PAIN      59 /*!< A monster is in pain! */
#define SOUND_DESTITEM  60 /*!< An item was destroyed by misc. means */
#define SOUND_MOAN      61 /*!< A monster makes a moan/beg/insult attack */
#define SOUND_SHOW      62 /*!< A monster makes a "show" attack */
#define SOUND_UNUSED    63 /*!< (no sound for gaze attacks) */
#define SOUND_EXPLODE   64 /*!< Something (or somebody) explodes */
#define SOUND_GLASS     65 /*!< A glass feature was crashed */
#define SOUND_REFLECT   66 /*!< A bolt was reflected */
#define SOUND_MAX 67 /*!< 効果音定義の最大数 / Maximum numbers of sound effect */

extern const concptr angband_sound_name[SOUND_MAX];
