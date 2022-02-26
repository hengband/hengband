/*
 * @brief 日記のサブタイトルを表すテキストの配列群
 * @date 2020/03/08
 * @author Hourier
 */

#include "cmd-io/diary-subtitle-table.h"

concptr subtitle[MAX_SUBTITLE] = {
#ifdef JP
    "最強の肉体を求めて",
    "人生それははかない",
    "勝てば秩序、負ければ混沌",
    "溺れる者は藁をも掴む",
    "明日に向かって",
    "棚からぼたもち",
    "あとの祭り",
    "それはいい考えだ",
    "何とでも言え",
    "兎にも角にも",
    "ウソだけど",
    "もはやこれまで",
    "なんでこうなるの",
    "それは無理だ",
    "倒すべき敵はゲ○ツ",
    "ん～？聞こえんなぁ",
    "オレの名を言ってみろ",
    "頭が変になっちゃった",
    "互換しません",
    "せっかくだから",
    "まだまだ甘いね",
    "むごいむごすぎる",
    "こんなもんじゃない",
    "だめだこりゃ",
    "次いってみよう",
    "ちょっとだけよ",
    "哀しき冒険者",
    "野望の果て",
    "無限地獄",
    "神に喧嘩を売る者",
    "未知の世界へ",
    "時は金なり",
    "最高の頭脳を求めて"
#else
    "Quest of The World's Toughest Body",
    "Attack is the best form of defence.",
    "Might is right.",
    "An unexpected windfall",
    "A drowning man will catch at a straw",
    "Don't count your chickens before they are hatched.",
    "It is no use crying over spilt milk.",
    "Seeing is believing.",
    "Strike the iron while it is hot.",
    "I don't care what follows.",
    "To dig a well to put out a house on fire.",
    "Tomorrow is another day.",
    "Easy come, easy go.",
    "The more haste, the less speed.",
    "Where there is life, there is hope.",
    "There is no royal road to *WINNER*.",
    "Danger past, God forgotten.",
    "The best thing to do now is to run away.",
    "Life is but an empty dream.",
    "Dead men tell no tales.",
    "A book that remains shut is but a block.",
    "Misfortunes never come singly.",
    "A little knowledge is a dangerous thing.",
    "History repeats itself.",
    "*WINNER* was not built in a day.",
    "Ignorance is bliss.",
    "To lose is to win?",
    "No medicine can cure folly.",
    "All good things come to an end.",
    "M$ Empire strikes back.",
    "To see is to believe",
    "Time is money.",
    "Quest of The World's Greatest Brain"
#endif
};
