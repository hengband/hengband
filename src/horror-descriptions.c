#include "h-type.h"
#include "horror-descriptions.h"

/*!
 * @var horror_desc
 * @brief ELDRITCH HORROR効果時のモンスターの形容メッセージ（通常時）
 */
concptr horror_desc[MAX_SAN_HORROR] =
{
#ifdef JP
	"忌まわしい",
	"底知れぬ",
	"ぞっとする",
	"破滅的な",
	"冒涜的な",

	"いやな",
	"恐ろしい",
	"不潔な",
	"容赦のない",
	"おぞましい",

	"地獄の",
	"身の毛もよだつ",
	"地獄の",
	"忌まわしい",
	"悪夢のような",

	"嫌悪を感じる",
	"罰当たりな",
	"恐い",
	"不浄な",
	"言うもおぞましい",
#else
	"abominable",
	"abysmal",
	"appalling",
	"baleful",
	"blasphemous",

	"disgusting",
	"dreadful",
	"filthy",
	"grisly",
	"hideous",

	"hellish",
	"horrible",
	"infernal",
	"loathsome",
	"nightmarish",

	"repulsive",
	"sacrilegious",
	"terrible",
	"unclean",
	"unspeakable",
#endif
};

/*!
 * @var funny_desc
 * @brief ELDRITCH HORROR効果時のモンスターの形容メッセージ(幻覚状態時)
 */
concptr funny_desc[MAX_SAN_FUNNY] =
{
#ifdef JP
	"間抜けな",
	"滑稽な",
	"ばからしい",
	"無味乾燥な",
	"馬鹿げた",

	"笑える",
	"ばかばかしい",
	"ぶっとんだ",
	"いかした",
	"ポストモダンな",

	"ファンタスティックな",
	"ダダイズム的な",
	"キュビズム的な",
	"宇宙的な",
	"卓越した",

	"理解不能な",
	"ものすごい",
	"驚くべき",
	"信じられない",
	"カオティックな",

	"野性的な",
	"非常識な",
#else
	"silly",
	"hilarious",
	"absurd",
	"insipid",
	"ridiculous",

	"laughable",
	"ludicrous",
	"far-out",
	"groovy",
	"postmodern",

	"fantastic",
	"dadaistic",
	"cubistic",
	"cosmic",
	"awesome",

	"incomprehensible",
	"fabulous",
	"amazing",
	"incredible",
	"chaotic",

	"wild",
	"preposterous",
#endif
};

/*!
 * @var funny_comments
 * @brief ELDRITCH HORROR効果時の幻覚時間延長を示す錯乱表現
 */
concptr funny_comments[MAX_SAN_COMMENT] =
{
#ifdef JP
	  "最高だぜ！",
	  "うひょー！",
	  "いかすぜ！",
	  "すんばらしい！",
	  "ぶっとびー！"
  #else
	  "Wow, cosmic, man!",
	  "Rad!",
	  "Groovy!",
	  "Cool!",
	  "Far out!"
  #endif
};
