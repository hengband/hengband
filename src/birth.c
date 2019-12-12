/*!
 * @file birth.c
 * @brief プレイヤーの作成を行う / Create a player character
 * @date 2013/12/28
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 *\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2013 Deskull Doxygen向けのコメント整理\n
 */

#include "angband.h"
#include "util.h"
#include "bldg.h"
#include "core.h"
#include "term.h"

#include "object-ego.h"
#include "artifact.h"
#include "avatar.h"
#include "cmd-dump.h"
#include "dungeon.h"
#include "history.h"
#include "monster.h"
#include "monsterrace-hook.h"
#include "store.h"
#include "patron.h"
#include "quest.h"
#include "player-class.h"
#include "player-status.h"
#include "player-personality.h"
#include "player-sex.h"
#include "spells.h"
#include "spells-status.h"
#include "wild.h"
#include "floor.h"
#include "cmd-pet.h"
#include "dungeon-file.h"
#include "floor-town.h"
#include "files.h"
#include "birth.h"
#include "player-race.h"
#include "player-skill.h"
#include "world.h"
#include "objectkind.h"
#include "monsterrace.h"
#include "autopick.h"
#include "save.h"
#include "realm.h"
#include "japanese.h"
#include "view-mainwindow.h"

 /*
  * The last character rolled,
  * holded for quick start
  */
birther previous_char;

/*!
 * オートローラーの内容を描画する間隔 /
 * How often the autoroller will update the display and pause
 * to check for user interuptions.
 * Bigger values will make the autoroller faster, but slower
 * system may have problems because the user can't stop the
 * autoroller for this number of rolls.
 */
#define AUTOROLLER_STEP 5431L

 /*!
  * ランダムクエストのモンスターを確定するために試行する回数 / Maximum number of tries for selection of a proper quest monster
  */
#define MAX_TRIES 100

  /* 選択可能な職業の最大数 */
#define MAX_CLASS_CHOICE     MAX_CLASS



/*! 種族の解説メッセージテーブル */
static concptr race_jouhou[MAX_RACES] =
{
#ifdef JP
"人間は基本となるキャラクタです。他の全ての種族は人間と比較されます。人間はどんな職業に就くこともでき、どの職業でも平均的にこなせます。人間は寿命が短いため、レベル上昇が他のどんな種族よりも早くなる傾向があります。また、特別な修正や特性は持っていません。",

"ハーフエルフは人間より賢いですが、強くはありません。彼らは探索, 解除, 魔法防御, 隠密行動, 射撃, そして魔法道具使用でわずかに優れています。しかし武器の取り扱いはそう得意ではありません。ハーフエルフはどの職業に就くこともでき、生まれつきの特性はありません。",

"エルフは人間より良い魔法使いになれますが、戦闘は苦手です。彼らは人間やハーフエルフよりも頭が良く、高い賢さを持っています。エルフは探索, 解除, 知覚, 隠密行動, 射撃, そして魔法道具使用で優れていますが、武器の扱いは得意ではありません。彼らは生まれつき光に対する耐性を持っています。",

"ホビット、またはハーフリングは弓や投擲に長け、魔法防御も優れています。また、探索, 解除, 知覚, そして隠密行動でもとても良い能力を示します。そのため、彼らは優れた盗賊となることができます（しかし、「忍びの者」と呼ばれることを好みます）。ホビットは人間より遥かに貧弱で、戦士としてはてんでダメです。彼らはかなり良い赤外線視力を持っており、温血動物を離れた場所から見つけることができます。彼らは経験値を保持する力が強く、経験値吸収攻撃に対して耐性を持っています。",

"ノームはドワーフより小さいですが、ホビットよりは大きい種族です。彼らはホビット同様地表の洞穴のような家に住んでいます。ノームはとても良い魔法防御を持ち、探索, 解除, 知覚, 隠密行動でも優れています。彼らは人間より低い腕力を持ち、武器を持っての戦闘は苦手です。ノームはかなり良い赤外線視力を持っており、温血動物を離れた場所から見つけることができます。ノームは生まれつき麻痺に対する耐性を持っています。",

"ドワーフは頑固な坑夫であり、伝説の戦士です。彼らは人間にくらべ強くタフですが、知能は劣ります。しかし、長命ゆえに彼らは非常に賢いです。彼らは良い魔法防御を持ち、探索, 知覚, 戦闘, 射撃では優れています。彼らは一つ大きな欠点を持っています。ドワーフの隠密行動は絶望的に悪いです。彼らは決して盲目にはなりません。",

"ハーフオークはよい戦士になれますが、魔法は期待できません。彼らはドワーフと同じくらい隠密行動が悪く、また探索や解除, 知覚もひどいです。ハーフオークは醜く、店での買い物ではより高い金額を要求されがちです。彼らは地下に住むことを好むため、ハーフオークは暗闇に対する耐性を備えています。",

"ハーフトロルは信じられないほど強く、他の大部分の種族より大きなＨＰを持ちます。彼らは不運にもとても愚かです。彼らの探索, 解除, 知覚, 隠密行動は悪く、その外見はハーフオークがしかめっ面をするほど醜悪です。ハーフトロルは腕力が下がることがありません。レベルが上がると、彼らは再生能力を手にいれ、戦士ならばさらに遅消化能力も獲得します。",

"アンバライトは多くのアドバンテージを授けられた、うわさによれば不死の種族です。彼らは知覚, 戦闘, 射撃に優れており、他の面でもかなり熟練しています。事実上あらゆるものを見てきており、新鮮なものはほとんどないため、彼らの成長は他のどの種族より遅いものです。彼らはとてもタフで頑強であり、彼らの耐久力が下がることはありません。また、怪我をすぐに治す再生能力があります。",

"ハイエルフは世界の始まりから存在する不死の種族です。彼らは全てのスキルに熟達しており、強く、知的で非常に人気があります - 誰もが彼らのことを好いています。ハイエルフは見えないものを見ることができ、普通のエルフ同様光に対する耐性を持っています。しかし、彼らにとって未知のものはほとんどなく、経験を得ることは大変に困難です。",

"野蛮人は北方から来た頑強な種族です。彼らは激しく戦い、彼らの激怒は世界中で恐れられています。戦闘が彼らの人生です。彼らは恐れを知らず、ハーフトロルよりもすぐに狂暴に戦闘に入ってしまうことを学びます。しかし、野蛮人は魔法を疑っており、そのため魔法の道具を使うことはかなり大変なこととなっています。",

"ハーフオーガはハーフオークに似ていますが、それだけではありません。彼らは大きく、邪悪で愚かです。戦士としては彼らは必要な資質を全て持っており、また魔法使いになることさえできます。結局、彼らはオーガ・メイジに関係があり、レベルが十分に上がったら彼らから罠のルーンをセットするスキルを学ぶのです。ハーフオークのように、彼らは暗闇に対する耐性を持ち、ハーフトロル同様に腕力が下がることはありません。",

"半巨人は大変力強いのですが、呪文を唱えられるほど利口ではありません。彼らはよい戦闘能力を持ちますが、それ以外のことは苦手です。彼らの厚い皮膚は破片に対する耐性を持ちます。また、ハーフオーガやハーフトロル同様腕力を下げられることがありません。",

"巨大なタイタンと人間の子孫であり、この強大な生物は他のほぼ全ての種族よりはるかに勝っています。彼らは多種族にみられるような魅力的な特殊能力は持っていませんが、その大変大きなＨＰはそれを補ってあまりあります。半タイタンはそこそこのスキルを持っていますが、その巨大さゆえに罠の解除やこっそり歩くことは困難です。法と秩序を愛する彼らは、カオスに対する耐性を持っています。",

"一つ目ではありますが、サイクロプスは多くの二つの目を持つ生物以上に見ることができます。サイクロプスは非常に力強いのですが、知的であるとはちょっと言えません。彼らに比べれば、ハーフトロルの方がハンサムに見えるということは言うまでもありません。サイクロプスは戦闘, 射撃に優れていますが、その他の大部分のスキルは苦手です。サイクロプスは音に対する耐性を持っています。",

"イークは最も哀れな生物の一つであり、並のモンスターであっても不注意なイークならば徹底的に打ちのめせるほど肉体的には強くありませんが、彼らはかなり知的でいくらか賢い生物です。イークは戦闘スキルは苦手ですが、他の分野では優れています。彼らの皮膚は、時間とともに酸への耐性を増していき、レベルが十分に上がれば完全に免疫を持つようになります。",

"クラッコンは奇怪な半知的の昆虫型生物です。彼らはすばらしい戦士になれますが、精神的な能力はひどく制限されています。彼らは探索を除けば大部分のスキルをそこそこにこなします。クラッコンは決して混乱させられることがなく、レベルが上がるごとに速くなります。",

"コボルドは弱いゴブリンの種族です。彼らは毒を持った武器を好み、毒矢（無制限に供給されます）を投げる能力を身につけることができます。コボルドはそこそこの戦士になれますが、その他のスキルは軒並み悪いです。彼らは生まれつき毒に対する耐性を持っています。",

"嫌われ、迫害されてきた小人族です。彼らは大抵のスキルをそつなくこなします。洞穴居住者である彼らは、暗闇に悩まされることはありませんし、生まれつき持っている魔法のアイテムに対する嗜好のため、彼らは装備による魔法のボーナスを奪う効果に耐性を持っています。",

"闇の、洞穴に住む種族であるダークエルフは魔法の知識に対する長い伝統を持っています。ダークエルフは魔法の道具をうまく使うことができ、他の多くの種族より簡単に呪文を唱えられるだけの知能を持っています。その鋭い視覚によって、彼らはハイエルフ同様見えないものをみる能力を学びますが、それはある程度レベルが上がったときです。ダークエルフは暗闇に対する耐性を持っています。",

"ドラゴンのような特性を持った人間型種族です。彼らはレベルが上がるにつれ、新しい元素への耐性を手にいれます。ドラコニアンは優れた能力値を持ってゲームを開始でき、大抵のスキルをうまくこなせます。その翼で、彼らは簡単に落とし穴や溶岩、水を無傷で飛び越えることができます。",

"秘密主義の神秘的な古代種族です。彼らの文明はこの惑星上の何よりも古いかもしれません。その肉体的資質は決して誉められたものではありませんが、彼らの知能と賢さはマインドフレアを他のどんな種族よりも強力な魔法使いにします。マインドフレアの知能と賢さは下がることがなく、レベルが上がれば見えないものをみる能力、テレパシー能力を獲得します。",

"地獄からやってきた悪魔的な生物です。彼らは他の種族から毛嫌いされていますが、大抵の職業をかなりうまくこなすことができます。インプは生まれつき火に耐性を持っており、レベルが上がれば見えないものを見る能力を獲得します。",

"ゴーレムは泥のような生命のない材料からつくられ、生命を吹き込まれた人工的な生物です。彼らには思考というものがほとんどなく、そのため魔法に依存する職業では役立たずです。しかし戦士としては大変にタフです。彼らは毒に耐性を持ち、見えないものを見ることができ、さらに麻痺知らずです。レベルが上がれば、彼らは生命力吸収攻撃に耐性を持つようになります。ゴーレムは通常の食物からはほとんど栄養を摂取できませんが、代わりに魔法棒や杖から魔力を吸収して動力源にする事ができます。また、その頑丈な身体のため、ACにボーナスを得ることができ、さらに決して気絶させられることがありません。",

"スケルトンには2つのタイプが存在します。普通の戦士タイプと、リッチと呼ばれる呪文を使うスケルトンです。アンデッドである彼らは、毒や生命力吸収攻撃を心配する必要はありません。彼らは物体を知覚するのに眼を利用していないため、見えない物に騙されません。彼らの骨はとがった破片のようなものに耐性を持ち、レベルが上がれば冷気に対する耐性を獲得します。薬や食物の持つ効果はスケルトンの胃（存在しませんが）を通過することなくその力を発揮しますが、薬や食物自体は彼の顎を通り抜けて落ちてしまい、栄養を吸収することはできません。その代わりに魔法棒や杖から魔力を吸収してエネルギー源にする事ができます。",

"ゾンビはアンデッドであり、生命力吸収攻撃に耐性を持ち、スケルトンのようにレベルが上がれば冷気の耐性を獲得します。また、毒に耐性を持ち見えないものを見ることができます。（スケルトンとは違い）切る攻撃には弱いですが、地獄に対する耐性を持っています。ゴーレムのように、彼らは食物からほとんど栄養を摂取できませんが、代わりに魔法棒や杖から魔力を吸収してエネルギー源にする事ができます。",

"強力なアンデッドの一種である吸血鬼は、畏敬の念を呼び起こす外見をしています。アンデッドの例にもれず、彼らも生命力を吸収されることがなく、地獄に対する耐性を持っています。また、冷気と毒に対する耐性も備えています。しかし、新鮮な血液に常に飢えており、それは近くにいる生物から血液を吸うことによってのみ満たされます。この強力な生物は深刻な弱点を持っています。太陽光線（や光源）は彼らの破滅を意味します。幸運にも、吸血鬼はその身体から「暗黒の光」のオーラを放出しています。一方、暗闇は彼らをより強力にするものです。",

"幽霊は強力なアンデッドの一種です。彼らは不気味な緑色の光に包まれています。半物質的な存在である彼らは、壁を通り抜けることができますが、そのときには壁の密度によって傷つけられてしまいます。他のアンデッド同様、彼らも生命力を吸収されることがなく、見えないものを見ることができ、毒と冷気に対して耐性を備え、さらに地獄に対する耐性も持っています。レベルが十分に上がると彼らはテレパシーを獲得します。幽霊は卓越した魔法使いになることができますが、その身体的特性は非常に貧弱です。彼らは食物からほとんど栄養を摂取できませんが、代わりに魔法棒や杖から魔力を吸収してエネルギー源にする事ができます。",

"妖精は非常に小さいです。彼らは小さな翼を持ち、罠や危険な地形を飛び越えることができます。彼らは日光を大変好み、光に対する耐性を持っています。身体的にはもっとも貧弱な種族の一つですが、妖精は魔法の面で大変な才能を持っており、非常に熟練した魔法使いになることができます。高レベルではより速く飛ぶことができるようになります。",

"この種族はカオスによってつくられた冒涜的で嫌悪される存在です。彼らは独立した種族ではなく、人間型種族、大抵は人間がカオスによって歪められた存在、もしくは人間と獣の悪夢のような交配種です。全ての獣人はカオスに盲従しており、そのため混乱と音に対して耐性を備えていますが、純粋なログルスはまだ彼らに対し効果を持っています。獣人は混沌を好み、それは彼らをさらに歪めます。獣人は突然変異を起こしやすい性質を持っています。彼らがつくられたとき、ランダムな変異を受けます。その後、レベルが上がるごとに違う変異を受ける可能性があります。",

"エントは非常に強く、賢いですが、その巨大さゆえに罠の解除やこっそりと歩くことは苦手です。成長するにつれて腕力や耐久力が上がりますが、器用さは下がっていきます。彼らには大きな欠点があり、炎によって通常よりも大きなダメージを受けてしまいます。彼らは食物からほとんど栄養を摂取できませんが、代わりに薬等から水分を摂取する事で活動できます。",

"天使の上位種であるアルコンは、全てのスキルに熟達しており、強くて賢く、非常に人気があります。彼らは目に見えないものを見ることができ、その翼で罠や危険な地形を飛び越えることができます。しかし、非常に成長が遅いという欠点もあります。",

"悪魔の上位種であるバルログは、強く、知的で、またタフでもあります。しかし、彼らは神を信じようとはせず、プリーストには全く向いていません。炎と地獄、経験値吸収への耐性を持っており、レベルが上がれば見えないものを見る能力を獲得します。また、地獄や火炎のブレスを吐くこともできます。彼等はほとんどの技能で優れていますが、静かに歩くことは苦手です。彼らは食物からほとんど栄養を摂取できませんが、人間タイプを生贄にする事で精力を回復する事ができます。",

"ドゥナダンは西方から来た屈強な種族です。このいにしえの種族は全ての領域において人間の能力を凌駕し、特に耐久力に関してはそれが顕著です。しかしながらこの種族は全てに卓越していることが災いして、この世界には新しい経験といったものがほとんどなく、レベルを上げることが非常に困難です。彼らはとてもタフで頑強であり、彼らの耐久力が下がることはありません。",

"影フェアリーは人間よりやや大きい妖精族で、翼を持ち、罠や危険な地形を飛び越えることができます。しかし、彼らは日光を嫌い、閃光によって通常よりも大きなダメージを受けてしまいます。肉体的には非常に貧弱ですが、魔法の面では優れた能力を持っています。彼らにはすばらしい長所が一つあり、モンスターの反感をかうような強力なアイテムを装備してもモンスターを怒らせることがありません。ただしその場合でも隠密行動能力が下がり、また、自分自身の性格によって反感をかっている場合には効果がありません。",

"クターとしている無表情の謎の生物です。彼らは外見がかわいらしいため、魅力が高いです。彼らは混乱しません。なぜなら、混乱してもクターとしているため変わりないからです。しかも、そのクターとしている外見から敵に見つかりにくいです。しかし、彼らは注意力が少ないため探索や知覚能力は悪いです。彼らはレベルが上がると横に伸びてACを上げる技を覚えますが、伸びている間は魔法防御能力は低くなってしまいます。",

"アンドロイドは機械の身体を持つ人工的な存在です。魔法をうまく使うことはできませんが、戦士としては非常に優れています。彼らは他の種族のように経験値を得て成長するということはありません。身体に身につける装備によって成長します。ただし、指輪、アミュレット、光源は成長に影響しません。彼らは毒の耐性を持ち、麻痺知らずで、生命力を吸収されることがありません。また、身体が頑丈なのでACにボーナスを得ます。しかし身体のいたるところに電子回路が組み込まれているため、電撃によって通常よりも大きなダメージを受けてしまいます。彼らは食物からほとんど動力を得られませんが、油を補給する事で動力源を得る事ができます。",

#else

"The human is the base character.  All other races are compared to them.  Humans can choose any class and are average at everything.  Humans tend to go up levels faster than most other races because of their shorter life spans.  No racial adjustments or intrinsics occur to characters choosing human.",

"Half-elves tend to be smarter and faster than humans, but not as strong.  Half-elves are slightly better at searching, disarming, saving throws, stealth, bows, and magic, but they are not as good at hand weapons.  Half-elves may choose any class and do not receive any intrinsic abilities.",

"Elves are better magicians then humans, but not as good at fighting.  They tend to be smarter and faster than either humans or half-elves and also have better wisdom.  Elves are better at searching, disarming, perception, stealth, bows, and magic, but they are not as good at hand weapons.  They resist light effects intrinsically.",

"Hobbits, or Halflings, are very good at bows, throwing, and have good saving throws.  They also are very good at searching, disarming, perception, and stealth; so they make excellent rogues, but prefer to be called burglars.  They are much weaker than humans, and no good at melee fighting.  Halflings have fair infravision, so they can detect warm creatures at a distance.  They have a strong hold on their life force, and are thus intrinsically resistant to life draining.",

"Gnomes are smaller than dwarves but larger than Halflings.  They, like the hobbits, live in the earth in burrow-like homes.  Gnomes make excellent mages, and have very good saving throws.  They are good at searching, disarming, perception, and stealth.  They have lower strength than humans so they are not very good at fighting with hand weapons.  Gnomes have fair infravision, so they can detect warm-blooded creatures at a distance.  Gnomes are intrinsically protected against paralysis.",

"Dwarves are the headstrong miners and fighters of legend.  Dwarves tend to be stronger and tougher but slower and less intelligent than humans.  Because they are so headstrong and are somewhat wise, they resist spells which are cast on them.  They are very good at searching, perception, fighting, and bows.  Dwarves have miserable stealth.  They can never be blinded.",

"Half-orcs make excellent warriors, but are terrible at magic.  They are as bad as dwarves at stealth, and horrible at searching, disarming, and perception.  Half-orcs are quite ugly, and tend to pay more for goods in town.  Because of their preference to living underground to on the surface, half-orcs resist darkness attacks.",

"Half-Trolls are incredibly strong and have more hit points than most other races.  They are also very stupid and slow.  They are bad at searching, disarming, perception, and stealth.  They are so ugly that a Half-Orc grimaces in their presence.  They also happen to be fun to run...  Half-trolls always have their strength sustained.  At higher levels, Half-Trolls regenerate wounds automatically and, if he or she is a warrior, require food less often.",

"The Amberites are a reputedly immortal race, who are endowed with numerous advantages in addition to their longevity.  They are very tough and their constitution cannot be reduced.  Their ability to heal wounds far surpasses that of any other race.  Having seen virtually everything, very little is new to them, and they gain levels much slower than the other races.",

"High-elves are a race of immortal beings dating from the beginning of time.  They are masters of all skills, and are strong and intelligent, although their wisdom is sometimes suspect.  High-elves begin their lives able to see the unseen, and resist light effects just like regular elves.  However, there are few things that they have not seen already, and experience is very hard for them to gain.",

"Barbarians are hardy men of the north.  They are fierce in combat, and their wrath is feared throughout the world.  Combat is their life: they feel no fear, and they learn to enter battle frenzy at will even sooner than half-trolls.  Barbarians are, however, suspicious of magic, which makes magic devices fairly hard for them to use. ",

"Half-Ogres are like Half-Orcs, only more so.  They are big, bad, and stupid.  For warriors, they have all the necessary attributes, and they can even become wizards: after all, they are related to Ogre Magi, from whom they have learned the skill of setting trapped runes once their level is high enough.  Like Half-Orcs, they resist darkness, and like Half-Trolls, they have their strength sustained.",

"Half-Giants' limited intelligence makes it difficult for them to become full spellcasters, but, with their great strength, they make excellent warriors.  Their thick skin makes them resistant to shards, and like Half-Ogres and Half-Trolls, they have their strength sustained.",

"Half-mortal descendants of the mighty titans, these immensely powerful creatures put almost any other race to shame.  They may lack the fascinating special powers of certain other races, but their enhanced attributes more than make up for that.  They learn to estimate the strengths of their foes, and their love for law and order makes them resistant to the effects of Chaos.",

"With but one eye, a Cyclops can see more than many with two eyes.  They are headstrong, and loud noises bother them very little.  They are not quite qualified for the magic using professions, but as a certain Mr.  Ulysses can testify, their accuracy with thrown rocks can be deadly...",

"Yeeks are among the most pathetic creatures.  Fortunately, their horrible screams can scare away less confident foes, and their skin becomes more and more resistant to acid, as they gain experience.  But having said that, even a mediocre monster can wipe the proverbial floor with an unwary Yeek.",

"Klackons are bizarre semi-intelligent ant-like insectoid creatures.  They make great fighters, but their mental abilities are severely limited.  Obedient and well-ordered, they can never be confused.  They are also very nimble, and become faster as they advance levels.  They are also very acidic, inherently resisting acid, and capable of spitting acid at higher levels. ",

"Kobolds are a weak goblin race.  They love poisoned weapons, and can learn to throw poisoned darts (of which they carry an unlimited supply).  They are also inherently resistant to poison, although they are not one of the more powerful races.",

"The hated and persecuted race of nocturnal dwarves, these cavedwellers are not much bothered by darkness.  Their natural inclination to magical items has made them immune to effects which could drain away magical energy.",

"Another dark, cavedwelling race, likewise unhampered by darkness attacks, the Dark Elves have a long tradition and knowledge of magic.  They have an inherent magic missile attack available to them at a low level.  With their keen sight, they also learn to see invisible things as their relatives High-Elves do, but at a higher level.",

"A humanoid race with dragon-like attributes.  As they advance levels, they gain new elemental resistances (up to Poison Resistance), and they also have a breath weapon, which becomes more powerful with experience.  The exact type of the breath weapon depends on the Draconian's class and level.  With their wings, they can easily escape any pit trap unharmed.",

"A secretive and mysterious ancient race.  Their civilization may well be older than any other on our planet, and their intelligence and wisdom are naturally sustained, and are so great that they enable Mind Flayers to become more powerful spellcasters than any other race, even if their physical attributes are a good deal less admirable.  As they advance levels, they gain the powers of See Invisible and Telepathy.",

"A demon-creature from the nether-world, naturally resistant to fire attacks, and capable of learning fire bolt and fire ball attacks.  They are little loved by other races, but can perform fairly well in most professions.  As they advance levels, they gain the powers of See Invisible.",

"A Golem is an artificial creature, built from a lifeless raw material like clay, and awakened to life.  They are nearly mindless, making them useless for professions which rely on magic, but as warriors they are very tough.  They are resistant to poison, they can see invisible things, and move freely.  At higher levels, they also become resistant to attacks which threaten to drain away their experience.  Golems gain very little nutrition from ordinary food, but can absorb mana from staves and wands as their power source.  Golems also gain a natural armor class bonus from their tough body.",

"There are two types of skeletons: the ordinary, warrior-like skeletons, and the spell-using skeletons, which are also called liches.  As undead beings, skeletons need to worry very little about poison or attacks that can drain life.  They do not really use eyes for perceiving things, and are thus not fooled by invisibility.  Their bones are resistant to sharp shrapnel, and they will quickly become resistant to cold.  Although the magical effects of these will affect the skeleton even without entering the skeleton's (non-existent) belly, the potion or food itself will fall through the skeleton's jaws, giving no nutritional benefit.  They can absorb mana from staves and wands as their energy source.",

"Much like Skeletons, Zombies too are undead horrors: they are resistant to exp-draining attacks, and can learn to restore their experience.  Like skeletons, they become resistant to cold-based attacks (actually earlier than skeletons), resist poison and can see invisible.  While still vulnerable to cuts (unlike skeletons), Zombies are resistant to Nether.  Like Golems, they gain very little nutrition from the food of mortals, but can absorb mana from staves and wands as their energy source.",

"One of the mightier undead creatures, the Vampire is an awe-inspiring sight.  Yet this dread creature has a serious weakness: the bright rays of sun are its bane, and it will need to flee the surface to the deep recesses of earth until the sun finally sets.  Darkness, on the other hand, only makes the Vampire stronger.  As undead, the Vampire has a firm hold on its experience, and resists nether attacks.  The Vampire also resists cold and poison based attacks.  It is, however, susceptible to its perpetual hunger for fresh blood, which can only be satiated by sucking the blood from a nearby monster.",

"Another powerful undead creature: the Spectre is a ghastly apparition, surrounded by an unearthly green glow.  They exist only partially on our plane of existence: half-corporeal, they can pass through walls, although the density of the wall will hurt them in the process of doing this.  As undead, they have a firm hold on their experience, see invisible, and resist poison and cold.  They also resist nether.  At higher levels they develop telepathic abilities.  Spectres make superb spellcasters, but their physical form is very weak.  They gain very little nutrition from the food of mortals, but can absorb mana from staves and wands as their energy source.",

"One of several fairy races, Sprites are very small.  They have tiny wings and can fly over traps that may open up beneath them.  They enjoy sunlight intensely, and need worry little about light based attacks.  Although physically among the weakest races, Sprites are very talented in magic, and can become highly skilled wizards.  Sprites have the special power of spraying Sleeping Dust, and at higher levels they learn to fly faster.",

"This race is a blasphemous abomination produced by Chaos.  It is not an independent race but rather a humanoid creature, most often a human, twisted by the Chaos, or a nightmarish crossbreed of a human and a beast.  All Beastmen are accustomed to Chaos so much that they are untroubled by confusion and sound, although raw logrus can still have effects on them.  Beastmen revel in chaos, as it twists them more and more.  Beastmen are subject to mutations: when they have been created, they receive a random mutation.  After that, every time they advance a level they have a small chance of gaining yet another mutation.",

"The Ents are a powerful race dating from the beginning of the world, oldest of all animals or plants who inhabit Arda.  Spirits of the land, they were summoned to guard the forests of Middle-earth.  Being much like trees they are very clumsy but strong, and very susceptible to fire.  They gain very little nutrition from the food of mortals, but they can absorb water from potions as their nutrition.",

"Archons are a higher class of angels.  They are good at all skills, and are strong, wise, and are a favorite with any people.  They are able to see the unseen, and their wings allow them to safely fly over traps and other dangerous places.  However, belonging to a higher plane as they do, the experiences of this world do not leave a strong impression on them and they gain levels slowly.",

"Balrogs are a higher class of demons.  They are strong, intelligent and tough.  They do not believe in gods, and are not suitable for priest at all.  Balrog are resistant to fire and nether, and have a firm hold on their experience.  They also eventually learn to see invisible things.  They are good at almost all skills except stealth.  They gain very little nutrition from the food of mortals, and need human corpses as sacrifices to regain their vitality.",

"Dunedain are a race of hardy men from the West.  This elder race surpasses human abilities in every field, especially constitution.  However, being men of the world, very little is new to them, and levels are very hard for them to gain.  Their constitution cannot be reduced. ",

"Shadow Fairies are one of several fairy races.  They have wings, and can fly over traps that may open up beneath them.  Shadow Fairies must beware of sunlight, as they are vulnerable to bright light.  They are physically weak, but have advantages in using magic and are amazingly stealthy.  Shadow Fairies have a wonderful advantage in that they never aggravate monsters (If their equipment normally aggravates monsters, they only suffer a penalty to stealth, but if they aggravate by their personality itself, the advantage will be lost).",

"A Kutar is an expressionless animal-like living creature.  The word 'kuta' means 'absentmindedly' or 'vacantly'.  Their absentmindedness hurts their searching and perception skills, but renders them incapable of being confused.  Their unearthly calmness and serenity make them among the most stealthy of any race.  Kutars, although expressionless, are beautiful and so have a high charisma.  Members of this race can learn to expand their body horizontally.  This increases armour class, but renders them vulnerable to magical attacks.",

"An android is a artificial creation with a body of machinery.  They are poor at spell casting, but they make excellent warriors.  They don't acquire experience like other races, but rather gain in power as they attach new equipment to their frame.  Rings, amulets, and lights do not influence growth.  Androids are resistant to poison, can move freely, and are immune to exp-draining attacks.  Moreover, because of their hard metallic bodies, they get a bonus to AC.  Androids have electronic circuits throughout their body and must beware of electric shocks.  They gain very little nutrition from the food of mortals, but they can use flasks of oil as their energy source.",

#endif

_(
	"マーフォークは人型生物と水棲生物の合いの子たちの総称です。彼らは知的であり、陸上よりも水中での営みに適応している点で共通しています。彼らの真価は水中でこそ発揮されますが、彼らの文明は大抵陸上に長く滞在する何らかの術も有しています。それを失わない限り、陸上でも不自由はないでしょう。また、彼らは致命的な水難にも抗える能力を生まれつき持っています。",
	"Merfolks are general terms for people of mixed races between humanoids and aquatic habitats. They have in common that they are intelligent and adapt to working underwater rather than on land. Their true value comes in water, but their civilization also has some form of staying longer on land. As long as you do not lose it, there will be no inconvenience on land. They also have the ability to withstand disaster by water."
),

};

