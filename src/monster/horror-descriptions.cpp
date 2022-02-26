/*!
 * @file horror-descriptions.c
 * @brief エルドリッチホラーの形容詞テーブル定義 / Definitions of adjectives on
 * @date 2020/02/21
 * @author Hourier
 * @deitals
 * いくら増やしてもよいが、horror_desc_evil とhorror_desc_neutralの個数は同一にすること
 */

#include "monster/horror-descriptions.h"

/*!
 * @var horror_desc_common
 * @brief ELDRITCH HORROR効果時のモンスターの形容メッセージ (正常時、邪悪・中立共通)
 */
concptr horror_desc_common[MAX_SAN_HORROR_COMMON] = {
#ifdef JP
    "底知れぬ",
    "破滅的な",
    "容赦のない",
    "悪夢のような",
    "名前を口にできない",
#else
    "abysmal",
    "baleful",
    "grisly",
    "nightmarish",
    "unspeakable",
#endif
};

/*!
 * @var horror_desc_evil
 * @brief ELDRITCH HORROR効果時のモンスターの形容メッセージ (正常時、邪悪)
 */
concptr horror_desc_evil[MAX_SAN_HORROR_EVIL] = {
#ifdef JP
    "忌まわしい",
    "ぞっとする",
    "冒涜的な",
    "いやな",
    "恐ろしい",

    "不潔な",
    "おぞましい",
    "非道なる",
    "身の毛もよだつ",
    "地獄の",

    "むかむかする",
    "嫌悪を感じる",
    "罰当たりな",
    "恐い",
    "不浄な",
#else
    "abominable",
    "appalling",
    "blasphemous",
    "disgusting",
    "dreadful",

    "filthy",
    "hideous",
    "hellish",
    "horrible",
    "infernal",

    "loathsome",
    "repulsive",
    "sacrilegious",
    "terrible",
    "unclean",
#endif
};

/*
 * @var horror_desc_neutral
 * @brief ELDRITCH HORROR効果時のモンスターの形容メッセージ (正常時、中立)
 */
concptr horror_desc_neutral[MAX_SAN_HORROR_NEUTRAL] = {
#ifdef JP
    "大いなる畏怖に包まれた",
    "妖しげな笑みの",
    "いるはずのない",
    "吸い込まれそうな",
    "五感を超越した",

    "別次元に浮かび上がった",
    "幻覚と見紛うような",
    "ゆらゆらと揺らめいた",
    "自らの実在を疑うほど圧倒的な",
    "理解できない",

    "サイケデリックな",
    "生気が吸い取られるような",
    "トリップ感に満ちた",
    "頭が真っ白になるような",
    "無限に加速した",
#else
    "awe-inspiring",
    "slyly grinning",
    "impossible",
    "sucking",
    "transcendent",

    "otherworldly",
    "hallucinating",
    "rhythmically bobbing",
    "overwhelming",
    "unintelligible",

    "psychedelic",
    "transfixing",
    "trippy",
    "hypnotic",
    "blurred, with motion,",
#endif
};

/*!
 * @var funny_desc
 * @brief ELDRITCH HORROR効果時のモンスターの形容メッセージ (幻覚状態時)
 */
concptr funny_desc[MAX_SAN_FUNNY] = {
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
concptr funny_comments[MAX_SAN_COMMENT] = {
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
