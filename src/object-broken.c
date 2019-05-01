#include "angband.h"
#include "spells.h"
#include "objectkind.h"

/*!
 * @brief 薬の破損効果処理 /
 * Potions "smash open" and cause an area effect when
 * @param who 薬破損の主体ID(プレイヤー所持アイテムが壊れた場合0、床上のアイテムの場合モンスターID)
 * @param y 破壊時のY座標
 * @param x 破壊時のX座標
 * @param k_idx 破損した薬のアイテムID
 * @return 薬を浴びたモンスターが起こるならばTRUEを返す
 * @details
 * <pre>
 * (1) they are shattered while in the player's p_ptr->inventory_list,
 * due to cold (etc) attacks;
 * (2) they are thrown at a monster, or obstacle;
 * (3) they are shattered by a "cold ball" or other such spell
 * while lying on the floor.
 *
 * Arguments:
 *    who   ---  who caused the potion to shatter (0=player)
 *          potions that smash on the floor are assumed to
 *          be caused by no-one (who = 1), as are those that
 *          shatter inside the player p_ptr->inventory_list.
 *          (Not anymore -- I changed this; TY)
 *    y, x  --- coordinates of the potion (or player if
 *          the potion was in her p_ptr->inventory_list);
 *    o_ptr --- pointer to the potion object.
 * </pre>
 */
bool potion_smash_effect(MONSTER_IDX who, POSITION y, POSITION x, KIND_OBJECT_IDX k_idx)
{
	int     radius = 2;
	int     dt = 0;
	int     dam = 0;
	bool    angry = FALSE;

	object_kind *k_ptr = &k_info[k_idx];

	switch (k_ptr->sval)
	{
	case SV_POTION_SALT_WATER:
	case SV_POTION_SLIME_MOLD:
	case SV_POTION_LOSE_MEMORIES:
	case SV_POTION_DEC_STR:
	case SV_POTION_DEC_INT:
	case SV_POTION_DEC_WIS:
	case SV_POTION_DEC_DEX:
	case SV_POTION_DEC_CON:
	case SV_POTION_DEC_CHR:
	case SV_POTION_WATER:   /* perhaps a 'water' attack? */
	case SV_POTION_APPLE_JUICE:
		return TRUE;

	case SV_POTION_INFRAVISION:
	case SV_POTION_DETECT_INVIS:
	case SV_POTION_SLOW_POISON:
	case SV_POTION_CURE_POISON:
	case SV_POTION_BOLDNESS:
	case SV_POTION_RESIST_HEAT:
	case SV_POTION_RESIST_COLD:
	case SV_POTION_HEROISM:
	case SV_POTION_BESERK_STRENGTH:
	case SV_POTION_RES_STR:
	case SV_POTION_RES_INT:
	case SV_POTION_RES_WIS:
	case SV_POTION_RES_DEX:
	case SV_POTION_RES_CON:
	case SV_POTION_RES_CHR:
	case SV_POTION_INC_STR:
	case SV_POTION_INC_INT:
	case SV_POTION_INC_WIS:
	case SV_POTION_INC_DEX:
	case SV_POTION_INC_CON:
	case SV_POTION_INC_CHR:
	case SV_POTION_AUGMENTATION:
	case SV_POTION_ENLIGHTENMENT:
	case SV_POTION_STAR_ENLIGHTENMENT:
	case SV_POTION_SELF_KNOWLEDGE:
	case SV_POTION_EXPERIENCE:
	case SV_POTION_RESISTANCE:
	case SV_POTION_INVULNERABILITY:
	case SV_POTION_NEW_LIFE:
		/* All of the above potions have no effect when shattered */
		return FALSE;
	case SV_POTION_SLOWNESS:
		dt = GF_OLD_SLOW;
		dam = 5;
		angry = TRUE;
		break;
	case SV_POTION_POISON:
		dt = GF_POIS;
		dam = 3;
		angry = TRUE;
		break;
	case SV_POTION_BLINDNESS:
		dt = GF_DARK;
		angry = TRUE;
		break;
	case SV_POTION_BOOZE: /* Booze */
		dt = GF_OLD_CONF;
		angry = TRUE;
		break;
	case SV_POTION_SLEEP:
		dt = GF_OLD_SLEEP;
		angry = TRUE;
		break;
	case SV_POTION_RUINATION:
	case SV_POTION_DETONATIONS:
		dt = GF_SHARDS;
		dam = damroll(25, 25);
		angry = TRUE;
		break;
	case SV_POTION_DEATH:
		dt = GF_DEATH_RAY;    /* !! */
		dam = k_ptr->level * 10;
		angry = TRUE;
		radius = 1;
		break;
	case SV_POTION_SPEED:
		dt = GF_OLD_SPEED;
		break;
	case SV_POTION_CURE_LIGHT:
		dt = GF_OLD_HEAL;
		dam = damroll(2, 3);
		break;
	case SV_POTION_CURE_SERIOUS:
		dt = GF_OLD_HEAL;
		dam = damroll(4, 3);
		break;
	case SV_POTION_CURE_CRITICAL:
	case SV_POTION_CURING:
		dt = GF_OLD_HEAL;
		dam = damroll(6, 3);
		break;
	case SV_POTION_HEALING:
		dt = GF_OLD_HEAL;
		dam = damroll(10, 10);
		break;
	case SV_POTION_RESTORE_EXP:
		dt = GF_STAR_HEAL;
		dam = 0;
		radius = 1;
		break;
	case SV_POTION_LIFE:
		dt = GF_STAR_HEAL;
		dam = damroll(50, 50);
		radius = 1;
		break;
	case SV_POTION_STAR_HEALING:
		dt = GF_OLD_HEAL;
		dam = damroll(50, 50);
		radius = 1;
		break;
	case SV_POTION_RESTORE_MANA:   /* MANA */
		dt = GF_MANA;
		dam = damroll(10, 10);
		radius = 1;
		break;
	default:
		/* Do nothing */;
	}

	(void)project(who, radius, y, x, dam, dt, (PROJECT_JUMP | PROJECT_ITEM | PROJECT_KILL), -1);

	/* XXX  those potions that explode need to become "known" */
	return angry;
}