/*! 職業の解説メッセージテーブル */
static concptr class_jouhou[MAX_CLASS] =
{
#ifdef JP
"戦士は、直面する問題のほとんどを細切れに叩き切ることで解決するキャラクタです。が、時折退却して魔法の道具の世話になることもあります。不運にも、高レベルなアイテムの多くは彼らが扱える範囲を越えています。",

"メイジは魔法使いであり、その機知によって生き延びなければなりません。戦士のように、単純に切りまくることで道を開くことは望めません。呪文書に加えて、メイジは助けになる魔法の道具を持ち運ぶべきです。これは他の何よりも遥かに簡単にマスターできます。魔法に必要な能力値は知能です。",

"プリーストは高貴な力を使うことに専念したキャラクタです。彼らは自身の神のためにダンジョンを探索し、もし宝を手にいれたなら、それは彼が信仰する宗教の栄光となります。プリーストは新しい祈りを神からの贈り物という形で受け取るため、どれを学ぶのか自分で選ぶことはできません。プリーストは魔法の道具の使い方をよく知っていますが、メイジほどうまくは使えません。刃のついた武器より鈍器を好み、祝福されていない刃のついた武器を装備すると不愉快な感覚に襲われ、戦闘能力が落ちてしまいます。魔法に必要な能力値は賢さです。",

"盗賊はその狡猾さで生き抜くことを好むキャラクタですが、肝心なときには戦闘で道を切り開くことができます、盗賊は罠やドアを見つける能力に優れ、罠の解除や鍵開けに熟達しています。盗賊は高い隠密行動を持ち、たくさんのモンスターの群れのそばを起こすことなく通り抜けたり、忍び寄って先制攻撃することができます。魔法に必要な能力値は知能です。",

"レンジャーは戦士とメイジを合わせたような職業で、身の回りの自然と特別な関係を作り上げています。彼はより戦士であり、弓のような遠距離武器を巧く使える職業です。レンジャーはよい隠密行動、よい知覚、よい探索、よい魔法防御を持ち、魔法の道具の使用にも長けています。魔法に必要な能力値は知能です。",

"パラディンは戦士とプリーストを合わせた職業です。パラディンはとてもよい戦士ですが、遠距離武器を扱うのは得意ではありません。パラディンには多くの能力が欠けています。隠密行動, 知覚, 探索, そして魔法道具使用が苦手ですが、その神との提携によって魔法防御はそこそこです。魔法に必要な能力値は賢さです。",

"魔法戦士はその名称が意味する通りの職業であり、戦士とメイジの資質をあわせ持ちます。彼らの同業者であるレンジャーが自然の魔法と生き抜くためのスキルに特化している一方、本当の魔法剣士はどちらの世界でも一番になろうとしています。戦士としては普通のメイジとは比べ物にならないほど優れています。しかし、実際には魔法でも戦闘でも専門の職業には及ばず、戦士とメイジの中間に位置するような職業です。魔法に必要な能力値は知能です。",

"混沌の戦士は恐るべきカオスの魔王の使いとして恐れられる存在です。混沌の戦士はパトロンとなる悪魔を持ち、レベルが上がる度に報酬を得ることがあります。彼は治療してくれたり、こちらを変化させたり、能力値を上げてくれるかもしれませんし、回りに怪物達を出現させたり、能力値や装備を奪うかも知れません。もしくは単にこちらを無視するだけかもしれません。カオスの魔王は無秩序で予測のつかない存在です。報酬の種類はパトロンとなる悪魔と偶然に依存します（違う悪魔は異なる報酬を与えます）。魔法に必要な能力値は知能です。",

"修行僧は他の職業とは著しく異なる職業です。彼らは他の職業同様武器と防具を使えますが、マーシャルアーツの訓練を積んでいるため、武器、防具なしでより強力な存在となります。高レベルでは、必要な耐性を身につけるためある種の防具を装備する必要がありますが、もしあまりに重すぎる防具を装備してしまうと、その体術に深刻な妨げとなります。レベルが上がると、彼らは新しい強力な攻撃法を学び、防御能力も上昇します。魔法に必要な能力値は賢さです。",

"超能力者は魔法のかわりにその精神の力を使う唯一の職業です。この力は超能力者独特のもので、単に超感覚的なものから他人の精神を支配するものまで様々です。彼らの力はある種の訓練によって開発されるものなので、超能力者は力を使うのに呪文書を必要としません。使える力は単純にキャラクタのレベルによって決まります。超能力に必要な能力値は賢さです。",

"ハイメイジは一つの領域に特化し、その領域を通常のメイジよりはるかに深く学んだメイジです。１つの領域に特化したおかげで、彼らは自らが選択した領域の呪文を唱える際の消費ＭＰ、最低レベル、失敗率で相当な恩恵を受けます。しかし、生命の領域ではプリーストほどうまくはなれないことには注意すべきです。魔法に必要な能力値は知能です。",

"観光客は観光のためにこの世界にやってきました。戦闘力が低く、強力な呪文を使うこともできないため、最も生きぬいていくのが厳しい職業と言えます。魔法に必要な能力値は知能です。",

"ものまね師は戦闘力はそこそこありますが、自分から特殊な能力を使うことは全くできません。しかし、自分の目の前にいる相手が特殊能力を使った場合、その能力と全く同じ能力をそっくりそのまま使うことができます。ものまねに必要な能力は基本的に器用さですが、まねる特殊能力に関係ある他の能力も必要です。",

"魔獣使いは変愚蛮怒世界のダンジョンに住む生物と心を通い合わせられます。彼らは最もうまくモンスターを乗りこなすことができ、召喚したり手なづけたりしたモンスターを自分の手足のように使います。魔法に必要な能力は魅力です。",

"スペルマスターは全ての魔法を極める者です。彼らは全分野において非常に優れた魔法使いであり、あらゆる魔法書のすべての呪文を学習の手間なく使いこなすことができます。その反面、彼らは戦士としては最低で、どんな武器も満足に扱えません。魔術師の杖だけは例外ですが、武器としては使い物にならないでしょう。すべての魔法をうまく生かさなければならないため、非常に上級者向けな職業と言えます。魔法に必要な能力は知能です。",

"アーチャーは魔法を使うことはできませんが、どんな職業よりも巧みに弓やスリングを使いこなします。大量の矢や弾を必要とするのは確かですが、岩石からスリング用の弾を作ったり、レベルが上がるとモンスターの骨やがらくたから矢を作ったりする技術を身につけます。また、戦士と比べて隠密行動、知覚、探索、魔法道具の使用などにも優れており、いざというときには魔法の道具に頼ることもできます。",

"魔道具術師は杖、魔法棒、ロッドといった魔法のアイテムから魔力を取り込むことによって魔法を使います。魔法のアイテムを発見することが他の職業よりもはるかに重要になります。戦闘力は高くはないですが、そこそこの強さがあります。魔法に必要な能力は知能です。",

"吟遊詩人は魔力を帯びた歌を歌うことができます。多くの歌は普通の魔法と異なり、歌を歌っている間継続して効果を発揮します。しかし、同時に2つの歌を歌うことができない、という欠点もあります。視界内全体に影響を及ぼす歌が多い、という特徴もあります。肉体的な能力は貧弱で、単純に切りまくることで道を開くことはできません。魔法に必要な能力は魅力です。",

"赤魔道師は下級魔法のほとんどを使うことができ、戦闘力も十分にあります。レベルが上がると強力な能力「連続魔」を身につけることができます。しかし、魔法を覚えるスピードは遅く、上級魔法を使えないので、メイジほどには魔法を頼りにすることができません。魔法道具使用と魔法防御はそこそこですが、それ以外の技能は苦手です。魔法に必要な能力は知能です。",

"剣術家は戦士に次ぐ戦闘力があり、様々な技を使うことができます。彼らのMPはレベルに依存せず、賢さだけで決まり、気合いをためることにより、最大値を越えてMPを増やすことができます。しかし、戦士と同様、高レベルの魔法のアイテムは彼らの扱える範囲を越えており、罠の解除や探索の能力も高いとはいえません。必殺技の使用に必要な能力は賢さです。",

"練気術師は「気」を使う達人です。修行僧と同様、武器や防具を持たずに戦うことを好み、武器・防具なしでより強力な存在となります。修行僧ほどの戦闘能力はありませんが、修行僧と同様の魔法が使え、さらに「気」の力を操ります。武器を持つことや、重すぎる防具を装備することは、「気」の力の使用を妨げます。魔法と練気術に必要な能力は賢さです。",

"青魔道師は優れた魔法使いであり、その機知によって生き延びなければなりません。メイジ等の他の魔法使いとの違いは魔法の覚え方で、青魔道師はモンスターの魔法の効果を受けることでその魔法を覚えます。覚えるためには「ラーニング」の状態になっていないといけません。魔法に必要な能力は知能です。",

"騎兵は馬に乗り戦場を駆け抜けるエリート戦士です。魔法は使えませんが、馬上からの圧倒的な攻撃力を誇る上に、高い機動力を生かした射撃をも得意としています。レベルが上がれば、野生のモンスターにまたがり無理矢理手なずけることができます。彼らは己の肉体と精神に誇りを持ち、魔法道具にはあまり頼ろうとはしません。",

"狂戦士は怒り狂って武器を振るう恐るべき戦士です。全職業中最高の肉体能力を誇り、恐怖と麻痺に対する耐性を持ち、レベルが上がればその強靭な肉体で矢の呪文を跳ね返すことができます。さらに武器なしで戦うことや、呪いのかけられた装備を力づくで剥がすことができ、いくつかの技を(反魔法状態でも)使うことができます。しかし、巻物や魔法道具は全く使うことができず、罠の解除や隠密行動、探索、魔法防御、飛び道具の技能に関しては絶望的です。ひたすら殴って道を開くしかありません。幽霊は非常に勝利しやすいですがスコアがかなり低く修正されます。",

"鍛冶師は武器や防具を自分で強化することができます。特殊効果を持つ武器や防具から特殊効果の元となるエッセンスを取り出し、別の武器や防具にエッセンスを付加することによってその特殊効果を付加できます。ある程度の戦闘能力も持ちますが、魔法は一切使用できず、隠密や魔法防御の技能も低くなります。",

"鏡使いは、魔力の込められた鏡を作り出して、それを触媒として攻撃を行なうことができる鏡魔法を使います。鏡使いは鏡の上で実力を発揮し、鏡の上では素早いテレポートが可能となります。魔法の鏡は、レベルによって一度に制御できる数が制限されます。鏡魔法に必要な能力は知能です。",

"忍者は暗闇に潜む恐るべき暗殺者であり、光源を持たずに行動し、相手の不意をつき一撃で息の根を止めます。また、相手を惑わすための忍術も身につけます。罠やドアを見つける能力に優れ、罠の解除や鍵開けに熟達しています。軽装を好み、重い鎧や武器を装備すると著しく動きが制限され、また、盾を装備しようとはしません。軽装ならば、レベルが上がるにつれより速くより静かに行動できます。さらに忍者は恐怖せず、成長すれば毒がほとんど効かなくなり、透明なものを見ることができるようになります。忍術に必要な能力は器用さです。",

"スナイパーは一撃必殺を狙う恐るべき射手です。精神を高めることにより、射撃の威力と精度を高めます。また、魔法を使うことはできませんが、研ぎ澄まされた精神から繰り出される射撃術はさらなる威力をもたらすことでしょう。テクニックが必要とされる職業です。"

#else

"A Warrior is a hack-and-slash character, who solves most of his or her problems by cutting them to pieces, but will occasionally fall back on the help of a magical device.  Unfortunately, many high-level devices may be forever beyond their use.",

"A Mage is a spell caster that must live by his or her wits as a mage cannot hope to simply hack his or her way through the dungeon like a warrior.  In addition to spellbooks, a mage should carry a range of magical devices to help his or her endeavors.  A mage can master those devices far more easily than anyone else.  A mage's prime statistic is Intelligence as this determines his or her spell casting ability.",

"A Priest is a character devoted to serving a higher power.  He or she explores the dungeon in the service of a God.  Since Priests receive new prayers as gifts from their patron deity, they cannot choose which ones they will learn.  Priests are familiar with magical devices which they believe act as foci for divine intervention in the natural order of things.  A priest wielding an edged weapon will be so uncomfortable with it that his or her fighting ability decreases.  A Priest's primary stat is Wisdom since this determines the success of the prayers to his or her deity.",

"A Rogue is a character that prefers to live by his or her cunning, but is capable of fighting out of a tight spot.  Rogues are good at locating hidden traps and doors and are the masters of disarming traps and picking locks.  A rogue is very stealthy, allowing him or her to sneak around many creatures without having to fight or to get in a telling first blow.  A rogue may also backstab a fleeing monster.  Intelligence determines a Rogue's spell casting ability.",

"A Ranger is a combination of a warrior and a mage who has developed a special affinity for the natural world.  He or she is a good fighter and also good with missile weapons such as bows.  A ranger has a good stealth, good perception, good searching, a good saving throw and is good with magical devices.  Intelligence determines a Ranger's spell casting ability.",

"A Paladin is a combination of a warrior and a priest.  Paladins are very good fighters, but not very good at missile weapons.  A paladin lacks much in the way of abilities.  He or she is poor at stealth, perception, searching, and magical devices but has a decent saving throw due to his or her divine alliance.  Wisdom determines a Paladin's success at praying to his or her deity.",

"A Warrior-Mage is precisely what the name suggests: a cross between the warrior and mage classes.  Unlike rangers who specialize in Nature magic and survival skills, true Warrior-Mages attempt to reach the best of both worlds.  As warriors they are much superior to the usual Mage class.  Intelligence determines a Warrior-Mage's spell casting ability.",

"Chaos Warriors are the feared servants of the terrible Demon Lords of Chaos.  Every Chaos Warrior has a Patron Demon and, when gaining a level, may receive a reward from his or her Patron.  He or she might be healed or polymorphed, given an awesome weapon, or have his or her stats increased.  On the other hand, the Patron might surround the Chaos Warrior with monsters, drain his or her stats, wreck his or her equipment, or simply ignore the Chaos Warrior.  The Demon Lords of Chaos are chaotic and unpredictable indeed.  The exact type of reward depends on both the Patron Demon (different Demons give different rewards) and chance.",

"The Monk character class is very different from all other classes.  Their training in martial arts makes them much more powerful with no armor or weapons.  To gain the resistances necessary for survival a monk may need to wear some kind of armor, but if the armor he or she wears is too heavy, it will severely disturb his or her martial arts maneuvers.  As the monk advances levels, new, powerful forms of attack become available.  Their defensive capabilities increase likewise, but if armour is being worn, this effect decreases.  Wisdom determines a Monk's spell casting ability.",

"The Mindcrafter is a unique class that uses the powers of the mind instead of magic.  These powers are unique to Mindcrafters, and vary from simple extrasensory powers to mental domination of others.  Since these powers are developed by the practice of certain disciplines, a Mindcrafter requires no spellbooks to use them.  The available powers are simply determined by the character's level.  Wisdom determines a Mindcrafter's ability to use mind powers.",

"High-mages are mages who specialize in one particular field of magic and learn it very well - much better than the ordinary mage.  For the price of giving up a second realm of magic, they gain substantial benefits in the mana costs, minimum levels, and failure rates in the spells of the realm of their specialty.  A high mage's prime statistic is intelligence as this determines his or her spell casting ability.",

"Tourists have visited this world for the purpose of sightseeing.  Their fighting skills are bad, and they cannot cast powerful spells.  They are the most difficult class to win the game with.  Intelligence determines a tourist's spell casting ability.",

"Imitators have enough fighting skills to survive, but rely on their ability to imitate monster spells.  When monsters in line of sight use spells, they are added to a temporary spell list which the imitator can choose among.  Spells should be imitated quickly, because timing and situation are everything.  An imitator can only repeat a spell once each time he or she observes it.  Dexterity determines general imitation ability, but a stat related to the specific action is often also taken into account.",

"Beastmasters are in tune with the minds of the creatures of the world of Hengband.  They are very good at riding and have enough fighting ability.  The monsters that a beastmaster summons or dominates become the beastmaster's hands and feet.  Beastmasters can cast trump magic, and are very good at summoning spells, but they can not summon non-living creatures.  Charisma determines a Beastmaster's spell casting ability.",

"Sorcerers are the all-around best magicians, being able to cast any spell from most magic realms without having to learn it.  On the downside, they are the worst fighters in the dungeon, being unable to use any weapon but a Wizardstaff.",

"Archers are to bows what warriors are to melee.  They are the best class around with any bow, crossbow, or sling.  They need a lot of ammunition, but will learn how to make it from junk found in the dungeon.  An archer is better than a warrior at stealth, perception, searching and magical devices.",

"Magic-Eaters can absorb the energy of wands, staffs, and rods, and can then use these magics as if they were carrying all of these absorbed devices.  They are middling-poor at fighting.  A Magic-Eater's prime statistic is intelligence.",

"Bards are something like traditional musicians.  Their magical attacks are sound-based, and last as long as the Bard has mana.  Although a bard cannot sing two or more songs at the same time, he or she does have the advantage that many songs affect all areas in sight.  A bard's prime statistic is charisma.",

"Red-Mages can use almost all spells from lower rank spellbooks of most realms without having to learn it.  At higher level, they develop the powerful ability \"Double Magic\".  However, they have large penalties in the mana costs, minimum levels, and failure rates of spells, and they cannot use any spells from higher rank spellbooks.  They are not bad at using magical devices and magic resistance, and are decent fighters, but are bad at other skills.  A red-mage's prime statistic is intelligence.",

"Samurai, masters of the art of the blade, are the next strongest fighters after Warriors.  Their spellpoints do not depend on level, but depend solely on wisdom, and they can use the technique Concentration to temporarily increase SP beyond its usual maximum value.  Samurai are not good at most other skills, and many magical devices may be too difficult for them to use.  Wisdom determines a Samurai's ability to use the special combat techniques available to him or her.",

"A ForceTrainer is a master of the spiritual Force.  They prefer fighting with neither weapons nor armor.  They are not as good fighters as are Monks, but they can use both magic and the spiritual Force.  Wielding weapons or wearing heavy armor disturbs use of the Force.  Wisdom is a ForceTrainer's primary stat.",

"A Blue-Mage is a spell caster that must live by his or her wits, as a Blue-Mage cannot hope to simply hack his or her way through the dungeon like a warrior.  A major difference between the Mage and the Blue-Mage is the method of learning spells: Blue-Mages may learn spells from monsters by activating his or her Learning ability.  A Blue-Mage's prime statistic is Intelligence as this determines his or her spell casting ability.",

"Cavalry ride on horses into battle.  Although they cannot cast spells, they are proud of their overwhelming offensive strength on horseback.  They are good at shooting.  At high levels, they learn to forcibly saddle and tame wild monsters.  Since they take pride in the body and the soul, they don't use magical devices well.",

"A Berserker is a fearful fighter indeed, immune to fear and paralysis.  At high levels, Berserkers can reflect bolt spells with their tough flesh.  Furthermore, they can fight without weapons, can remove cursed equipment by force, and can even use their special combat techniques when surrounded by an anti-magic barrier.  Berserkers, however, cannot use any magical devices or read any scrolls, and are hopeless at all non-combat skills.  Since Berserker Spectres are quite easy to *win* with, their scores are lowered.",

"A Weaponsmith can improve weapons and armor for him or herself.  They can extract the essences of special effects from weapons or armor which have various special abilities, and can add these essences to another weapon or armor.  They are good at fighting, but cannot cast spells, and are poor at skills such as stealth or magic defense.",

"Mirror-Masters are spell casters; like other mages, they must live by their wits.  They can create magical mirrors, and employ them in the casting of Mirror-Magic spells.  A Mirror-Master standing on a mirror has greater ability and, for example, can perform quick teleports.  The maximum number of Magical Mirrors which can be controlled simultaneously depends on the level.  Intelligence determines a Mirror-Master's spell casting ability.",

"A Ninja is a fearful assassin lurking in darkness.  He or she can navigate effectively with no light source, catch enemies unawares, and kill with a single blow.  Ninjas can use Ninjutsu, and are good at locating hidden traps and doors, disarming traps and picking locks.  Since heavy armor, heavy weapons, or shields will restrict their motion greatly, they prefer light clothes, and become faster and more stealthy as they gain levels.  A Ninja knows no fear and, at high level, becomes almost immune to poison and able to see invisible things.  Dexterity determines a Ninja's ability to use Ninjutsu.",

"Snipers are good at shooting, and they can kill targets by a few shots. After they concentrate deeply, they can demonstrate their shooting talents. You can see incredibly firepower of their shots."
#endif
};

