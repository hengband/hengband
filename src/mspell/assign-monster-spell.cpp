/*!
 * @brief モンスターのスペル振り分け処理 / Spell launch by a monster
 * @date 2014/07/14
 * @author Habu
 */

#include "mspell/assign-monster-spell.h"
#include "blue-magic/blue-magic-checker.h"
#include "monster-race/race-ability-flags.h"
#include "monster/monster-status.h"
#include "mspell/mspell-attack/mspell-ball.h"
#include "mspell/mspell-attack/mspell-bolt.h"
#include "mspell/mspell-attack/mspell-breath.h"
#include "mspell/mspell-attack/mspell-curse.h"
#include "mspell/mspell-attack/mspell-particularity.h"
#include "mspell/mspell-dispel.h"
#include "mspell/mspell-floor.h"
#include "mspell/mspell-learn-checker.h"
#include "mspell/mspell-result.h"
#include "mspell/mspell-special.h"
#include "mspell/mspell-status.h"
#include "mspell/mspell-summon.h"
#include "mspell/mspell-util.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"

static MonsterSpellResult monspell_to_player_impl(PlayerType *player_ptr, MonsterAbilityType ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx)
{
    // clang-format off
    switch (ms_type) {
    case MonsterAbilityType::SHRIEK: return spell_RF4_SHRIEK(m_idx, player_ptr, 0, MONSTER_TO_PLAYER); /* RF4_SHRIEK */
    case MonsterAbilityType::XXX1: break;   /* RF4_XXX1 */
    case MonsterAbilityType::DISPEL: return spell_RF4_DISPEL(m_idx, player_ptr, 0, MONSTER_TO_PLAYER); /* RF4_DISPEL */
    case MonsterAbilityType::ROCKET: return spell_RF4_ROCKET(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER);  /* RF4_ROCKET */
    case MonsterAbilityType::XXX2: break;   /* RF4_XXX2 */
    case MonsterAbilityType::XXX3: break;   /* RF4_XXX3 */
    case MonsterAbilityType::XXX4: break;   /* RF4_XXX4 */

    case MonsterAbilityType::BR_ACID:
    case MonsterAbilityType::BR_ELEC:
    case MonsterAbilityType::BR_FIRE:
    case MonsterAbilityType::BR_COLD:
    case MonsterAbilityType::BR_POIS:
    case MonsterAbilityType::BR_NETH:
    case MonsterAbilityType::BR_LITE:
    case MonsterAbilityType::BR_DARK:
    case MonsterAbilityType::BR_CONF:
    case MonsterAbilityType::BR_SOUN:
    case MonsterAbilityType::BR_CHAO:
    case MonsterAbilityType::BR_DISE:
    case MonsterAbilityType::BR_NEXU:
    case MonsterAbilityType::BR_TIME:
    case MonsterAbilityType::BR_INER:
    case MonsterAbilityType::BR_GRAV:
    case MonsterAbilityType::BR_SHAR:
    case MonsterAbilityType::BR_PLAS:
    case MonsterAbilityType::BR_FORC:
    case MonsterAbilityType::BR_MANA:
    case MonsterAbilityType::BR_NUKE:
    case MonsterAbilityType::BR_DISI:
    case MonsterAbilityType::BR_VOID:
    case MonsterAbilityType::BR_ABYSS:
     return spell_RF4_BREATH(player_ptr, ms_type, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_ABYSS */
    
    case MonsterAbilityType::BA_ACID:
    case MonsterAbilityType::BA_ELEC:
    case MonsterAbilityType::BA_FIRE:
    case MonsterAbilityType::BA_COLD:
         {
         auto rad = monster_is_powerful(player_ptr->current_floor_ptr, m_idx) ? 4 : 2;
         return MSpellBall(player_ptr, m_idx, ms_type, rad, MONSTER_TO_PLAYER).shoot(y, x);
         }

    case MonsterAbilityType::BA_POIS:
    case MonsterAbilityType::BA_NETH:
    case MonsterAbilityType::BA_NUKE:
         return MSpellBall(player_ptr, m_idx, ms_type, 2, MONSTER_TO_PLAYER).shoot(y, x);

    case MonsterAbilityType::BA_CHAO:
    case MonsterAbilityType::BA_WATE:
    case MonsterAbilityType::BA_MANA:
    case MonsterAbilityType::BA_LITE:
    case MonsterAbilityType::BA_DARK:
    case MonsterAbilityType::BA_VOID:
    case MonsterAbilityType::BA_ABYSS:
         return MSpellBall(player_ptr, m_idx, ms_type, 4, MONSTER_TO_PLAYER).shoot(y, x);

    case MonsterAbilityType::DRAIN_MANA: return spell_RF5_DRAIN_MANA(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER);  /* RF5_DRAIN_MANA */
    case MonsterAbilityType::MIND_BLAST: return spell_RF5_MIND_BLAST(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER);  /* RF5_MIND_BLAST */
    case MonsterAbilityType::BRAIN_SMASH: return spell_RF5_BRAIN_SMASH(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_MIND_BLAST */

    case MonsterAbilityType::CAUSE_1:
    case MonsterAbilityType::CAUSE_2:
    case MonsterAbilityType::CAUSE_3:
    case MonsterAbilityType::CAUSE_4: 
        return spell_RF5_CAUSE(player_ptr, ms_type, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_CAUSE_* */

    case MonsterAbilityType::SHOOT:
    case MonsterAbilityType::BO_ACID:
    case MonsterAbilityType::BO_ELEC:
    case MonsterAbilityType::BO_FIRE:
    case MonsterAbilityType::BO_COLD:
    case MonsterAbilityType::BO_NETH:
    case MonsterAbilityType::BO_WATE:
    case MonsterAbilityType::BO_MANA:
    case MonsterAbilityType::BO_PLAS:
    case MonsterAbilityType::BO_ICEE:
    case MonsterAbilityType::BO_VOID:
    case MonsterAbilityType::BO_ABYSS:
    case MonsterAbilityType::MISSILE: 
        return MSpellBolt(player_ptr, m_idx, ms_type, MONSTER_TO_PLAYER).shoot(y,x);

    case MonsterAbilityType::SCARE: return spell_RF5_SCARE(m_idx, player_ptr, 0, MONSTER_TO_PLAYER); /* RF5_SCARE */
    case MonsterAbilityType::BLIND: return spell_RF5_BLIND(m_idx, player_ptr, 0, MONSTER_TO_PLAYER); /* RF5_BLIND */
    case MonsterAbilityType::CONF: return spell_RF5_CONF(m_idx, player_ptr, 0, MONSTER_TO_PLAYER); /* RF5_CONF */
    case MonsterAbilityType::SLOW: return spell_RF5_SLOW(m_idx, player_ptr, 0, MONSTER_TO_PLAYER); /* RF5_SLOW */
    case MonsterAbilityType::HOLD: return spell_RF5_HOLD(m_idx, player_ptr, 0, MONSTER_TO_PLAYER); /* RF5_HOLD */
    case MonsterAbilityType::HASTE: return spell_RF6_HASTE(player_ptr, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_HASTE */
    case MonsterAbilityType::HAND_DOOM: return spell_RF6_HAND_DOOM(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_HAND_DOOM */
    case MonsterAbilityType::HEAL: return spell_RF6_HEAL(player_ptr, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_HEAL */
    case MonsterAbilityType::INVULNER: return spell_RF6_INVULNER(player_ptr, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_INVULNER */
    case MonsterAbilityType::BLINK: return spell_RF6_BLINK(player_ptr, m_idx, MONSTER_TO_PLAYER, false); /* RF6_BLINK */
    case MonsterAbilityType::TPORT: return spell_RF6_TPORT(player_ptr, m_idx, MONSTER_TO_PLAYER); /* RF6_TPORT */
    case MonsterAbilityType::WORLD: return spell_RF6_WORLD(player_ptr, m_idx); /* RF6_WORLD */
    case MonsterAbilityType::SPECIAL: return spell_RF6_SPECIAL(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER);   /* RF6_SPECIAL */
    case MonsterAbilityType::TELE_TO: return spell_RF6_TELE_TO(player_ptr, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_TELE_TO */
    case MonsterAbilityType::TELE_AWAY: return spell_RF6_TELE_AWAY(player_ptr, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_TELE_AWAY */
    case MonsterAbilityType::TELE_LEVEL: return spell_RF6_TELE_LEVEL(player_ptr, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_TELE_LEVEL */
    case MonsterAbilityType::PSY_SPEAR: return spell_RF6_PSY_SPEAR(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_PSY_SPEAR */
    case MonsterAbilityType::DARKNESS: return spell_RF6_DARKNESS(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_DARKNESS */
    case MonsterAbilityType::TRAPS: return spell_RF6_TRAPS(player_ptr, y, x, m_idx); /* RF6_TRAPS */
    case MonsterAbilityType::FORGET: return spell_RF6_FORGET(player_ptr, m_idx); /* RF6_FORGET */
    case MonsterAbilityType::RAISE_DEAD: return spell_RF6_RAISE_DEAD(player_ptr, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_RAISE_DEAD */
    case MonsterAbilityType::S_KIN: return spell_RF6_S_KIN(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_S_KIN */
    case MonsterAbilityType::S_CYBER: return spell_RF6_S_CYBER(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_S_CYBER */
    case MonsterAbilityType::S_MONSTER: return spell_RF6_S_MONSTER(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_S_MONSTER */
    case MonsterAbilityType::S_MONSTERS: return spell_RF6_S_MONSTERS(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_S_MONSTER */
    case MonsterAbilityType::S_ANT: return spell_RF6_S_ANT(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_S_ANT */
    case MonsterAbilityType::S_SPIDER: return spell_RF6_S_SPIDER(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_S_SPIDER */
    case MonsterAbilityType::S_HOUND: return spell_RF6_S_HOUND(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_S_HOUND */
    case MonsterAbilityType::S_HYDRA: return spell_RF6_S_HYDRA(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_S_HYDRA */
    case MonsterAbilityType::S_ANGEL: return spell_RF6_S_ANGEL(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_S_ANGEL */
    case MonsterAbilityType::S_DEMON: return spell_RF6_S_DEMON(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_S_DEMON */
    case MonsterAbilityType::S_UNDEAD: return spell_RF6_S_UNDEAD(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_S_UNDEAD */
    case MonsterAbilityType::S_DRAGON: return spell_RF6_S_DRAGON(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_S_DRAGON */
    case MonsterAbilityType::S_HI_UNDEAD: return spell_RF6_S_HI_UNDEAD(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_S_HI_UNDEAD */
    case MonsterAbilityType::S_HI_DRAGON: return spell_RF6_S_HI_DRAGON(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_S_HI_DRAGON */
    case MonsterAbilityType::S_AMBERITES: return spell_RF6_S_AMBERITES(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_S_AMBERITES */
    case MonsterAbilityType::S_UNIQUE: return spell_RF6_S_UNIQUE(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_S_UNIQUE */
    case MonsterAbilityType::S_DEAD_UNIQUE: return spell_RF6_S_DEAD_UNIQUE(player_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_S_DEAD_UNIQUE */
    default: break;
    }
    // clang-format on

    return MonsterSpellResult::make_invalid();
}

static MonsterSpellResult monspell_to_monster_impl(
    PlayerType *player_ptr, MonsterAbilityType ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, bool is_special_spell)
{
    // clang-format off
    switch (ms_type) {
    case MonsterAbilityType::SHRIEK: return spell_RF4_SHRIEK(m_idx, player_ptr, t_idx, MONSTER_TO_MONSTER); /* RF4_SHRIEK */
    case MonsterAbilityType::XXX1: break;   /* RF4_XXX1 */
    case MonsterAbilityType::DISPEL: return spell_RF4_DISPEL(m_idx, player_ptr, t_idx, MONSTER_TO_MONSTER); /* RF4_DISPEL */
    case MonsterAbilityType::ROCKET: return spell_RF4_ROCKET(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF4_ROCKET */
    case MonsterAbilityType::XXX2: break;   /* RF4_XXX2 */
    case MonsterAbilityType::XXX3: break;   /* RF4_XXX3 */
    case MonsterAbilityType::XXX4: break;   /* RF4_XXX4 */

    case MonsterAbilityType::BR_ACID:
    case MonsterAbilityType::BR_ELEC:
    case MonsterAbilityType::BR_FIRE:
    case MonsterAbilityType::BR_COLD:
    case MonsterAbilityType::BR_POIS:
    case MonsterAbilityType::BR_NETH:
    case MonsterAbilityType::BR_LITE:
    case MonsterAbilityType::BR_DARK:
    case MonsterAbilityType::BR_CONF:
    case MonsterAbilityType::BR_SOUN:
    case MonsterAbilityType::BR_CHAO:
    case MonsterAbilityType::BR_DISE:
    case MonsterAbilityType::BR_NEXU:
    case MonsterAbilityType::BR_TIME:
    case MonsterAbilityType::BR_INER:
    case MonsterAbilityType::BR_GRAV:
    case MonsterAbilityType::BR_SHAR:
    case MonsterAbilityType::BR_PLAS:
    case MonsterAbilityType::BR_FORC:
    case MonsterAbilityType::BR_MANA:
    case MonsterAbilityType::BR_NUKE:
    case MonsterAbilityType::BR_DISI:
    case MonsterAbilityType::BR_VOID:
    case MonsterAbilityType::BR_ABYSS:
    return spell_RF4_BREATH(player_ptr, ms_type, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF4_BR_ABYSS */
    
    case MonsterAbilityType::BA_ACID:
    case MonsterAbilityType::BA_ELEC:
    case MonsterAbilityType::BA_FIRE:
    case MonsterAbilityType::BA_COLD:
         {
         auto rad = monster_is_powerful(player_ptr->current_floor_ptr, m_idx) ? 4 : 2;
         return MSpellBall(player_ptr, m_idx, t_idx, ms_type, rad, MONSTER_TO_MONSTER).shoot(y, x);
         }
         
    case MonsterAbilityType::BA_POIS:
    case MonsterAbilityType::BA_NETH:
    case MonsterAbilityType::BA_NUKE:
         return MSpellBall(player_ptr, m_idx, t_idx, ms_type, 2, MONSTER_TO_MONSTER).shoot(y, x);

    case MonsterAbilityType::BA_CHAO:
    case MonsterAbilityType::BA_WATE:
    case MonsterAbilityType::BA_MANA:
    case MonsterAbilityType::BA_LITE:
    case MonsterAbilityType::BA_DARK:
    case MonsterAbilityType::BA_VOID:
    case MonsterAbilityType::BA_ABYSS:
         return MSpellBall(player_ptr, m_idx, t_idx, ms_type, 4, MONSTER_TO_MONSTER).shoot(y, x);

    case MonsterAbilityType::DRAIN_MANA: return spell_RF5_DRAIN_MANA(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF5_DRAIN_MANA */
    case MonsterAbilityType::MIND_BLAST: return spell_RF5_MIND_BLAST(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF5_MIND_BLAST */
    case MonsterAbilityType::BRAIN_SMASH: return spell_RF5_BRAIN_SMASH(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF5_BRAIN_SMASH */

    case MonsterAbilityType::CAUSE_1:
    case MonsterAbilityType::CAUSE_2:
    case MonsterAbilityType::CAUSE_3:
    case MonsterAbilityType::CAUSE_4:
        return spell_RF5_CAUSE(player_ptr, ms_type, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF5_CAUSE_* */

    case MonsterAbilityType::SHOOT:
    case MonsterAbilityType::BO_ACID:
    case MonsterAbilityType::BO_ELEC:
    case MonsterAbilityType::BO_FIRE:
    case MonsterAbilityType::BO_COLD:
    case MonsterAbilityType::BO_NETH:
    case MonsterAbilityType::BO_WATE:
    case MonsterAbilityType::BO_MANA:
    case MonsterAbilityType::BO_PLAS:
    case MonsterAbilityType::BO_ICEE:
    case MonsterAbilityType::BO_VOID:
    case MonsterAbilityType::BO_ABYSS:
    case MonsterAbilityType::MISSILE: 
         return MSpellBolt(player_ptr, m_idx, t_idx, ms_type, MONSTER_TO_MONSTER).shoot(y, x);

    case MonsterAbilityType::SCARE: return spell_RF5_SCARE(m_idx, player_ptr, t_idx, MONSTER_TO_MONSTER); /* RF5_SCARE */
    case MonsterAbilityType::BLIND: return spell_RF5_BLIND(m_idx, player_ptr, t_idx, MONSTER_TO_MONSTER); /* RF5_BLIND */
    case MonsterAbilityType::CONF: return spell_RF5_CONF(m_idx, player_ptr, t_idx, MONSTER_TO_MONSTER); /* RF5_CONF */
    case MonsterAbilityType::SLOW: return spell_RF5_SLOW(m_idx, player_ptr, t_idx, MONSTER_TO_MONSTER); /* RF5_SLOW */
    case MonsterAbilityType::HOLD: return spell_RF5_HOLD(m_idx, player_ptr, t_idx, MONSTER_TO_MONSTER); /* RF5_HOLD */
    case MonsterAbilityType::HASTE: return spell_RF6_HASTE(player_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_HASTE */
    case MonsterAbilityType::HAND_DOOM: return spell_RF6_HAND_DOOM(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_HAND_DOOM */
    case MonsterAbilityType::HEAL: return spell_RF6_HEAL(player_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_HEAL */
    case MonsterAbilityType::INVULNER: return spell_RF6_INVULNER(player_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_INVULNER */
    case MonsterAbilityType::BLINK: return spell_RF6_BLINK(player_ptr, m_idx, MONSTER_TO_MONSTER, is_special_spell); /* RF6_BLINK */
    case MonsterAbilityType::TPORT: return spell_RF6_TPORT(player_ptr, m_idx, MONSTER_TO_MONSTER); /* RF6_TPORT */
    case MonsterAbilityType::WORLD: break; /* RF6_WORLD */
    case MonsterAbilityType::SPECIAL: return spell_RF6_SPECIAL(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);   /* RF6_SPECIAL */
    case MonsterAbilityType::TELE_TO: return spell_RF6_TELE_TO(player_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_TELE_TO */
    case MonsterAbilityType::TELE_AWAY: return spell_RF6_TELE_AWAY(player_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_TELE_AWAY */
    case MonsterAbilityType::TELE_LEVEL: return spell_RF6_TELE_LEVEL(player_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_TELE_LEVEL */
    case MonsterAbilityType::PSY_SPEAR: return spell_RF6_PSY_SPEAR(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_PSY_SPEAR */
    case MonsterAbilityType::DARKNESS: return spell_RF6_DARKNESS(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_DARKNESS */
    case MonsterAbilityType::TRAPS: break; /* RF6_TRAPS */
    case MonsterAbilityType::FORGET: break; /* RF6_FORGET */
    case MonsterAbilityType::RAISE_DEAD: return spell_RF6_RAISE_DEAD(player_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_RAISE_DEAD */
    case MonsterAbilityType::S_KIN: return spell_RF6_S_KIN(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_S_KIN */
    case MonsterAbilityType::S_CYBER: return spell_RF6_S_CYBER(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_S_CYBER */
    case MonsterAbilityType::S_MONSTER: return spell_RF6_S_MONSTER(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_S_MONSTER */
    case MonsterAbilityType::S_MONSTERS: return spell_RF6_S_MONSTERS(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_S_MONSTER */
    case MonsterAbilityType::S_ANT: return spell_RF6_S_ANT(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_S_ANT */
    case MonsterAbilityType::S_SPIDER: return spell_RF6_S_SPIDER(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_S_SPIDER */
    case MonsterAbilityType::S_HOUND: return spell_RF6_S_HOUND(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_S_HOUND */
    case MonsterAbilityType::S_HYDRA: return spell_RF6_S_HYDRA(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_S_HYDRA */
    case MonsterAbilityType::S_ANGEL: return spell_RF6_S_ANGEL(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_S_ANGEL */
    case MonsterAbilityType::S_DEMON: return spell_RF6_S_DEMON(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_S_DEMON */
    case MonsterAbilityType::S_UNDEAD: return spell_RF6_S_UNDEAD(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_S_UNDEAD */
    case MonsterAbilityType::S_DRAGON: return spell_RF6_S_DRAGON(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_S_DRAGON */
    case MonsterAbilityType::S_HI_UNDEAD: return spell_RF6_S_HI_UNDEAD(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_S_HI_UNDEAD */
    case MonsterAbilityType::S_HI_DRAGON: return spell_RF6_S_HI_DRAGON(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_S_HI_DRAGON */
    case MonsterAbilityType::S_AMBERITES: return spell_RF6_S_AMBERITES(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_S_AMBERITES */
    case MonsterAbilityType::S_UNIQUE: return spell_RF6_S_UNIQUE(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_S_UNIQUE */
    case MonsterAbilityType::S_DEAD_UNIQUE: return spell_RF6_S_DEAD_UNIQUE(player_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_S_DEAD_UNIQUE */
    default: break;
    }
    // clang-format on

    return MonsterSpellResult::make_invalid();
}

/*!
 * @brief モンスターからプレイヤーへの魔法使用。ラーニング処理も行う。
 * @param ms_type モンスター魔法ID (monster_spell_typeのenum値とは異なる)
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 */
MonsterSpellResult monspell_to_player(PlayerType *player_ptr, MonsterAbilityType ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx)
{
    // 特技使用前の時点でプレイヤーがモンスターを視認できているかチェック(ラーニングの必要条件)。
    const bool player_could_see_monster = spell_learnable(player_ptr, m_idx);

    auto res = monspell_to_player_impl(player_ptr, ms_type, y, x, m_idx);
    if (!player_could_see_monster) {
        res.learnable = false;
    }

    // 条件を満たしていればラーニングを試みる。
    if (res.valid && res.learnable) {
        learn_spell(player_ptr, ms_type);
    }

    return res;
}

/*!
 * @brief モンスターからモンスターへの魔法使用。ラーニング処理も行う。
 * @param player_ptr プレイヤーへの参照ポインタ (monster_spell_typeのenum値とは異なる)
 * @param ms_type モンスター魔法ID
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param is_special_spell 特殊な行動である時TRUE
 * @todo モンスターからモンスターへの呪文なのにPlayerTypeが引数になり得るのは間違っている……
 */
MonsterSpellResult monspell_to_monster(
    PlayerType *player_ptr, MonsterAbilityType ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, bool is_special_spell)
{
    // 特技使用前の時点でプレイヤーがモンスターを視認できているかチェック(ラーニングの必要条件)。
    const bool player_could_see_monster = spell_learnable(player_ptr, m_idx);

    auto res = monspell_to_monster_impl(player_ptr, ms_type, y, x, m_idx, t_idx, is_special_spell);
    if (!player_could_see_monster) {
        res.learnable = false;
    }

    // 条件を満たしていればラーニングを試みる。
    if (res.valid && res.learnable) {
        learn_spell(player_ptr, ms_type);
    }

    return res;
}
