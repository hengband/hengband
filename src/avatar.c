/*!
    @file avatar.c
    @brief ウルティマ４を参考にした徳のシステムの実装 / Enable an Ultima IV style "avatar" game where you try to achieve perfection in various virtues.
    @date 2013/12/23
    @author
    Topi Ylinen 1998\n
    f1toyl@uta.fi\n
    topi.ylinen@noodi.fi\n
    \n
    Copyright (c) 1989 James E. Wilson, Christopher J. Stuart
    This software may be copied and distributed for educational, research, and
    not for profit purposes provided that this copyright and statement are
    included in all such copies.
*/


#include "angband.h"

/*!
 * 徳の名称 / The names of the virtues
 */
cptr virtue[MAX_VIRTUE] =
{
	_("情", "Compassion"),
	_("誉", "Honour"),
	_("正", "Justice"),
	_("犠", "Sacrifice"),
	_("識", "Knowledge"),
	_("誠", "Faith"),
	_("啓", "Enlightenment"),
	_("秘", "Mysticism"),
	_("運", "Chance"),
	_("然", "Nature"),
	_("調", "Harmony"),
	_("活", "Vitality"),
	_("死", "Unlife"),
	_("忍", "Patience"),
	_("節", "Temperance"),
	_("勤", "Diligence"),
	_("勇", "Valour"),
	_("個", "Individualism"),
};

/*!
 * @brief 該当の徳がプレイヤーに指定されているか否かに応じつつ、大小を比較する。
 * @details 徳がない場合は値0として比較する。
 * @param type 比較したい徳のID
 * @param num 比較基準値
 * @param tekitou VIRTUE_LARGE = 基準値より大きいか / VIRTUE_SMALL = 基準値より小さいか
 * @return 比較の真偽値を返す
 * @todo 引数名を直しておく
 */
bool compare_virtue(int type, int num, int tekitou)
{
	int vir;
	if (virtue_number(type))
		vir = p_ptr->virtues[virtue_number(type) - 1];
	else
		vir = 0;

	switch (tekitou)
	{
	case VIRTUE_LARGE:
		if (vir > num) return TRUE;
		else return FALSE;
	case VIRTUE_SMALL:
		if (vir < num) return TRUE;
		else return FALSE;
	}

	return FALSE;
}

/*!
 * @brief プレイヤーの指定の徳が何番目のスロットに登録されているかを返す。 / Aux function
 * @param type 確認したい徳のID
 * @return スロットがあるならばスロットのID(0～7)+1、ない場合は0を返す。
 */
int virtue_number(int type)
{
	int i;

	/* Search */
	for (i = 0; i < 8; i++)
	{
		if (p_ptr->vir_types[i] == type) return i + 1;
	}

	/* No match */
	return 0;
}

/*!
 * @brief プレイヤーの職業や種族に依存しないランダムな徳を取得する / Aux function
 * @param which 確認したい徳のID
 * @return なし
 */
static void get_random_virtue(int which)
{
	int type = 0;

	/* Randomly choose a type */
	while (!(type) || virtue_number(type))
	{
		switch (randint1(29))
		{
		case 1: case 2: case 3:
			type = V_SACRIFICE;
			break;
		case 4: case 5: case 6:
			type = V_COMPASSION;
			break;
		case 7: case 8: case 9: case 10: case 11: case 12:
			type = V_VALOUR;
			break;
		case 13: case 14: case 15: case 16: case 17:
			type = V_HONOUR;
			break;
		case 18: case 19: case 20: case 21:
			type = V_JUSTICE;
			break;
		case 22: case 23:
			type = V_TEMPERANCE;
			break;
		case 24: case 25:
			type = V_HARMONY;
			break;
		case 26: case 27: case 28:
			type = V_PATIENCE;
			break;
		default:
			type = V_DILIGENCE;
			break;
		}
	}

	/* Chosen */
	p_ptr->vir_types[which] = type;
}

/*!
 * @brief プレイヤーの選んだ魔法領域に応じて対応する徳を返す。
 * @param realm 魔法領域のID
 * @return 対応する徳のID
 */
static s16b get_realm_virtues(byte realm)
{
	switch (realm)
	{
	case REALM_LIFE:
		if (virtue_number(V_VITALITY)) return V_TEMPERANCE;
		else return V_VITALITY;
	case REALM_SORCERY:
		if (virtue_number(V_KNOWLEDGE)) return V_ENCHANT;
		else return V_KNOWLEDGE;
	case REALM_NATURE:
		if (virtue_number(V_NATURE)) return V_HARMONY;
		else return V_NATURE;
	case REALM_CHAOS:
		if (virtue_number(V_CHANCE)) return V_INDIVIDUALISM;
		else return V_CHANCE;
	case REALM_DEATH:
		return V_UNLIFE;
	case REALM_TRUMP:
		return V_KNOWLEDGE;
	case REALM_ARCANE:
		return 0;
	case REALM_CRAFT:
		if (virtue_number(V_ENCHANT)) return V_INDIVIDUALISM;
		else return V_ENCHANT;
	case REALM_DAEMON:
		if (virtue_number(V_JUSTICE)) return V_FAITH;
		else return V_JUSTICE;
	case REALM_CRUSADE:
		if (virtue_number(V_JUSTICE)) return V_HONOUR;
		else return V_JUSTICE;
	case REALM_HEX:
		if (virtue_number(V_COMPASSION)) return V_JUSTICE;
		else return V_COMPASSION;
	};

	return 0;
}