/*! 性格の解説メッセージテーブル */
static concptr seikaku_jouhou[MAX_SEIKAKU] =
{
#ifdef JP
"ふつうは、特に特筆するべき部分がない性格です。あらゆる技能を平均的にこなします。",

"ちからじまんは、肉体的な能力や技能が上昇します。しかし、魔法に関係する能力や技能は劣り、戦士よりのステータスを持ちます。",

"きれものは、肉体的な能力は下がりますが、知能や魔法に関係する技能は上昇し、メイジよりのステータスを持ちます。",

"しあわせものは、神を信仰する能力が高くなります。肉体的には平均的な能力を持ち、プリーストに近いステータスとなります。",

"すばしっこいは、どのスキルも比較的うまくこなしますが、肉体的な能力は低くなります。",

"いのちしらずは、戦闘力、魔法能力の両方が上昇しますが、魔法防御、ＨＰといった能力は悪くなります。",

"好きな食べ物は焼きビーフン。抑えてはいるが、冒険心旺盛な一匹狼。正義感、勇気とも平均以上だがカッとしやすい所もある。計画的人生より行き当たりばったりの人生を選んでしまうタイプで、異性の扱いは苦手。",

"なまけものは、あらゆるスキルが低く、何をやってもうまくいきません。",

"セクシーギャルは、あらゆるスキルをうまくこなすことができます。しかし、その人をなめた性格は全てのモンスターを怒らせることになるでしょう。この性格は女性しか選ぶことができません。",

"ラッキーマンは、能力値はなまけものに匹敵するくらい低いにもかかわらず、どんなことをしてもなぜかうまくいってしまいます。この性格は男性しか選ぶことができません。",

"がまんづよいは、じっくりと物事にとりくむ慎重な性格で、他の性格に比べて高い耐久力を得ることができます。しかし、自分から行動するのは苦手で、多くの技能は低くなってしまいます。",

"いかさまは、初心者の練習用の性格です。あらゆる能力が高くなっています。この性格を使えば勝利者になることは容易ですが、勝利しても全く自慢になりません。",

"チャージマンは「こんなところ」に連れて行かれても仕方のない可愛そうなお友達なんＤＡ。腕っ節やタフさはマンモス並みに強いのだけれど知能面はまるで駄目なのが分かるだろう？この性格は最初から気が狂っているので、混乱したり幻覚を見る心配がないのです。",

#else

"\"Ordinary\" is a personality with no special skills or talents, with unmodified stats and skills.",

"\"Mighty\" raises your physical stats and skills, but reduces stats and skills which influence magic.  It makes your stats suitable for a warrior.  Also it directly influences your hit-points and spell fail rate.",

"\"Shrewd\" reduces your physical stats, and raises your intelligence and magical skills.  It makes your stats suitable for a mage.  Also it directly influences your hit-points and spell fail rate.",

"\"Pious\" deepens your faith in your God.  It makes your physical ability average, and your stats suitable for priest. ",

"\"Nimble\" improves most skills except for melee combat.",

"\"Fearless\" raises both your melee and magical ability.  Stats such as magic defense and constitution are reduced.  Also it has a direct bad influence on your hit-points.",

"\"Combat\" gives you comparatively high melee and shooting abilities, and average constitution.  Other skills such as stealth, magic defence, and magical devices are weakened.  All \"Combat\" people have great respect for the legendary \"Combat Echizen\".\n\
(See \"Death Crimson\" / Ecole Software Corp.)",

"A \"Lazy\" person has no good stats and can do no action well.  Also it has a direct bad influence on your spell fail rate.",

"\"Sexy\" rises all of your abilities, but your haughty attitude will aggravate all monsters.  Only females can choose this personality.",

"A \"Lucky\" man has poor stats, equivalent to a \"Lazy\" person.  Mysteriously, however, he can do all things well.  Only males can choose this personality.",

"A \"Patient\" person does things carefully.  Patient people have high constitution, and high resilience, but poor abilities in most other skills.  Also it directly influences your hit-points.",

"\"Munchkin\" is a personality for beginners.  It raises all your stats and skills.  With this personality, you can win the game easily, but gain little honor in doing so.",

"\"ChargeMan\" is a crazy killer. You are strong and tough but have poor intelligence.  Since you're already insane, confusion and hallucinations do not affect you.",

#endif
};

/*! 魔法領域の詳細解説メッセージテーブル */
static concptr realm_jouhou[VALID_REALM] =
{
#ifdef JP
"生命は回復能力に優れた魔法です。治療や防御、感知魔法が多く含まれていますが、攻撃呪文もわずかに持っています。特に高レベルの呪文にはアンデッドを塵に帰す力をあると言われています。",

"仙術は「meta」領域であり、感知や鑑定、さらに退却用の呪文や自身の能力を高める呪文などの便利な呪文が含まれています。しかし、直接攻撃用の呪文は持っていません。",

"自然の魔法は使用者を元素のマスターにします。これには防御、探知、治療と攻撃呪文が含まれています。また、生命以外の領域で最高の治療呪文もこの領域にあります。",

"カオスの魔法は制御が困難で、予測のできない魔法もあります。カオスは非常に非元素的であり、カオスの呪文は想像できる最も恐るべき破壊兵器です。この呪文を唱えるものはカオスの尖兵に対し、敵や自分自身さえも変異させるよう要求します。",

"黒魔術である暗黒の魔法ほど邪悪なカテゴリーはありません。これらの呪文は比較的学ぶのが困難ですが、高レベルになると術者に生物とアンデッドを自由に操る能力を与えます。残念なことに、もっとも強力な呪文はその触媒として術者自身の血を必要とし、詠唱中にしばしば術者を傷つけます。",

"トランプの魔法はテレポート系の呪文で精選されたものを持っており、その出入り口は他の生物を召喚するためにも使えるため、召喚呪文から選りすぐられたものも同様に持っています。しかし、この魔法によって全ての怪物が別の場所へ呼ばれるのを理解するわけではなく、もし召喚呪文に失敗するとその生物は敵となります。",

"秘術の魔法は、全ての領域から有用な呪文だけを取り入れようとした多用途領域です。必要な「道具」的呪文を持っていても高レベルの強力な呪文は持っていません。結果として、全ての呪文書は街で買い求めることができます。また、他の領域に存在する同様な呪文の方がより低レベル、低コストで唱えることができます。",

"匠の魔法は、自分や道具を強化するための魔法が含まれています。魔法によって自分自身の戦闘力を非常に高めることができますが、相手を直接攻撃するような呪文は含まれていません。",

"悪魔の魔法は暗黒と同様非常に邪悪なカテゴリーです。様々な攻撃魔法に優れ、また悪魔のごとき知覚能力を得ることができます。高レベルの呪文は悪魔を自在に操り、自分自身の肉体をも悪魔化させることができます。",

"破邪は「正義」の魔法です。直接敵を傷つける魔法が多く含まれ、特に邪悪な敵に対する力は恐るべきものがあります。しかし、善良な敵にはあまり効果がありません。",

"歌集は、歌によって効果を発揮する魔法です。魔法と同様、使った時に効果のあるものと、歌い続けることによって持続して効果を発揮するものがあります。後者の場合は、MPの続く限り効果を発揮することができますが、同時に歌える歌は1つだけという制限もあります。",

"武芸の書は、様々な戦闘の技について書かれています。この本は技を覚えるときに読む必要がありますが、一度覚えた技は使うのに本を持つ必要はありません。技を使うときには必ず武器を装備していなければいけません。",

"呪術は忌むべき領域です。複数の呪いの言葉を歌のように紡ぎながら詠唱します。多くの呪文は詠唱し続けることによって効果が持続されます。呪文には相手の行動を束縛するもの、ダメージを与えるもの、攻撃に対して反撃するものが多くあります。"
#else

"Life magic is very good for healing; it relies mostly on healing, protection and detection spells.  Also life magic has a few attack spells as well.  It's said that some high level spells of life magic can disintegrate Undead monsters into ash.",

"Sorcery is a `meta` realm, including enchantment and general spells.  It provides superb protection spells, spells to enhance your odds in combat and, most importantly, a vast selection of spells for gathering information.  However, Sorcery has one weakness: it has no spells to deal direct damage to your enemies.",

"Nature magic makes you a master of elements; it provides protection, detection, curing and attack spells.  Nature also has a spell of Herbal Healing, which is the only powerful healing spell outside the realm of Life magic.",

"There are few types of magic more unpredictable and difficult to control than Chaos magic.  Chaos is the very element of unmaking, and the Chaos spells are the most terrible weapons of destruction imaginable.  The caster can also call on the primal forces of Chaos to induce mutations in his/her opponents and even him/herself.",

"There is no fouler nor more evil category of spells than the necromantic spells of Death Magic.  These spells are relatively hard to learn, but at higher levels the spells give the caster power over living and the (un)dead, but the most powerful spells need his / her own blood as the focus, often hurting the caster in the process of casting.",

"Trump magic has, indeed, an admirable selection of teleportation spells.  Since the Trump gateways can also be used to summon other creatures, Trump magic has an equally impressive selection of summoning spells.  However, not all monsters appreciate being drawn to another place by Trump user.",

"Arcane magic is a general purpose realm of magic.  It attempts to encompass all 'useful' spells from all realms.  This is the downside of Arcane magic: while Arcane does have all the necessary 'tool' spells for a dungeon delver, it has no ultra-powerful high level spells.  As a consequence, all Arcane spellbooks can be bought in town.  It should also be noted that the 'specialized' realms usually offer the same spell at a lower level and cost. ",

"Craft magic can strengthen the caster or his or her equipment.  These spells can greatly improve the caster's fighting ability.  Using them against opponents directly is not possible.",

"Demon is a very evil realm, same as Death.  It provides various attack spells and devilish detection spells.  At higher levels, Demon magic provides the ability to dominate demons and to polymorph yourself into a demon.",

"Crusade is a magic of 'Justice'.  It includes damage spells, which are greatly effective against foul and evil monsters, but have poor effects against good monsters.",

"Music magic works through the caster singing songs.  There are two types of songs; one which shows effects instantly and another which shows effects continuously until SP runs out.  The latter type has a limit:  only one song can be sung at the same time.",

"The books of Kendo describe various combat techniques.  When learning new techniques, you are required to carry the books, but once you memorizes them, you don't have to carry them.  When using a technique, wielding a weapon is required.",

"Hex is an unsavory realm, like the death and demon realms.  Many spells can act continuously by stringing together curses like a song.  Spells may obstruct monsters' actions, deal damage to monsters in sight, or return damage to monsters who have damaged the caster."
#endif
};

/*! 魔法領域の簡易解説メッセージテーブル */
static concptr realm_subinfo[VALID_REALM] =
{
#ifdef JP
"感知と防御と回復に優れています",
"攻撃はできませんが非常に便利です",
"感知と防御に優れています",
"破壊的な攻撃に優れています",
"生命のある敵への攻撃に優れています",
"召喚とテレポートに優れています",
"やや弱いながらも非常に便利です",
"直接戦闘の補助に優れています",
"攻撃と防御の両面に優れています",
"邪悪な怪物に対する攻撃に優れています",
"様々な魔法効果を持った歌を歌います",
"打撃攻撃に特殊能力を付加します",
"敵を邪魔しつつ復讐を狙います"
#else
"Good at detection and healing.",
"Utility and protective spells.",
"Good at detection and defence.",
"Offensive and destructive.",
"Ruins living creatures.",
"Good at summoning and teleportation.",
"Very useful but not as potent.",
"Support for melee fighting.",
"Good at both offence and defence.",
"Destroys evil creatures.",
"Songs with magical effects.",
"Special attacks on melee.",
"Good at obstacle and revenge."
#endif
};


/*! オートローラの能力値的要求水準 / Autoroll limit */
static s16b stat_limit[6];

/*! オートローラの年齢、身長、体重、社会的地位の要求水準 */
static struct {
	s16b agemin, agemax;
	s16b htmin, htmax;
	s16b wtmin, wtmax;
	s16b scmin, scmax;
} chara_limit;

/*! オートローラ中、各能力値が水準を超えた回数 / Autoroll matches */
static s32b stat_match[6];

/*! オートローラの試行回数 / Autoroll round */
static s32b auto_round;

/*!
 * @brief プレイヤー作成を中断して変愚蛮怒を終了する
 * @return なし
 */
static void birth_quit(void)
{
	quit(NULL);
}

/*!
 * @brief 指定されたヘルプファイルを表示する / Show specific help file
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param helpfile ファイル名
 * @return なし
 */
static void show_help(player_type *creature_ptr, concptr helpfile)
{
	screen_save();
	(void)show_file(creature_ptr, TRUE, helpfile, NULL, 0, 0);
	screen_load();
}


/*!
 * @brief プレイヤーの魔法領域を選択する / Choose from one of the available magical realms
 * @param choices 選択可能な魔法領域のビット配列
 * @param count 選択可能な魔法領域を返すポインタ群。
 * @return 選択した魔法領域のID
 */
static byte choose_realm(player_type *creature_ptr, s32b choices, int *count)
{
	int picks[VALID_REALM] = { 0 };
	int k, i, cs, os;
	byte auto_select = REALM_NONE;
	int n = 0;
	char c;
	char sym[VALID_REALM];
	char p2 = ')';
	char buf[80], cur[80];

	/* Count the choices */
	if (choices & CH_LIFE)
	{
		(*count)++;
		auto_select = REALM_LIFE;
	}
	if (choices & CH_SORCERY)
	{
		(*count)++;
		auto_select = REALM_SORCERY;
	}
	if (choices & CH_NATURE)
	{
		(*count)++;
		auto_select = REALM_NATURE;
	}
	if (choices & CH_CHAOS)
	{
		(*count)++;
		auto_select = REALM_CHAOS;
	}
	if (choices & CH_DEATH)
	{
		(*count)++;
		auto_select = REALM_DEATH;
	}
	if (choices & CH_TRUMP)
	{
		(*count)++;
		auto_select = REALM_TRUMP;
	}
	if (choices & CH_ARCANE)
	{
		(*count)++;
		auto_select = REALM_ARCANE;
	}
	if (choices & CH_ENCHANT)
	{
		(*count)++;
		auto_select = REALM_CRAFT;
	}
	if (choices & CH_DAEMON)
	{
		(*count)++;
		auto_select = REALM_DAEMON;
	}
	if (choices & CH_CRUSADE)
	{
		(*count)++;
		auto_select = REALM_CRUSADE;
	}
	if (choices & CH_MUSIC)
	{
		(*count)++;
		auto_select = REALM_MUSIC;
	}
	if (choices & CH_HISSATSU)
	{
		(*count)++;
		auto_select = REALM_HISSATSU;
	}
	if (choices & CH_HEX)
	{
		(*count)++;
		auto_select = REALM_HEX;
	}

	clear_from(10);

	/* Auto-select the realm */
	if ((*count) < 2) return auto_select;

	/* Constraint to the 1st realm */
	if (creature_ptr->realm2 != 255)
	{
		if (creature_ptr->pclass == CLASS_PRIEST)
		{
			if (is_good_realm(creature_ptr->realm1))
			{
				choices &= ~(CH_DEATH | CH_DAEMON);
			}
			else
			{
				choices &= ~(CH_LIFE | CH_CRUSADE);
			}
		}
	}

	/* Extra info */
	put_str(_("注意：魔法の領域の選択によりあなたが習得する呪文のタイプが決まります。", "Note: The realm of magic will determine which spells you can learn."), 23, 5);

	cs = 0;
	for (i = 0; i < 32; i++)
	{
		/* Analize realms */
		if (choices & (1L << i))
		{
			if (creature_ptr->realm1 == i + 1)
			{
				if (creature_ptr->realm2 == 255)
					cs = n;
				else
					continue;
			}
			if (creature_ptr->realm2 == i + 1)
				cs = n;

			sym[n] = I2A(n);

			sprintf(buf, "%c%c %s", sym[n], p2, realm_names[i + 1]);
			put_str(buf, 12 + (n / 5), 2 + 15 * (n % 5));
			picks[n++] = i + 1;
		}
	}
	sprintf(cur, "%c%c %s", '*', p2, _("ランダム", "Random"));

	/* Get a realm */
	k = -1;
	os = n;
	while (TRUE) {
		/* Move Cursol */
		if (cs != os)
		{
			c_put_str(TERM_WHITE, cur, 12 + (os / 5), 2 + 15 * (os % 5));
			put_str("                                   ", 3, 40);
			put_str("                                   ", 4, 40);

			if (cs == n)
			{
				sprintf(cur, "%c%c %s", '*', p2, _("ランダム", "Random"));
			}
			else
			{
				sprintf(cur, "%c%c %s", sym[cs], p2, realm_names[picks[cs]]);
				sprintf(buf, "%s", realm_names[picks[cs]]);
#ifdef JP
				c_put_str(TERM_L_BLUE, buf, 3, 40);
				put_str("の特徴", 3, 40 + strlen(buf));
#else
				c_put_str(TERM_L_BLUE, realm_names[picks[cs]], 3, 40);
				put_str(": Characteristic", 3, 40 + strlen(realm_names[picks[cs]]));
#endif
				put_str(realm_subinfo[technic2magic(picks[cs]) - 1], 4, 40);
			}
			c_put_str(TERM_YELLOW, cur, 12 + (cs / 5), 2 + 15 * (cs % 5));
			os = cs;
		}

		if (k >= 0) break;

		sprintf(buf, _("領域を選んで下さい(%c-%c) ('='初期オプション設定): ", "Choose a realm (%c-%c) ('=' for options): "), sym[0], sym[n - 1]);

		put_str(buf, 10, 10);
		c = inkey();
		if (c == 'Q') birth_quit();
		if (c == 'S') return 255;
		if (c == ' ' || c == '\r' || c == '\n')
		{
			if (cs == n)
			{
				k = randint0(n);
				break;
			}
			else
			{
				k = cs;
				break;
			}
		}
		if (c == '*')
		{
			k = randint0(n);
			break;
		}
		if (c == '8')
		{
			if (cs >= 5) cs -= 5;
		}
		if (c == '4')
		{
			if (cs > 0) cs--;
		}
		if (c == '6')
		{
			if (cs < n) cs++;
		}
		if (c == '2')
		{
			if ((cs + 5) <= n) cs += 5;
		}
		k = (islower(c) ? A2I(c) : -1);
		if ((k >= 0) && (k < n))
		{
			cs = k;
			continue;
		}
		k = (isupper(c) ? (26 + c - 'A') : -1);
		if ((k >= 26) && (k < n))
		{
			cs = k;
			continue;
		}
		else k = -1;
		if (c == '?')
		{
			show_help(creature_ptr, _("jmagic.txt#MagicRealms", "magic.txt#MagicRealms"));
		}
		else if (c == '=')
		{
			screen_save();
			do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth option((*)s effect score)"));

			screen_load();
		}
		else if (c != '2' && c != '4' && c != '6' && c != '8') bell();
	}

	/* Clean up */
	clear_from(10);

	return (byte_hack)(picks[k]);
}


