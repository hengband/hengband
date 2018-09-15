/*!
    @file cmd-spell.c
    @brief 魔法のインターフェイスと発動 / Purpose: Do everything for each spell
    @date 2013/12/31
    @author
    2013 Deskull rearranged comment for Doxygen.
 */

#include "angband.h"
#include "selfinfo.h"
#include "spells-summon.h"
#include "realm-arcane.h"
#include "realm-chaos.h"
#include "realm-craft.h"
#include "realm-crusade.h"
#include "realm-daemon.h"
#include "realm-death.h"
#include "realm-hex.h"
#include "realm-hissatsu.h"
#include "realm-life.h"
#include "realm-nature.h"
#include "realm-song.h"
#include "realm-sorcery.h"
#include "realm-trump.h"

/*!
 * @brief
 * 魔法の効果を「キャプション:ダイス＋定数値」のフォーマットで出力する / Generate dice info string such as "foo 2d10"
 * @param str キャプション
 * @param dice ダイス数
 * @param sides ダイス目
 * @param base 固定値
 * @return フォーマットに従い整形された文字列
 */
cptr info_string_dice(cptr str, int dice, int sides, int base)
{
	/* Fix value */
	if (!dice)
		return format("%s%d", str, base);

	/* Dice only */
	else if (!base)
		return format("%s%dd%d", str, dice, sides);

	/* Dice plus base value */
	else
		return format("%s%dd%d%+d", str, dice, sides, base);
}


/*!
 * @brief 魔法によるダメージを出力する / Generate damage-dice info string such as "dam 2d10"
 * @param dice ダイス数
 * @param sides ダイス目
 * @param base 固定値
 * @return フォーマットに従い整形された文字列
 */
cptr info_damage(int dice, int sides, int base)
{
	return info_string_dice(_("損傷:", "dam "), dice, sides, base);
}

/*!
 * @brief 魔法の効果時間を出力する / Generate duration info string such as "dur 20+1d20"
 * @param base 固定値
 * @param sides ダイス目
 * @return フォーマットに従い整形された文字列
 */
cptr info_duration(int base, int sides)
{
	return format(_("期間:%d+1d%d", "dur %d+1d%d"), base, sides);
}

/*!
 * @brief 魔法の効果範囲を出力する / Generate range info string such as "range 5"
 * @param range 効果範囲
 * @return フォーマットに従い整形された文字列
 */
cptr info_range(POSITION range)
{
	return format(_("範囲:%d", "range %d"), range);
}

/*!
 * @brief 魔法による回復量を出力する / Generate heal info string such as "heal 2d8"
 * @param dice ダイス数
 * @param sides ダイス目
 * @param base 固定値
 * @return フォーマットに従い整形された文字列
 */
cptr info_heal(int dice, int sides, int base)
{
	return info_string_dice(_("回復:", "heal "), dice, sides, base);
}

/*!
 * @brief 魔法効果発動までの遅延ターンを出力する / Generate delay info string such as "delay 15+1d15"
 * @param base 固定値
 * @param sides ダイス目
 * @return フォーマットに従い整形された文字列
 */
cptr info_delay(int base, int sides)
{
	return format(_("遅延:%d+1d%d", "delay %d+1d%d"), base, sides);
}


/*!
 * @brief 魔法によるダメージを出力する(固定値＆複数回処理) / Generate multiple-damage info string such as "dam 25 each"
 * @param dam 固定値
 * @return フォーマットに従い整形された文字列
 */
cptr info_multi_damage(HIT_POINT dam)
{
	return format(_("損傷:各%d", "dam %d each"), dam);
}


/*!
 * @brief 魔法によるダメージを出力する(ダイスのみ＆複数回処理) / Generate multiple-damage-dice info string such as "dam 5d2 each"
 * @param dice ダイス数
 * @param sides ダイス目
 * @return フォーマットに従い整形された文字列
 */
cptr info_multi_damage_dice(int dice, int sides)
{
	return format(_("損傷:各%dd%d", "dam %dd%d each"), dice, sides);
}

/*!
 * @brief 魔法による一般的な効力値を出力する（固定値） / Generate power info string such as "power 100"
 * @param power 固定値
 * @return フォーマットに従い整形された文字列
 */
cptr info_power(int power)
{
	return format(_("効力:%d", "power %d"), power);
}


/*!
 * @brief 魔法による一般的な効力値を出力する（ダイス値） / Generate power info string such as "power 100"
 * @param dice ダイス数
 * @param sides ダイス目
 * @return フォーマットに従い整形された文字列
 */
/*
 * Generate power info string such as "power 1d100"
 */
cptr info_power_dice(int dice, int sides)
{
	return format(_("効力:%dd%d", "power %dd%d"), dice, sides);
}


/*!
 * @brief 魔法の効果半径を出力する / Generate radius info string such as "rad 100"
 * @param rad 効果半径
 * @return フォーマットに従い整形された文字列
 */
cptr info_radius(int rad)
{
	return format(_("半径:%d", "rad %d"), rad);
}


/*!
 * @brief 魔法効果の限界重量を出力する / Generate weight info string such as "max wgt 15"
 * @param weight 最大重量
 * @return フォーマットに従い整形された文字列
 */
cptr info_weight(int weight)
{
#ifdef JP
	return format("最大重量:%d.%dkg", lbtokg1(weight), lbtokg2(weight));
#else
	return format("max wgt %d", weight/10);
#endif
}





/*!
 * @brief 魔法処理のメインルーチン
 * @param realm 魔法領域のID
 * @param spell 各領域の魔法ID
 * @param mode 求める処理
 * @return 各領域魔法に各種テキストを求めた場合は文字列参照ポインタ、そうでない場合はNULLポインタを返す。
 */
cptr do_spell(REALM_IDX realm, SPELL_IDX spell, BIT_FLAGS mode)
{
	switch (realm)
	{
	case REALM_LIFE:     return do_life_spell(spell, mode);
	case REALM_SORCERY:  return do_sorcery_spell(spell, mode);
	case REALM_NATURE:   return do_nature_spell(spell, mode);
	case REALM_CHAOS:    return do_chaos_spell(spell, mode);
	case REALM_DEATH:    return do_death_spell(spell, mode);
	case REALM_TRUMP:    return do_trump_spell(spell, mode);
	case REALM_ARCANE:   return do_arcane_spell(spell, mode);
	case REALM_CRAFT:    return do_craft_spell(spell, mode);
	case REALM_DAEMON:   return do_daemon_spell(spell, mode);
	case REALM_CRUSADE:  return do_crusade_spell(spell, mode);
	case REALM_MUSIC:    return do_music_spell(spell, mode);
	case REALM_HISSATSU: return do_hissatsu_spell(spell, mode);
	case REALM_HEX:      return do_hex_spell(spell, mode);
	}

	return NULL;
}