/*!
 * @brief 作成中のプレイヤーキャラクターに徳8種類を与える。 / Select virtues & reset values for a new character
 * @details 職業に応じて1～4種が固定、種族に応じて1種類が与えられ、後は重複なくランダムに選択される。
 * @return なし
 */
void get_virtues(void)
{
	int i = 0, j = 0;
	s16b tmp_vir;

	/* Reset */
	for (i = 0; i < 8; i++)
	{
		p_ptr->virtues[i] = 0;
		p_ptr->vir_types[i] = 0;
	}

	i = 0;

	/* Get pre-defined types */
	/* 1 or more virtues based on class */
	switch (p_ptr->pclass)
	{
	case CLASS_WARRIOR:
	case CLASS_SAMURAI:
		p_ptr->vir_types[i++] = V_VALOUR;
		p_ptr->vir_types[i++] = V_HONOUR;
		break;
	case CLASS_MAGE:
		p_ptr->vir_types[i++] = V_KNOWLEDGE;
		p_ptr->vir_types[i++] = V_ENCHANT;
		break;
	case CLASS_PRIEST:
		p_ptr->vir_types[i++] = V_FAITH;
		p_ptr->vir_types[i++] = V_TEMPERANCE;
		break;
	case CLASS_ROGUE:
	case CLASS_SNIPER:
		p_ptr->vir_types[i++] = V_HONOUR;
		break;
	case CLASS_RANGER:
	case CLASS_ARCHER:
		p_ptr->vir_types[i++] = V_NATURE;
		p_ptr->vir_types[i++] = V_TEMPERANCE;
		break;
	case CLASS_PALADIN:
		p_ptr->vir_types[i++] = V_JUSTICE;
		p_ptr->vir_types[i++] = V_VALOUR;
		p_ptr->vir_types[i++] = V_HONOUR;
		p_ptr->vir_types[i++] = V_FAITH;
		break;
	case CLASS_WARRIOR_MAGE:
	case CLASS_RED_MAGE:
		p_ptr->vir_types[i++] = V_ENCHANT;
		p_ptr->vir_types[i++] = V_VALOUR;
		break;
	case CLASS_CHAOS_WARRIOR:
		p_ptr->vir_types[i++] = V_CHANCE;
		p_ptr->vir_types[i++] = V_INDIVIDUALISM;
		break;
	case CLASS_MONK:
	case CLASS_FORCETRAINER:
		p_ptr->vir_types[i++] = V_FAITH;
		p_ptr->vir_types[i++] = V_HARMONY;
		p_ptr->vir_types[i++] = V_TEMPERANCE;
		p_ptr->vir_types[i++] = V_PATIENCE;
		break;
	case CLASS_MINDCRAFTER:
	case CLASS_MIRROR_MASTER:
		p_ptr->vir_types[i++] = V_HARMONY;
		p_ptr->vir_types[i++] = V_ENLIGHTEN;
		p_ptr->vir_types[i++] = V_PATIENCE;
		break;
	case CLASS_HIGH_MAGE:
	case CLASS_SORCERER:
		p_ptr->vir_types[i++] = V_ENLIGHTEN;
		p_ptr->vir_types[i++] = V_ENCHANT;
		p_ptr->vir_types[i++] = V_KNOWLEDGE;
		break;
	case CLASS_TOURIST:
		p_ptr->vir_types[i++] = V_ENLIGHTEN;
		p_ptr->vir_types[i++] = V_CHANCE;
		break;
	case CLASS_IMITATOR:
		p_ptr->vir_types[i++] = V_CHANCE;
		break;
	case CLASS_BLUE_MAGE:
		p_ptr->vir_types[i++] = V_CHANCE;
		p_ptr->vir_types[i++] = V_KNOWLEDGE;
		break;
	case CLASS_BEASTMASTER:
		p_ptr->vir_types[i++] = V_NATURE;
		p_ptr->vir_types[i++] = V_CHANCE;
		p_ptr->vir_types[i++] = V_VITALITY;
		break;
	case CLASS_MAGIC_EATER:
		p_ptr->vir_types[i++] = V_ENCHANT;
		p_ptr->vir_types[i++] = V_KNOWLEDGE;
		break;
	case CLASS_BARD:
		p_ptr->vir_types[i++] = V_HARMONY;
		p_ptr->vir_types[i++] = V_COMPASSION;
		break;
	case CLASS_CAVALRY:
		p_ptr->vir_types[i++] = V_VALOUR;
		p_ptr->vir_types[i++] = V_HARMONY;
		break;
	case CLASS_BERSERKER:
		p_ptr->vir_types[i++] = V_VALOUR;
		p_ptr->vir_types[i++] = V_INDIVIDUALISM;
		break;
	case CLASS_SMITH:
		p_ptr->vir_types[i++] = V_HONOUR;
		p_ptr->vir_types[i++] = V_KNOWLEDGE;
		break;
	case CLASS_NINJA:
		p_ptr->vir_types[i++] = V_PATIENCE;
		p_ptr->vir_types[i++] = V_KNOWLEDGE;
		p_ptr->vir_types[i++] = V_FAITH;
		p_ptr->vir_types[i++] = V_UNLIFE;
		break;
	};

	/* Get one virtue based on race */
	switch (p_ptr->prace)
	{
	case RACE_HUMAN: case RACE_HALF_ELF: case RACE_DUNADAN:
		p_ptr->vir_types[i++] = V_INDIVIDUALISM;
		break;
	case RACE_ELF: case RACE_SPRITE: case RACE_ENT:
		p_ptr->vir_types[i++] = V_NATURE;
		break;
	case RACE_HOBBIT: case RACE_HALF_OGRE:
		p_ptr->vir_types[i++] = V_TEMPERANCE;
		break;
	case RACE_DWARF: case RACE_KLACKON: case RACE_ANDROID:
		p_ptr->vir_types[i++] = V_DILIGENCE;
		break;
	case RACE_GNOME: case RACE_CYCLOPS:
		p_ptr->vir_types[i++] = V_KNOWLEDGE;
		break;
	case RACE_HALF_ORC: case RACE_AMBERITE: case RACE_KOBOLD:
		p_ptr->vir_types[i++] = V_HONOUR;
		break;
	case RACE_HALF_TROLL: case RACE_BARBARIAN:
		p_ptr->vir_types[i++] = V_VALOUR;
		break;
	case RACE_HIGH_ELF: case RACE_KUTAR:
		p_ptr->vir_types[i++] = V_VITALITY;
		break;
	case RACE_HALF_GIANT: case RACE_GOLEM: case RACE_ANGEL: case RACE_DEMON:
		p_ptr->vir_types[i++] = V_JUSTICE;
		break;
	case RACE_HALF_TITAN:
		p_ptr->vir_types[i++] = V_HARMONY;
		break;
	case RACE_YEEK:
		p_ptr->vir_types[i++] = V_SACRIFICE;
		break;
	case RACE_MIND_FLAYER:
		p_ptr->vir_types[i++] = V_ENLIGHTEN;
		break;
	case RACE_DARK_ELF: case RACE_DRACONIAN: case RACE_S_FAIRY:
		p_ptr->vir_types[i++] = V_ENCHANT;
		break;
	case RACE_NIBELUNG:
		p_ptr->vir_types[i++] = V_PATIENCE;
		break;
	case RACE_IMP:
		p_ptr->vir_types[i++] = V_FAITH;
		break;
	case RACE_ZOMBIE: case RACE_SKELETON:
	case RACE_VAMPIRE: case RACE_SPECTRE:
		p_ptr->vir_types[i++] = V_UNLIFE;
		break;
	case RACE_BEASTMAN:
		p_ptr->vir_types[i++] = V_CHANCE;
		break;
	}

	/* Get a virtue for realms */
	if (p_ptr->realm1)
	{
		tmp_vir = get_realm_virtues(p_ptr->realm1);
		if (tmp_vir) p_ptr->vir_types[i++] = tmp_vir;
	}
	if (p_ptr->realm2)
	{
		tmp_vir = get_realm_virtues(p_ptr->realm2);
		if (tmp_vir) p_ptr->vir_types[i++] = tmp_vir;
	}

	/* Eliminate doubles */
	for (i = 0; i < 8; i++)
	{
		for (j = i + 1; j < 8; j++)
		{
			if ((p_ptr->vir_types[j] != 0) && (p_ptr->vir_types[j] == p_ptr->vir_types[i]))
				p_ptr->vir_types[j] = 0;
		}
	}

	/* Fill in the blanks */
	for (i = 0; i < 8; i++)
	{
		if (p_ptr->vir_types[i] == 0) get_random_virtue(i);
	}
}