/*!
 * @brief 選択した魔法領域の解説を表示する / Choose the magical realms
 * @return ユーザが魔法領域の確定を選んだらTRUEを返す。
 */
static bool get_player_realms(player_type *creature_ptr)
{
	int i, count;

	/* Clean up infomation of modifications */
	put_str("                                   ", 3, 40);
	put_str("                                   ", 4, 40);
	put_str("                                   ", 5, 40);

	/* Select the first realm */
	creature_ptr->realm1 = REALM_NONE;
	creature_ptr->realm2 = 255;
	while (TRUE)
	{
		char temp[80 * 10];
		concptr t;
		count = 0;
		creature_ptr->realm1 = choose_realm(creature_ptr, realm_choices1[creature_ptr->pclass], &count);

		if (255 == creature_ptr->realm1) return FALSE;
		if (!creature_ptr->realm1) break;

		/* Clean up*/
		clear_from(10);
		put_str("                                   ", 3, 40);
		put_str("                                   ", 4, 40);
		put_str("                                   ", 5, 40);

		roff_to_buf(realm_jouhou[technic2magic(creature_ptr->realm1) - 1], 74, temp, sizeof(temp));
		t = temp;
		for (i = 0; i < 10; i++)
		{
			if (t[0] == 0)
				break;
			else
			{
				prt(t, 12 + i, 3);
				t += strlen(t) + 1;
			}
		}

		if (count < 2)
		{
			prt(_("何かキーを押してください", "Hit any key."), 0, 0);
			(void)inkey();
			prt("", 0, 0);
			break;
		}
		else if (get_check_strict(_("よろしいですか？", "Are you sure? "), CHECK_DEFAULT_Y)) break;
	}

	/* Select the second realm */
	creature_ptr->realm2 = REALM_NONE;
	if (creature_ptr->realm1)
	{
		/* Print the realm */
		put_str(_("魔法        :", "Magic       :"), 6, 1);
		c_put_str(TERM_L_BLUE, realm_names[creature_ptr->realm1], 6, 15);

		/* Select the second realm */
		while (TRUE)
		{
			char temp[80 * 8];
			concptr t;

			count = 0;
			creature_ptr->realm2 = choose_realm(creature_ptr, realm_choices2[creature_ptr->pclass], &count);

			if (255 == creature_ptr->realm2) return FALSE;
			if (!creature_ptr->realm2) break;

			/* Clean up*/
			clear_from(10);
			put_str("                                   ", 3, 40);
			put_str("                                   ", 4, 40);
			put_str("                                   ", 5, 40);

			roff_to_buf(realm_jouhou[technic2magic(creature_ptr->realm2) - 1], 74, temp, sizeof(temp));
			t = temp;
			for (i = 0; i < A_MAX; i++)
			{
				if (t[0] == 0)
					break;
				else
				{
					prt(t, 12 + i, 3);
					t += strlen(t) + 1;
				}
			}

			if (count < 2)
			{
				prt(_("何かキーを押してください", "Hit any key."), 0, 0);
				(void)inkey();
				prt("", 0, 0);
				break;
			}
			else if (get_check_strict(_("よろしいですか？", "Are you sure? "), CHECK_DEFAULT_Y)) break;
		}
		if (creature_ptr->realm2)
		{
			/* Print the realm */
			c_put_str(TERM_L_BLUE, format("%s, %s", realm_names[creature_ptr->realm1], realm_names[creature_ptr->realm2]), 6, 15);
		}
	}

	return TRUE;
}


/*!
 * @brief プレイヤーのクイックスタート情報をプレイヤー構造体から保存する / Save the current data for later
 * @param birther_ptr クイックスタート構造体の参照ポインタ
 * @return なし。
 */
static void save_prev_data(player_type *creature_ptr, birther *birther_ptr)
{
	int i;

	/* Save the data */
	birther_ptr->psex = creature_ptr->psex;
	birther_ptr->prace = creature_ptr->prace;
	birther_ptr->pclass = creature_ptr->pclass;
	birther_ptr->pseikaku = creature_ptr->pseikaku;
	birther_ptr->realm1 = creature_ptr->realm1;
	birther_ptr->realm2 = creature_ptr->realm2;
	birther_ptr->age = creature_ptr->age;
	birther_ptr->ht = creature_ptr->ht;
	birther_ptr->wt = creature_ptr->wt;
	birther_ptr->sc = creature_ptr->sc;
	birther_ptr->au = creature_ptr->au;

	/* Save the stats */
	for (i = 0; i < A_MAX; i++)
	{
		birther_ptr->stat_max[i] = creature_ptr->stat_max[i];
		birther_ptr->stat_max_max[i] = creature_ptr->stat_max_max[i];
	}

	/* Save the hp */
	for (i = 0; i < PY_MAX_LEVEL; i++)
	{
		birther_ptr->player_hp[i] = creature_ptr->player_hp[i];
	}

	birther_ptr->chaos_patron = creature_ptr->chaos_patron;

	/* Save the virtues */
	for (i = 0; i < 8; i++)
	{
		birther_ptr->vir_types[i] = creature_ptr->vir_types[i];
	}

	/* Save the history */
	for (i = 0; i < 4; i++)
	{
		strcpy(birther_ptr->history[i], creature_ptr->history[i]);
	}
}


/*!
 * @brief プレイヤーのクイックスタート情報をプレイヤー構造体へ読み込む / Load the previous data
 * @param swap TRUEならば現在のプレイヤー構造体上との内容をスワップする形で読み込む。
 * @return なし。
 */
static void load_prev_data(player_type *creature_ptr, bool swap)
{
	int i;

	birther	temp;

	/*** Save the current data ***/
	if (swap) save_prev_data(creature_ptr, &temp);


	/*** Load the previous data ***/

	/* Load the data */
	creature_ptr->psex = previous_char.psex;
	creature_ptr->prace = previous_char.prace;
	creature_ptr->pclass = previous_char.pclass;
	creature_ptr->pseikaku = previous_char.pseikaku;
	creature_ptr->realm1 = previous_char.realm1;
	creature_ptr->realm2 = previous_char.realm2;
	creature_ptr->age = previous_char.age;
	creature_ptr->ht = previous_char.ht;
	creature_ptr->wt = previous_char.wt;
	creature_ptr->sc = previous_char.sc;
	creature_ptr->au = previous_char.au;

	/* Load the stats */
	for (i = 0; i < A_MAX; i++)
	{
		creature_ptr->stat_cur[i] = creature_ptr->stat_max[i] = previous_char.stat_max[i];
		creature_ptr->stat_max_max[i] = previous_char.stat_max_max[i];
	}

	/* Load the hp */
	for (i = 0; i < PY_MAX_LEVEL; i++)
	{
		creature_ptr->player_hp[i] = previous_char.player_hp[i];
	}
	creature_ptr->mhp = creature_ptr->player_hp[0];
	creature_ptr->chp = creature_ptr->player_hp[0];

	creature_ptr->chaos_patron = previous_char.chaos_patron;

	for (i = 0; i < 8; i++)
	{
		creature_ptr->vir_types[i] = previous_char.vir_types[i];
	}

	/* Load the history */
	for (i = 0; i < 4; i++)
	{
		strcpy(creature_ptr->history[i], previous_char.history[i]);
	}

	/*** Save the previous data ***/
	if (swap)
	{
		(void)COPY(&previous_char, &temp, birther);
	}
}




/*!
 * @brief プレイヤーの能力値表現に基づいて加減算を行う。
 * @param value 現在の能力値
 * @param amount 加減算する値
 * @return 加減算の結果
 */
static int adjust_stat(int value, int amount)
{
	int i;

	/* Negative amounts */
	if (amount < 0)
	{
		/* Apply penalty */
		for (i = 0; i < (0 - amount); i++)
		{
			if (value >= 18 + 10)
			{
				value -= 10;
			}
			else if (value > 18)
			{
				value = 18;
			}
			else if (value > 3)
			{
				value--;
			}
		}
	}

	/* Positive amounts */
	else if (amount > 0)
	{
		/* Apply reward */
		for (i = 0; i < amount; i++)
		{
			if (value < 18)
			{
				value++;
			}
			else
			{
				value += 10;
			}
		}
	}

	/* Return the result */
	return (value);
}




/*!
 * @brief プレイヤーの能力値を一通りロールする。 / Roll for a characters stats
 * @details
 * calc_bonuses()による、独立ステータスからの副次ステータス算出も行っている。
 * For efficiency, we include a chunk of "calc_bonuses()".\n
 * @return なし
 */
static void get_stats(player_type *creature_ptr)
{
	/* Roll and verify some stats */
	while (TRUE)
	{
		int i;
		int sum = 0;

		/* Roll some dice */
		for (i = 0; i < 2; i++)
		{
			s32b tmp = randint0(60 * 60 * 60);
			BASE_STATUS val;

			/* Extract 5 + 1d3 + 1d4 + 1d5 */
			val = 5 + 3;
			val += tmp % 3; tmp /= 3;
			val += tmp % 4; tmp /= 4;
			val += tmp % 5; tmp /= 5;

			/* Save that value */
			sum += val;
			creature_ptr->stat_cur[3 * i] = creature_ptr->stat_max[3 * i] = val;

			/* Extract 5 + 1d3 + 1d4 + 1d5 */
			val = 5 + 3;
			val += tmp % 3; tmp /= 3;
			val += tmp % 4; tmp /= 4;
			val += tmp % 5; tmp /= 5;

			/* Save that value */
			sum += val;
			creature_ptr->stat_cur[3 * i + 1] = creature_ptr->stat_max[3 * i + 1] = val;

			/* Extract 5 + 1d3 + 1d4 + 1d5 */
			val = 5 + 3;
			val += tmp % 3; tmp /= 3;
			val += tmp % 4; tmp /= 4;
			val += (BASE_STATUS)tmp;

			/* Save that value */
			sum += val;
			creature_ptr->stat_cur[3 * i + 2] = creature_ptr->stat_max[3 * i + 2] = val;
		}

		/* Verify totals */
		if ((sum > 42 + 5 * 6) && (sum < 57 + 5 * 6)) break;
		/* 57 was 54... I hate 'magic numbers' :< TY */
	}
}

/*!
 * @brief プレイヤーの限界ステータスを決める。
 * @return なし
 */
void get_max_stats(player_type *creature_ptr)
{
	int i, j;
	int dice[6];

	/* Roll and verify some stats */
	while (TRUE)
	{
		/* Roll some dice */
		for (j = i = 0; i < A_MAX; i++)
		{
			/* Roll the dice */
			dice[i] = randint1(7);

			/* Collect the maximum */
			j += dice[i];
		}

		/* Verify totals */
		if (j == 24) break;
	}

	/* Acquire the stats */
	for (i = 0; i < A_MAX; i++)
	{
		BASE_STATUS max_max = 18 + 60 + dice[i] * 10;

		/* Save that value */
		creature_ptr->stat_max_max[i] = max_max;
		if (creature_ptr->stat_max[i] > max_max)
			creature_ptr->stat_max[i] = max_max;
		if (creature_ptr->stat_cur[i] > max_max)
			creature_ptr->stat_cur[i] = max_max;
	}
	creature_ptr->knowledge &= ~(KNOW_STAT);
	creature_ptr->redraw |= (PR_STATS);
}


/*!
 * @brief その他「オートローラ中は算出の対象にしない」副次ステータスを処理する / Roll for some info that the auto-roller ignores
 * @return なし
 */
static void get_extra(player_type *creature_ptr, bool roll_hitdie)
{
	int i, j;

	/* Experience factor */
	if (creature_ptr->prace == RACE_ANDROID) creature_ptr->expfact = rp_ptr->r_exp;
	else creature_ptr->expfact = rp_ptr->r_exp + cp_ptr->c_exp;

	if (((creature_ptr->pclass == CLASS_MONK) || (creature_ptr->pclass == CLASS_FORCETRAINER) || (creature_ptr->pclass == CLASS_NINJA)) && ((creature_ptr->prace == RACE_KLACKON) || (creature_ptr->prace == RACE_SPRITE)))
		creature_ptr->expfact -= 15;

	/* Reset record of race/realm changes */
	creature_ptr->start_race = creature_ptr->prace;
	creature_ptr->old_race1 = 0L;
	creature_ptr->old_race2 = 0L;
	creature_ptr->old_realm = 0;

	for (i = 0; i < 64; i++)
	{
		if (creature_ptr->pclass == CLASS_SORCERER) creature_ptr->spell_exp[i] = SPELL_EXP_MASTER;
		else if (creature_ptr->pclass == CLASS_RED_MAGE) creature_ptr->spell_exp[i] = SPELL_EXP_SKILLED;
		else creature_ptr->spell_exp[i] = SPELL_EXP_UNSKILLED;
	}

	for (i = 0; i < 5; i++)
		for (j = 0; j < 64; j++)
			creature_ptr->weapon_exp[i][j] = s_info[creature_ptr->pclass].w_start[i][j];
	if ((creature_ptr->pseikaku == SEIKAKU_SEXY) && (creature_ptr->weapon_exp[TV_HAFTED - TV_WEAPON_BEGIN][SV_WHIP] < WEAPON_EXP_BEGINNER))
	{
		creature_ptr->weapon_exp[TV_HAFTED - TV_WEAPON_BEGIN][SV_WHIP] = WEAPON_EXP_BEGINNER;
	}

	for (i = 0; i < GINOU_MAX; i++)
		creature_ptr->skill_exp[i] = s_info[creature_ptr->pclass].s_start[i];

	/* Hitdice */
	if (creature_ptr->pclass == CLASS_SORCERER)
		creature_ptr->hitdie = rp_ptr->r_mhp / 2 + cp_ptr->c_mhp + ap_ptr->a_mhp;
	else
		creature_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;

	/* Roll for hit point unless quick-start */
	if (roll_hitdie) roll_hitdice(creature_ptr, SPOP_NO_UPDATE);

	/* Initial hitpoints */
	creature_ptr->mhp = creature_ptr->player_hp[0];
}


/*!
 * @brief プレイヤーの生い立ちの自動生成を行う。 / Get the racial history, and social class, using the "history charts".
 * @return なし
 */
static void get_history(player_type *creature_ptr)
{
	int i, n, chart, roll, social_class;

	char *s, *t;

	char buf[240];

	/* Clear the previous history strings */
	for (i = 0; i < 4; i++) creature_ptr->history[i][0] = '\0';

	/* Clear the history text */
	buf[0] = '\0';

	/* Initial social class */
	social_class = randint1(4);

	/* Starting place */
	switch (creature_ptr->prace)
	{
	case RACE_AMBERITE:
	{
		chart = 67;
		break;
	}
	case RACE_HUMAN:
	case RACE_BARBARIAN:
	case RACE_DUNADAN:
	{
		chart = 1;
		break;
	}
	case RACE_HALF_ELF:
	{
		chart = 4;
		break;
	}
	case RACE_ELF:
	case RACE_HIGH_ELF:
	{
		chart = 7;
		break;
	}
	case RACE_HOBBIT:
	{
		chart = 10;
		break;
	}
	case RACE_GNOME:
	{
		chart = 13;
		break;
	}
	case RACE_DWARF:
	{
		chart = 16;
		break;
	}
	case RACE_HALF_ORC:
	{
		chart = 19;
		break;
	}
	case RACE_HALF_TROLL:
	{
		chart = 22;
		break;
	}
	case RACE_DARK_ELF:
	{
		chart = 69;
		break;
	}
	case RACE_HALF_OGRE:
	{
		chart = 74;
		break;
	}
	case RACE_HALF_GIANT:
	{
		chart = 75;
		break;
	}
	case RACE_HALF_TITAN:
	{
		chart = 76;
		break;
	}
	case RACE_CYCLOPS:
	{
		chart = 77;
		break;
	}
	case RACE_YEEK:
	{
		chart = 78;
		break;
	}
	case RACE_KOBOLD:
	{
		chart = 82;
		break;
	}
	case RACE_KLACKON:
	{
		chart = 84;
		break;
	}
	case RACE_NIBELUNG:
	{
		chart = 87;
		break;
	}
	case RACE_DRACONIAN:
	{
		chart = 89;
		break;
	}
	case RACE_MIND_FLAYER:
	{
		chart = 92;
		break;
	}
	case RACE_IMP:
	{
		chart = 94;
		break;
	}
	case RACE_GOLEM:
	{
		chart = 98;
		break;
	}
	case RACE_SKELETON:
	{
		chart = 102;
		break;
	}
	case RACE_ZOMBIE:
	{
		chart = 107;
		break;
	}
	case RACE_VAMPIRE:
	{
		chart = 113;
		break;
	}
	case RACE_SPECTRE:
	{
		chart = 118;
		break;
	}
	case RACE_SPRITE:
	{
		chart = 124;
		break;
	}
	case RACE_BEASTMAN:
	{
		chart = 129;
		break;
	}
	case RACE_ENT:
	{
		chart = 137;
		break;
	}
	case RACE_ANGEL:
	{
		chart = 142;
		break;
	}
	case RACE_DEMON:
	{
		chart = 145;
		break;
	}
	case RACE_S_FAIRY:
	{
		chart = 148;
		break;
	}
	case RACE_KUTAR:
	{
		chart = 154;
		break;
	}
	case RACE_ANDROID:
	{
		chart = 155;
		break;
	}
	case RACE_MERFOLK:
	{
		chart = 170;
		break;
	}
	default:
	{
		chart = 0;
		break;
	}
	}


	/* Process the history */
	while (chart)
	{
		/* Start over */
		i = 0;

		/* Roll for nobility */
		roll = randint1(100);


		/* Access the proper entry in the table */
		while ((chart != bg[i].chart) || (roll > bg[i].roll))
		{
			i++;
		}

		/* Acquire the textual history */
		(void)strcat(buf, bg[i].info);

		/* Add in the social class */
		social_class += (int)(bg[i].bonus) - 50;

		/* Enter the next chart */
		chart = bg[i].next;
	}


	/* Verify social class */
	if (social_class > 100) social_class = 100;
	else if (social_class < 1) social_class = 1;

	/* Save the social class */
	creature_ptr->sc = (s16b)social_class;


	/* Skip leading spaces */
	for (s = buf; *s == ' '; s++) /* loop */;

	/* Get apparent length */
	n = strlen(s);

	/* Kill trailing spaces */

	while ((n > 0) && (s[n - 1] == ' ')) s[--n] = '\0';

	{
		char temp[64 * 4];
		roff_to_buf(s, 60, temp, sizeof(temp));
		t = temp;
		for (i = 0; i < 4; i++) {
			if (t[0] == 0)break;
			else { strcpy(creature_ptr->history[i], t); t += strlen(t) + 1; }
		}
	}
}

/*!
 * @brief プレイヤーの身長体重を決める / Get character's height and weight
 * @return なし
 */
void get_height_weight(player_type *creature_ptr)
{
	int h_percent; /* 身長が平均にくらべてどのくらい違うか. */

	/* Calculate the height/weight for males */
	if (creature_ptr->psex == SEX_MALE)
	{
		creature_ptr->ht = randnor(rp_ptr->m_b_ht, rp_ptr->m_m_ht);
		h_percent = (int)(creature_ptr->ht) * 100 / (int)(rp_ptr->m_b_ht);
		creature_ptr->wt = randnor((int)(rp_ptr->m_b_wt) * h_percent / 100
			, (int)(rp_ptr->m_m_wt) * h_percent / 300);
	}

	/* Calculate the height/weight for females */
	else if (creature_ptr->psex == SEX_FEMALE)
	{
		creature_ptr->ht = randnor(rp_ptr->f_b_ht, rp_ptr->f_m_ht);
		h_percent = (int)(creature_ptr->ht) * 100 / (int)(rp_ptr->f_b_ht);
		creature_ptr->wt = randnor((int)(rp_ptr->f_b_wt) * h_percent / 100
			, (int)(rp_ptr->f_m_wt) * h_percent / 300);
	}
}


/*!
 * @brief プレイヤーの年齢を決める。 / Computes character's age, height, and weight by henkma
 * @details 内部でget_height_weight()も呼び出している。
 * @return なし
 */
static void get_ahw(player_type *creature_ptr)
{
	/* Get character's age */
	creature_ptr->age = rp_ptr->b_age + randint1(rp_ptr->m_age);

	/* Get character's height and weight */
	get_height_weight(creature_ptr);
}

/*!
 * @brief プレイヤーの初期所持金を決める。 / Get the player's starting money
 * @return なし
 */
static void get_money(player_type *creature_ptr)
{
	int i, gold;

	/* Social Class determines starting gold */
	gold = (creature_ptr->sc * 6) + randint1(100) + 300;
	if (creature_ptr->pclass == CLASS_TOURIST)
		gold += 2000;

	/* Process the stats */
	for (i = 0; i < A_MAX; i++)
	{
		/* Mega-Hack -- reduce gold for high stats */
		if (creature_ptr->stat_max[i] >= 18 + 50) gold -= 300;
		else if (creature_ptr->stat_max[i] >= 18 + 20) gold -= 200;
		else if (creature_ptr->stat_max[i] > 18) gold -= 150;
		else gold -= (creature_ptr->stat_max[i] - 8) * 10;
	}

	/* Minimum 100 gold */
	if (gold < 100) gold = 100;

	if (creature_ptr->pseikaku == SEIKAKU_NAMAKE)
		gold /= 2;
	else if (creature_ptr->pseikaku == SEIKAKU_MUNCHKIN)
		gold = 10000000;
	if (creature_ptr->prace == RACE_ANDROID) gold /= 5;

	/* Save the gold */
	creature_ptr->au = gold;
}



