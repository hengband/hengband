/*!
 * @file horror-descriptions.cpp
 * @brief エルドリッチホラーの形容詞テーブル定義
 * @date 2023/04/29
 * @author Hourier
 * @deitals
 * いくら増やしてもよいが、horror_desc_evil とhorror_desc_neutralの個数は同一にすること
 */

#include "monster/horror-descriptions.h"
#include "locale/language-switcher.h"

/*!
 * @brief ELDRITCH HORROR効果時のモンスターの形容メッセージ (正常時、邪悪・中立共通)
 */
const std::vector<std::string> horror_desc_common = {
    _("底知れぬ", "abysmal"),
    _("破滅的な", "baleful"),
    _("容赦のない", "grisly"),
    _("悪夢のような", "nightmarish"),
    _("名前を口にできない", "unspeakable"),
};

/*!
 * @brief ELDRITCH HORROR効果時のモンスターの形容メッセージ (正常時、邪悪)
 */
const std::vector<std::string> horror_desc_evil = {
    _("忌まわしい", "abominable"),
    _("ぞっとする", "appalling"),
    _("冒涜的な", "blasphemous"),
    _("いやな", "disgusting"),
    _("恐ろしい", "dreadful"),
    _("不潔な", "filthy"),
    _("おぞましい", "hideous"),
    _("非道なる", "hellish"),
    _("身の毛もよだつ", "horrible"),
    _("地獄の", "infernal"),
    _("むかむかする", "loathsome"),
    _("嫌悪を感じる", "repulsive"),
    _("罰当たりな", "sacrilegious"),
    _("恐い", "terrible"),
    _("不浄な", "unclean"),
};

/*
 * @brief ELDRITCH HORROR効果時のモンスターの形容メッセージ (正常時、中立)
 */
const std::vector<std::string> horror_desc_neutral = {
    _("大いなる畏怖に包まれた", "awe-inspiring"),
    _("妖しげな笑みの", "slyly grinning"),
    _("いるはずのない", "impossible"),
    _("吸い込まれそうな", "sucking"),
    _("五感を超越した", "transcendent"),
    _("別次元に浮かび上がった", "otherworldly"),
    _("幻覚と見紛うような", "hallucinating"),
    _("ゆらゆらと揺らめいた", "rhythmically bobbing"),
    _("自らの実在を疑うほど圧倒的な", "overwhelming"),
    _("理解できない", "unintelligible"),
    _("サイケデリックな", "psychedelic"),
    _("生気が吸い取られるような", "transfixing"),
    _("トリップ感に満ちた", "trippy"),
    _("頭が真っ白になるような", "hypnotic"),
    _("無限に加速した", "blurred, with motion,"),
};

/*!
 * @brief ELDRITCH HORROR効果時のモンスターの形容メッセージ (幻覚状態時)
 */
const std::vector<std::string> funny_desc = {
    _("間抜けな", "silly"),
    _("滑稽な", "hilarious"),
    _("ばからしい", "absurd"),
    _("無味乾燥な", "insipid"),
    _("馬鹿げた", "ridiculous"),
    _("笑える", "laughable"),
    _("ばかばかしい", "ludicrous"),
    _("ぶっとんだ", "far-out"),
    _("いかした", "groovy"),
    _("ポストモダンな", "postmodern"),
    _("ファンタスティックな", "fantastic"),
    _("ダダイズム的な", "dadaistic"),
    _("キュビズム的な", "cubistic"),
    _("宇宙的な", "cosmic"),
    _("卓越した", "awesome"),
    _("理解不能な", "incomprehensible"),
    _("ものすごい", "fabulous"),
    _("驚くべき", "amazing"),
    _("信じられない", "incredible"),
    _("カオティックな", "chaotic"),
    _("野性的な", "wild"),
    _("非常識な", "preposterous"),
};

/*!
 * @brief ELDRITCH HORROR効果時の幻覚時間延長を示す錯乱表現
 */
const std::vector<std::string> funny_comments = {
    _("最高だぜ！", "Wow, cosmic, man!"),
    _("うひょー！", "Rad!"),
    _("いかすぜ！", "Groovy!"),
    _("すんばらしい！", "Cool!"),
    _("ぶっとびー！", "Far out!"),
};