/*!
 * @brief 対応する徳をプレイヤーがスロットに登録している場合に加減を行う。
 * @details 範囲は-125～125、基本的に絶対値が大きいほど絶対値が上がり辛くなる。
 * @param virtue 徳のID
 * @param amount 加減量
 * @return なし
 */
void chg_virtue(int virtue, int amount)
{
	int i = 0;

	for (i = 0; i < 8; i++)
	{
		if (p_ptr->vir_types[i] == virtue)
		{
			if (amount > 0)
			{
				if ((amount + p_ptr->virtues[i] > 50) && one_in_(2))
				{
					p_ptr->virtues[i] = MAX(p_ptr->virtues[i], 50);
					return;
				}
				if ((amount + p_ptr->virtues[i] > 80) && one_in_(2))
				{
					p_ptr->virtues[i] = MAX(p_ptr->virtues[i], 80);
					return;
				}
				if ((amount + p_ptr->virtues[i] > 100) && one_in_(2))
				{
					p_ptr->virtues[i] = MAX(p_ptr->virtues[i], 100);
					return;
				}
				if (amount + p_ptr->virtues[i] > 125)
					p_ptr->virtues[i] = 125;
				else
					p_ptr->virtues[i] = p_ptr->virtues[i] + amount;
			}
			else
			{
				if ((amount + p_ptr->virtues[i] < -50) && one_in_(2))
				{
					p_ptr->virtues[i] = MIN(p_ptr->virtues[i], -50);
					return;
				}
				if ((amount + p_ptr->virtues[i] < -80) && one_in_(2))
				{
					p_ptr->virtues[i] = MIN(p_ptr->virtues[i], -80);
					return;
				}
				if ((amount + p_ptr->virtues[i] < -100) && one_in_(2))
				{
					p_ptr->virtues[i] = MIN(p_ptr->virtues[i], -100);
					return;
				}
				if (amount + p_ptr->virtues[i] < -125)
					p_ptr->virtues[i] = -125;
				else
					p_ptr->virtues[i] = p_ptr->virtues[i] + amount;
			}
			p_ptr->update |= (PU_BONUS);
			return;
		}
	}
}