/*!
 * @brief put_stats()のサブルーチンとして、オートロール中のステータスを表示する / Display stat values, subset of "put_stats()"
 * @details See 'display_player(p_ptr, )' for screen layout constraints.
 * @return なし
 */
static void birth_put_stats(player_type *creature_ptr)
{
	int i, j, m, p;
	int col;
	TERM_COLOR attr;
	char buf[80];


	if (autoroller)
	{
		col = 42;
		/* Put the stats (and percents) */
		for (i = 0; i < A_MAX; i++)
		{
			/* Race/Class bonus */
			j = rp_ptr->r_adj[i] + cp_ptr->c_adj[i] + ap_ptr->a_adj[i];

			/* Obtain the current stat */
			m = adjust_stat(creature_ptr->stat_max[i], j);

			/* Put the stat */
			cnv_stat(m, buf);
			c_put_str(TERM_L_GREEN, buf, 3 + i, col + 24);

			/* Put the percent */
			if (stat_match[i])
			{
				if (stat_match[i] > 1000000L)
				{
					/* Prevent overflow */
					p = stat_match[i] / (auto_round / 1000L);
				}
				else
				{
					p = 1000L * stat_match[i] / auto_round;
				}

				attr = (p < 100) ? TERM_YELLOW : TERM_L_GREEN;
				sprintf(buf, "%3d.%d%%", p / 10, p % 10);
				c_put_str(attr, buf, 3 + i, col + 13);
			}

			/* Never happened */
			else
			{
				c_put_str(TERM_RED, _("(なし)", "(NONE)"), 3 + i, col + 13);
			}
		}
	}
}


/*!
 * @brief ベースアイテム構造体の鑑定済みフラグをリセットする。
 * @return なし
 */
static void k_info_reset(void)
{
	int i;

	/* Reset the "objects" */
	for (i = 1; i < max_k_idx; i++)
	{
		object_kind *k_ptr = &k_info[i];

		/* Reset "tried" */
		k_ptr->tried = FALSE;

		/* Reset "aware" */
		k_ptr->aware = FALSE;
	}
}


/*!
 * @brief プレイヤー構造体の内容を初期値で消去する(名前を除く) / Clear all the global "character" data (without name)
 * @return なし
 */
static void player_wipe_without_name(player_type *creature_ptr)
{
	int i;
	player_type tmp;

	/* Temporary copy for migration - written back later */
	COPY(&tmp, creature_ptr, player_type);

	/* Hack -- free the "last message" string */
	if (creature_ptr->last_message) string_free(creature_ptr->last_message);

	if (creature_ptr->inventory_list != NULL) C_WIPE(creature_ptr->inventory_list, INVEN_TOTAL, object_type);

	/* Hack -- zero the struct */
	(void)WIPE(creature_ptr, player_type);

	//TODO: キャラ作成からゲーム開始までに  current_floor_ptr を参照しなければならない処理は今後整理して外す。
	creature_ptr->current_floor_ptr = &floor_info;

	C_MAKE(creature_ptr->inventory_list, INVEN_TOTAL, object_type);

	/* Wipe the history */
	for (i = 0; i < 4; i++)
	{
		strcpy(creature_ptr->history[i], "");
	}

	/* Wipe the quests */
	for (i = 0; i < max_q_idx; i++)
	{
		quest_type* const q_ptr = &quest[i];

		q_ptr->status = QUEST_STATUS_UNTAKEN;

		q_ptr->cur_num = 0;
		q_ptr->max_num = 0;
		q_ptr->type = 0;
		q_ptr->level = 0;
		q_ptr->r_idx = 0;
		q_ptr->complev = 0;
		q_ptr->comptime = 0;
	}

	/* No weight */
	creature_ptr->total_weight = 0;

	/* No items */
	creature_ptr->inven_cnt = 0;
	creature_ptr->equip_cnt = 0;

	/* Clear the inventory */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		object_wipe(&creature_ptr->inventory_list[i]);
	}

	/* Start with no artifacts made yet */
	for (i = 0; i < max_a_idx; i++)
	{
		artifact_type *a_ptr = &a_info[i];
		a_ptr->cur_num = 0;
	}

	/* Reset the objects */
	k_info_reset();

	/* Reset the "monsters" */
	for (i = 1; i < max_r_idx; i++)
	{
		monster_race *r_ptr = &r_info[i];

		/* Hack -- Reset the counter */
		r_ptr->cur_num = 0;

		/* Hack -- Reset the max counter */
		r_ptr->max_num = 100;

		/* Hack -- Reset the max counter */
		if (r_ptr->flags1 & RF1_UNIQUE) r_ptr->max_num = 1;

		/* Hack -- Non-unique Nazguls are semi-unique */
		else if (r_ptr->flags7 & RF7_NAZGUL) r_ptr->max_num = MAX_NAZGUL_NUM;

		/* Clear visible kills in this life */
		r_ptr->r_pkills = 0;

		/* Clear all kills in this life */
		r_ptr->r_akills = 0;
	}


	/* Hack -- Well fed player */
	creature_ptr->food = PY_FOOD_FULL - 1;


	/* Wipe the spells */
	if (creature_ptr->pclass == CLASS_SORCERER)
	{
		creature_ptr->spell_learned1 = creature_ptr->spell_learned2 = 0xffffffffL;
		creature_ptr->spell_worked1 = creature_ptr->spell_worked2 = 0xffffffffL;
	}
	else
	{
		creature_ptr->spell_learned1 = creature_ptr->spell_learned2 = 0L;
		creature_ptr->spell_worked1 = creature_ptr->spell_worked2 = 0L;
	}
	creature_ptr->spell_forgotten1 = creature_ptr->spell_forgotten2 = 0L;
	for (i = 0; i < 64; i++) creature_ptr->spell_order[i] = 99;
	creature_ptr->learned_spells = 0;
	creature_ptr->add_spells = 0;
	creature_ptr->knowledge = 0;

	/* Clean the mutation count */
	creature_ptr->mutant_regenerate_mod = 100;

	/* Clear "cheat" options */
	cheat_peek = FALSE;
	cheat_hear = FALSE;
	cheat_room = FALSE;
	cheat_xtra = FALSE;
	cheat_know = FALSE;
	cheat_live = FALSE;
	cheat_save = FALSE;
	cheat_diary_output = FALSE;
	cheat_turn = FALSE;

	/* Assume no winning game */
	current_world_ptr->total_winner = FALSE;

	creature_ptr->timewalk = FALSE;

	/* Assume no panic save */
	creature_ptr->panic_save = 0;

	/* Assume no cheating */
	current_world_ptr->noscore = 0;
	current_world_ptr->wizard = FALSE;

	/* Not waiting to report score */
	creature_ptr->wait_report_score = FALSE;

	/* Default pet command settings */
	creature_ptr->pet_follow_distance = PET_FOLLOW_DIST;
	creature_ptr->pet_extra_flags = (PF_TELEPORT | PF_ATTACK_SPELL | PF_SUMMON_SPELL);

	/* Wipe the recall depths */
	for (i = 0; i < current_world_ptr->max_d_idx; i++)
	{
		max_dlv[i] = 0;
	}

	creature_ptr->visit = 1;

	/* Reset wild_mode to FALSE */
	creature_ptr->wild_mode = FALSE;

	for (i = 0; i < 108; i++)
	{
		creature_ptr->magic_num1[i] = 0;
		creature_ptr->magic_num2[i] = 0;
	}

	/* Level one */
	creature_ptr->max_plv = creature_ptr->lev = 1;

	/* Initialize arena and rewards information -KMW- */
	creature_ptr->arena_number = 0;
	creature_ptr->current_floor_ptr->inside_arena = FALSE;
	creature_ptr->current_floor_ptr->inside_quest = 0;
	for (i = 0; i < MAX_MANE; i++)
	{
		creature_ptr->mane_spell[i] = -1;
		creature_ptr->mane_dam[i] = 0;
	}

	creature_ptr->mane_num = 0;
	creature_ptr->exit_bldg = TRUE; /* only used for arena now -KMW- */

	/* Bounty */
	creature_ptr->today_mon = 0;

	/* Reset monster arena */
	update_gambling_monsters(creature_ptr);

	/* Reset mutations */
	creature_ptr->muta1 = 0;
	creature_ptr->muta2 = 0;
	creature_ptr->muta3 = 0;

	/* Reset virtues */
	for (i = 0; i < 8; i++) creature_ptr->virtues[i] = 0;

	creature_ptr->dungeon_idx = 0;

	/* Set the recall dungeon accordingly */
	if (vanilla_town || ironman_downward)
	{
		creature_ptr->recall_dungeon = DUNGEON_ANGBAND;
	}
	else
	{
		creature_ptr->recall_dungeon = DUNGEON_GALGALS;
	}

	/* Data migration */
	memcpy(creature_ptr->name, tmp.name, sizeof(tmp.name));
}



/*!
 * @brief ダンジョン内部のクエストを初期化する / Initialize random quests and final quests
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void init_dungeon_quests(player_type *creature_ptr)
{
	int number_of_quests = MAX_RANDOM_QUEST - MIN_RANDOM_QUEST + 1;
	int i;

	/* Init the random quests */
	init_flags = INIT_ASSIGN;
	floor_type *floor_ptr = creature_ptr->current_floor_ptr;
	floor_ptr->inside_quest = MIN_RANDOM_QUEST;

	process_dungeon_file(creature_ptr, "q_info.txt", 0, 0, 0, 0);

	floor_ptr->inside_quest = 0;

	/* Generate quests */
	for (i = MIN_RANDOM_QUEST + number_of_quests - 1; i >= MIN_RANDOM_QUEST; i--)
	{
		quest_type *q_ptr = &quest[i];
		monster_race *quest_r_ptr;

		q_ptr->status = QUEST_STATUS_TAKEN;
		determine_random_questor(creature_ptr, q_ptr);

		/* Mark uniques */
		quest_r_ptr = &r_info[q_ptr->r_idx];
		quest_r_ptr->flags1 |= RF1_QUESTOR;

		q_ptr->max_num = 1;
	}

	/* Init the two main quests (Oberon + Serpent) */
	init_flags = INIT_ASSIGN;
	floor_ptr->inside_quest = QUEST_OBERON;

	process_dungeon_file(creature_ptr, "q_info.txt", 0, 0, 0, 0);

	quest[QUEST_OBERON].status = QUEST_STATUS_TAKEN;

	floor_ptr->inside_quest = QUEST_SERPENT;

	process_dungeon_file(creature_ptr, "q_info.txt", 0, 0, 0, 0);

	quest[QUEST_SERPENT].status = QUEST_STATUS_TAKEN;
	floor_ptr->inside_quest = 0;
}


/*!
 * @brief ゲームターンを初期化する / Reset turn
 * @details アンデッド系種族は開始時刻を夜からにする。
 * @return なし
 */
static void init_turn(player_type *creature_ptr)
{
	if ((creature_ptr->prace == RACE_VAMPIRE) ||
		(creature_ptr->prace == RACE_SKELETON) ||
		(creature_ptr->prace == RACE_ZOMBIE) ||
		(creature_ptr->prace == RACE_SPECTRE))
	{
		/* Undead start just after midnight */
		current_world_ptr->game_turn = (TURNS_PER_TICK * 3 * TOWN_DAWN) / 4 + 1;
		current_world_ptr->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * MAX_DAYS + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
	}
	else
	{
		current_world_ptr->game_turn = 1;
		current_world_ptr->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
	}

	current_world_ptr->dungeon_turn = 1;
	current_world_ptr->dungeon_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
}


/*!
 * @brief 所持状態にあるアイテムの中から一部枠の装備可能なものを装備させる。
 * @return なし
 */
static void wield_all(player_type *creature_ptr)
{
	object_type *o_ptr;
	object_type *i_ptr;
	object_type object_type_body;

	int slot;
	INVENTORY_IDX item;

	/* Scan through the slots backwards */
	for (item = INVEN_PACK - 1; item >= 0; item--)
	{
		o_ptr = &creature_ptr->inventory_list[item];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Make sure we can wield it and that there's nothing else in that slot */
		slot = wield_slot(creature_ptr, o_ptr);
		if (slot < INVEN_RARM) continue;
		if (slot == INVEN_LITE) continue; /* Does not wield toaches because buys a lantern soon */
		if (creature_ptr->inventory_list[slot].k_idx) continue;

		i_ptr = &object_type_body;
		object_copy(i_ptr, o_ptr);
		i_ptr->number = 1;

		/* Decrease the item (from the pack) */
		if (item >= 0)
		{
			inven_item_increase(creature_ptr, item, -1);
			inven_item_optimize(creature_ptr, item);
		}

		/* Decrease the item (from the floor) */
		else
		{
			floor_item_increase(creature_ptr->current_floor_ptr, 0 - item, -1);
			floor_item_optimize(creature_ptr, 0 - item);
		}

		o_ptr = &creature_ptr->inventory_list[slot];
		object_copy(o_ptr, i_ptr);
		creature_ptr->total_weight += i_ptr->weight;

		/* Increment the equip counter by hand */
		creature_ptr->equip_cnt++;

	}
	return;
}


/*!
 * プレイヤーの職業毎の初期装備テーブル。/\n
 * Each player starts out with a few items, given as tval/sval pairs.\n
 * In addition, he always has some food and a few torches.\n
 */
static byte player_init[MAX_CLASS][3][2] =
{
	{
		/* Warrior */
		{ TV_RING, SV_RING_RES_FEAR }, /* Warriors need it! */
		{ TV_HARD_ARMOR, SV_CHAIN_MAIL },
		{ TV_SWORD, SV_BROAD_SWORD }
	},

	{
		/* Mage */
		{ TV_SORCERY_BOOK, 0 }, /* Hack: for realm1 book */
		{ TV_DEATH_BOOK, 0 }, /* Hack: for realm2 book */
		{ TV_SWORD, SV_DAGGER }
	},

	{
		/* Priest */
		{ TV_SORCERY_BOOK, 0 }, /* Hack: for Life / Death book */
		{ TV_DEATH_BOOK, 0 }, /* Hack: for realm2 book */
		{ TV_HAFTED, SV_MACE }
	},

	{
		/* Rogue */
		{ TV_SORCERY_BOOK, 0 }, /* Hack: for realm1 book */
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
		{ TV_SWORD, SV_DAGGER }
	},

	{
		/* Ranger */
		{ TV_NATURE_BOOK, 0 },
		{ TV_DEATH_BOOK, 0 },		/* Hack: for realm2 book */
		{ TV_SWORD, SV_DAGGER }
	},

	{
		/* Paladin */
		{ TV_SORCERY_BOOK, 0 },
		{ TV_SCROLL, SV_SCROLL_PROTECTION_FROM_EVIL },
		{ TV_SWORD, SV_BROAD_SWORD }
	},

	{
		/* Warrior-Mage */
		{ TV_SORCERY_BOOK, 0 }, /* Hack: for realm1 book */
		{ TV_DEATH_BOOK, 0 }, /* Hack: for realm2 book */
		{ TV_SWORD, SV_SHORT_SWORD }
	},

	{
		/* Chaos Warrior */
		{ TV_SORCERY_BOOK, 0 }, /* Hack: For realm1 book */
		{ TV_HARD_ARMOR, SV_METAL_SCALE_MAIL },
		{ TV_SWORD, SV_BROAD_SWORD }
	},

	{
		/* Monk */
		{ TV_SORCERY_BOOK, 0 },
		{ TV_POTION, SV_POTION_SPEED },
		{ TV_POTION, SV_POTION_HEROISM }
	},

	{
		/* Mindcrafter */
		{ TV_POTION, SV_POTION_SPEED },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
		{ TV_SWORD, SV_SMALL_SWORD }
	},

	{
		/* High Mage */
		{ TV_SORCERY_BOOK, 0 }, /* Hack: for realm1 book */
		{ TV_RING, SV_RING_SUSTAIN_INT},
		{ TV_SWORD, SV_DAGGER }
	},

	{
		/* Tourist */
		{ TV_FOOD, SV_FOOD_JERKY},
		{ TV_SCROLL, SV_SCROLL_MAPPING },
		{ TV_BOW, SV_SLING}
	},

	{
		/* Imitator */
		{ TV_POTION, SV_POTION_SPEED },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
		{ TV_SWORD, SV_SHORT_SWORD}
	},

	{
		/* Beastmaster */
		{ TV_TRUMP_BOOK, 0 },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
		{ TV_POLEARM, SV_SPEAR}
	},

	{
		/* Sorcerer */
		{ TV_HAFTED, SV_WIZSTAFF }, /* Hack: for realm1 book */
		{ TV_RING, SV_RING_SUSTAIN_INT},
		{ TV_WAND, SV_WAND_MAGIC_MISSILE }
	},

	{
		/* Archer */
		{ TV_BOW, SV_SHORT_BOW },
		{ TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL},
		{ TV_SWORD, SV_SHORT_SWORD },
	},

	{
		/* Magic eater */
		{ TV_WAND, SV_WAND_MAGIC_MISSILE },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR},
		{ TV_SWORD, SV_SHORT_SWORD },
	},

	{
		/* Bard */
		{ TV_MUSIC_BOOK, 0 },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR},
		{ TV_SWORD, SV_SHORT_SWORD },
	},

	{
		/* Red Mage */
		{ TV_ARCANE_BOOK, 0 },
		{ TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR},
		{ TV_SWORD, SV_SHORT_SWORD },
	},

	{
		/* Samurai */
		{ TV_HISSATSU_BOOK, 0 },
		{ TV_HARD_ARMOR, SV_CHAIN_MAIL },
		{ TV_SWORD, SV_BROAD_SWORD }
	},

	{
		/* ForceTrainer */
		{ TV_SORCERY_BOOK, 0 },
		{ TV_POTION, SV_POTION_SPEED },
		{ TV_POTION, SV_POTION_RESTORE_MANA }
	},

	{
		/* Blue Mage */
		{ TV_SOFT_ARMOR, SV_ROBE },
		{ TV_WAND, SV_WAND_MAGIC_MISSILE },
		{ TV_SWORD, SV_DAGGER }
	},

	{
		/* Cavalry */
		{ TV_BOW, SV_SHORT_BOW },
		{ TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL},
		{ TV_POLEARM, SV_BROAD_SPEAR}
	},

	{
		/* Berserker */
		{ TV_POTION, SV_POTION_HEALING },
		{ TV_HARD_ARMOR, SV_AUGMENTED_CHAIN_MAIL },
		{ TV_POLEARM, SV_BROAD_AXE }
	},

	{
		/* Weaponsmith */
		{ TV_RING, SV_RING_RES_FEAR }, /* Warriors need it! */
		{ TV_HARD_ARMOR, SV_CHAIN_MAIL },
		{ TV_POLEARM, SV_BROAD_AXE }
	},
	{
		/* Mirror-Master */
		{ TV_POTION, SV_POTION_SPEED },
		{ TV_RING, SV_RING_SUSTAIN_INT},
		{ TV_SWORD, SV_DAGGER }
	},
	{
		/* Ninja */
		{ TV_POTION, SV_POTION_SPEED },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
		{ TV_SWORD, SV_DAGGER }
	},
	{
		/* Sniper */
		{ TV_BOW, SV_LIGHT_XBOW },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
		{ TV_SWORD, SV_DAGGER }
	},
};

/*!
 * @brief 初期所持アイテムの処理 / Add an outfit object
 * @details アイテムを既知のものとした上でwield_all()関数により装備させる。
 * @param o_ptr 処理したいオブジェクト構造体の参照ポインタ
 * @return なし
 */
static void add_outfit(player_type *creature_ptr, object_type *o_ptr)
{
	s16b slot;

	object_aware(creature_ptr, o_ptr);
	object_known(o_ptr);
	slot = inven_carry(creature_ptr, o_ptr);

	/* Auto-inscription */
	autopick_alter_item(creature_ptr, slot, FALSE);

	/* Now try wielding everything */
	wield_all(creature_ptr);
}


/*!
 * @brief 種族/職業/性格などに基づき初期所持アイテムを設定するメインセット関数。 / Init players with some belongings
 * @details Having an item makes the player "aware" of its purpose.
 * @return なし
 */
