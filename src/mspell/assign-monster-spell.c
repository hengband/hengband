/*!
 * @brief モンスターのスペル振り分け処理 / Spell launch by a monster
 * @date 2014/07/14
 * @author Habu
 */

#include "mspell/assign-monster-spell.h"
#include "mspell/mspell-ball.h"
#include "mspell/mspell-bolt.h"
#include "mspell/mspell-breath.h"
#include "mspell/mspell-curse.h"
#include "mspell/mspell-dispel.h"
#include "mspell/mspell-floor.h"
#include "mspell/mspell-particularity.h"
#include "mspell/mspell-special.h"
#include "mspell/mspell-status.h"
#include "mspell/mspell-summon.h"
#include "mspell/mspell-type.h"
#include "mspell/mspell-util.h"
#include "spell/spell-types.h"

/*!
 * @brief モンスターからプレイヤーへの呪文の振り分け関数。 /
 * @param SPELL_NUM モンスター魔法ID (monster_spell_typeのenum値とは異なる)
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @return 攻撃呪文のダメージ、または召喚したモンスターの数を返す。その他の場合0。以降の処理を中断するなら-1を返す。
 */
HIT_POINT monspell_to_player(player_type *target_ptr, SPELL_IDX ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx)
{
    switch (ms_type)
	{
	case RF4_SPELL_START + 0:   spell_RF4_SHRIEK(m_idx, target_ptr, 0, MONSTER_TO_PLAYER); break;	/* RF4_SHRIEK */
	case RF4_SPELL_START + 1:   break;   /* RF4_XXX1 */
	case RF4_SPELL_START + 2:   spell_RF4_DISPEL(m_idx, target_ptr, 0, MONSTER_TO_PLAYER); break;	/* RF4_DISPEL */
	case RF4_SPELL_START + 3:   return spell_RF4_ROCKET(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER);  /* RF4_ROCKET */
	case RF4_SPELL_START + 4:   return spell_RF4_SHOOT(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER);   /* RF4_SHOOT */
	case RF4_SPELL_START + 5:   break;   /* RF4_XXX2 */
	case RF4_SPELL_START + 6:   break;   /* RF4_XXX3 */
	case RF4_SPELL_START + 7:   break;   /* RF4_XXX4 */
	case RF4_SPELL_START + 8:   return spell_RF4_BREATH(target_ptr, GF_ACID, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_ACID */
	case RF4_SPELL_START + 9:   return spell_RF4_BREATH(target_ptr, GF_ELEC, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_ELEC */
	case RF4_SPELL_START + 10:  return spell_RF4_BREATH(target_ptr, GF_FIRE, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_FIRE */
	case RF4_SPELL_START + 11:  return spell_RF4_BREATH(target_ptr, GF_COLD, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_COLD */
	case RF4_SPELL_START + 12:  return spell_RF4_BREATH(target_ptr, GF_POIS, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_POIS */
	case RF4_SPELL_START + 13:  return spell_RF4_BREATH(target_ptr, GF_NETHER, y, x, m_idx, 0, MONSTER_TO_PLAYER);   /* RF4_BR_NETH */
	case RF4_SPELL_START + 14:  return spell_RF4_BREATH(target_ptr, GF_LITE, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_LITE */
	case RF4_SPELL_START + 15:  return spell_RF4_BREATH(target_ptr, GF_DARK, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_DARK */
	case RF4_SPELL_START + 16:  return spell_RF4_BREATH(target_ptr, GF_CONFUSION, y, x, m_idx, 0, MONSTER_TO_PLAYER);	/* RF4_BR_CONF */
	case RF4_SPELL_START + 17:  return spell_RF4_BREATH(target_ptr, GF_SOUND, y, x, m_idx, 0, MONSTER_TO_PLAYER);	/* RF4_BR_SOUN */
	case RF4_SPELL_START + 18:  return spell_RF4_BREATH(target_ptr, GF_CHAOS, y, x, m_idx, 0, MONSTER_TO_PLAYER);	/* RF4_BR_CHAO */
	case RF4_SPELL_START + 19:  return spell_RF4_BREATH(target_ptr, GF_DISENCHANT, y, x, m_idx, 0, MONSTER_TO_PLAYER);   /* RF4_BR_DISE */
	case RF4_SPELL_START + 20:  return spell_RF4_BREATH(target_ptr, GF_NEXUS, y, x, m_idx, 0, MONSTER_TO_PLAYER);	/* RF4_BR_NEXU */
	case RF4_SPELL_START + 21:  return spell_RF4_BREATH(target_ptr, GF_TIME, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_TIME */
	case RF4_SPELL_START + 22:  return spell_RF4_BREATH(target_ptr, GF_INERTIAL, y, x, m_idx, 0, MONSTER_TO_PLAYER);  /* RF4_BR_INER */
	case RF4_SPELL_START + 23:  return spell_RF4_BREATH(target_ptr, GF_GRAVITY, y, x, m_idx, 0, MONSTER_TO_PLAYER);  /* RF4_BR_GRAV */
	case RF4_SPELL_START + 24:  return spell_RF4_BREATH(target_ptr, GF_SHARDS, y, x, m_idx, 0, MONSTER_TO_PLAYER);   /* RF4_BR_SHAR */
	case RF4_SPELL_START + 25:  return spell_RF4_BREATH(target_ptr, GF_PLASMA, y, x, m_idx, 0, MONSTER_TO_PLAYER);   /* RF4_BR_PLAS */
	case RF4_SPELL_START + 26:  return spell_RF4_BREATH(target_ptr, GF_FORCE, y, x, m_idx, 0, MONSTER_TO_PLAYER);	/* RF4_BR_WALL */
	case RF4_SPELL_START + 27:  return spell_RF4_BREATH(target_ptr, GF_MANA, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_MANA */
	case RF4_SPELL_START + 28:  return spell_RF4_BA_NUKE(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BA_NUKE */
	case RF4_SPELL_START + 29:  return spell_RF4_BREATH(target_ptr, GF_NUKE, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_NUKE */
	case RF4_SPELL_START + 30:  return spell_RF4_BA_CHAO(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BA_CHAO */
	case RF4_SPELL_START + 31:  return spell_RF4_BREATH(target_ptr, GF_DISINTEGRATE, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF4_BR_DISI */
	case RF5_SPELL_START + 0:  return spell_RF5_BA_ACID(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_ACID */
	case RF5_SPELL_START + 1:  return spell_RF5_BA_ELEC(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_ELEC */
	case RF5_SPELL_START + 2:  return spell_RF5_BA_FIRE(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_FIRE */
	case RF5_SPELL_START + 3:  return spell_RF5_BA_COLD(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_COLD */
	case RF5_SPELL_START + 4:  return spell_RF5_BA_POIS(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_POIS */
	case RF5_SPELL_START + 5:  return spell_RF5_BA_NETH(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_NETH */
	case RF5_SPELL_START + 6:  return spell_RF5_BA_WATE(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_WATE */
	case RF5_SPELL_START + 7:  return spell_RF5_BA_MANA(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_MANA */
	case RF5_SPELL_START + 8:  return spell_RF5_BA_DARK(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_DARK */
	case RF5_SPELL_START + 9:  return spell_RF5_DRAIN_MANA(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER);  /* RF5_DRAIN_MANA */
	case RF5_SPELL_START + 10: return spell_RF5_MIND_BLAST(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER);  /* RF5_MIND_BLAST */
	case RF5_SPELL_START + 11: return spell_RF5_BRAIN_SMASH(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_MIND_BLAST */
	case RF5_SPELL_START + 12: return spell_RF5_CAUSE_1(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_CAUSE_1 */
	case RF5_SPELL_START + 13: return spell_RF5_CAUSE_2(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_CAUSE_2 */
	case RF5_SPELL_START + 14: return spell_RF5_CAUSE_3(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_CAUSE_3 */
	case RF5_SPELL_START + 15: return spell_RF5_CAUSE_4(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_CAUSE_4 */
	case RF5_SPELL_START + 16: return spell_RF5_BO_ACID(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BO_ACID */
	case RF5_SPELL_START + 17: return spell_RF5_BO_ELEC(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BO_ELEC */
	case RF5_SPELL_START + 18: return spell_RF5_BO_FIRE(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BO_FIRE */
	case RF5_SPELL_START + 19: return spell_RF5_BO_COLD(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BO_COLD */
	case RF5_SPELL_START + 20: return spell_RF5_BA_LITE(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BA_LITE */
	case RF5_SPELL_START + 21: return spell_RF5_BO_NETH(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BO_NETH */
	case RF5_SPELL_START + 22: return spell_RF5_BO_WATE(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BO_WATE */
	case RF5_SPELL_START + 23: return spell_RF5_BO_MANA(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BO_MANA */
	case RF5_SPELL_START + 24: return spell_RF5_BO_PLAS(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BO_PLAS */
	case RF5_SPELL_START + 25: return spell_RF5_BO_ICEE(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_BO_ICEE */
	case RF5_SPELL_START + 26: return spell_RF5_MISSILE(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF5_MISSILE */
	case RF5_SPELL_START + 27: spell_RF5_SCARE(m_idx, target_ptr, 0, MONSTER_TO_PLAYER); break;   /* RF5_SCARE */
	case RF5_SPELL_START + 28: spell_RF5_BLIND(m_idx, target_ptr, 0, MONSTER_TO_PLAYER); break;   /* RF5_BLIND */
	case RF5_SPELL_START + 29: spell_RF5_CONF(m_idx, target_ptr, 0, MONSTER_TO_PLAYER); break;  /* RF5_CONF */
	case RF5_SPELL_START + 30: spell_RF5_SLOW(m_idx, target_ptr, 0, MONSTER_TO_PLAYER); break;  /* RF5_SLOW */
	case RF5_SPELL_START + 31: spell_RF5_HOLD(m_idx, target_ptr, 0, MONSTER_TO_PLAYER); break;  /* RF5_HOLD */
	case RF6_SPELL_START + 0:  spell_RF6_HASTE(target_ptr, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_HASTE */
	case RF6_SPELL_START + 1:  return spell_RF6_HAND_DOOM(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); /* RF6_HAND_DOOM */
	case RF6_SPELL_START + 2:  spell_RF6_HEAL(target_ptr, m_idx, 0, MONSTER_TO_PLAYER); break;	/* RF6_HEAL */
	case RF6_SPELL_START + 3:  spell_RF6_INVULNER(target_ptr, m_idx, 0, MONSTER_TO_PLAYER); break;	/* RF6_INVULNER */
	case RF6_SPELL_START + 4:  spell_RF6_BLINK(target_ptr, m_idx, MONSTER_TO_PLAYER, FALSE); break;   /* RF6_BLINK */
	case RF6_SPELL_START + 5:  spell_RF6_TPORT(target_ptr, m_idx, MONSTER_TO_PLAYER); break;   /* RF6_TPORT */
	case RF6_SPELL_START + 6:  return spell_RF6_WORLD(target_ptr, m_idx); break;	/* RF6_WORLD */
	case RF6_SPELL_START + 7:  return spell_RF6_SPECIAL(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER);   /* RF6_SPECIAL */
	case RF6_SPELL_START + 8:  spell_RF6_TELE_TO(target_ptr, m_idx, 0, MONSTER_TO_PLAYER); break; /* RF6_TELE_TO */
	case RF6_SPELL_START + 9:  spell_RF6_TELE_AWAY(target_ptr, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_TELE_AWAY */
	case RF6_SPELL_START + 10: spell_RF6_TELE_LEVEL(target_ptr, m_idx, 0, MONSTER_TO_PLAYER); break;  /* RF6_TELE_LEVEL */
	case RF6_SPELL_START + 11: spell_RF6_PSY_SPEAR(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break; /* RF6_PSY_SPEAR */
	case RF6_SPELL_START + 12: spell_RF6_DARKNESS(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;	/* RF6_DARKNESS */
	case RF6_SPELL_START + 13: spell_RF6_TRAPS(target_ptr, y, x, m_idx); break; /* RF6_TRAPS */
	case RF6_SPELL_START + 14: spell_RF6_FORGET(target_ptr, m_idx); break;  /* RF6_FORGET */
	case RF6_SPELL_START + 15: spell_RF6_RAISE_DEAD(target_ptr, m_idx, 0, MONSTER_TO_PLAYER); break;  /* RF6_RAISE_DEAD */
	case RF6_SPELL_START + 16: spell_RF6_S_KIN(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break; /* RF6_S_KIN */
	case RF6_SPELL_START + 17: spell_RF6_S_CYBER(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_S_CYBER */
	case RF6_SPELL_START + 18: spell_RF6_S_MONSTER(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break; /* RF6_S_MONSTER */
	case RF6_SPELL_START + 19: spell_RF6_S_MONSTERS(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;	/* RF6_S_MONSTER */
	case RF6_SPELL_START + 20: spell_RF6_S_ANT(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break; /* RF6_S_ANT */
	case RF6_SPELL_START + 21: spell_RF6_S_SPIDER(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;  /* RF6_S_SPIDER */
	case RF6_SPELL_START + 22: spell_RF6_S_HOUND(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_S_HOUND */
	case RF6_SPELL_START + 23: spell_RF6_S_HYDRA(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_S_HYDRA */
	case RF6_SPELL_START + 24: spell_RF6_S_ANGEL(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_S_ANGEL */
	case RF6_SPELL_START + 25: spell_RF6_S_DEMON(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_S_DEMON */
	case RF6_SPELL_START + 26: spell_RF6_S_UNDEAD(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;  /* RF6_S_UNDEAD */
	case RF6_SPELL_START + 27: spell_RF6_S_DRAGON(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;  /* RF6_S_DRAGON */
	case RF6_SPELL_START + 28: spell_RF6_S_HI_UNDEAD(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_S_HI_UNDEAD */
	case RF6_SPELL_START + 29: spell_RF6_S_HI_DRAGON(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_S_HI_DRAGON */
	case RF6_SPELL_START + 30: spell_RF6_S_AMBERITES(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;   /* RF6_S_AMBERITES */
	case RF6_SPELL_START + 31: spell_RF6_S_UNIQUE(target_ptr, y, x, m_idx, 0, MONSTER_TO_PLAYER); break;  /* RF6_S_UNIQUE */
	}

	return 0;
}


/*!
 * todo モンスターからモンスターへの呪文なのにplayer_typeが引数になり得るのは間違っている……
 * @brief モンスターからモンスターへの呪文の振り分け関数。 /
 * @param target_ptr プレーヤーへの参照ポインタ (monster_spell_typeのenum値とは異なる)
 * @param SPELL_NUM モンスター魔法ID
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param is_special_spell 特殊な行動である時TRUE
 * @return 攻撃呪文のダメージ、または召喚したモンスターの数を返す。その他の場合0。以降の処理を中断するなら-1を返す。
 */
HIT_POINT monspell_to_monster(player_type *target_ptr, SPELL_IDX ms_type, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, bool is_special_spell)
{
    switch (ms_type)
	{
	case RF4_SPELL_START + 0:   spell_RF4_SHRIEK(m_idx, target_ptr, t_idx, MONSTER_TO_MONSTER); break;   /* RF4_SHRIEK */
	case RF4_SPELL_START + 1:   return -1;   /* RF4_XXX1 */
	case RF4_SPELL_START + 2:   spell_RF4_DISPEL(m_idx, target_ptr, t_idx, MONSTER_TO_MONSTER); break;   /* RF4_DISPEL */
	case RF4_SPELL_START + 3:   return spell_RF4_ROCKET(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF4_ROCKET */
	case RF4_SPELL_START + 4:   return spell_RF4_SHOOT(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);  /* RF4_SHOOT */
	case RF4_SPELL_START + 5:   return -1;   /* RF4_XXX2 */
	case RF4_SPELL_START + 6:   return -1;   /* RF4_XXX3 */
	case RF4_SPELL_START + 7:   return -1;   /* RF4_XXX4 */
	case RF4_SPELL_START + 8:   return spell_RF4_BREATH(target_ptr, GF_ACID, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_ACID */
	case RF4_SPELL_START + 9:   return spell_RF4_BREATH(target_ptr, GF_ELEC, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_ELEC */
	case RF4_SPELL_START + 10:  return spell_RF4_BREATH(target_ptr, GF_FIRE, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_FIRE */
	case RF4_SPELL_START + 11:  return spell_RF4_BREATH(target_ptr, GF_COLD, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_COLD */
	case RF4_SPELL_START + 12:  return spell_RF4_BREATH(target_ptr, GF_POIS, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_POIS */
	case RF4_SPELL_START + 13:  return spell_RF4_BREATH(target_ptr, GF_NETHER, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);  /* RF4_BR_NETH */
	case RF4_SPELL_START + 14:  return spell_RF4_BREATH(target_ptr, GF_LITE, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_LITE */
	case RF4_SPELL_START + 15:  return spell_RF4_BREATH(target_ptr, GF_DARK, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_DARK */
	case RF4_SPELL_START + 16:  return spell_RF4_BREATH(target_ptr, GF_CONFUSION, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);   /* RF4_BR_CONF */
	case RF4_SPELL_START + 17:  return spell_RF4_BREATH(target_ptr, GF_SOUND, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);   /* RF4_BR_SOUN */
	case RF4_SPELL_START + 18:  return spell_RF4_BREATH(target_ptr, GF_CHAOS, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);   /* RF4_BR_CHAO */
	case RF4_SPELL_START + 19:  return spell_RF4_BREATH(target_ptr, GF_DISENCHANT, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);  /* RF4_BR_DISE */
	case RF4_SPELL_START + 20:  return spell_RF4_BREATH(target_ptr, GF_NEXUS, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);   /* RF4_BR_NEXU */
	case RF4_SPELL_START + 21:  return spell_RF4_BREATH(target_ptr, GF_TIME, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_TIME */
	case RF4_SPELL_START + 22:  return spell_RF4_BREATH(target_ptr, GF_INERTIAL, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF4_BR_INER */
	case RF4_SPELL_START + 23:  return spell_RF4_BREATH(target_ptr, GF_GRAVITY, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF4_BR_GRAV */
	case RF4_SPELL_START + 24:  return spell_RF4_BREATH(target_ptr, GF_SHARDS, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);  /* RF4_BR_SHAR */
	case RF4_SPELL_START + 25:  return spell_RF4_BREATH(target_ptr, GF_PLASMA, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);  /* RF4_BR_PLAS */
	case RF4_SPELL_START + 26:  return spell_RF4_BREATH(target_ptr, GF_FORCE, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);   /* RF4_BR_WALL */
	case RF4_SPELL_START + 27:  return spell_RF4_BREATH(target_ptr, GF_MANA, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_MANA */
	case RF4_SPELL_START + 28:  return spell_RF4_BA_NUKE(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BA_NUKE */
	case RF4_SPELL_START + 29:  return spell_RF4_BREATH(target_ptr, GF_NUKE, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_NUKE */
	case RF4_SPELL_START + 30:  return spell_RF4_BA_CHAO(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BA_CHAO */
	case RF4_SPELL_START + 31:  return spell_RF4_BREATH(target_ptr, GF_DISINTEGRATE, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF4_BR_DISI */
	case RF5_SPELL_START + 0:  return spell_RF5_BA_ACID(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_ACID */
	case RF5_SPELL_START + 1:  return spell_RF5_BA_ELEC(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_ELEC */
	case RF5_SPELL_START + 2:  return spell_RF5_BA_FIRE(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_FIRE */
	case RF5_SPELL_START + 3:  return spell_RF5_BA_COLD(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_COLD */
	case RF5_SPELL_START + 4:  return spell_RF5_BA_POIS(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_POIS */
	case RF5_SPELL_START + 5:  return spell_RF5_BA_NETH(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_NETH */
	case RF5_SPELL_START + 6:  return spell_RF5_BA_WATE(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_WATE */
	case RF5_SPELL_START + 7:  return spell_RF5_BA_MANA(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_MANA */
	case RF5_SPELL_START + 8:  return spell_RF5_BA_DARK(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_DARK */
	case RF5_SPELL_START + 9:  return spell_RF5_DRAIN_MANA(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF5_DRAIN_MANA */
	case RF5_SPELL_START + 10: return spell_RF5_MIND_BLAST(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF5_MIND_BLAST */
	case RF5_SPELL_START + 11: return spell_RF5_BRAIN_SMASH(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BRAIN_SMASH */
	case RF5_SPELL_START + 12: return spell_RF5_CAUSE_1(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_CAUSE_1 */
	case RF5_SPELL_START + 13: return spell_RF5_CAUSE_2(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_CAUSE_2 */
	case RF5_SPELL_START + 14: return spell_RF5_CAUSE_3(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_CAUSE_3 */
	case RF5_SPELL_START + 15: return spell_RF5_CAUSE_4(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_CAUSE_4 */
	case RF5_SPELL_START + 16: return spell_RF5_BO_ACID(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BO_ACID */
	case RF5_SPELL_START + 17: return spell_RF5_BO_ELEC(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BO_ELEC */
	case RF5_SPELL_START + 18: return spell_RF5_BO_FIRE(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BO_FIRE */
	case RF5_SPELL_START + 19: return spell_RF5_BO_COLD(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BO_COLD */
	case RF5_SPELL_START + 20: return spell_RF5_BA_LITE(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BA_LITE */
	case RF5_SPELL_START + 21: return spell_RF5_BO_NETH(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BO_NETH */
	case RF5_SPELL_START + 22: return spell_RF5_BO_WATE(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BO_WATE */
	case RF5_SPELL_START + 23: return spell_RF5_BO_MANA(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BO_MANA */
	case RF5_SPELL_START + 24: return spell_RF5_BO_PLAS(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BO_PLAS */
	case RF5_SPELL_START + 25: return spell_RF5_BO_ICEE(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_BO_ICEE */
	case RF5_SPELL_START + 26: return spell_RF5_MISSILE(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);	/* RF5_MISSILE */
	case RF5_SPELL_START + 27: spell_RF5_SCARE(m_idx, target_ptr, t_idx, MONSTER_TO_MONSTER); break;  /* RF5_SCARE */
	case RF5_SPELL_START + 28: spell_RF5_BLIND(m_idx, target_ptr, t_idx, MONSTER_TO_MONSTER); break;  /* RF5_BLIND */
	case RF5_SPELL_START + 29: spell_RF5_CONF(m_idx, target_ptr, t_idx, MONSTER_TO_MONSTER); break;   /* RF5_CONF */
	case RF5_SPELL_START + 30: spell_RF5_SLOW(m_idx, target_ptr, t_idx, MONSTER_TO_MONSTER); break;   /* RF5_SLOW */
	case RF5_SPELL_START + 31: spell_RF5_HOLD(m_idx, target_ptr, t_idx, MONSTER_TO_MONSTER); break;  /* RF5_HOLD */
	case RF6_SPELL_START + 0:  spell_RF6_HASTE(target_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_HASTE */
	case RF6_SPELL_START + 1:  return spell_RF6_HAND_DOOM(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); /* RF6_HAND_DOOM */
	case RF6_SPELL_START + 2:  spell_RF6_HEAL(target_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); break;	/* RF6_HEAL */
	case RF6_SPELL_START + 3:  spell_RF6_INVULNER(target_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); break;	/* RF6_INVULNER */
	case RF6_SPELL_START + 4:  spell_RF6_BLINK(target_ptr, m_idx, MONSTER_TO_MONSTER, is_special_spell); break;   /* RF6_BLINK */
	case RF6_SPELL_START + 5:  spell_RF6_TPORT(target_ptr, m_idx, MONSTER_TO_MONSTER); break;   /* RF6_TPORT */
	case RF6_SPELL_START + 6:  return -1; break;	/* RF6_WORLD */
	case RF6_SPELL_START + 7:  return spell_RF6_SPECIAL(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER);   /* RF6_SPECIAL */
	case RF6_SPELL_START + 8:  spell_RF6_TELE_TO(target_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF6_TELE_TO */
	case RF6_SPELL_START + 9:  spell_RF6_TELE_AWAY(target_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_TELE_AWAY */
	case RF6_SPELL_START + 10: spell_RF6_TELE_LEVEL(target_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); break;  /* RF6_TELE_LEVEL */
	case RF6_SPELL_START + 11: return spell_RF6_PSY_SPEAR(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF6_PSY_SPEAR */
	case RF6_SPELL_START + 12: spell_RF6_DARKNESS(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;	/* RF6_DARKNESS */
	case RF6_SPELL_START + 13: return -1; /* RF6_TRAPS */
	case RF6_SPELL_START + 14: return -1;  /* RF6_FORGET */
	case RF6_SPELL_START + 15: spell_RF6_RAISE_DEAD(target_ptr, m_idx, t_idx, MONSTER_TO_MONSTER); break;  /* RF6_RAISE_DEAD */
	case RF6_SPELL_START + 16: spell_RF6_S_KIN(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF6_S_KIN */
	case RF6_SPELL_START + 17: spell_RF6_S_CYBER(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_S_CYBER */
	case RF6_SPELL_START + 18: spell_RF6_S_MONSTER(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF6_S_MONSTER */
	case RF6_SPELL_START + 19: spell_RF6_S_MONSTERS(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;	/* RF6_S_MONSTER */
	case RF6_SPELL_START + 20: spell_RF6_S_ANT(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF6_S_ANT */
	case RF6_SPELL_START + 21: spell_RF6_S_SPIDER(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;  /* RF6_S_SPIDER */
	case RF6_SPELL_START + 22: spell_RF6_S_HOUND(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_S_HOUND */
	case RF6_SPELL_START + 23: spell_RF6_S_HYDRA(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_S_HYDRA */
	case RF6_SPELL_START + 24: spell_RF6_S_ANGEL(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_S_ANGEL */
	case RF6_SPELL_START + 25: spell_RF6_S_DEMON(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_S_DEMON */
	case RF6_SPELL_START + 26: spell_RF6_S_UNDEAD(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;  /* RF6_S_UNDEAD */
	case RF6_SPELL_START + 27: spell_RF6_S_DRAGON(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;  /* RF6_S_DRAGON */
	case RF6_SPELL_START + 28: spell_RF6_S_HI_UNDEAD(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_S_HI_UNDEAD */
	case RF6_SPELL_START + 29: spell_RF6_S_HI_DRAGON(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_S_HI_DRAGON */
	case RF6_SPELL_START + 30: spell_RF6_S_AMBERITES(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;   /* RF6_S_AMBERITES */
	case RF6_SPELL_START + 31: spell_RF6_S_UNIQUE(target_ptr, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break;  /* RF6_S_UNIQUE */
	}

	return 0;
}