/*!
 * @brief 対応する徳をプレイヤーがスロットに登録している場合に固定値をセットする。
 * @param virtue 徳のID
 * @param amount セットしたい値。
 * @return なし
 */
void set_virtue(int virtue, int amount)
{
	int i = 0;

	for (i = 0; i < 8; i++)
	{
		if (p_ptr->vir_types[i] == virtue)
		{
			p_ptr->virtues[i] = amount;
			return;
		}
	}
}

/*!
 * @brief 徳のダンプ表示を行う。
 * @param OutFile ファイルポインタ。
 * @return なし
 */
void dump_virtues(FILE *OutFile)
{
	int v_nr = 0;

	if (!OutFile) return;

	for (v_nr = 0; v_nr < 8; v_nr++)
	{
		char v_name [20];
		int tester = p_ptr->virtues[v_nr];

		strcpy(v_name, virtue[(p_ptr->vir_types[v_nr])-1]);

		if (p_ptr->vir_types[v_nr] == 0 || p_ptr->vir_types[v_nr] > MAX_VIRTUE)
			fprintf(OutFile, _("おっと。%sの情報なし。", "Oops. No info about %s."), v_name);

		else if (tester < -100)
			fprintf(OutFile, _("[%s]の対極", "You are the polar opposite of %s."), v_name);
		else if (tester < -80)
			fprintf(OutFile, _("[%s]の大敵", "You are an arch-enemy of %s."), v_name);
		else if (tester < -60)
			fprintf(OutFile, _("[%s]の強敵", "You are a bitter enemy of %s."), v_name);
		else if (tester < -40)
			fprintf(OutFile, _("[%s]の敵", "You are an enemy of %s."), v_name);
		else if (tester < -20)
			fprintf(OutFile, _("[%s]の罪者", "You have sinned against %s."), v_name);
		else if (tester < 0)
			fprintf(OutFile, _("[%s]の迷道者", "You have strayed from the path of %s."), v_name);
		else if (tester == 0)
			fprintf(OutFile,_("[%s]の中立者", "You are neutral to %s."), v_name);
		else if (tester < 20)
			fprintf(OutFile,_("[%s]の小徳者", "You are somewhat virtuous in %s."), v_name);
		else if (tester < 40)
			fprintf(OutFile,_("[%s]の中徳者", "You are virtuous in %s."), v_name);
		else if (tester < 60)
			fprintf(OutFile,_("[%s]の高徳者", "You are very virtuous in %s."), v_name);
		else if (tester < 80)
			fprintf(OutFile,_("[%s]の覇者", "You are a champion of %s."), v_name);
		else if (tester < 100)
			fprintf(OutFile,_("[%s]の偉大な覇者", "You are a great champion of %s."), v_name);
		else
			fprintf(OutFile,_("[%s]の具現者", "You are the living embodiment of %s."), v_name);

	    fprintf(OutFile, "\n");
	}
}