void player_outfit(player_type *creature_ptr)
{
	int i;
	OBJECT_TYPE_VALUE tv;
	OBJECT_SUBTYPE_VALUE sv;

	object_type	forge;
	object_type	*q_ptr;

	q_ptr = &forge;

	/* Give the player some food */
	switch (creature_ptr->prace)
	{
	case RACE_VAMPIRE:
		/* Nothing! */
		/* Vampires can drain blood of creatures */
		break;

	case RACE_DEMON:
		/* Demon can drain vitality from humanoid corpse */
		get_mon_num_prep(creature_ptr, monster_hook_human, NULL);

		for (i = rand_range(3, 4); i > 0; i--)
		{
			object_prep(q_ptr, lookup_kind(TV_CORPSE, SV_CORPSE));
			q_ptr->pval = get_mon_num(creature_ptr, 2);
			if (q_ptr->pval)
			{
				q_ptr->number = 1;
				add_outfit(creature_ptr, q_ptr);
			}
		}
		break;

	case RACE_SKELETON:
	case RACE_GOLEM:
	case RACE_ZOMBIE:
	case RACE_SPECTRE:
		/* Staff (of Nothing) */
		object_prep(q_ptr, lookup_kind(TV_STAFF, SV_STAFF_NOTHING));
		q_ptr->number = 1;

		add_outfit(creature_ptr, q_ptr);
		break;

	case RACE_ENT:
		/* Potions of Water */
		object_prep(q_ptr, lookup_kind(TV_POTION, SV_POTION_WATER));
		q_ptr->number = (ITEM_NUMBER)rand_range(15, 23);
		add_outfit(creature_ptr, q_ptr);

		break;

	case RACE_ANDROID:
		/* Flasks of oil */
		object_prep(q_ptr, lookup_kind(TV_FLASK, SV_ANY));

		/* Fuel with oil (move pval to xtra4) */
		apply_magic(creature_ptr, q_ptr, 1, AM_NO_FIXED_ART);

		q_ptr->number = (ITEM_NUMBER)rand_range(7, 12);
		add_outfit(creature_ptr, q_ptr);

		break;

	default:
		/* Food rations */
		object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_RATION));
		q_ptr->number = (ITEM_NUMBER)rand_range(3, 7);

		add_outfit(creature_ptr, q_ptr);
	}
	q_ptr = &forge;

	if ((creature_ptr->prace == RACE_VAMPIRE) && (creature_ptr->pclass != CLASS_NINJA))
	{
		/* Hack -- Give the player scrolls of DARKNESS! */
		object_prep(q_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_DARKNESS));

		q_ptr->number = (ITEM_NUMBER)rand_range(2, 5);

		add_outfit(creature_ptr, q_ptr);
	}
	else if (creature_ptr->pclass != CLASS_NINJA)
	{
		/* Hack -- Give the player some torches */
		object_prep(q_ptr, lookup_kind(TV_LITE, SV_LITE_TORCH));
		q_ptr->number = (ITEM_NUMBER)rand_range(3, 7);
		q_ptr->xtra4 = rand_range(3, 7) * 500;

		add_outfit(creature_ptr, q_ptr);
	}
	q_ptr = &forge;

	if (creature_ptr->prace == RACE_MERFOLK)
	{
		object_prep(q_ptr, lookup_kind(TV_RING, SV_RING_LEVITATION_FALL));
		q_ptr->number = 1;
		add_outfit(creature_ptr, q_ptr);
	}

	if ((creature_ptr->pclass == CLASS_RANGER) || (creature_ptr->pclass == CLASS_CAVALRY))
	{
		/* Hack -- Give the player some arrows */
		object_prep(q_ptr, lookup_kind(TV_ARROW, SV_AMMO_NORMAL));
		q_ptr->number = (byte)rand_range(15, 20);

		add_outfit(creature_ptr, q_ptr);
	}
	if (creature_ptr->pclass == CLASS_RANGER)
	{
		/* Hack -- Give the player some arrows */
		object_prep(q_ptr, lookup_kind(TV_BOW, SV_SHORT_BOW));

		add_outfit(creature_ptr, q_ptr);
	}
	else if (creature_ptr->pclass == CLASS_ARCHER)
	{
		/* Hack -- Give the player some arrows */
		object_prep(q_ptr, lookup_kind(TV_ARROW, SV_AMMO_NORMAL));
		q_ptr->number = (ITEM_NUMBER)rand_range(15, 20);

		add_outfit(creature_ptr, q_ptr);
	}
	else if (creature_ptr->pclass == CLASS_HIGH_MAGE)
	{
		/* Hack -- Give the player some arrows */
		object_prep(q_ptr, lookup_kind(TV_WAND, SV_WAND_MAGIC_MISSILE));
		q_ptr->number = 1;
		q_ptr->pval = (PARAMETER_VALUE)rand_range(25, 30);

		add_outfit(creature_ptr, q_ptr);
	}
	else if (creature_ptr->pclass == CLASS_SORCERER)
	{
		OBJECT_TYPE_VALUE book_tval;
		for (book_tval = TV_LIFE_BOOK; book_tval <= TV_LIFE_BOOK + MAX_MAGIC - 1; book_tval++)
		{
			/* Hack -- Give the player some arrows */
			object_prep(q_ptr, lookup_kind(book_tval, 0));
			q_ptr->number = 1;

			add_outfit(creature_ptr, q_ptr);
		}
	}
	else if (creature_ptr->pclass == CLASS_TOURIST)
	{
		if (creature_ptr->pseikaku != SEIKAKU_SEXY)
		{
			/* Hack -- Give the player some arrows */
			object_prep(q_ptr, lookup_kind(TV_SHOT, SV_AMMO_LIGHT));
			q_ptr->number = rand_range(15, 20);

			add_outfit(creature_ptr, q_ptr);
		}

		object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_BISCUIT));
		q_ptr->number = rand_range(2, 4);

		add_outfit(creature_ptr, q_ptr);

		object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_WAYBREAD));
		q_ptr->number = rand_range(2, 4);

		add_outfit(creature_ptr, q_ptr);

		object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_JERKY));
		q_ptr->number = rand_range(1, 3);

		add_outfit(creature_ptr, q_ptr);

		object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_PINT_OF_ALE));
		q_ptr->number = rand_range(2, 4);

		add_outfit(creature_ptr, q_ptr);

		object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_PINT_OF_WINE));
		q_ptr->number = rand_range(2, 4);

		add_outfit(creature_ptr, q_ptr);
	}
	else if (creature_ptr->pclass == CLASS_NINJA)
	{
		/* Hack -- Give the player some arrows */
		object_prep(q_ptr, lookup_kind(TV_SPIKE, 0));
		q_ptr->number = rand_range(15, 20);

		add_outfit(creature_ptr, q_ptr);
	}
	else if (creature_ptr->pclass == CLASS_SNIPER)
	{
		/* Hack -- Give the player some bolts */
		object_prep(q_ptr, lookup_kind(TV_BOLT, SV_AMMO_NORMAL));
		q_ptr->number = rand_range(15, 20);

		add_outfit(creature_ptr, q_ptr);
	}

	if (creature_ptr->pseikaku == SEIKAKU_SEXY)
	{
		player_init[creature_ptr->pclass][2][0] = TV_HAFTED;
		player_init[creature_ptr->pclass][2][1] = SV_WHIP;
	}

	/* Hack -- Give the player three useful objects */
	for (i = 0; i < 3; i++)
	{
		/* Look up standard equipment */
		tv = player_init[creature_ptr->pclass][i][0];
		sv = player_init[creature_ptr->pclass][i][1];

		if ((creature_ptr->prace == RACE_ANDROID) && ((tv == TV_SOFT_ARMOR) || (tv == TV_HARD_ARMOR))) continue;
		/* Hack to initialize spellbooks */
		if (tv == TV_SORCERY_BOOK) tv = TV_LIFE_BOOK + creature_ptr->realm1 - 1;
		else if (tv == TV_DEATH_BOOK) tv = TV_LIFE_BOOK + creature_ptr->realm2 - 1;

		else if (tv == TV_RING && sv == SV_RING_RES_FEAR &&
			creature_ptr->prace == RACE_BARBARIAN)
			/* Barbarians do not need a ring of resist fear */
			sv = SV_RING_SUSTAIN_STR;

		else if (tv == TV_RING && sv == SV_RING_SUSTAIN_INT && creature_ptr->prace == RACE_MIND_FLAYER)
		{
			tv = TV_POTION;
			sv = SV_POTION_RESTORE_MANA;
		}
		q_ptr = &forge;

		/* Hack -- Give the player an object */
		object_prep(q_ptr, lookup_kind(tv, sv));

		/* Assassins begin the game with a poisoned dagger */
		if ((tv == TV_SWORD || tv == TV_HAFTED) && (creature_ptr->pclass == CLASS_ROGUE &&
			creature_ptr->realm1 == REALM_DEATH)) /* Only assassins get a poisoned weapon */
		{
			q_ptr->name2 = EGO_BRAND_POIS;
		}

		add_outfit(creature_ptr, q_ptr);
	}

	/* Hack -- make aware of the water */
	k_info[lookup_kind(TV_POTION, SV_POTION_WATER)].aware = TRUE;
}

/*!
 * @brief プレイヤーの種族選択を行う / Player race
 * @return なし
 */
static bool get_player_race(player_type *creature_ptr)
{
	int     k, n, cs, os;
	concptr    str;
	char    c;
	char	sym[MAX_RACES];
	char    p2 = ')';
	char    buf[80], cur[80];

	/* Extra info */
	clear_from(10);
	put_str(_("注意：《種族》によってキャラクターの先天的な資質やボーナスが変化します。",
		"Note: Your 'race' determines various intrinsic factors and bonuses."), 23, 5);

	/* Dump races */
	for (n = 0; n < MAX_RACES; n++)
	{
		/* Analyze */
		rp_ptr = &race_info[n];
		str = rp_ptr->title;

		/* Display */
		if (n < 26)
			sym[n] = I2A(n);
		else
			sym[n] = ('A' + n - 26);
		sprintf(buf, "%c%c%s", sym[n], p2, str);
		put_str(buf, 12 + (n / 5), 1 + 16 * (n % 5));

	}

	sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));

	/* Choose */
	k = -1;
	cs = creature_ptr->prace;
	os = MAX_RACES;
	while (TRUE)
	{
		/* Move Cursol */
		if (cs != os)
		{
			c_put_str(TERM_WHITE, cur, 12 + (os / 5), 1 + 16 * (os % 5));
			put_str("                                   ", 3, 40);
			if (cs == MAX_RACES)
			{
				sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));
				put_str("                                   ", 4, 40);
				put_str("                                   ", 5, 40);
			}
			else
			{
				rp_ptr = &race_info[cs];
				str = rp_ptr->title;
				sprintf(cur, "%c%c%s", sym[cs], p2, str);
				c_put_str(TERM_L_BLUE, rp_ptr->title, 3, 40);
				put_str(_("腕力 知能 賢さ 器用 耐久 魅力 経験 ", "Str  Int  Wis  Dex  Con  Chr   EXP "), 4, 40);
				put_str(_("の種族修正", ": Race modification"), 3, 40 + strlen(rp_ptr->title));

				sprintf(buf, "%+3d  %+3d  %+3d  %+3d  %+3d  %+3d %+4d%% ",
					rp_ptr->r_adj[0], rp_ptr->r_adj[1], rp_ptr->r_adj[2], rp_ptr->r_adj[3],
					rp_ptr->r_adj[4], rp_ptr->r_adj[5], (rp_ptr->r_exp - 100));
				c_put_str(TERM_L_BLUE, buf, 5, 40);
			}
			c_put_str(TERM_YELLOW, cur, 12 + (cs / 5), 1 + 16 * (cs % 5));
			os = cs;
		}

		if (k >= 0) break;

		sprintf(buf, _("種族を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a race (%c-%c) ('=' for options): "), sym[0], sym[MAX_RACES - 1]);

		put_str(buf, 10, 10);
		c = inkey();
		if (c == 'Q') birth_quit();
		if (c == 'S') return FALSE;
		if (c == ' ' || c == '\r' || c == '\n')
		{
			if (cs == MAX_RACES)
			{
				k = randint0(MAX_RACES);
				cs = k;
				continue;
			}
			else
			{
				k = cs;
				break;
			}
		}
		if (c == '*')
		{
			k = randint0(MAX_RACES);
			cs = k;
			continue;
		}
		if (c == '8')
		{
			if (cs >= 5) cs -= 5;
		}
		if (c == '4')
		{
			if (cs > 0) cs--;
		}
		if (c == '6')
		{
			if (cs < MAX_RACES) cs++;
		}
		if (c == '2')
		{
			if ((cs + 5) <= MAX_RACES) cs += 5;
		}
		k = (islower(c) ? A2I(c) : -1);
		if ((k >= 0) && (k < MAX_RACES))
		{
			cs = k;
			continue;
		}
		k = (isupper(c) ? (26 + c - 'A') : -1);
		if ((k >= 26) && (k < MAX_RACES))
		{
			cs = k;
			continue;
		}
		else k = -1;
		if (c == '?')
		{
#ifdef JP
			show_help(creature_ptr, "jraceclas.txt#TheRaces");
#else
			show_help(creature_ptr, "raceclas.txt#TheRaces");
#endif
		}
		else if (c == '=')
		{
			screen_save();
			do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
			screen_load();
		}
		else if (c != '2' && c != '4' && c != '6' && c != '8') bell();
	}

	/* Set race */
	creature_ptr->prace = (byte_hack)k;

	rp_ptr = &race_info[creature_ptr->prace];

	/* Display */
	c_put_str(TERM_L_BLUE, rp_ptr->title, 4, 15);

	/* Success */
	return TRUE;
}


/*!
 * @brief プレイヤーの職業選択を行う / Player class
 * @return なし
 */
static bool get_player_class(player_type *creature_ptr)
{
	int     k, n, cs, os;
	char    c;
	char	sym[MAX_CLASS_CHOICE];
	char    p2 = ')';
	char    buf[80], cur[80];
	concptr    str;


	/* Extra info */
	clear_from(10);
	put_str(_("注意：《職業》によってキャラクターの先天的な能力やボーナスが変化します。",
		"Note: Your 'class' determines various intrinsic abilities and bonuses."), 23, 5);

	put_str(_("()で囲まれた選択肢はこの種族には似合わない職業です。",
		"Any entries in parentheses should only be used by advanced players."), 11, 5);


	/* Dump classes */
	for (n = 0; n < MAX_CLASS_CHOICE; n++)
	{
		/* Analyze */
		cp_ptr = &class_info[n];
		mp_ptr = &m_info[n];
		str = cp_ptr->title;
		if (n < 26)
			sym[n] = I2A(n);
		else
			sym[n] = ('A' + n - 26);

		/* Display */
		if (!(rp_ptr->choice & (1L << n)))
			sprintf(buf, "%c%c(%s)", sym[n], p2, str);
		else
			sprintf(buf, "%c%c%s", sym[n], p2, str);

		put_str(buf, 13 + (n / 4), 2 + 19 * (n % 4));
	}

	sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));

	/* Get a class */
	k = -1;
	cs = creature_ptr->pclass;
	os = MAX_CLASS_CHOICE;
	while (TRUE)
	{
		/* Move Cursol */
		if (cs != os)
		{
			c_put_str(TERM_WHITE, cur, 13 + (os / 4), 2 + 19 * (os % 4));
			put_str("                                   ", 3, 40);
			if (cs == MAX_CLASS_CHOICE)
			{
				sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));
				put_str("                                   ", 4, 40);
				put_str("                                   ", 5, 40);
			}
			else
			{
				cp_ptr = &class_info[cs];
				mp_ptr = &m_info[cs];
				str = cp_ptr->title;
				if (!(rp_ptr->choice & (1L << cs)))
					sprintf(cur, "%c%c(%s)", sym[cs], p2, str);
				else
					sprintf(cur, "%c%c%s", sym[cs], p2, str);

				c_put_str(TERM_L_BLUE, cp_ptr->title, 3, 40);
				put_str(_("の職業修正", ": Class modification"), 3, 40 + strlen(cp_ptr->title));
				put_str(_("腕力 知能 賢さ 器用 耐久 魅力 経験 ", "Str  Int  Wis  Dex  Con  Chr   EXP "), 4, 40);
				sprintf(buf, "%+3d  %+3d  %+3d  %+3d  %+3d  %+3d %+4d%% ",
					cp_ptr->c_adj[0], cp_ptr->c_adj[1], cp_ptr->c_adj[2], cp_ptr->c_adj[3],
					cp_ptr->c_adj[4], cp_ptr->c_adj[5], cp_ptr->c_exp);
				c_put_str(TERM_L_BLUE, buf, 5, 40);
			}
			c_put_str(TERM_YELLOW, cur, 13 + (cs / 4), 2 + 19 * (cs % 4));
			os = cs;
		}

		if (k >= 0) break;

		sprintf(buf, _("職業を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a class (%c-%c) ('=' for options): "), sym[0], sym[MAX_CLASS_CHOICE - 1]);

		put_str(buf, 10, 10);
		c = inkey();
		if (c == 'Q') birth_quit();
		if (c == 'S') return FALSE;
		if (c == ' ' || c == '\r' || c == '\n')
		{
			if (cs == MAX_CLASS_CHOICE)
			{
				k = randint0(MAX_CLASS_CHOICE);
				cs = k;
				continue;
			}
			else
			{
				k = cs;
				break;
			}
		}
		if (c == '*')
		{
			k = randint0(MAX_CLASS_CHOICE);
			cs = k;
			continue;
		}
		if (c == '8')
		{
			if (cs >= 4) cs -= 4;
		}
		if (c == '4')
		{
			if (cs > 0) cs--;
		}
		if (c == '6')
		{
			if (cs < MAX_CLASS_CHOICE) cs++;
		}
		if (c == '2')
		{
			if ((cs + 4) <= MAX_CLASS_CHOICE) cs += 4;
		}
		k = (islower(c) ? A2I(c) : -1);
		if ((k >= 0) && (k < MAX_CLASS_CHOICE))
		{
			cs = k;
			continue;
		}
		k = (isupper(c) ? (26 + c - 'A') : -1);
		if ((k >= 26) && (k < MAX_CLASS_CHOICE))
		{
			cs = k;
			continue;
		}
		else k = -1;
		if (c == '?')
		{
#ifdef JP
			show_help(creature_ptr, "jraceclas.txt#TheClasses");
#else
			show_help(creature_ptr, "raceclas.txt#TheClasses");
#endif
		}
		else if (c == '=')
		{
			screen_save();
			do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
			screen_load();
		}
		else if (c != '2' && c != '4' && c != '6' && c != '8') bell();
	}

	/* Set class */
	creature_ptr->pclass = (byte_hack)k;
	cp_ptr = &class_info[creature_ptr->pclass];
	mp_ptr = &m_info[creature_ptr->pclass];

	/* Display */
	c_put_str(TERM_L_BLUE, cp_ptr->title, 5, 15);

	return TRUE;
}


/*!
 * @brief プレイヤーの性格選択を行う / Player Player seikaku
 * @return なし
 */
static bool get_player_seikaku(player_type *creature_ptr)
{
	int k;
	int n, os, cs;
	char c;
	char sym[MAX_SEIKAKU];
	char p2 = ')';
	char buf[80], cur[80];
	char tmp[64];
	concptr str;

	/* Extra info */
	clear_from(10);
	put_str(_("注意：《性格》によってキャラクターの能力やボーナスが変化します。", "Note: Your personality determines various intrinsic abilities and bonuses."), 23, 5);

	/* Dump seikakus */
	for (n = 0; n < MAX_SEIKAKU; n++)
	{
		if (seikaku_info[n].sex && (seikaku_info[n].sex != (creature_ptr->psex + 1))) continue;

		/* Analyze */
		ap_ptr = &seikaku_info[n];
		str = ap_ptr->title;
		if (n < 26)
			sym[n] = I2A(n);
		else
			sym[n] = ('A' + n - 26);

		/* Display */
		sprintf(buf, "%c%c%s", I2A(n), p2, str);
		put_str(buf, 12 + (n / 4), 2 + 18 * (n % 4));
	}

	sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));

	/* Get a seikaku */
	k = -1;
	cs = creature_ptr->pseikaku;
	os = MAX_SEIKAKU;
	while (TRUE)
	{
		/* Move Cursol */
		if (cs != os)
		{
			c_put_str(TERM_WHITE, cur, 12 + (os / 4), 2 + 18 * (os % 4));
			put_str("                                   ", 3, 40);
			if (cs == MAX_SEIKAKU)
			{
				sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));
				put_str("                                   ", 4, 40);
				put_str("                                   ", 5, 40);
			}
			else
			{
				ap_ptr = &seikaku_info[cs];
				str = ap_ptr->title;
				sprintf(cur, "%c%c%s", sym[cs], p2, str);
				c_put_str(TERM_L_BLUE, ap_ptr->title, 3, 40);
				put_str(_("の性格修正", ": Personality modification"), 3, 40 + strlen(ap_ptr->title));
				put_str(_("腕力 知能 賢さ 器用 耐久 魅力      ", "Str  Int  Wis  Dex  Con  Chr       "), 4, 40);
				sprintf(buf, "%+3d  %+3d  %+3d  %+3d  %+3d  %+3d       ",
					ap_ptr->a_adj[0], ap_ptr->a_adj[1], ap_ptr->a_adj[2], ap_ptr->a_adj[3],
					ap_ptr->a_adj[4], ap_ptr->a_adj[5]);
				c_put_str(TERM_L_BLUE, buf, 5, 40);
			}
			c_put_str(TERM_YELLOW, cur, 12 + (cs / 4), 2 + 18 * (cs % 4));
			os = cs;
		}

		if (k >= 0) break;

		sprintf(buf, _("性格を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a personality (%c-%c) ('=' for options): "), sym[0], sym[MAX_SEIKAKU - 1]);

		put_str(buf, 10, 10);
		c = inkey();
		if (c == 'Q') birth_quit();
		if (c == 'S') return FALSE;
		if (c == ' ' || c == '\r' || c == '\n')
		{
			if (cs == MAX_SEIKAKU)
			{
				do
				{
					k = randint0(MAX_SEIKAKU);
				} while (seikaku_info[k].sex && (seikaku_info[k].sex != (creature_ptr->psex + 1)));
				cs = k;
				continue;
			}
			else
			{
				k = cs;
				break;
			}
		}
		if (c == '*')
		{
			do
			{
				k = randint0(n);
			} while (seikaku_info[k].sex && (seikaku_info[k].sex != (creature_ptr->psex + 1)));
			cs = k;
			continue;
		}
		if (c == '8')
		{
			if (cs >= 4) cs -= 4;
			if (cs != MAX_SEIKAKU && seikaku_info[cs].sex && (seikaku_info[cs].sex != (creature_ptr->psex + 1)))
			{
				if ((cs - 4) > 0)
					cs -= 4;
				else
					cs += 4;
			}
		}
		if (c == '4')
		{
			if (cs > 0) cs--;
			if (cs != MAX_SEIKAKU && seikaku_info[cs].sex && (seikaku_info[cs].sex != (creature_ptr->psex + 1)))
			{
				if ((cs - 1) > 0)
					cs--;
				else
					cs++;
			}
		}
		if (c == '6')
		{
			if (cs < MAX_SEIKAKU) cs++;
			if (cs != MAX_SEIKAKU && seikaku_info[cs].sex && (seikaku_info[cs].sex != (creature_ptr->psex + 1)))
			{
				if ((cs + 1) <= MAX_SEIKAKU)
					cs++;
				else
					cs--;
			}
		}
		if (c == '2')
		{
			if ((cs + 4) <= MAX_SEIKAKU) cs += 4;
			if (cs != MAX_SEIKAKU && seikaku_info[cs].sex && (seikaku_info[cs].sex != (creature_ptr->psex + 1)))
			{
				if ((cs + 4) <= MAX_SEIKAKU)
					cs += 4;
				else
					cs -= 4;
			}
		}
		k = (islower(c) ? A2I(c) : -1);
		if ((k >= 0) && (k < MAX_SEIKAKU))
		{
			if ((seikaku_info[k].sex == 0) || (seikaku_info[k].sex == (creature_ptr->psex + 1)))
			{
				cs = k;
				continue;
			}
		}
		k = (isupper(c) ? (26 + c - 'A') : -1);
		if ((k >= 26) && (k < MAX_SEIKAKU))
		{
			if ((seikaku_info[k].sex == 0) || (seikaku_info[k].sex == (creature_ptr->psex + 1)))
			{
				cs = k;
				continue;
			}
		}
		else k = -1;
		if (c == '?')
		{
#ifdef JP
			show_help(creature_ptr, "jraceclas.txt#ThePersonalities");
#else
			show_help(creature_ptr, "raceclas.txt#ThePersonalities");
#endif
		}
		else if (c == '=')
		{
			screen_save();
			do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
			screen_load();
		}
		else if (c != '2' && c != '4' && c != '6' && c != '8') bell();
	}

	/* Set seikaku */
	creature_ptr->pseikaku = (CHARACTER_IDX)k;
	ap_ptr = &seikaku_info[creature_ptr->pseikaku];
#ifdef JP
	strcpy(tmp, ap_ptr->title);
	if (ap_ptr->no == 1)
		strcat(tmp, "の");
#else
	strcpy(tmp, ap_ptr->title);
	strcat(tmp, " ");
