#pragma once
/*!
 * @file sound-definitions-table.h
 * @brief 設定ファイル用の効果音名定義ヘッダ
 */

#include "system/angband.h"

enum sound_type {
    SOUND_HIT,
    SOUND_MISS,
    SOUND_FLEE,
    SOUND_DROP,
    SOUND_KILL,
    SOUND_LEVEL,
    SOUND_DEATH,
    SOUND_STUDY,
    SOUND_TELEPORT,
    SOUND_SHOOT,
    SOUND_SHOOT_HIT,
    SOUND_QUAFF,
    SOUND_ZAP,
    SOUND_WALK, /*!< unused */
    SOUND_TPOTHER,
    SOUND_HITWALL,
    SOUND_EAT,
    SOUND_STORE1,
    SOUND_STORE2,
    SOUND_STORE3,
    SOUND_STORE4,
    SOUND_DIG,
    SOUND_OPENDOOR,
    SOUND_SHUTDOOR,
    SOUND_TPLEVEL,
    SOUND_SCROLL,
    SOUND_BUY,
    SOUND_SELL,
    SOUND_WARN,
    SOUND_ROCKET, /*!< (unused) Somebody's shooting rockets */
    SOUND_N_KILL, /*!< The player kills a non-living/undead monster */
    SOUND_U_KILL, /*!< (unused) The player kills a unique*/
    SOUND_QUEST, /*!< (unused) The player has just completed a quest */
    SOUND_HEAL, /*!< (unused) The player was healed a little bit */
    SOUND_X_HEAL, /*!< (unused) The player was healed full health */
    SOUND_BITE, /*!< A monster bites you */
    SOUND_CLAW, /*!< A monster claws you */
    SOUND_M_SPELL, /*!< (unused) A monster casts a miscellaneous spell */
    SOUND_SUMMON, /*!< A monster casts a summoning spell  */
    SOUND_BREATH, /*!< A monster breathes */
    SOUND_BALL, /*!< (unused) A monster casts a ball / bolt spell */
    SOUND_M_HEAL, /*!< (unused) A monster heals itself somehow */
    SOUND_ATK_SPELL, /*!< (unused) A monster casts a misc. offensive spell */
    SOUND_EVIL, /*!< Something nasty has just happened! */
    SOUND_TOUCH, /*!< A monster touches you */
    SOUND_STING, /*!< A monster stings you */
    SOUND_CRUSH, /*!< A monster crushes / envelopes you */
    SOUND_SLIME, /*!< A monster drools/spits/etc on you */
    SOUND_WAIL, /*!< A monster wails */
    SOUND_WINNER, /*!< (unused) Just won the game! */
    SOUND_FIRE, /*!< (unused) An item was burned  */
    SOUND_ACID, /*!< (unused) An item was destroyed by acid */
    SOUND_ELEC, /*!< (unused) An item was destroyed by electricity */
    SOUND_COLD, /*!< (unused) An item was shattered */
    SOUND_ILLEGAL, /*!< Illegal command attempted */
    SOUND_FAIL, /*!< Fail to get a spell off / activate an item */
    SOUND_WAKEUP, /*!< (unused) A monster wakes up */
    SOUND_INVULN, /*!< (unused) Invulnerability! */
    SOUND_FALL, /*!< Falling through a trapdoor... */
    SOUND_PAIN, /*!< A monster is in pain! */
    SOUND_DESTITEM, /*!< An item was destroyed by misc. means */
    SOUND_MOAN, /*!< A monster makes a moan/beg/insult attack */
    SOUND_SHOW, /*!< A monster makes a "show" attack */
    SOUND_UNUSED, /*!< (unused) (no sound for gaze attacks) */
    SOUND_EXPLODE, /*!< Something (or somebody) explodes */
    SOUND_GLASS, /*!< A glass feature was crashed */
    SOUND_REFLECT, /*!< A bolt was reflected */
    SOUND_HUNGRY, /*!< getting hungry */
    SOUND_WEAK, /*!< getting weak from hunger */
    SOUND_FAINT, /*!< getting faint from hunger */
    SOUND_GOOD_HIT, /*!< critical hit - good */
    SOUND_GREAT_HIT, /*!< critical hit - great */
    SOUND_SUPERB_HIT, /*!< critical hit - superb */
    SOUND_STAR_GREAT_HIT, /*!< critical hit - *great* */
    SOUND_STAR_SUPERB_HIT, /*!< critical hit - *superb* */
    SOUND_GOUGE_HIT, /*!< vorpal hit - gouge */
    SOUND_MAIM_HIT, /*!< vorpal hit - maim */
    SOUND_CARVE_HIT, /*!< vorpal hit - carve */
    SOUND_CLEAVE_HIT, /*!< vorpal hit - cleave */
    SOUND_SMITE_HIT, /*!< vorpal hit - smite */
    SOUND_EVISCERATE_HIT, /*!< vorpal hit - eviscerate */
    SOUND_SHRED_HIT, /*!< vorpal hit - shred */
    SOUND_WIELD,
    SOUND_TAKE_OFF,
    SOUND_TERRAIN_DAMAGE,
    SOUND_DAMAGE_OVER_TIME,
    SOUND_STAIRWAY,
    SOUND_MAX, /*!< 効果音定義の最大数 / Maximum numbers of sound effect */
};

extern const concptr angband_sound_name[SOUND_MAX];
