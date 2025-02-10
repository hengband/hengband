#pragma once
/*!
 * @file sound-definitions-table.h
 * @brief 設定ファイル用の効果音名定義ヘッダ
 */

#include <map>
#include <string>

enum class SoundKind {
    HIT,
    ENEMY_HIT,
    MISS,
    ATTACK_FAILED,
    FLEE,
    DROP,
    KILL,
    LEVEL,
    DEATH,
    STUDY,
    TELEPORT,
    SHOOT,
    SHOOT_HIT,
    QUAFF,
    ZAP,
    WALK, /*!< unused */
    TPOTHER,
    HITWALL,
    EAT,
    STORE1,
    STORE2,
    STORE3,
    STORE4,
    DIG,
    DIG_THROUGH,
    OPENDOOR,
    SHUTDOOR,
    TPLEVEL,
    SCROLL,
    BUY,
    SELL,
    WARN,
    ROCKET, /*!< (unused) Somebody's shooting rockets */
    N_KILL, /*!< The player kills a non-living/undead monster */
    U_KILL, /*!< (unused) The player kills a unique*/
    QUEST, /*!< (unused) The player has just completed a quest */
    HEAL, /*!< (unused) The player was healed a little bit */
    X_HEAL, /*!< (unused) The player was healed full health */
    BITE, /*!< A monster bites you */
    CLAW, /*!< A monster claws you */
    M_SPELL, /*!< (unused) A monster casts a miscellaneous spell */
    SUMMON, /*!< A monster casts a summoning spell  */
    BREATH, /*!< A monster breathes */
    BALL, /*!< (unused) A monster casts a ball / bolt spell */
    M_HEAL, /*!< (unused) A monster heals itself somehow */
    ATK_SPELL, /*!< (unused) A monster casts a misc. offensive spell */
    EVIL, /*!< Something nasty has just happened! */
    TOUCH, /*!< A monster touches you */
    STING, /*!< A monster stings you */
    CRUSH, /*!< A monster crushes / envelopes you */
    SLIME, /*!< A monster drools/spits/etc on you */
    WAIL, /*!< A monster wails */
    WINNER, /*!< (unused) Just won the game! */
    FIRE, /*!< (unused) An item was burned  */
    ACID, /*!< (unused) An item was destroyed by acid */
    ELEC, /*!< (unused) An item was destroyed by electricity */
    COLD, /*!< (unused) An item was shattered */
    ILLEGAL, /*!< Illegal command attempted */
    FAIL, /*!< Fail to get a spell off / activate an item */
    WAKEUP, /*!< (unused) A monster wakes up */
    INVULN, /*!< (unused) Invulnerability! */
    FALL, /*!< Falling through a trapdoor... */
    PAIN, /*!< A monster is in pain! */
    DESTITEM, /*!< An item was destroyed by misc. means */
    MOAN, /*!< A monster makes a moan/beg/insult attack */
    SHOW, /*!< A monster makes a "show" attack */
    UNUSED, /*!< (unused) (no sound for gaze attacks) */
    EXPLODE, /*!< Something (or somebody) explodes */
    GLASS, /*!< A glass feature was crashed */
    REFLECT, /*!< A bolt was reflected */
    HUNGRY, /*!< getting hungry */
    WEAK, /*!< getting weak from hunger */
    FAINT, /*!< getting faint from hunger */
    GOOD_HIT, /*!< critical hit - good */
    GREAT_HIT, /*!< critical hit - great */
    SUPERB_HIT, /*!< critical hit - superb */
    STAR_GREAT_HIT, /*!< critical hit - *great* */
    STAR_SUPERB_HIT, /*!< critical hit - *superb* */
    GOUGE_HIT, /*!< vorpal hit - gouge */
    MAIM_HIT, /*!< vorpal hit - maim */
    CARVE_HIT, /*!< vorpal hit - carve */
    CLEAVE_HIT, /*!< vorpal hit - cleave */
    SMITE_HIT, /*!< vorpal hit - smite */
    EVISCERATE_HIT, /*!< vorpal hit - eviscerate */
    SHRED_HIT, /*!< vorpal hit - shred */
    WIELD,
    TAKE_OFF,
    TERRAIN_DAMAGE,
    DAMAGE_OVER_TIME,
    STAIRWAY,
    MAX, /*!< 効果音定義の最大数 / Maximum numbers of sound effect */
};

extern const std::map<SoundKind, std::string> sound_names;