#endif
	strcat(tmp, creature_ptr->name);

	c_put_str(TERM_L_BLUE, tmp, 1, 34);

	return TRUE;
}

/*!
 * @brief オートローラで得たい能力値の基準を決める。
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static bool get_stat_limits(player_type *creature_ptr)
{
	int i, j, m, cs, os;
	int cval[6];
	char c;
	char buf[80], cur[80];
	char inp[80];

	/* Clean up */
	clear_from(10);

	/* Extra infomation */
	put_str(_("最低限得たい能力値を設定して下さい。", "Set minimum stats."), 10, 10);
	put_str(_("2/8で項目選択、4/6で値の増減、Enterで次へ", "2/8 for Select, 4/6 for Change value, Enter for Goto next"), 11, 10);

	put_str(_("         基本値  種族 職業 性格     合計値  最大値", "           Base   Rac  Cla  Per      Total  Maximum"), 13, 10);

	/* Output the maximum stats */
	for (i = 0; i < A_MAX; i++)
	{
		/* Reset the "success" counter */
		stat_match[i] = 0;
		cval[i] = 3;

		/* Race/Class bonus */
		j = rp_ptr->r_adj[i] + cp_ptr->c_adj[i] + ap_ptr->a_adj[i];

		/* Obtain the "maximal" stat */
		m = adjust_stat(17, j);

		/* Above 18 */
		if (m > 18)
		{
			sprintf(cur, "18/%02d", (m - 18));
		}

		/* From 3 to 18 */
		else
		{
			sprintf(cur, "%2d", m);
		}

		/* Obtain the current stat */
		m = adjust_stat(cval[i], j);

		/* Above 18 */
		if (m > 18)
		{
			sprintf(inp, "18/%02d", (m - 18));
		}

		/* From 3 to 18 */
		else
		{
			sprintf(inp, "%2d", m);
		}

		/* Prepare a prompt */
		sprintf(buf, "%6s       %2d   %+3d  %+3d  %+3d  =  %6s  %6s",
			stat_names[i], cval[i], rp_ptr->r_adj[i], cp_ptr->c_adj[i],
			ap_ptr->a_adj[i], inp, cur);

		/* Dump the prompt */
		put_str(buf, 14 + i, 10);
	}

	/* Get a minimum stat */
	cs = 0;
	os = 6;
	while (TRUE)
	{
		/* Move Cursol */
		if (cs != os)
		{
			if (os == 6)
			{
				c_put_str(TERM_WHITE, _("決定する", "Accept"), 21, 35);
			}
			else if (os < A_MAX)
			{
				c_put_str(TERM_WHITE, cur, 14 + os, 10);
			}
			if (cs == 6)
			{
				c_put_str(TERM_YELLOW, _("決定する", "Accept"), 21, 35);
			}
			else
			{
				/* Race/Class bonus */
				j = rp_ptr->r_adj[cs] + cp_ptr->c_adj[cs] + ap_ptr->a_adj[cs];

				/* Obtain the current stat */
				m = adjust_stat(cval[cs], j);

				/* Above 18 */
				if (m > 18)
				{
					sprintf(inp, "18/%02d", (m - 18));
				}

				/* From 3 to 18 */
				else
				{
					sprintf(inp, "%2d", m);
				}

				/* Prepare a prompt */
				sprintf(cur, "%6s       %2d   %+3d  %+3d  %+3d  =  %6s",
					stat_names[cs], cval[cs], rp_ptr->r_adj[cs],
					cp_ptr->c_adj[cs], ap_ptr->a_adj[cs], inp);
				c_put_str(TERM_YELLOW, cur, 14 + cs, 10);
			}
			os = cs;
		}

		/* Prompt for the minimum stats */
		c = inkey();
		switch (c) {
		case 'Q':
			birth_quit();
		case 'S':
			return FALSE;
		case ESCAPE:
			break;
		case ' ':
		case '\r':
		case '\n':
			if (cs == 6) break;
			cs++;
			c = '2';
			break;
		case '8':
		case 'k':
			if (cs > 0) cs--;
			break;
		case '2':
		case 'j':
			if (cs < A_MAX) cs++;
			break;
		case '4':
		case 'h':
			if (cs != 6)
			{
				if (cval[cs] == 3)
				{
					cval[cs] = 17;
					os = 7;
				}
				else if (cval[cs] > 3)
				{
					cval[cs]--;
					os = 7;
				}
				else return FALSE;
			}
			break;
		case '6':
		case 'l':
			if (cs != 6)
			{
				if (cval[cs] == 17)
				{
					cval[cs] = 3;
					os = 7;
				}
				else if (cval[cs] < 17)
				{
					cval[cs]++;
					os = 7;
				}
				else return FALSE;
			}
			break;
		case 'm':
			if (cs != 6)
			{
				cval[cs] = 17;
				os = 7;
			}
			break;
		case 'n':
			if (cs != 6)
			{
				cval[cs] = 3;
				os = 7;
			}
			break;
		case '?':
#ifdef JP
			show_help(creature_ptr, "jbirth.txt#AutoRoller");
#else
			show_help(creature_ptr, "birth.txt#AutoRoller");
#endif
			break;
		case '=':
			screen_save();
#ifdef JP
			do_cmd_options_aux(OPT_PAGE_BIRTH, "初期オプション((*)はスコアに影響)");
#else
			do_cmd_options_aux(OPT_PAGE_BIRTH, "Birth Option((*)s effect score)");
#endif

			screen_load();
			break;
		default:
			bell();
			break;
		}
		if (c == ESCAPE || ((c == ' ' || c == '\r' || c == '\n') && cs == 6))break;
	}

	for (i = 0; i < A_MAX; i++)
	{
		/* Save the minimum stat */
		stat_limit[i] = (s16b)cval[i];
	}

	return TRUE;
}


/*!
 * @brief オートローラで得たい年齢、身長、体重、社会的地位の基準を決める。
 * @return なし
 */
static bool get_chara_limits(player_type *creature_ptr)
{
#define MAXITEMS 8

	int i, j, m, cs, os;
	int mval[MAXITEMS], cval[MAXITEMS];
	int max_percent, min_percent;
	char c;
	char buf[80], cur[80];
	concptr itemname[] = {
		_("年齢", "age"),
		_("身長(インチ)", "height"),
		_("体重(ポンド)", "weight"),
		_("社会的地位", "social class")
	};

	clear_from(10);

	/* Prompt for the minimum stats */
	put_str(_("2/4/6/8で項目選択、+/-で値の増減、Enterで次へ", "2/4/6/8 for Select, +/- for Change value, Enter for Goto next"), 11, 10);
	put_str(_("注意：身長と体重の最大値/最小値ぎりぎりの値は非常に出現確率が低くなります。", "Caution: Values near minimum or maximum is extremery rare."), 23, 2);

	if (creature_ptr->psex == SEX_MALE)
	{
		max_percent = (int)(rp_ptr->m_b_ht + rp_ptr->m_m_ht * 4 - 1) * 100 / (int)(rp_ptr->m_b_ht);
		min_percent = (int)(rp_ptr->m_b_ht - rp_ptr->m_m_ht * 4 + 1) * 100 / (int)(rp_ptr->m_b_ht);
	}
	else
	{
		max_percent = (int)(rp_ptr->f_b_ht + rp_ptr->f_m_ht * 4 - 1) * 100 / (int)(rp_ptr->f_b_ht);
		min_percent = (int)(rp_ptr->f_b_ht - rp_ptr->f_m_ht * 4 + 1) * 100 / (int)(rp_ptr->f_b_ht);
	}

	put_str(_("体格/地位の最小値/最大値を設定して下さい。", "Set minimum/maximum attribute."), 10, 10);
	put_str(_("  項    目                 最小値  最大値", " Parameter                    Min     Max"), 13, 20);

	/* Output the maximum stats */
	for (i = 0; i < MAXITEMS; i++)
	{
		/* Obtain the "maximal" stat */
		switch (i)
		{
		case 0:	/* Minimum age */
			m = rp_ptr->b_age + 1;
			break;
		case 1:	/* Maximum age */
			m = rp_ptr->b_age + rp_ptr->m_age;
			break;

		case 2:	/* Minimum height */
			if (creature_ptr->psex == SEX_MALE) m = rp_ptr->m_b_ht - rp_ptr->m_m_ht * 4 + 1;
			else m = rp_ptr->f_b_ht - rp_ptr->f_m_ht * 4 + 1;
			break;
		case 3:	/* Maximum height */
			if (creature_ptr->psex == SEX_MALE) m = rp_ptr->m_b_ht + rp_ptr->m_m_ht * 4 - 1;
			else m = rp_ptr->f_b_ht + rp_ptr->f_m_ht * 4 - 1;
			break;
		case 4:	/* Minimum weight */
			if (creature_ptr->psex == SEX_MALE) m = (rp_ptr->m_b_wt * min_percent / 100) - (rp_ptr->m_m_wt * min_percent / 75) + 1;
			else m = (rp_ptr->f_b_wt * min_percent / 100) - (rp_ptr->f_m_wt * min_percent / 75) + 1;
			break;
		case 5:	/* Maximum weight */
			if (creature_ptr->psex == SEX_MALE) m = (rp_ptr->m_b_wt * max_percent / 100) + (rp_ptr->m_m_wt * max_percent / 75) - 1;
			else m = (rp_ptr->f_b_wt * max_percent / 100) + (rp_ptr->f_m_wt * max_percent / 75) - 1;
			break;
		case 6:	/* Minimum social class */
			m = 1;
			break;
		case 7:	/* Maximum social class */
			m = 100;
			break;
		default:
			m = 1;
			break;
		}

		/* Save the maximum or minimum */
		mval[i] = m;
		cval[i] = m;
	}

	for (i = 0; i < 4; i++)
	{
		/* Prepare a prompt */
		sprintf(buf, "%-12s (%3d - %3d)", itemname[i], mval[i * 2], mval[i * 2 + 1]);

		/* Dump the prompt */
		put_str(buf, 14 + i, 20);

		for (j = 0; j < 2; j++)
		{
			sprintf(buf, "     %3d", cval[i * 2 + j]);
			put_str(buf, 14 + i, 45 + 8 * j);
		}
	}

	/* Get a minimum stat */
	cs = 0;
	os = MAXITEMS;
	while (TRUE)
	{
		/* Move Cursol */
		if (cs != os)
		{
			const char accept[] = _("決定する", "Accept");

			if (os == MAXITEMS)
			{
				c_put_str(TERM_WHITE, accept, 19, 35);
			}
			else
			{
				c_put_str(TERM_WHITE, cur, 14 + os / 2, 45 + 8 * (os % 2));
			}

			if (cs == MAXITEMS)
			{
				c_put_str(TERM_YELLOW, accept, 19, 35);
			}
			else
			{
				/* Prepare a prompt */
				sprintf(cur, "     %3d", cval[cs]);
				c_put_str(TERM_YELLOW, cur, 14 + cs / 2, 45 + 8 * (cs % 2));
			}
			os = cs;
		}

		/* Prompt for the minimum stats */
		c = inkey();
		switch (c) {
		case 'Q':
			birth_quit();
		case 'S':
			return FALSE;
		case ESCAPE:
			break; /*後でもう一回breakせんと*/
		case ' ':
		case '\r':
		case '\n':
			if (cs == MAXITEMS) break;
			cs++;
			c = '6';
			break;
		case '8':
		case 'k':
			if (cs - 2 >= 0) cs -= 2;
			break;
		case '2':
		case 'j':
			if (cs < MAXITEMS) cs += 2;
			if (cs > MAXITEMS) cs = MAXITEMS;
			break;
		case '4':
		case 'h':
			if (cs > 0) cs--;
			break;
		case '6':
		case 'l':
			if (cs < MAXITEMS) cs++;
			break;
		case '-':
		case '<':
			if (cs != MAXITEMS)
			{
				if (cs % 2)
				{
					if (cval[cs] > cval[cs - 1])
					{
						cval[cs]--;
						os = 127;
					}
				}
				else
				{
					if (cval[cs] > mval[cs])
					{
						cval[cs]--;
						os = 127;
					}
				}
			}
			break;
		case '+':
		case '>':
			if (cs != MAXITEMS)
			{
				if (cs % 2)
				{
					if (cval[cs] < mval[cs])
					{
						cval[cs]++;
						os = 127;
					}
				}
				else
				{
					if (cval[cs] < cval[cs + 1])
					{
						cval[cs]++;
						os = 127;
					}
				}
			}
			break;
		case 'm':
			if (cs != MAXITEMS)
			{
				if (cs % 2)
				{
					if (cval[cs] < mval[cs])
					{
						cval[cs] = mval[cs];
						os = 127;
					}
				}
				else
				{
					if (cval[cs] < cval[cs + 1])
					{
						cval[cs] = cval[cs + 1];
						os = 127;
					}
				}
			}
			break;
		case 'n':
			if (cs != MAXITEMS)
			{
				if (cs % 2)
				{
					if (cval[cs] > cval[cs - 1])
					{
						cval[cs] = cval[cs - 1];
						os = 255;
					}
				}
				else
				{
					if (cval[cs] > mval[cs])
					{
						cval[cs] = mval[cs];
						os = 255;
					}
				}
			}
			break;
		case '?':
#ifdef JP
			show_help(creature_ptr, "jbirth.txt#AutoRoller");
#else
			show_help(creature_ptr, "birth.txt#AutoRoller");
#endif
			break;
		case '=':
			screen_save();
			do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
			screen_load();
			break;
		default:
			bell();
			break;
		}
		if (c == ESCAPE || ((c == ' ' || c == '\r' || c == '\n') && cs == MAXITEMS))break;
	}

	/* Input the minimum stats */
	chara_limit.agemin = (s16b)cval[0];
	chara_limit.agemax = (s16b)cval[1];
	chara_limit.htmin = (s16b)cval[2];
	chara_limit.htmax = (s16b)cval[3];
	chara_limit.wtmin = (s16b)cval[4];
	chara_limit.wtmax = (s16b)cval[5];
	chara_limit.scmin = (s16b)cval[6];
	chara_limit.scmax = (s16b)cval[7];

	return TRUE;
}

#define HISTPREF_LIMIT 1024
static char *histpref_buf = NULL;

/*!
 * @brief 生い立ちメッセージの内容をバッファに加える。 / Hook function for reading the histpref.prf file.
 * @return なし
 */
void add_history_from_pref_line(concptr t)
{
	/* Do nothing if the buffer is not ready */
	if (!histpref_buf) return;

	my_strcat(histpref_buf, t, HISTPREF_LIMIT);
}

/*!
 * @brief 生い立ちメッセージをファイルからロードする。
 * @return なし
 */
static bool do_cmd_histpref(player_type *creature_ptr)
{
	char buf[80];
	errr err;
	int i, j, n;
	char *s, *t;
	char temp[64 * 4];
	char histbuf[HISTPREF_LIMIT];

	if (!get_check(_("生い立ち設定ファイルをロードしますか? ", "Load background history preference file? "))) return FALSE;

	/* Prepare the buffer */
	histbuf[0] = '\0';
	histpref_buf = histbuf;

#ifdef JP
	sprintf(buf, "histedit-%s.prf", creature_ptr->base_name);
#else
	sprintf(buf, "histpref-%s.prf", creature_ptr->base_name);
#endif
	err = process_histpref_file(creature_ptr, buf);

	/* Process 'hist????.prf' if 'hist????-<name>.prf' doesn't exist */
	if (0 > err)
	{
#ifdef JP
		strcpy(buf, "histedit.prf");
#else
		strcpy(buf, "histpref.prf");
#endif
		err = process_histpref_file(creature_ptr, buf);
	}

	if (err)
	{
		msg_print(_("生い立ち設定ファイルの読み込みに失敗しました。", "Failed to load background history preference."));
		msg_print(NULL);

		/* Kill the buffer */
		histpref_buf = NULL;

		return FALSE;
	}
	else if (!histpref_buf[0])
	{
		msg_print(_("有効な生い立ち設定はこのファイルにありません。", "There does not exist valid background history preference."));
		msg_print(NULL);

		/* Kill the buffer */
		histpref_buf = NULL;

		return FALSE;
	}

	/* Clear the previous history strings */
	for (i = 0; i < 4; i++) creature_ptr->history[i][0] = '\0';

	/* Skip leading spaces */
	for (s = histpref_buf; *s == ' '; s++) /* loop */;

	/* Get apparent length */
	n = strlen(s);

	/* Kill trailing spaces */
	while ((n > 0) && (s[n - 1] == ' ')) s[--n] = '\0';

	roff_to_buf(s, 60, temp, sizeof(temp));
	t = temp;
	for (i = 0; i < 4; i++)
	{
		if (t[0] == 0) break;
		else
		{
			strcpy(creature_ptr->history[i], t);
			t += strlen(t) + 1;
		}
	}

	/* Fill the remaining spaces */
	for (i = 0; i < 4; i++)
	{
		for (j = 0; creature_ptr->history[i][j]; j++) /* loop */;

		for (; j < 59; j++) creature_ptr->history[i][j] = ' ';
		creature_ptr->history[i][59] = '\0';
	}

	/* Kill the buffer */
	histpref_buf = NULL;

	return TRUE;
}

/*!
 * @brief 生い立ちメッセージを編集する。/Character background edit-mode
 * @return なし
 */
static void edit_history(player_type *creature_ptr)
{
	char old_history[4][60];
	TERM_LEN y = 0, x = 0;
	int i, j;

	/* Edit character background */
	for (i = 0; i < 4; i++)
	{
		sprintf(old_history[i], "%s", creature_ptr->history[i]);
	}
	/* Turn 0 to space */
	for (i = 0; i < 4; i++)
	{
		for (j = 0; creature_ptr->history[i][j]; j++) /* loop */;

		for (; j < 59; j++) creature_ptr->history[i][j] = ' ';
		creature_ptr->history[i][59] = '\0';
	}

	display_player(creature_ptr, 1);
#ifdef JP
	c_put_str(TERM_L_GREEN, "(キャラクターの生い立ち - 編集モード)", 11, 20);
	put_str("[ カーソルキーで移動、Enterで終了、Ctrl-Aでファイル読み込み ]", 17, 10);
#else
	c_put_str(TERM_L_GREEN, "(Character Background - Edit Mode)", 11, 20);
	put_str("[ Cursor key for Move, Enter for End, Ctrl-A for Read pref ]", 17, 10);
#endif

	while (TRUE)
	{
		int skey;
		char c;

		for (i = 0; i < 4; i++)
		{
			put_str(creature_ptr->history[i], i + 12, 10);
		}
#ifdef JP
		if (iskanji2(creature_ptr->history[y], x))
			c_put_str(TERM_L_BLUE, format("%c%c", creature_ptr->history[y][x], creature_ptr->history[y][x + 1]), y + 12, x + 10);
		else
#endif
			c_put_str(TERM_L_BLUE, format("%c", creature_ptr->history[y][x]), y + 12, x + 10);

		/* Place cursor just after cost of current stat */
		Term_gotoxy(x + 10, y + 12);

		/* Get special key code */
		skey = inkey_special(TRUE);

		/* Get a character code */
		if (!(skey & SKEY_MASK)) c = (char)skey;
		else c = 0;

		if (skey == SKEY_UP || c == KTRL('p'))
		{
			y--;
			if (y < 0) y = 3;
#ifdef JP
			if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1))) x--;
#endif
		}
		else if (skey == SKEY_DOWN || c == KTRL('n'))
		{
			y++;
			if (y > 3) y = 0;
#ifdef JP
			if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1))) x--;
#endif
		}
		else if (skey == SKEY_RIGHT || c == KTRL('f'))
		{
#ifdef JP
			if (iskanji2(creature_ptr->history[y], x)) x++;
#endif
			x++;
			if (x > 58)
			{
				x = 0;
				if (y < 3) y++;
			}
		}
		else if (skey == SKEY_LEFT || c == KTRL('b'))
		{
			x--;
			if (x < 0)
			{
				if (y)
				{
					y--;
					x = 58;
				}
				else x = 0;
			}

#ifdef JP
			if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1))) x--;
#endif
		}
		else if (c == '\r' || c == '\n')
		{
			Term_erase(0, 11, 255);
			Term_erase(0, 17, 255);
#ifdef JP
			put_str("(キャラクターの生い立ち - 編集済み)", 11, 20);
#else
			put_str("(Character Background - Edited)", 11, 20);
#endif
			break;
		}
		else if (c == ESCAPE)
		{
			clear_from(11);
#ifdef JP
			put_str("(キャラクターの生い立ち)", 11, 25);
#else
			put_str("(Character Background)", 11, 25);
#endif

			for (i = 0; i < 4; i++)
			{
				sprintf(creature_ptr->history[i], "%s", old_history[i]);
				put_str(creature_ptr->history[i], i + 12, 10);
			}
			break;
		}
		else if (c == KTRL('A'))
		{
			if (do_cmd_histpref(creature_ptr))
			{
#ifdef JP
				if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1))) x--;
#endif
			}
		}
		else if (c == '\010')
		{
			x--;
			if (x < 0)
			{
				if (y)
				{
					y--;
					x = 58;
				}
				else x = 0;
			}

			creature_ptr->history[y][x] = ' ';
#ifdef JP
			if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1)))
			{
				x--;
				creature_ptr->history[y][x] = ' ';
			}
#endif
		}
#ifdef JP
		else if (iskanji(c) || isprint(c))
#else
		else if (isprint(c)) /* BUGFIX */
#endif
		{
#ifdef JP
			if (iskanji2(creature_ptr->history[y], x))
			{
				creature_ptr->history[y][x + 1] = ' ';
			}

			if (iskanji(c))
			{
				if (x > 57)
				{
					x = 0;
					y++;
					if (y > 3) y = 0;
				}

				if (iskanji2(creature_ptr->history[y], x + 1))
				{
					creature_ptr->history[y][x + 2] = ' ';
				}

				creature_ptr->history[y][x++] = c;

				c = inkey();
			}
#endif
			creature_ptr->history[y][x++] = c;
			if (x > 58)
			{
				x = 0;
				y++;
				if (y > 3) y = 0;
			}
		}
	} /* while (TRUE) */

}


/*!
 * @brief player_birth()関数のサブセット/Helper function for 'player_birth()'
 * @details
 * The delay may be reduced, but is recommended to keep players
 * from continuously rolling up characters, which can be VERY
 * expensive CPU wise.  And it cuts down on player stupidity.
 * @return なし
 */
