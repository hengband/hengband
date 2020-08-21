#include "store/say-comments.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "store/store-owner-comments.h"
#include "store/store-util.h"
#include "player-info/avatar.h"
#include "store/rumor.h"
#include "view/display-messages.h"

#define RUMOR_CHANCE 8

/*!
 * @brief 取引成功時の店主のメッセージ処理 /
 * ブラックマーケットのときは別のメッセージを出す
 * Successful haggle.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void say_comment_1(player_type *player_ptr)
{
#ifdef JP
	if (cur_store_num == STORE_BLACK)
	{
		msg_print(comment_1_B[randint0(MAX_COMMENT_1)]);
	}
	else
	{
		msg_print(comment_1[randint0(MAX_COMMENT_1)]);
	}
#else
	msg_print(comment_1[randint0(MAX_COMMENT_1)]);
#endif

	if (one_in_(RUMOR_CHANCE))
	{
		msg_print(_("店主は耳うちした:", "The shopkeeper whispers something into your ear:"));
		display_rumor(player_ptr, TRUE);
	}
}


/*!
 * @brief プレイヤーがアイテムを買う時の価格代案メッセージ処理 /
 * Continue haggling (player is buying)
 * @param value 店主の提示価格
 * @param annoyed 店主のいらつき度
 * @return なし
 */
void say_comment_2(PRICE value, int annoyed)
{
	char tmp_val[80];
	sprintf(tmp_val, "%ld", (long)value);

	if (annoyed > 0)
	{
		msg_format(comment_2a[randint0(MAX_COMMENT_2A)], tmp_val);
		return;
	}

#ifdef JP
	if (cur_store_num == STORE_BLACK)
	{
		msg_format(comment_2b_B[randint0(MAX_COMMENT_2B)], tmp_val);
	}
	else
	{
		msg_format(comment_2b[randint0(MAX_COMMENT_2B)], tmp_val);
	}
#else
	msg_format(comment_2b[randint0(MAX_COMMENT_2B)], tmp_val);
#endif
}


/*!
 * @brief プレイヤーがアイテムを売る時の価格代案メッセージ処理 /
 * ブラックマーケットのときは別のメッセージを出す
 * Continue haggling (player is selling)
 * @param value 店主の提示価格
 * @param annoyed 店主のいらつき度
 * @return なし
 */
void say_comment_3(PRICE value, int annoyed)
{
	char tmp_val[80];
	sprintf(tmp_val, "%ld", (long)value);
	if (annoyed > 0)
	{
		msg_format(comment_3a[randint0(MAX_COMMENT_3A)], tmp_val);
	}
	else
	{
#ifdef JP
		if (cur_store_num == STORE_BLACK)
		{
			msg_format(comment_3b_B[randint0(MAX_COMMENT_3B)], tmp_val);
		}
		else
		{
			msg_format(comment_3b[randint0(MAX_COMMENT_3B)], tmp_val);
		}
#else
		msg_format(comment_3b[randint0(MAX_COMMENT_3B)], tmp_val);
#endif
	}
}


/*!
 * @brief 店主がプレイヤーを追い出す時のメッセージ処理 /
 * ブラックマーケットの時は別のメッセージを出す
 * Kick 'da bum out.					-RAK-
 * @return なし
 */
void say_comment_4(void)
{
#ifdef JP
	if (cur_store_num == STORE_BLACK)
	{
		msg_print(comment_4a_B[randint0(MAX_COMMENT_4A)]);
		msg_print(comment_4b_B[randint0(MAX_COMMENT_4B)]);
	}
	else
	{
		msg_print(comment_4a[randint0(MAX_COMMENT_4A)]);
		msg_print(comment_4b[randint0(MAX_COMMENT_4B)]);
	}
#else
	msg_print(comment_4a[randint0(MAX_COMMENT_4A)]);
	msg_print(comment_4b[randint0(MAX_COMMENT_4B)]);
#endif

}


/*!
 * @brief 店主がプレイヤーに取り合わない時のメッセージ処理 /
 * ブラックマーケットの時は別のメッセージを出す
 * You are insulting me
 * @return なし
 */
void say_comment_5(void)
{
#ifdef JP
	if (cur_store_num == STORE_BLACK)
	{
		msg_print(comment_5_B[randint0(MAX_COMMENT_5)]);
	}
	else
	{
		msg_print(comment_5[randint0(MAX_COMMENT_5)]);
	}
#else
	msg_print(comment_5[randint0(MAX_COMMENT_5)]);
#endif

}


/*!
 * @brief 店主がプレイヤーの提示を理解できなかった時のメッセージ処理 /
 * That makes no sense.
 * @return なし
 */
void say_comment_6(void)
{
	msg_print(comment_6[randint0(MAX_COMMENT_6)]);
}

/*!
 * @brief 店主が交渉を終えた際の反応を返す処理 /
 * Let a shop-keeper React to a purchase
 * @param price アイテムの取引額
 * @param value アイテムの実際価値
 * @param guess 店主が当初予想していた価値
 * @return なし
 * @details
 * We paid "price", it was worth "value", and we thought it was worth "guess"
 */
void purchase_analyze(player_type *player_ptr, PRICE price, PRICE value, PRICE guess)
{
	/* Item was worthless, but we bought it */
	if ((value <= 0) && (price > value))
	{
		msg_print(comment_7a[randint0(MAX_COMMENT_7A)]);
		chg_virtue(player_ptr, V_HONOUR, -1);
		chg_virtue(player_ptr, V_JUSTICE, -1);
		sound(SOUND_STORE1);
		return;
	}

	/* Item was cheaper than we thought, and we paid more than necessary */
	if ((value < guess) && (price > value))
	{
		msg_print(comment_7b[randint0(MAX_COMMENT_7B)]);
		chg_virtue(player_ptr, V_JUSTICE, -1);
		if (one_in_(4)) chg_virtue(player_ptr, V_HONOUR, -1);
		sound(SOUND_STORE2);
		return;
	}

	/* Item was a good bargain, and we got away with it */
	if ((value > guess) && (value < (4 * guess)) && (price < value))
	{
		msg_print(comment_7c[randint0(MAX_COMMENT_7C)]);
		if (one_in_(4)) chg_virtue(player_ptr, V_HONOUR, -1);
		else if (one_in_(4)) chg_virtue(player_ptr, V_HONOUR, 1);
		sound(SOUND_STORE3);
		return;
	}

	/* Item was a great bargain, and we got away with it */
	if ((value > guess) && (price < value))
	{
		msg_print(comment_7d[randint0(MAX_COMMENT_7D)]);
		if (one_in_(2)) chg_virtue(player_ptr, V_HONOUR, -1);
		if (one_in_(4)) chg_virtue(player_ptr, V_HONOUR, 1);
		if (10 * price < value) chg_virtue(player_ptr, V_SACRIFICE, 1);
		sound(SOUND_STORE4);
		return;
	}
}