static bool player_birth_aux(player_type *creature_ptr)
{
	int i, k, n, cs, os;

	BIT_FLAGS mode = 0;

	bool flag = FALSE;
	bool prev = FALSE;

	concptr str;

	char c;

	char p2 = ')';
	char b1 = '[';
	char b2 = ']';

	char buf[80], cur[80];


	/*** Intro ***/
	Term_clear();

	/* Title everything */
	put_str(_("名前  :", "Name  :"), 1, 26);
	put_str(_("性別        :", "Sex         :"), 3, 1);
	put_str(_("種族        :", "Race        :"), 4, 1);
	put_str(_("職業        :", "Class       :"), 5, 1);

	/* Dump the default name */
	c_put_str(TERM_L_BLUE, creature_ptr->name, 1, 34);

	/*** Instructions ***/

	/* Display some helpful information */
	put_str(_("キャラクターを作成します。('S'やり直す, 'Q'終了, '?'ヘルプ)", "Make your charactor. ('S' Restart, 'Q' Quit, '?' Help)"), 8, 10);

	/*** Player sex ***/

	/* Extra info */
	put_str(_("注意：《性別》の違いはゲーム上ほとんど影響を及ぼしません。", "Note: Your 'sex' does not have any significant gameplay effects."), 23, 5);

	/* Prompt for "Sex" */
	for (n = 0; n < MAX_SEXES; n++)
	{
		/* Analyze */
		sp_ptr = &sex_info[n];

		/* Display */
#ifdef JP
		sprintf(buf, "%c%c%s", I2A(n), p2, sp_ptr->title);
#else
		sprintf(buf, "%c%c %s", I2A(n), p2, sp_ptr->title);
#endif
		put_str(buf, 12 + (n / 5), 2 + 15 * (n % 5));
	}

#ifdef JP
	sprintf(cur, "%c%c%s", '*', p2, "ランダム");
#else
	sprintf(cur, "%c%c %s", '*', p2, "Random");
#endif

	/* Choose */
	k = -1;
	cs = 0;
	os = MAX_SEXES;
	while (TRUE)
	{
		if (cs != os)
		{
			put_str(cur, 12 + (os / 5), 2 + 15 * (os % 5));
			if (cs == MAX_SEXES)
#ifdef JP
				sprintf(cur, "%c%c%s", '*', p2, "ランダム");
#else
				sprintf(cur, "%c%c %s", '*', p2, "Random");
#endif
			else
			{
				sp_ptr = &sex_info[cs];
				str = sp_ptr->title;
#ifdef JP
				sprintf(cur, "%c%c%s", I2A(cs), p2, str);
#else
				sprintf(cur, "%c%c %s", I2A(cs), p2, str);
#endif
			}
			c_put_str(TERM_YELLOW, cur, 12 + (cs / 5), 2 + 15 * (cs % 5));
			os = cs;
		}

		if (k >= 0) break;

#ifdef JP
		sprintf(buf, "性別を選んで下さい (%c-%c) ('='初期オプション設定): ", I2A(0), I2A(n - 1));
#else
		sprintf(buf, "Choose a sex (%c-%c) ('=' for options): ", I2A(0), I2A(n - 1));
#endif

		put_str(buf, 10, 10);
		c = inkey();
		if (c == 'Q') birth_quit();
		if (c == 'S') return FALSE;
		if (c == ' ' || c == '\r' || c == '\n')
		{
			if (cs == MAX_SEXES)
				k = randint0(MAX_SEXES);
			else
				k = cs;
			break;
		}
		if (c == '*')
		{
			k = randint0(MAX_SEXES);
			break;
		}
		if (c == '4')
		{
			if (cs > 0) cs--;
		}
		if (c == '6')
		{
			if (cs < MAX_SEXES) cs++;
		}
		k = (islower(c) ? A2I(c) : -1);
		if ((k >= 0) && (k < MAX_SEXES))
		{
			cs = k;
			continue;
		}
		else k = -1;
		if (c == '?') do_cmd_help(creature_ptr);
		else if (c == '=')
		{
			screen_save();
#ifdef JP
			do_cmd_options_aux(OPT_PAGE_BIRTH, "初期オプション((*)はスコアに影響)");
#else
			do_cmd_options_aux(OPT_PAGE_BIRTH, "Birth Option((*)s effect score)");
#endif

			screen_load();
		}
		else if (c != '4' && c != '6')bell();
	}

	/* Set sex */
	creature_ptr->psex = (byte_hack)k;
	sp_ptr = &sex_info[creature_ptr->psex];

	/* Display */
	c_put_str(TERM_L_BLUE, sp_ptr->title, 3, 15);

	/* Clean up */
	clear_from(10);

	/* Choose the players race */
	creature_ptr->prace = 0;
	while (TRUE)
	{
		char temp[80 * 10];
		concptr t;

		if (!get_player_race(creature_ptr)) return FALSE;

		clear_from(10);

		roff_to_buf(race_jouhou[creature_ptr->prace], 74, temp, sizeof(temp));
		t = temp;

		for (i = 0; i < 10; i++)
		{
			if (t[0] == 0)
				break;
			else
			{
				prt(t, 12 + i, 3);
				t += strlen(t) + 1;
			}
		}
#ifdef JP
		if (get_check_strict("よろしいですか？", CHECK_DEFAULT_Y)) break;
#else
		if (get_check_strict("Are you sure? ", CHECK_DEFAULT_Y)) break;
#endif
		clear_from(10);
		c_put_str(TERM_WHITE, "              ", 4, 15);
	}

	/* Clean up */
	clear_from(10);

	/* Choose the players class */
	creature_ptr->pclass = 0;
	while (TRUE)
	{
		char temp[80 * 9];
		concptr t;

		if (!get_player_class(creature_ptr)) return FALSE;

		clear_from(10);
		roff_to_buf(class_jouhou[creature_ptr->pclass], 74, temp, sizeof(temp));
		t = temp;

		for (i = 0; i < 9; i++)
		{
			if (t[0] == 0)
				break;
			else
			{
				prt(t, 12 + i, 3);
				t += strlen(t) + 1;
			}
		}

#ifdef JP
		if (get_check_strict("よろしいですか？", CHECK_DEFAULT_Y)) break;
#else
		if (get_check_strict("Are you sure? ", CHECK_DEFAULT_Y)) break;
#endif
		c_put_str(TERM_WHITE, "              ", 5, 15);
	}

	/* Choose the magic realms */
	if (!get_player_realms(creature_ptr)) return FALSE;

	/* Choose the players seikaku */
	creature_ptr->pseikaku = 0;
	while (TRUE)
	{
		char temp[80 * 8];
		concptr t;

		if (!get_player_seikaku(creature_ptr)) return FALSE;

		clear_from(10);
		roff_to_buf(seikaku_jouhou[creature_ptr->pseikaku], 74, temp, sizeof(temp));
		t = temp;

		for (i = 0; i < A_MAX; i++)
		{
			if (t[0] == 0)
				break;
			else
			{
				prt(t, 12 + i, 3);
				t += strlen(t) + 1;
			}
		}
#ifdef JP
		if (get_check_strict("よろしいですか？", CHECK_DEFAULT_Y)) break;
#else
		if (get_check_strict("Are you sure? ", CHECK_DEFAULT_Y)) break;
#endif
		c_put_str(TERM_L_BLUE, creature_ptr->name, 1, 34);
		prt("", 1, 34 + strlen(creature_ptr->name));
	}

	/* Clean up */
	clear_from(10);
	put_str("                                   ", 3, 40);
	put_str("                                   ", 4, 40);
	put_str("                                   ", 5, 40);

	screen_save();
#ifdef JP
	do_cmd_options_aux(OPT_PAGE_BIRTH, "初期オプション((*)はスコアに影響)");
#else
	do_cmd_options_aux(OPT_PAGE_BIRTH, "Birth Option((*)s effect score)");
#endif

	screen_load();

	if (autoroller || autochara)
	{
		/* Clear fields */
		auto_round = 0L;
	}

	if (autoroller)
	{
		if (!get_stat_limits(creature_ptr)) return FALSE;
	}

	if (autochara)
	{
		if (!get_chara_limits(creature_ptr)) return FALSE;
	}

	clear_from(10);

	/* Reset turn; before auto-roll and after choosing race */
	init_turn(creature_ptr);

	/*** Generate ***/

	/* Roll */
	while (TRUE)
	{
		int col;

		col = 42;

		if (autoroller || autochara)
		{
			Term_clear();

			/* Label count */
#ifdef JP
			put_str("回数 :", 10, col + 13);
#else
			put_str("Round:", 10, col + 13);
#endif


			/* Indicate the state */
#ifdef JP
			put_str("(ESCで停止)", 12, col + 13);
#else
			put_str("(Hit ESC to stop)", 12, col + 13);
#endif
		}

		/* Otherwise just get a character */
		else
		{
			get_stats(creature_ptr);
			get_ahw(creature_ptr);
			get_history(creature_ptr);
		}

		/* Feedback */
		if (autoroller)
		{
			/* Label */
#ifdef JP
			put_str("最小値", 2, col + 5);
#else
			put_str(" Limit", 2, col + 5);
#endif


			/* Label */
#ifdef JP
			put_str("成功率", 2, col + 13);
#else
			put_str("  Freq", 2, col + 13);
#endif


			/* Label */
#ifdef JP
			put_str("現在値", 2, col + 24);
#else
			put_str("  Roll", 2, col + 24);
#endif


			/* Put the minimal stats */
			for (i = 0; i < A_MAX; i++)
			{
				int j, m;

				/* Label stats */
				put_str(stat_names[i], 3 + i, col);

				/* Race/Class bonus */
				j = rp_ptr->r_adj[i] + cp_ptr->c_adj[i] + ap_ptr->a_adj[i];

				/* Obtain the current stat */
				m = adjust_stat(stat_limit[i], j);

				/* Put the stat */
				cnv_stat(m, buf);
				c_put_str(TERM_L_BLUE, buf, 3 + i, col + 5);
			}
		}

		/* Auto-roll */
		while (autoroller || autochara)
		{
			bool accept = TRUE;

			/* Get a new character */
			get_stats(creature_ptr);

			/* Advance the round */
			auto_round++;

			/* Hack -- Prevent overflow */
			if (auto_round >= 1000000000L)
			{
				auto_round = 1;

				if (autoroller)
				{
					for (i = 0; i < A_MAX; i++)
					{
						stat_match[i] = 0;
					}
				}
			}

			if (autoroller)
			{
				/* Check and count acceptable stats */
				for (i = 0; i < A_MAX; i++)
				{
					/* This stat is okay */
					if (creature_ptr->stat_max[i] >= stat_limit[i])
					{
						stat_match[i]++;
					}

					/* This stat is not okay */
					else
					{
						accept = FALSE;
					}
				}
			}

			/* Break if "happy" */
			if (accept)
			{
				get_ahw(creature_ptr);
				get_history(creature_ptr);

				if (autochara)
				{
					if ((creature_ptr->age < chara_limit.agemin) || (creature_ptr->age > chara_limit.agemax)) accept = FALSE;
					if ((creature_ptr->ht < chara_limit.htmin) || (creature_ptr->ht > chara_limit.htmax)) accept = FALSE;
					if ((creature_ptr->wt < chara_limit.wtmin) || (creature_ptr->wt > chara_limit.wtmax)) accept = FALSE;
					if ((creature_ptr->sc < chara_limit.scmin) || (creature_ptr->sc > chara_limit.scmax)) accept = FALSE;
				}
				if (accept) break;
			}

			/* Take note every x rolls */
			flag = (!(auto_round % AUTOROLLER_STEP));

			/* Update display occasionally */
			if (flag)
			{
				/* Dump data */
				birth_put_stats(creature_ptr);

				/* Dump round */
				put_str(format("%10ld", auto_round), 10, col + 20);

#ifdef AUTOROLLER_DELAY
				/* Delay 1/10 second */
				if (flag) Term_xtra(TERM_XTRA_DELAY, 10);
#endif

				/* Make sure they see everything */
				Term_fresh();

				/* Do not wait for a key */
				inkey_scan = TRUE;

				/* Check for a keypress */
				if (inkey())
				{
					get_ahw(creature_ptr);
					get_history(creature_ptr);
					break;
				}
			}
		}

		if (autoroller || autochara) sound(SOUND_LEVEL);

		flush();

		/*** Display ***/

		mode = 0;

		/* Roll for base hitpoints */
		get_extra(creature_ptr, TRUE);

		/* Roll for gold */
		get_money(creature_ptr);

		/* Hack -- get a chaos patron even if you are not a chaos warrior */
		creature_ptr->chaos_patron = (s16b)randint0(MAX_PATRON);

		/* Input loop */
		while (TRUE)
		{
			/* Calculate the bonuses and hitpoints */
			creature_ptr->update |= (PU_BONUS | PU_HP);
			update_creature(creature_ptr);

			creature_ptr->chp = creature_ptr->mhp;
			creature_ptr->csp = creature_ptr->msp;

			display_player(creature_ptr, mode);

			/* Prepare a prompt (must squeeze everything in) */
			Term_gotoxy(2, 23);
			Term_addch(TERM_WHITE, b1);
			Term_addstr(-1, TERM_WHITE, _("'r' 次の数値", "'r'eroll"));

			if (prev) Term_addstr(-1, TERM_WHITE, _(", 'p' 前の数値", "'p'previous"));
			if (mode) Term_addstr(-1, TERM_WHITE, _(", 'h' その他の情報", ", 'h' Misc."));
			else Term_addstr(-1, TERM_WHITE, _(", 'h' 生い立ちを表示", ", 'h'istory"));
			Term_addstr(-1, TERM_WHITE, _(", Enter この数値に決定", ", or Enter to accept"));
			Term_addch(TERM_WHITE, b2);

			c = inkey();

			/* Quit */
			if (c == 'Q') birth_quit();

			/* Start over */
			if (c == 'S') return FALSE;

			/* Escape accepts the roll */
			if (c == '\r' || c == '\n' || c == ESCAPE) break;

			/* Reroll this character */
			if ((c == ' ') || (c == 'r')) break;

			/* Previous character */
			if (prev && (c == 'p'))
			{
				load_prev_data(creature_ptr, TRUE);
				continue;
			}

			if ((c == 'H') || (c == 'h'))
			{
				mode = ((mode != 0) ? 0 : 1);
				continue;
			}

			/* Help */
			if (c == '?')
			{
#ifdef JP
				show_help(creature_ptr, "jbirth.txt#AutoRoller");
#else
				show_help(creature_ptr, "birth.txt#AutoRoller");
#endif
				continue;
			}
			else if (c == '=')
			{
				screen_save();
				do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
				screen_load();
				continue;
			}

			/* Warning */
			bell();
		}

		/* Are we done? */
		if (c == '\r' || c == '\n' || c == ESCAPE) break;

		/* Save this for the "previous" character */
		save_prev_data(creature_ptr, &previous_char);
		previous_char.quick_ok = FALSE;

		/* Note that a previous roll exists */
		prev = TRUE;
	}

	/* Clear prompt */
	clear_from(23);

	/* Get a name, recolor it, prepare savefile */
	get_name(creature_ptr);

	/* Process the player name */
	process_player_name(creature_ptr, current_world_ptr->creating_savefile);

	/*** Edit character background ***/
	edit_history(creature_ptr);

	/*** Finish up ***/

	get_max_stats(creature_ptr);

	get_virtues(creature_ptr);

	/* Prompt for it */
#ifdef JP
	prt("[ 'Q' 中断, 'S' 初めから, Enter ゲーム開始 ]", 23, 14);
#else
	prt("['Q'uit, 'S'tart over, or Enter to continue]", 23, 10);
#endif


	/* Get a key */
	c = inkey();

	/* Quit */
	if (c == 'Q') birth_quit();

	/* Start over */
	if (c == 'S') return FALSE;


	/* Initialize random quests */
	init_dungeon_quests(creature_ptr);

	/* Save character data for quick start */
	save_prev_data(creature_ptr, &previous_char);
	previous_char.quick_ok = TRUE;

	/* Accept */
	return TRUE;
}

/*!
 * @brief クイックスタート処理の問い合わせと実行を行う。/Ask whether the player use Quick Start or not.
 * @return なし
 */
static bool ask_quick_start(player_type *creature_ptr)
{
	/* Doesn't have previous data */
	if (!previous_char.quick_ok) return FALSE;

	Term_clear();

	/* Extra info */
	put_str(_("クイック・スタートを使うと以前と全く同じキャラクターで始められます。", "Do you want to use the quick start function(same character as your last one)."), 11, 2);

	/* Choose */
	while (TRUE)
	{
		char c;

		put_str(_("クイック・スタートを使いますか？[y/N]", "Use quick start? [y/N]"), 14, 10);
		c = inkey();

		if (c == 'Q') quit(NULL);
		else if (c == 'S') return FALSE;
		else if (c == '?')
		{
#ifdef JP
			show_help(creature_ptr, "jbirth.txt#QuickStart");
#else
			show_help(creature_ptr, "birth.txt#QuickStart");
#endif
		}
		else if ((c == 'y') || (c == 'Y'))
		{
			/* Yes */
			break;
		}
		else
		{
			/* No */
			return FALSE;
		}
	}

	load_prev_data(creature_ptr, FALSE);
	init_turn(creature_ptr);
	init_dungeon_quests(creature_ptr);

	sp_ptr = &sex_info[creature_ptr->psex];
	rp_ptr = &race_info[creature_ptr->prace];
	cp_ptr = &class_info[creature_ptr->pclass];
	mp_ptr = &m_info[creature_ptr->pclass];
	ap_ptr = &seikaku_info[creature_ptr->pseikaku];

	/* Calc hitdie, but don't roll */
	get_extra(creature_ptr, FALSE);

	creature_ptr->update |= (PU_BONUS | PU_HP);
	update_creature(creature_ptr);
	creature_ptr->chp = creature_ptr->mhp;
	creature_ptr->csp = creature_ptr->msp;

	/* Process the player name */
	process_player_name(creature_ptr, FALSE);

	return TRUE;
}


/*!
 * @brief プレイヤー作成処理のメインルーチン/ Create a new character.
 * @details
 * Note that we may be called with "junk" leftover in the various
 * fields, so we must be sure to clear them first.
 * @return なし
 */
void player_birth(player_type *creature_ptr)
{
	int i, j;
	char buf[80];

	current_world_ptr->play_time = 0;

	wipe_monsters_list(creature_ptr);

	/* Wipe the player */
	player_wipe_without_name(creature_ptr);

	/* Create a new character */

	/* Quick start? */
	if (!ask_quick_start(creature_ptr))
	{
		play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_DEFAULT);

		/* No, normal start */
		while (TRUE)
		{
			/* Roll up a new character */
			if (player_birth_aux(creature_ptr)) break;

			/* Wipe the player */
			player_wipe_without_name(creature_ptr);
		}
	}

	/* Note player birth in the message recall */
	message_add(" ");
	message_add("  ");
	message_add("====================");
	message_add(" ");
	message_add("  ");

	exe_write_diary(creature_ptr, DIARY_GAMESTART, 1, _("-------- 新規ゲーム開始 --------", "-------- Start New Game --------"));
	exe_write_diary(creature_ptr, DIARY_DIALY, 0, NULL);

	sprintf(buf, _("                            性別に%sを選択した。", "                            choose %s personality."), sex_info[creature_ptr->psex].title);
	exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);

	sprintf(buf, _("                            種族に%sを選択した。", "                            choose %s race."), race_info[creature_ptr->prace].title);
	exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);

	sprintf(buf, _("                            職業に%sを選択した。", "                            choose %s class."), class_info[creature_ptr->pclass].title);
	exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);

	if (creature_ptr->realm1)
	{
		sprintf(buf, _("                            魔法の領域に%s%sを選択した。", "                            choose %s%s realm."), realm_names[creature_ptr->realm1], creature_ptr->realm2 ? format("と%s", realm_names[creature_ptr->realm2]) : "");
		exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
	}

	sprintf(buf, _("                            性格に%sを選択した。", "                            choose %s."), seikaku_info[creature_ptr->pseikaku].title);
	exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);

	/* Init the shops */
	for (i = 1; i < max_towns; i++)
	{
		for (j = 0; j < MAX_STORES; j++)
		{
			store_init(i, j);
		}
	}

	/* Generate the random seeds for the wilderness */
	seed_wilderness();

	/* Give beastman a mutation at character birth */
	if (creature_ptr->prace == RACE_BEASTMAN) creature_ptr->hack_mutation = TRUE;
	else creature_ptr->hack_mutation = FALSE;

	/* Set the message window flag as default */
	if (!window_flag[1])
		window_flag[1] |= PW_MESSAGE;

	/* Set the inv/equip window flag as default */
	if (!window_flag[2])
		window_flag[2] |= PW_INVEN;
}

/*!
 * @brief プレイヤー作成処理中のステータス表示処理
 * @param fff ファイルポインタ
 * @return なし
 */
void dump_yourself(player_type *creature_ptr, FILE *fff)
{
	char temp[80 * 10];
	int i;
	concptr t;

	if (!fff) return;

	roff_to_buf(race_jouhou[creature_ptr->prace], 78, temp, sizeof(temp));
	fprintf(fff, "\n\n");
	fprintf(fff, _("種族: %s\n", "Race: %s\n"), race_info[creature_ptr->prace].title);

	t = temp;
	for (i = 0; i < 10; i++)
	{
		if (t[0] == 0)
			break;
		fprintf(fff, "%s\n", t);
		t += strlen(t) + 1;
	}
	roff_to_buf(class_jouhou[creature_ptr->pclass], 78, temp, sizeof(temp));
	fprintf(fff, "\n");
	fprintf(fff, _("職業: %s\n", "Class: %s\n"), class_info[creature_ptr->pclass].title);

	t = temp;
	for (i = 0; i < 10; i++)
	{
		if (t[0] == 0)
			break;
		fprintf(fff, "%s\n", t);
		t += strlen(t) + 1;
	}
	roff_to_buf(seikaku_jouhou[creature_ptr->pseikaku], 78, temp, sizeof(temp));
	fprintf(fff, "\n");
	fprintf(fff, _("性格: %s\n", "Pesonality: %s\n"), seikaku_info[creature_ptr->pseikaku].title);

	t = temp;
	for (i = 0; i < A_MAX; i++)
	{
		if (t[0] == 0)
			break;
		fprintf(fff, "%s\n", t);
		t += strlen(t) + 1;
	}
	fprintf(fff, "\n");
	if (creature_ptr->realm1)
	{
		roff_to_buf(realm_jouhou[technic2magic(creature_ptr->realm1) - 1], 78, temp, sizeof(temp));
		fprintf(fff, _("魔法: %s\n", "Realm: %s\n"), realm_names[creature_ptr->realm1]);

		t = temp;
		for (i = 0; i < A_MAX; i++)
		{
			if (t[0] == 0)
				break;
			fprintf(fff, "%s\n", t);
			t += strlen(t) + 1;
		}
	}
	fprintf(fff, "\n");
	if (creature_ptr->realm2)
	{
		roff_to_buf(realm_jouhou[technic2magic(creature_ptr->realm2) - 1], 78, temp, sizeof(temp));
		fprintf(fff, _("魔法: %s\n", "Realm: %s\n"), realm_names[creature_ptr->realm2]);

		t = temp;
		for (i = 0; i < A_MAX; i++)
		{
			if (t[0] == 0)
				break;
			fprintf(fff, "%s\n", t);
			t += strlen(t) + 1;
		}
	}
}
