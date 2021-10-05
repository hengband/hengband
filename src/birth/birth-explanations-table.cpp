#include "birth/birth-explanations-table.h"
#include "system/angband.h"

/*! 種族の解説メッセージテーブル */
const std::vector<std::string_view> race_explanations = {
    _("人間は基本となるキャラクタです。他の全ての種族は人間と比較されます。人間はどんな職業に就くこともでき、どの職業でも平均的にこなせます。人間は寿命が短いため、レベル上昇が他のどんな種族よりも早くなる傾向があります。また、特別な修正や特性は持っていません。",
        "The human is the base character.  All other races are compared to them.  Humans can choose any class and are average at everything.  Humans tend to go up levels faster than most other races because of their shorter life spans.  No racial adjustments or intrinsics occur to characters choosing human."),

    _("ハーフエルフは人間より賢いですが、強くはありません。彼らは探索, 解除, 魔法防御, 隠密行動, 射撃, そして魔法道具使用でわずかに優れています。しかし武器の取り扱いはそう得意ではありません。ハーフエルフはどの職業に就くこともでき、生まれつきの特性はありません。",
        "Half-elves tend to be smarter and faster than humans, but not as strong.  Half-elves are slightly better at searching, disarming, saving throws, stealth, bows, and magic, but they are not as good at hand weapons.  Half-elves may choose any class and do not receive any intrinsic abilities."),

    _("エルフは人間より良い魔法使いになれますが、戦闘は苦手です。彼らは人間やハーフエルフよりも頭が良く、高い賢さを持っています。エルフは探索, 解除, 知覚, 隠密行動, 射撃, そして魔法道具使用で優れていますが、武器の扱いは得意ではありません。彼らは生まれつき光に対する耐性を持っています。",
        "Elves are better magicians then humans, but not as good at fighting.  They tend to be smarter and faster than either humans or half-elves and also have better wisdom.  Elves are better at searching, disarming, perception, stealth, bows, and magic, but they are not as good at hand weapons.  They resist light effects intrinsically."),

    _("ホビット、またはハーフリングは弓や投擲に長け、魔法防御も優れています。また、探索, 解除, 知覚, そして隠密行動でもとても良い能力を示します。そのため、彼らは優れた盗賊となることができます（しかし、「忍びの者」と呼ばれることを好みます）。ホビットは人間より遥かに貧弱で、戦士としてはてんでダメです。彼らはかなり良い赤外線視力を持っており、温血動物を離れた場所から見つけることができます。彼らは経験値を保持する力が強く、経験値吸収攻撃に対して耐性を持っています。",
        "Hobbits, or Halflings, are very good at bows, throwing, and have good saving throws.  They also are very good at searching, disarming, perception, and stealth; so they make excellent rogues, but prefer to be called burglars.  They are much weaker than humans and no good at melee fighting.  Halflings have fair infravision, so they can detect warm creatures at a distance.  They have a strong hold on their life force and are thus intrinsically resistant to life draining."),

    _("ノームはドワーフより小さいですが、ホビットよりは大きい種族です。彼らはホビット同様地表の洞穴のような家に住んでいます。ノームはとても良い魔法防御を持ち、探索, 解除, 知覚, 隠密行動でも優れています。彼らは人間より低い腕力を持ち、武器を持っての戦闘は苦手です。ノームはかなり良い赤外線視力を持っており、温血動物を離れた場所から見つけることができます。ノームは生まれつき麻痺に対する耐性を持っています。",
        "Gnomes are smaller than dwarves but larger than Halflings.  They, like the hobbits, live in the earth in burrow-like homes.  Gnomes make excellent mages and have very good saving throws.  They are good at searching, disarming, perception, and stealth.  They have lower strength than humans so they are not very good at fighting with hand weapons.  Gnomes have fair infravision, so they can detect warm-blooded creatures at a distance.  Gnomes are intrinsically protected against paralysis."),

    _("ドワーフは頑固な坑夫であり、伝説の戦士です。彼らは人間にくらべ強くタフですが、知能は劣ります。しかし、長命ゆえに彼らは非常に賢いです。彼らは良い魔法防御を持ち、探索, 知覚, 戦闘, 射撃では優れています。彼らは一つ大きな欠点を持っています。ドワーフの隠密行動は絶望的に悪いです。彼らは決して盲目にはなりません。",
        "Dwarves are the headstrong miners and fighters of legend.  Dwarves tend to be stronger and tougher but slower and less intelligent than humans.  Because they are so headstrong and are somewhat wise, they resist spells which are cast on them.  They are very good at searching, perception, fighting, and bows.  Dwarves have miserable stealth.  They can never be blinded."),

    _("ハーフオークはよい戦士になれますが、魔法は期待できません。彼らはドワーフと同じくらい隠密行動が悪く、また探索や解除, 知覚もひどいです。ハーフオークは醜く、店での買い物ではより高い金額を要求されがちです。彼らは地下に住むことを好むため、ハーフオークは暗闇に対する耐性を備えています。",
        "Half-orcs make excellent warriors, but are terrible at magic.  They are as bad as dwarves at stealth, and horrible at searching, disarming, and perception.  Half-orcs are quite ugly and tend to pay more for goods in town.  Because of their preference to living underground to on the surface, half-orcs resist darkness attacks."),

    _("ハーフトロルは信じられないほど強く、他の大部分の種族より大きなＨＰを持ちます。彼らは不運にもとても愚かです。彼らの探索, 解除, 知覚, 隠密行動は悪く、その外見はハーフオークがしかめっ面をするほど醜悪です。ハーフトロルは腕力が下がることがありません。レベルが上がると、彼らは再生能力を手にいれ、戦士ならばさらに遅消化能力も獲得します。",
        "Half-Trolls are incredibly strong and have more hit points than most other races.  They are also very stupid and slow.  They are bad at searching, disarming, perception, and stealth.  They are so ugly that a Half-Orc grimaces in their presence.  They also happen to be fun to run...  Half-trolls always have their strength sustained.  At higher levels, Half-Trolls regenerate wounds automatically and, if a warrior, require food less often."),

    _("アンバライトは多くのアドバンテージを授けられた、うわさによれば不死の種族です。彼らは知覚, 戦闘, 射撃に優れており、他の面でもかなり熟練しています。事実上あらゆるものを見てきており、新鮮なものはほとんどないため、彼らの成長は他のどの種族より遅いものです。彼らはとてもタフで頑強であり、彼らの耐久力が下がることはありません。また、怪我をすぐに治す再生能力があります。",
        "The Amberites are a reputedly immortal race, who are endowed with numerous advantages in addition to their longevity.  They are very tough and their constitution cannot be reduced.  Their ability to heal wounds far surpasses that of any other race.  Having seen virtually everything, very little is new to them, and they gain levels much more slowly than the other races."),

    _("ハイエルフは世界の始まりから存在する不死の種族です。彼らは全てのスキルに熟達しており、強く、知的で非常に人気があります - 誰もが彼らのことを好いています。ハイエルフは見えないものを見ることができ、普通のエルフ同様光に対する耐性を持っています。しかし、彼らにとって未知のものはほとんどなく、経験を得ることは大変に困難です。",
        "High-elves are a race of immortal beings dating from the beginning of time.  They are masters of all skills, and are strong and intelligent, although their wisdom is sometimes suspect.  High-elves begin their lives able to see the unseen, and resist light effects just like regular elves.  However, there are few things that they have not seen already, and experience is very hard for them to gain."),

    _("野蛮人は北方から来た頑強な種族です。彼らは激しく戦い、彼らの激怒は世界中で恐れられています。戦闘が彼らの人生です。彼らは恐れを知らず、ハーフトロルよりもすぐに狂暴に戦闘に入ってしまうことを学びます。しかし、野蛮人は魔法を疑っており、そのため魔法の道具を使うことはかなり大変なこととなっています。",
        "Barbarians are hardy men of the north.  They are fierce in combat, and their wrath is feared throughout the world.  Combat is their life: they feel no fear, and they learn to enter battle frenzy at will even sooner than half-trolls.  Barbarians are, however, suspicious of magic, which makes magic devices fairly hard for them to use."),

    _("ハーフオーガはハーフオークに似ていますが、それだけではありません。彼らは大きく、邪悪で愚かです。戦士としては彼らは必要な資質を全て持っており、また魔法使いになることさえできます。結局、彼らはオーガ・メイジに関係があり、レベルが十分に上がったら彼らから罠のルーンをセットするスキルを学ぶのです。ハーフオークのように、彼らは暗闇に対する耐性を持ち、ハーフトロル同様に腕力が下がることはありません。",
        "Half-Ogres are like Half-Orcs, only more so.  They are big, bad, and stupid.  For warriors, they have all the necessary attributes, and they can even become wizards: after all, they are related to Ogre Magi, from whom they have learned the skill of setting trapped runes once their level is high enough.  Like Half-Orcs, they resist darkness, and like Half-Trolls, they have their strength sustained."),

    _("半巨人は大変力強いのですが、呪文を唱えられるほど利口ではありません。彼らはよい戦闘能力を持ちますが、それ以外のことは苦手です。彼らの厚い皮膚は破片に対する耐性を持ちます。また、ハーフオーガやハーフトロル同様腕力を下げられることがありません。",
        "Half-Giants' limited intelligence makes it difficult for them to become full spellcasters, but, with their great strength, they make excellent warriors.  Their thick skin makes them resistant to shards, and, like Half-Ogres and Half-Trolls, they have their strength sustained."),

    _("巨大なタイタンと人間の子孫であり、この強大な生物は他のほぼ全ての種族よりはるかに勝っています。彼らは多種族にみられるような魅力的な特殊能力は持っていませんが、その大変大きなＨＰはそれを補ってあまりあります。半タイタンはそこそこのスキルを持っていますが、その巨大さゆえに罠の解除やこっそり歩くことは困難です。法と秩序を愛する彼らは、カオスに対する耐性を持っています。",
        "Half-mortal descendants of the mighty titans, these immensely powerful creatures put almost any other race to shame.  They may lack the fascinating special powers of certain other races, but their enhanced attributes more than make up for that.  They learn to estimate the strengths of their foes, and their love for law and order makes them resistant to the effects of Chaos."),

    _("一つ目ではありますが、サイクロプスは多くの二つの目を持つ生物以上に見ることができます。サイクロプスは非常に力強いのですが、知的であるとはちょっと言えません。彼らに比べれば、ハーフトロルの方がハンサムに見えるということは言うまでもありません。サイクロプスは戦闘, 射撃に優れていますが、その他の大部分のスキルは苦手です。サイクロプスは音に対する耐性を持っています。",
        "With but one eye, a Cyclops can see more than many with two eyes.  They are headstrong, and loud noises bother them very little.  They are not quite qualified for the magic using professions, but as a certain Mr.  Ulysses can testify, their accuracy with thrown rocks can be deadly..."),

    _("イークは最も哀れな生物の一つであり、並のモンスターであっても不注意なイークならば徹底的に打ちのめせるほど肉体的には強くありませんが、彼らはかなり知的でいくらか賢い生物です。イークは戦闘スキルは苦手ですが、他の分野では優れています。彼らの皮膚は、時間とともに酸への耐性を増していき、レベルが十分に上がれば完全に免疫を持つようになります。",
        "Yeeks are among the most pathetic creatures.  Fortunately, their horrible screams can scare away less confident foes, and their skin becomes more and more resistant to acid, as they gain experience.  But having said that, even a mediocre monster can wipe the proverbial floor with an unwary Yeek."),

    _("クラッコンは奇怪な半知的の昆虫型生物です。彼らはすばらしい戦士になれますが、精神的な能力はひどく制限されています。彼らは探索を除けば大部分のスキルをそこそこにこなします。クラッコンは決して混乱させられることがなく、レベルが上がるごとに速くなります。",
        "Klackons are bizarre semi-intelligent ant-like insectoid creatures.  They make great fighters, but their mental abilities are severely limited.  Obedient and well-ordered, they can never be confused.  They are also very nimble, and become faster as they advance levels.  They are also very acidic, inherently resisting acid, and capable of spitting acid at higher levels."),

    _("コボルドは弱いゴブリンの種族です。彼らは毒を持った武器を好み、毒矢（無制限に供給されます）を投げる能力を身につけることができます。コボルドはそこそこの戦士になれますが、その他のスキルは軒並み悪いです。彼らは生まれつき毒に対する耐性を持っています。",
        "Kobolds are a weak goblin race.  They love poisoned weapons, and can learn to throw poisoned darts (of which they carry an unlimited supply).  They are also inherently resistant to poison, although they are not one of the more powerful races."),

    _("嫌われ、迫害されてきた小人族です。彼らは大抵のスキルをそつなくこなします。洞穴居住者である彼らは、暗闇に悩まされることはありませんし、生まれつき持っている魔法のアイテムに対する嗜好のため、彼らは装備による魔法のボーナスを奪う効果に耐性を持っています。",
        "The hated and persecuted race of nocturnal dwarves, these cavedwellers are not much bothered by darkness.  Their natural inclination to magical items has made them immune to effects which could drain away magical energy."),

    _("闇の、洞穴に住む種族であるダークエルフは魔法の知識に対する長い伝統を持っています。ダークエルフは魔法の道具をうまく使うことができ、他の多くの種族より簡単に呪文を唱えられるだけの知能を持っています。その鋭い視覚によって、彼らはハイエルフ同様見えないものをみる能力を学びますが、それはある程度レベルが上がったときです。ダークエルフは暗闇に対する耐性を持っています。",
        "Another dark, cavedwelling race, likewise unhampered by darkness attacks, the Dark Elves have a long tradition and knowledge of magic.  They have an inherent magic missile attack available to them at a low level.  With their keen sight, they also learn to see invisible things as their relatives High-Elves do, but at a higher level."),

    _("ドラゴンのような特性を持った人間型種族です。彼らはレベルが上がるにつれ、新しい元素への耐性を手にいれます。ドラコニアンは優れた能力値を持ってゲームを開始でき、大抵のスキルをうまくこなせます。その翼で、彼らは簡単に落とし穴や溶岩、水を無傷で飛び越えることができます。",
        "A humanoid race with dragon-like attributes.  As they advance levels, they gain new elemental resistances (up to Poison Resistance), and they also have a breath weapon, which becomes more powerful with experience.  The exact type of the breath weapon depends on the Draconian's class and level.  With their wings, they can easily escape any pit trap unharmed."),

    _("秘密主義の神秘的な古代種族です。彼らの文明はこの惑星上の何よりも古いかもしれません。その肉体的資質は決して誉められたものではありませんが、彼らの知能と賢さはマインドフレアを他のどんな種族よりも強力な魔法使いにします。マインドフレアの知能と賢さは下がることがなく、レベルが上がれば見えないものをみる能力、テレパシー能力を獲得します。",
        "A secretive and mysterious ancient race.  Their civilization may well be older than any other on our planet, and their intelligence and wisdom are naturally sustained, and are so great that they enable Mind Flayers to become more powerful spellcasters than any other race, even if their physical attributes are a good deal less admirable.  As they advance levels, they gain the powers of See Invisible and Telepathy."),

    _("地獄からやってきた悪魔的な生物です。彼らは他の種族から毛嫌いされていますが、大抵の職業をかなりうまくこなすことができます。インプは生まれつき火に耐性を持っており、レベルが上がれば見えないものを見る能力を獲得します。",
        "Demon-creatures from the nether-world, imps are naturally resistant to fire attacks and capable of learning fire bolt and fire ball attacks.  They are little loved by other races but can perform fairly well in most professions.  As they advance levels, they gain the powers of See Invisible."),

    _("ゴーレムは泥のような生命のない材料からつくられ、生命を吹き込まれた人工的な生物です。彼らには思考というものがほとんどなく、そのため魔法に依存する職業では役立たずです。しかし戦士としては大変にタフです。彼らは毒に耐性を持ち、見えないものを見ることができ、さらに麻痺知らずです。レベルが上がれば、彼らは生命力吸収攻撃に耐性を持つようになります。ゴーレムは通常の食物からはほとんど栄養を摂取できませんが、代わりに魔法棒や杖から魔力を吸収して動力源にする事ができます。また、その頑丈な身体のため、ACにボーナスを得ることができ、さらに決して気絶させられることがありません。",
        "A Golem is an artificial creature, built from a lifeless raw material like clay, and awakened to life.  They are nearly mindless, making them useless for professions which rely on magic, but as warriors they are very tough.  They are resistant to poison, they can see invisible things, and move freely.  At higher levels, they also become resistant to attacks which threaten to drain away their experience.  Golems gain very little nutrition from ordinary food, but can absorb mana from staffs and wands as their power source.  Golems also gain a natural armor class bonus from their tough body."),

    _("スケルトンには2つのタイプが存在します。普通の戦士タイプと、リッチと呼ばれる呪文を使うスケルトンです。アンデッドである彼らは、毒や生命力吸収攻撃を心配する必要はありません。彼らは物体を知覚するのに眼を利用していないため、見えない物に騙されません。彼らの骨はとがった破片のようなものに耐性を持ち、レベルが上がれば冷気に対する耐性を獲得します。薬や食物の持つ効果はスケルトンの胃（存在しませんが）を通過することなくその力を発揮しますが、薬や食物自体は彼の顎を通り抜けて落ちてしまい、栄養を吸収することはできません。その代わりに魔法棒や杖から魔力を吸収してエネルギー源にする事ができます。",
        "There are two types of skeletons: the ordinary, warrior-like skeletons, and the spell-using skeletons, which are also called liches.  As undead beings, skeletons need to worry very little about poison or attacks that can drain life.  They do not really use eyes for perceiving things, and are thus not fooled by invisibility.  Their bones are resistant to sharp shrapnel, and they will quickly become resistant to cold.  Although the magical effects of these will affect the skeleton even without entering the skeleton's (non-existent) belly, the potion or food itself will fall through the skeleton's jaws, giving no nutritional benefit.  They can absorb mana from staffs and wands as their energy source."),

    _("ゾンビはアンデッドであり、生命力吸収攻撃に耐性を持ち、スケルトンのようにレベルが上がれば冷気の耐性を獲得します。また、毒に耐性を持ち見えないものを見ることができます。（スケルトンとは違い）切る攻撃には弱いですが、地獄に対する耐性を持っています。ゴーレムのように、彼らは食物からほとんど栄養を摂取できませんが、代わりに魔法棒や杖から魔力を吸収してエネルギー源にする事ができます。",
        "Much like Skeletons, Zombies too are undead horrors: they are resistant to exp-draining attacks, and can learn to restore their experience.  Like skeletons, they become resistant to cold-based attacks (actually earlier than skeletons), resist poison and can see invisible.  While still vulnerable to cuts (unlike skeletons), Zombies are resistant to Nether.  Like Golems, they gain very little nutrition from the food of mortals, but can absorb mana from staffs and wands as their energy source."),

    _("強力なアンデッドの一種である吸血鬼は、畏敬の念を呼び起こす外見をしています。アンデッドの例にもれず、彼らも生命力を吸収されることがなく、地獄に対する耐性を持っています。また、冷気と毒に対する耐性も備えています。しかし、新鮮な血液に常に飢えており、それは近くにいる生物から血液を吸うことによってのみ満たされます。この強力な生物は深刻な弱点を持っています。太陽光線（や光源）は彼らの破滅を意味します。幸運にも、吸血鬼はその身体から「暗黒の光」のオーラを放出しています。一方、暗闇は彼らをより強力にするものです。",
        "One of the mightier undead creatures, the Vampire is an awe-inspiring sight.  Yet this dread creature has a serious weakness: the bright rays of sun are its bane, and it will need to flee the surface to the deep recesses of earth until the sun finally sets.  Darkness, on the other hand, only makes the Vampire stronger.  As undead, the Vampire has a firm hold on its experience, and resists nether attacks.  The Vampire also resists cold and poison based attacks.  It is, however, susceptible to its perpetual hunger for fresh blood, which can only be satiated by sucking the blood from a nearby monster."),

    _("幽霊は強力なアンデッドの一種です。彼らは不気味な緑色の光に包まれています。半物質的な存在である彼らは、壁を通り抜けることができますが、そのときには壁の密度によって傷つけられてしまいます。他のアンデッド同様、彼らも生命力を吸収されることがなく、見えないものを見ることができ、毒と冷気に対して耐性を備え、さらに地獄に対する耐性も持っています。レベルが十分に上がると彼らはテレパシーを獲得します。幽霊は卓越した魔法使いになることができますが、その身体的特性は非常に貧弱です。彼らは食物からほとんど栄養を摂取できませんが、代わりに魔法棒や杖から魔力を吸収してエネルギー源にする事ができます。",
        "Another powerful undead creature: the Spectre is a ghastly apparition, surrounded by an unearthly green glow.  They exist only partially on our plane of existence: half-corporeal, they can pass through walls, although the density of the wall will hurt them in the process of doing this.  As undead, they have a firm hold on their experience, see invisible, and resist poison and cold.  They also resist nether.  At higher levels they develop telepathic abilities.  Spectres make superb spellcasters, but their physical form is very weak.  They gain very little nutrition from the food of mortals, but can absorb mana from staffs and wands as their energy source."),

    _("妖精は非常に小さいです。彼らは小さな翼を持ち、罠や危険な地形を飛び越えることができます。彼らは日光を大変好み、光に対する耐性を持っています。身体的にはもっとも貧弱な種族の一つですが、妖精は魔法の面で大変な才能を持っており、非常に熟練した魔法使いになることができます。高レベルではより速く飛ぶことができるようになります。",
        "One of several fairy races, Sprites are very small.  They have tiny wings and can fly over traps that may open up beneath them.  They enjoy sunlight intensely, and need worry little about light based attacks.  Although physically among the weakest races, Sprites are very talented in magic, and can become highly skilled wizards.  Sprites have the special power of spraying Sleeping Dust, and at higher levels they learn to fly faster."),

    _("この種族はカオスによってつくられた冒涜的で嫌悪される存在です。彼らは独立した種族ではなく、人間型種族、大抵は人間がカオスによって歪められた存在、もしくは人間と獣の悪夢のような交配種です。全ての獣人はカオスに盲従しており、そのため混乱と音に対して耐性を備えていますが、純粋なログルスはまだ彼らに対し効果を持っています。獣人は混沌を好み、それは彼らをさらに歪めます。獣人は突然変異を起こしやすい性質を持っています。彼らがつくられたとき、ランダムな変異を受けます。その後、レベルが上がるごとに違う変異を受ける可能性があります。",
        "This race is a blasphemous abomination produced by Chaos.  It is not an independent race but rather a humanoid creature, most often a human, twisted by the Chaos, or a nightmarish crossbreed of a human and a beast.  All Beastmen are accustomed to Chaos so much that they are untroubled by confusion and sound, although raw logrus can still have effects on them.  Beastmen revel in chaos, as it twists them more and more.  Beastmen are subject to mutations: when they have been created, they receive a random mutation.  After that, every time they advance a level they have a small chance of gaining yet another mutation."),

    _("エントは非常に強く、賢いですが、その巨大さゆえに罠の解除やこっそりと歩くことは苦手です。成長するにつれて腕力や耐久力が上がりますが、器用さは下がっていきます。彼らには大きな欠点があり、炎によって通常よりも大きなダメージを受けてしまいます。彼らは食物からほとんど栄養を摂取できませんが、代わりに薬等から水分を摂取する事で活動できます。",
        "The Ents are a powerful race dating from the beginning of the world, oldest of all animals or plants who inhabit Arda.  Spirits of the land, they were summoned to guard the forests of Middle-earth.  Being much like trees they are very clumsy but strong, and very susceptible to fire.  They gain very little nutrition from the food of mortals, but they can absorb water from potions as their nutrition."),

    _("天使の上位種であるアルコンは、全てのスキルに熟達しており、強くて賢く、非常に人気があります。彼らは目に見えないものを見ることができ、その翼で罠や危険な地形を飛び越えることができます。しかし、非常に成長が遅いという欠点もあります。",
        "Archons are a higher class of angels.  They are good at all skills, and are strong, wise, and are a favorite with any people.  They are able to see the unseen, and their wings allow them to safely fly over traps and other dangerous places.  However, belonging to a higher plane as they do, the experiences of this world do not leave a strong impression on them and they gain levels slowly."),

    _("悪魔の上位種であるバルログは、強く、知的で、またタフでもあります。しかし、彼らは神を信じようとはせず、プリーストには全く向いていません。炎と地獄、経験値吸収への耐性を持っており、レベルが上がれば見えないものを見る能力を獲得します。また、地獄や火炎のブレスを吐くこともできます。彼等はほとんどの技能で優れていますが、静かに歩くことは苦手です。彼らは食物からほとんど栄養を摂取できませんが、人間タイプを生贄にする事で精力を回復する事ができます。",
        "Balrogs are a higher class of demons.  They are strong, intelligent and tough.  They do not believe in gods, and are not suitable for priest at all.  Balrogs are resistant to fire and nether, and have a firm hold on their experience.  They also eventually learn to see invisible things.  They are good at almost all skills except stealth.  They gain very little nutrition from the food of mortals, and need human corpses as sacrifices to regain their vitality."),

    _("ドゥナダンは西方から来た屈強な種族です。このいにしえの種族は全ての領域において人間の能力を凌駕し、特に耐久力に関してはそれが顕著です。しかしながらこの種族は全てに卓越していることが災いして、この世界には新しい経験といったものがほとんどなく、レベルを上げることが非常に困難です。彼らはとてもタフで頑強であり、彼らの耐久力が下がることはありません。",
        "Dunedain are a race of hardy men from the West.  This elder race surpasses human abilities in every field, especially constitution.  However, being men of the world, very little is new to them, and levels are very hard for them to gain.  Their constitution cannot be reduced."),

    _("影フェアリーは人間よりやや大きい妖精族で、翼を持ち、罠や危険な地形を飛び越えることができます。しかし、彼らは日光を嫌い、閃光によって通常よりも大きなダメージを受けてしまいます。肉体的には非常に貧弱ですが、魔法の面では優れた能力を持っています。彼らにはすばらしい長所が一つあり、モンスターの反感をかうような強力なアイテムを装備してもモンスターを怒らせることがありません。ただしその場合でも隠密行動能力が下がり、また、自分自身の性格によって反感をかっている場合には効果がありません。",
        "Shadow Fairies are one of several fairy races.  They have wings, and can fly over traps that may open up beneath them.  Shadow Fairies must beware of sunlight, as they are vulnerable to bright light.  They are physically weak, but have advantages in using magic and are amazingly stealthy.  Shadow Fairies have a wonderful advantage in that they never aggravate monsters (If their equipment normally aggravates monsters, they only suffer a penalty to stealth, but if they aggravate by their personality itself, the advantage will be lost)."),

    _("クターとしている無表情の謎の生物です。彼らは外見がかわいらしいため、魅力が高いです。彼らは混乱しません。なぜなら、混乱してもクターとしているため変わりないからです。しかも、そのクターとしている外見から敵に見つかりにくいです。しかし、彼らは注意力が少ないため探索や知覚能力は悪いです。彼らはレベルが上がると横に伸びてACを上げる技を覚えますが、伸びている間は魔法防御能力は低くなってしまいます。",
        "A Kutar is an expressionless animal-like living creature.  The word 'kuta' means 'absentmindedly' or 'vacantly'.  Their absentmindedness hurts their searching and perception skills, but renders them incapable of being confused.  Their unearthly calmness and serenity make them among the most stealthy of any race.  Kutars, although expressionless, are beautiful and so have a high charisma.  Members of this race can learn to expand their body horizontally.  This increases armour class, but renders them vulnerable to magical attacks."),

    _("アンドロイドは機械の身体を持つ人工的な存在です。魔法をうまく使うことはできませんが、戦士としては非常に優れています。彼らは他の種族のように経験値を得て成長するということはありません。身体に身につける装備によって成長します。ただし、指輪、アミュレット、光源は成長に影響しません。彼らは毒の耐性を持ち、麻痺知らずで、生命力を吸収されることがありません。また、身体が頑丈なのでACにボーナスを得ます。しかし身体のいたるところに電子回路が組み込まれているため、電撃によって通常よりも大きなダメージを受けてしまいます。彼らは食物からほとんど動力を得られませんが、油を補給する事で動力源を得る事ができます。",
        "An android is a artificial creation with a body of machinery.  They are poor at spell casting, but they make excellent warriors.  They don't acquire experience like other races, but rather gain in power as they attach new equipment to their frame.  Rings, amulets, and lights do not influence growth.  Androids are resistant to poison, can move freely, and are immune to exp-draining attacks.  Moreover, because of their hard metallic bodies, they get a bonus to AC.  Androids have electronic circuits throughout their body and must beware of electric shocks.  They gain very little nutrition from the food of mortals, but they can use flasks of oil as their energy source."),

    _("マーフォークは人型生物と水棲生物の合いの子たちの総称です。彼らは知的であり、陸上よりも水中での営みに適応している点で共通しています。彼らの真価は水中でこそ発揮されますが、彼らの文明は大抵陸上に長く滞在する何らかの術も有しています。それを失わない限り、陸上でも不自由はないでしょう。また、彼らは致命的な水難にも抗える能力を生まれつき持っています。",
        "Merfolk is a general term for children of humanoids and aquatic creatures. They have in common that they are intelligent and adapted to working underwater rather than on land. Their true value comes in water, but their civilizations also have some form of staying longer on land. As long as you do not lose it, there will be no inconvenience on land. They also have the ability to withstand disaster by water."),
};

/*! 職業の解説メッセージテーブル */
const std::vector<std::string_view> class_explanations = {
    _("戦士は、直面する問題のほとんどを細切れに叩き切ることで解決するキャラクタです。が、時折退却して魔法の道具の世話になることもあります。不運にも、高レベルなアイテムの多くは彼らが扱える範囲を越えています。",
        "A Warrior is a hack-and-slash character, who solves most of his or her problems by cutting them to pieces, but will occasionally fall back on the help of a magical device.  Unfortunately, many high-level devices may be forever beyond their use."),

    _("メイジは魔法使いであり、その機知によって生き延びなければなりません。戦士のように、単純に切りまくることで道を開くことは望めません。呪文書に加えて、メイジは助けになる魔法の道具を持ち運ぶべきです。これは他の何よりも遥かに簡単にマスターできます。魔法に必要な能力値は知能です。",
        "A Mage is a spell caster that must live by his or her wits as a mage cannot hope to simply hack his or her way through the dungeon like a warrior.  In addition to spellbooks, a mage should carry a range of magical devices to help his or her endeavors.  A mage can master those devices far more easily than anyone else.  A mage's prime statistic is Intelligence as this determines his or her spell casting ability."),

    _("プリーストは高貴な力を使うことに専念したキャラクタです。彼らは自身の神のためにダンジョンを探索し、もし宝を手にいれたなら、それは彼が信仰する宗教の栄光となります。プリーストは新しい祈りを神からの贈り物という形で受け取るため、どれを学ぶのか自分で選ぶことはできません。プリーストは魔法の道具の使い方をよく知っていますが、メイジほどうまくは使えません。刃のついた武器より鈍器を好み、祝福されていない刃のついた武器を装備すると不愉快な感覚に襲われ、戦闘能力が落ちてしまいます。魔法に必要な能力値は賢さです。",
        "A Priest is a character devoted to serving a higher power.  He or she explores the dungeon in the service of a God.  Since Priests receive new prayers as gifts from their patron deity, they cannot choose which ones they will learn.  Priests are familiar with magical devices which they believe act as foci for divine intervention in the natural order of things.  A priest wielding an edged weapon will be so uncomfortable with it that his or her fighting ability decreases.  A Priest's primary stat is Wisdom since this determines the success of the prayers to his or her deity."),

    _("盗賊はその狡猾さで生き抜くことを好むキャラクタですが、肝心なときには戦闘で道を切り開くことができます、盗賊は罠やドアを見つける能力に優れ、罠の解除や鍵開けに熟達しています。盗賊は高い隠密行動を持ち、たくさんのモンスターの群れのそばを起こすことなく通り抜けたり、忍び寄って先制攻撃することができます。魔法に必要な能力値は知能です。",
        "A Rogue is a character that prefers to live by his or her cunning, but is capable of fighting out of a tight spot.  Rogues are good at locating hidden traps and doors and are the masters of disarming traps and picking locks.  A rogue is very stealthy, allowing him or her to sneak around many creatures without having to fight or to get in a telling first blow.  A rogue may also backstab a fleeing monster.  Intelligence determines a Rogue's spell casting ability."),

    _("レンジャーは戦士とメイジを合わせたような職業で、身の回りの自然と特別な関係を作り上げています。彼はより戦士であり、弓のような遠距離武器を巧く使える職業です。レンジャーはよい隠密行動、よい知覚、よい探索、よい魔法防御を持ち、魔法の道具の使用にも長けています。魔法に必要な能力値は知能です。",
        "A Ranger is a combination of a warrior and a mage who has developed a special affinity for the natural world.  He or she is a good fighter and also good with missile weapons such as bows.  A ranger has good stealth, good perception, good searching, good saving throws and is good with magical devices.  Intelligence determines a Ranger's spell casting ability."),

    _("パラディンは戦士とプリーストを合わせた職業です。パラディンはとてもよい戦士ですが、遠距離武器を扱うのは得意ではありません。パラディンには多くの能力が欠けています。隠密行動, 知覚, 探索, そして魔法道具使用が苦手ですが、その神との提携によって魔法防御はそこそこです。魔法に必要な能力値は賢さです。",
        "A Paladin is a combination of a warrior and a priest.  Paladins are very good fighters, but not very good at missile weapons.  A paladin lacks much in the way of abilities.  He or she is poor at stealth, perception, searching, and magical devices but has a decent saving throw due to his or her divine alliance.  Wisdom determines a Paladin's success at praying to his or her deity."),

    _("魔法戦士はその名称が意味する通りの職業であり、戦士とメイジの資質をあわせ持ちます。彼らの同業者であるレンジャーが自然の魔法と生き抜くためのスキルに特化している一方、本当の魔法剣士はどちらの世界でも一番になろうとしています。戦士としては普通のメイジとは比べ物にならないほど優れています。しかし、実際には魔法でも戦闘でも専門の職業には及ばず、戦士とメイジの中間に位置するような職業です。魔法に必要な能力値は知能です。",
        "A Warrior-Mage is precisely what the name suggests: a cross between the warrior and mage classes.  Unlike rangers who specialize in Nature magic and survival skills, true Warrior-Mages attempt to reach the best of both worlds.  As warriors they are much superior to the usual Mage class.  Intelligence determines a Warrior-Mage's spell casting ability."),

    _("混沌の戦士は恐るべきカオスの魔王の使いとして恐れられる存在です。混沌の戦士はパトロンとなる悪魔を持ち、レベルが上がる度に報酬を得ることがあります。彼は治療してくれたり、こちらを変化させたり、能力値を上げてくれるかもしれませんし、回りに怪物達を出現させたり、能力値や装備を奪うかも知れません。もしくは単にこちらを無視するだけかもしれません。カオスの魔王は無秩序で予測のつかない存在です。報酬の種類はパトロンとなる悪魔と偶然に依存します（違う悪魔は異なる報酬を与えます）。魔法に必要な能力値は知能です。",
        "Chaos Warriors are the feared servants of the terrible Demon Lords of Chaos.  Every Chaos Warrior has a Patron Demon and, when gaining a level, may receive a reward from his or her Patron.  He or she might be healed or polymorphed, given an awesome weapon, or have his or her stats increased.  On the other hand, the Patron might surround the Chaos Warrior with monsters, drain his or her stats, wreck his or her equipment, or simply ignore the Chaos Warrior.  The Demon Lords of Chaos are chaotic and unpredictable indeed.  The exact type of reward depends on both the Patron Demon (different Demons give different rewards) and chance."),

    _("修行僧は他の職業とは著しく異なる職業です。彼らは他の職業同様武器と防具を使えますが、マーシャルアーツの訓練を積んでいるため、武器、防具なしでより強力な存在となります。高レベルでは、必要な耐性を身につけるためある種の防具を装備する必要がありますが、もしあまりに重すぎる防具を装備してしまうと、その体術に深刻な妨げとなります。レベルが上がると、彼らは新しい強力な攻撃法を学び、防御能力も上昇します。魔法に必要な能力値は賢さです。",
        "The Monk character class is very different from all other classes.  Their training in martial arts makes them much more powerful with no armor or weapons.  To gain the resistances necessary for survival a monk may need to wear some kind of armor, but if the armor he or she wears is too heavy, it will severely disturb his or her martial arts maneuvers.  As the monk advances levels, new, powerful forms of attack become available.  Their defensive capabilities increase likewise, but if armour is being worn, this effect decreases.  Wisdom determines a Monk's spell casting ability."),

    _("超能力者は魔法のかわりにその精神の力を使う唯一の職業です。この力は超能力者独特のもので、単に超感覚的なものから他人の精神を支配するものまで様々です。彼らの力はある種の訓練によって開発されるものなので、超能力者は力を使うのに呪文書を必要としません。使える力は単純にキャラクタのレベルによって決まります。超能力に必要な能力値は賢さです。",
        "The Mindcrafter is a unique class that uses the powers of the mind instead of magic.  These powers are unique to Mindcrafters, and vary from simple extrasensory powers to mental domination of others.  Since these powers are developed by the practice of certain disciplines, a Mindcrafter requires no spellbooks to use them.  The available powers are simply determined by the character's level.  Wisdom determines a Mindcrafter's ability to use mind powers."),

    _("ハイメイジは一つの領域に特化し、その領域を通常のメイジよりはるかに深く学んだメイジです。１つの領域に特化したおかげで、彼らは自らが選択した領域の呪文を唱える際の消費ＭＰ、最低レベル、失敗率で相当な恩恵を受けます。しかし、生命の領域ではプリーストほどうまくはなれないことには注意すべきです。魔法に必要な能力値は知能です。",
        "High-mages are mages who specialize in one particular field of magic and learn it very well - much better than the ordinary mage.  For the price of giving up a second realm of magic, they gain substantial benefits in the mana costs, minimum levels, and failure rates in the spells of the realm of their specialty.  A high mage's prime statistic is intelligence as this determines his or her spell casting ability."),

    _("観光客は観光のためにこの世界にやってきました。戦闘力が低く、強力な呪文を使うこともできないため、最も生きぬいていくのが厳しい職業と言えます。魔法に必要な能力値は知能です。",
        "Tourists visit this world for the purpose of sightseeing.  Their fighting skills are bad, and they cannot cast powerful spells.  They are the most difficult class to win the game with.  Intelligence determines a tourist's spell casting ability."),

    _("ものまね師は戦闘力はそこそこありますが、自分から特殊な能力を使うことは全くできません。しかし、自分の目の前にいる相手が特殊能力を使った場合、その能力と全く同じ能力をそっくりそのまま使うことができます。ものまねに必要な能力は基本的に器用さですが、まねる特殊能力に関係ある他の能力も必要です。",
        "Imitators have enough fighting skills to survive, but rely on their ability to imitate monster spells.  When monsters in line of sight use spells, they are added to a temporary spell list which the imitator can choose among.  Spells should be imitated quickly, because timing and situation are everything.  An imitator can only repeat a spell once each time he or she observes it.  Dexterity determines general imitation ability, but a stat related to the specific action is often also taken into account."),

    _("魔獣使いは変愚蛮怒世界のダンジョンに住む生物と心を通い合わせられます。彼らは最もうまくモンスターを乗りこなすことができ、召喚したり手なづけたりしたモンスターを自分の手足のように使います。魔法に必要な能力は魅力です。",
        "Beastmasters are in tune with the minds of the creatures of the world of Hengband.  They are very good at riding and have enough fighting ability.  The monsters that a beastmaster summons or dominates become the beastmaster's hands and feet.  Beastmasters can cast trump magic and are very good at summoning spells, but they can not summon non-living creatures.  Charisma determines a Beastmaster's spell casting ability."),

    _("スペルマスターは全ての魔法を極める者です。彼らは全分野において非常に優れた魔法使いであり、あらゆる魔法書のすべての呪文を学習の手間なく使いこなすことができます。その反面、彼らは戦士としては最低で、どんな武器も満足に扱えません。魔術師の杖だけは例外ですが、武器としては使い物にならないでしょう。すべての魔法をうまく生かさなければならないため、非常に上級者向けな職業と言えます。魔法に必要な能力は知能です。",
        "Sorcerers are the all-around best magicians, being able to cast any spell from most magic realms without having to learn it.  On the downside, they are the worst fighters in the dungeon, being unable to use any weapon but a Wizardstaff."),

    _("アーチャーは魔法を使うことはできませんが、どんな職業よりも巧みに弓やスリングを使いこなします。大量の矢や弾を必要とするのは確かですが、岩石からスリング用の弾を作ったり、レベルが上がるとモンスターの骨やがらくたから矢を作ったりする技術を身につけます。また、戦士と比べて隠密行動、知覚、探索、魔法道具の使用などにも優れており、いざというときには魔法の道具に頼ることもできます。",
        "Archers are to bows what warriors are to melee.  They are the best class around with any bow, crossbow, or sling.  They need a lot of ammunition, but will learn how to make it from junk found in the dungeon.  An archer is better than a warrior at stealth, perception, searching and magical devices."),

    _("魔道具術師は杖、魔法棒、ロッドといった魔法のアイテムから魔力を取り込むことによって魔法を使います。魔法のアイテムを発見することが他の職業よりもはるかに重要になります。戦闘力は高くはないですが、そこそこの強さがあります。魔法に必要な能力は知能です。",
        "Magic-Eaters can absorb the energy of wands, staffs, and rods, and can then use these magics as if they were carrying all of these absorbed devices.  They are middling-poor at fighting.  A Magic-Eater's prime statistic is intelligence."),

    _("吟遊詩人は魔力を帯びた歌を歌うことができます。多くの歌は普通の魔法と異なり、歌を歌っている間継続して効果を発揮します。しかし、同時に2つの歌を歌うことができない、という欠点もあります。視界内全体に影響を及ぼす歌が多い、という特徴もあります。肉体的な能力は貧弱で、単純に切りまくることで道を開くことはできません。魔法に必要な能力は魅力です。",
        "Bards are something like traditional musicians.  Their magical attacks are sound-based, and last as long as the Bard has mana.  Although a bard cannot sing two or more songs at the same time, he or she does have the advantage that many songs affect all areas in sight.  A bard's prime statistic is charisma."),

    _("赤魔道師は全ての下級魔法を使うことができ、戦闘力も十分にあります。レベルが上がると強力な能力「連続魔」を身につけることができます。しかし、魔法を覚えるスピードは遅く、上級魔法を使えないので、メイジほどには魔法を頼りにすることができません。魔法道具使用と魔法防御はそこそこですが、それ以外の技能は苦手です。魔法に必要な能力は知能です。",
        "Red-Mages can use almost all spells from lower rank spellbooks of most realms without having to learn it.  At higher level, they develop the powerful ability \"Double Magic\".  However, they have large penalties in the mana costs, minimum levels, and failure rates of spells, and they cannot use any spells from higher rank spellbooks.  They are not bad at using magical devices and magic resistance, and are decent fighters, but are bad at other skills.  A red-mage's prime statistic is intelligence."),

    _("剣術家は戦士に次ぐ戦闘力があり、様々な技を使うことができます。彼らのMPはレベルに依存せず、賢さだけで決まり、気合いをためることにより、最大値を越えてMPを増やすことができます。しかし、戦士と同様、高レベルの魔法のアイテムは彼らの扱える範囲を越えており、罠の解除や探索の能力も高いとはいえません。必殺技の使用に必要な能力は賢さです。",
        "Samurai, masters of the art of the blade, are the next strongest fighters after Warriors.  Their spellpoints do not depend on level, but depend solely on wisdom, and they can use the technique Concentration to temporarily increase SP beyond its usual maximum value.  Samurai are not good at most other skills, and many magical devices may be too difficult for them to use.  Wisdom determines a Samurai's ability to use the special combat techniques available to him or her."),

    _("練気術師は「気」を使う達人です。修行僧と同様、武器や防具を持たずに戦うことを好み、武器・防具なしでより強力な存在となります。修行僧ほどの戦闘能力はありませんが、修行僧と同様の魔法が使え、さらに「気」の力を操ります。武器を持つことや、重すぎる防具を装備することは、「気」の力の使用を妨げます。魔法と練気術に必要な能力は賢さです。",
        "A ForceTrainer is a master of the spiritual Force.  They prefer fighting with neither weapons nor armor.  They are not as good fighters as are Monks, but they can use both magic and the spiritual Force.  Wielding weapons or wearing heavy armor disturbs use of the Force.  Wisdom is a ForceTrainer's primary stat."),

    _("青魔道師は優れた魔法使いであり、その機知によって生き延びなければなりません。メイジ等の他の魔法使いとの違いは魔法の覚え方で、青魔道師はモンスターの魔法の効果を受けることでその魔法を覚えます。覚えるためには「ラーニング」の状態になっていないといけません。魔法に必要な能力は知能です。",
        "A Blue-Mage is a spell caster that must live by his or her wits, as a Blue-Mage cannot hope to simply hack his or her way through the dungeon like a warrior.  A major difference between the Mage and the Blue-Mage is the method of learning spells: a Blue-Mage may learn spells from monsters by activating his or her Learning ability.  A Blue-Mage's prime statistic is Intelligence as this determines his or her spell casting ability."),

    _("騎兵は馬に乗り戦場を駆け抜けるエリート戦士です。魔法は使えませんが、馬上からの圧倒的な攻撃力を誇る上に、高い機動力を生かした射撃をも得意としています。レベルが上がれば、野生のモンスターにまたがり無理矢理手なずけることができます。彼らは己の肉体と精神に誇りを持ち、魔法道具にはあまり頼ろうとはしません。",
        "Cavalry ride on horses into battle.  Although they cannot cast spells, they are proud of their overwhelming offensive strength on horseback.  They are good at shooting.  At high levels, they learn to forcibly saddle and tame wild monsters.  Since they take pride in the body and the soul, they don't use magical devices well."),

    _("狂戦士は怒り狂って武器を振るう恐るべき戦士です。全職業中最高の肉体能力を誇り、恐怖と麻痺に対する耐性を持ち、レベルが上がればその強靭な肉体で矢の呪文を跳ね返すことができます。さらに武器なしで戦うことや、呪いのかけられた装備を力づくで剥がすことができ、いくつかの技を(反魔法状態でも)使うことができます。しかし、巻物や魔法道具は全く使うことができず、罠の解除や隠密行動、探索、魔法防御、飛び道具の技能に関しては絶望的です。ひたすら殴って道を開くしかありません。幽霊は非常に勝利しやすいですがスコアがかなり低く修正されます。",
        "A Berserker is a fearful fighter indeed, immune to fear and paralysis.  At high levels, Berserkers can reflect bolt spells with their tough flesh.  Furthermore, they can fight without weapons, can remove cursed equipment by force, and can even use their special combat techniques when surrounded by an anti-magic barrier.  Berserkers, however, cannot use any magical devices or read any scrolls, and are hopeless at all non-combat skills.  Since Berserker Spectres are quite easy to *win* with, their scores are lowered."),

    _("鍛冶師は武器や防具を自分で強化することができます。特殊効果を持つ武器や防具から特殊効果の元となるエッセンスを取り出し、別の武器や防具にエッセンスを付加することによってその特殊効果を付加できます。ある程度の戦闘能力も持ちますが、魔法は一切使用できず、隠密や魔法防御の技能も低くなります。",
        "A Weaponsmith can improve weapons and armor for him or herself.  They can extract the essences of special effects from weapons or armor which have various special abilities, and can add these essences to another weapon or armor.  They are good at fighting, but cannot cast spells, and are poor at skills such as stealth or magic defense."),

    _("鏡使いは、魔力の込められた鏡を作り出して、それを触媒として攻撃を行なうことができる鏡魔法を使います。鏡使いは鏡の上で実力を発揮し、鏡の上では素早いテレポートが可能となります。魔法の鏡は、レベルによって一度に制御できる数が制限されます。鏡魔法に必要な能力は知能です。",
        "Mirror-Masters are spell casters; like other mages, they must live by their wits.  They can create magical mirrors, and employ them in the casting of Mirror-Magic spells.  A Mirror-Master standing on a mirror has greater ability and, for example, can perform quick teleports.  The maximum number of Magical Mirrors which can be controlled simultaneously depends on the level.  Intelligence determines a Mirror-Master's spell casting ability."),

    _("忍者は暗闇に潜む恐るべき暗殺者であり、光源を持たずに行動し、相手の不意をつき一撃で息の根を止めます。また、相手を惑わすための忍術も身につけます。罠やドアを見つける能力に優れ、罠の解除や鍵開けに熟達しています。軽装を好み、重い鎧や武器を装備すると著しく動きが制限され、また、盾を装備しようとはしません。軽装ならば、レベルが上がるにつれより速くより静かに行動できます。さらに忍者は恐怖せず、成長すれば毒がほとんど効かなくなり、透明なものを見ることができるようになります。忍術に必要な能力は器用さです。",
        "A Ninja is a fearful assassin lurking in darkness.  He or she can navigate effectively with no light source, catch enemies unawares, and kill with a single blow.  Ninjas can use Ninjutsu, and are good at locating hidden traps and doors, disarming traps and picking locks.  Since heavy armor, heavy weapons, or shields will restrict their motion greatly, they prefer light clothes, and become faster and more stealthy as they gain levels.  A Ninja knows no fear and, at high level, becomes almost immune to poison and able to see invisible things.  Dexterity determines a Ninja's ability to use Ninjutsu."),

    _("スナイパーは一撃必殺を狙う恐るべき射手です。精神を高めることにより、射撃の威力と精度を高めます。また、魔法を使うことはできませんが、研ぎ澄まされた精神から繰り出される射撃術はさらなる威力をもたらすことでしょう。テクニックが必要とされる職業です。",
        "Snipers are good at shooting, and they can kill targets by a few shots. After they concentrate deeply, they can demonstrate their shooting talents. You can see the incredible firepower of their shots."),

    _("元素使いはエレメンタルの力を駆使して戦う魔法使いです。扱える属性は限られますが、その力を十二分に引き出すことができます。魔法に必要な能力は賢さです。",
        "An Elementalist is a spell caster that specializes in tapping elemental forces.  They have a limited, but powerful, repertoire of spells.  Wisdom determines an Elementalist's spell casting ability.")
};

/*! 性格の解説メッセージテーブル */
const std::vector<std::string_view> personality_explanations = {
    _("ふつうは、特に特筆するべき部分がない性格です。あらゆる技能を平均的にこなします。",
        "\"Ordinary\" is a personality with no special skills or talents, with unmodified stats and skills."),

    _("ちからじまんは、肉体的な能力や技能が上昇します。しかし、魔法に関係する能力や技能は劣り、戦士よりのステータスを持ちます。",
        "\"Mighty\" raises your physical stats and skills, but reduces stats and skills which influence magic.  It makes your stats suitable for a warrior.  Also it directly influences your hit-points and spell fail rate."),

    _("きれものは、肉体的な能力は下がりますが、知能や魔法に関係する技能は上昇し、メイジよりのステータスを持ちます。",
        "\"Shrewd\" reduces your physical stats, and raises your intelligence and magical skills.  It makes your stats suitable for a mage.  Also it directly influences your hit-points and spell fail rate."),

    _("しあわせものは、神を信仰する能力が高くなります。肉体的には平均的な能力を持ち、プリーストに近いステータスとなります。",
        "\"Pious\" deepens your faith in your God.  It makes your physical ability average, and your stats suitable for priest."),

    _("すばしっこいは、どのスキルも比較的うまくこなしますが、肉体的な能力は低くなります。",
        "\"Nimble\" improves most skills except for melee combat."),

    _("いのちしらずは、戦闘力、魔法能力の両方が上昇しますが、魔法防御、ＨＰといった能力は悪くなります。",
        "\"Fearless\" raises both your melee and magical ability.  Stats such as magic defense and constitution are reduced.  Also it has a direct bad influence on your hit-points."),

    _("好きな食べ物は焼きビーフン。抑えてはいるが、冒険心旺盛な一匹狼。正義感、勇気とも平均以上だがカッとしやすい所もある。計画的人生より行き当たりばったりの人生を選んでしまうタイプで、異性の扱いは苦手。",
        "\"Combat\" gives you comparatively high melee and shooting abilities, and average constitution.  Other skills such as stealth, magic defence, and magical devices are weakened.  All \"Combat\" people have great respect for the legendary \"Combat Echizen\".\n\
(See \"Death Crimson\" / Ecole Software Corp.)"),

    _("なまけものは、あらゆるスキルが低く、何をやってもうまくいきません。",
        "A \"Lazy\" person has no good stats and can do no action well.  Also it has a direct bad influence on your spell fail rate."),

    _("セクシーギャルは、あらゆるスキルをうまくこなすことができます。しかし、その人をなめた性格は全てのモンスターを怒らせることになるでしょう。この性格は女性しか選ぶことができません。",
        "\"Sexy\" rises all of your abilities, but your haughty attitude will aggravate all monsters.  Only females can choose this personality."),

    _("ラッキーマンは、能力値はなまけものに匹敵するくらい低いにもかかわらず、どんなことをしてもなぜかうまくいってしまいます。この性格は男性しか選ぶことができません。",
        "A \"Lucky\" man has poor stats, equivalent to a \"Lazy\" person.  Mysteriously, however, he can do all things well.  Only males can choose this personality."),

    _("がまんづよいは、じっくりと物事にとりくむ慎重な性格で、他の性格に比べて高い耐久力を得ることができます。しかし、自分から行動するのは苦手で、多くの技能は低くなってしまいます。",
        "A \"Patient\" person does things carefully.  Patient people have high constitution, and high resilience, but poor abilities in most other skills.  Also it directly influences your hit-points."),

    _("いかさまは、初心者の練習用の性格です。あらゆる能力が高くなっています。この性格を使えば勝利者になることは容易ですが、勝利しても全く自慢になりません。",
        "\"Munchkin\" is a personality for beginners.  It raises all your stats and skills.  With this personality, you can win the game easily, but gain little honor in doing so."),

    _("チャージマンは「こんなところ」に連れて行かれても仕方のない可愛そうなお友達なんＤＡ。腕っ節やタフさはマンモス並みに強いのだけれど知能面はまるで駄目なのが分かるだろう？この性格は最初から気が狂っているので、混乱したり幻覚を見る心配がないのです。",
        "\"ChargeMan\" is a crazy killer.  You are strong and tough but have poor intelligence.  Since you're already insane, confusion and hallucinations do not affect you."),
};

/*! 魔法領域の詳細解説メッセージテーブル */
const std::vector<std::string_view> realm_explanations = {
    _("生命は回復能力に優れた魔法です。治療や防御、感知魔法が多く含まれていますが、攻撃呪文もわずかに持っています。特に高レベルの呪文にはアンデッドを塵に帰す力をあると言われています。",
        "Life magic is very good for healing; it relies mostly on healing, protection and detection spells.  Also life magic has a few attack spells as well.  It's said that some high level spells of life magic can disintegrate Undead monsters into ash."),

    _("仙術は「meta」領域であり、感知や鑑定、さらに退却用の呪文や自身の能力を高める呪文などの便利な呪文が含まれています。しかし、直接攻撃用の呪文は持っていません。",
        "Sorcery is a `meta` realm, including enchantment and general spells.  It provides superb protection spells, spells to enhance your odds in combat and, most importantly, a vast selection of spells for gathering information.  However, Sorcery has one weakness: it has no spells to deal direct damage to your enemies."),

    _("自然の魔法は使用者を元素のマスターにします。これには防御、探知、治療と攻撃呪文が含まれています。また、生命以外の領域で最高の治療呪文もこの領域にあります。",
        "Nature magic makes you a master of the elements; it provides protection, detection, curing and attack spells.  Nature also has a spell of Herbal Healing, which is the only powerful healing spell outside the realm of Life magic."),

    _("カオスの魔法は制御が困難で、予測のできない魔法もあります。カオスは非常に非元素的であり、カオスの呪文は想像できる最も恐るべき破壊兵器です。この呪文を唱えるものはカオスの尖兵に対し、敵や自分自身さえも変異させるよう要求します。",
        "There are few types of magic more unpredictable and difficult to control than Chaos magic.  Chaos is the very element of unmaking, and the Chaos spells are the most terrible weapons of destruction imaginable.  The caster can also call on the primal forces of Chaos to induce mutations in his/her opponents and even him/herself."),

    _("黒魔術である暗黒の魔法ほど邪悪なカテゴリーはありません。これらの呪文は比較的学ぶのが困難ですが、高レベルになると術者に生物とアンデッドを自由に操る能力を与えます。残念なことに、もっとも強力な呪文はその触媒として術者自身の血を必要とし、詠唱中にしばしば術者を傷つけます。",
        "There is no fouler nor more evil category of spells than the necromantic spells of Death Magic.  These spells are relatively hard to learn, but at higher levels the spells give the caster power over living and the (un)dead, but the most powerful spells need his / her own blood as the focus, often hurting the caster in the process of casting."),

    _("トランプの魔法はテレポート系の呪文で精選されたものを持っており、その出入り口は他の生物を召喚するためにも使えるため、召喚呪文から選りすぐられたものも同様に持っています。しかし、この魔法によって全ての怪物が別の場所へ呼ばれるのを理解するわけではなく、もし召喚呪文に失敗するとその生物は敵となります。",
        "Trump magic has, indeed, an admirable selection of teleportation spells.  Since the Trump gateways can also be used to summon other creatures, Trump magic has an equally impressive selection of summoning spells.  However, not all monsters appreciate being drawn to another place by Trump user."),

    _("秘術の魔法は、全ての領域から有用な呪文だけを取り入れようとした多用途領域です。必要な「道具」的呪文を持っていても高レベルの強力な呪文は持っていません。結果として、全ての呪文書は街で買い求めることができます。また、他の領域に存在する同様な呪文の方がより低レベル、低コストで唱えることができます。",
        "Arcane magic is a general purpose realm of magic.  It attempts to encompass all 'useful' spells from all realms.  This is the downside of Arcane magic: while Arcane does have all the necessary 'tool' spells for a dungeon delver, it has no ultra-powerful high level spells.  As a consequence, all Arcane spellbooks can be bought in town.  It should also be noted that the 'specialized' realms usually offer the same spell at a lower level and cost."),

    _("匠の魔法は、自分や道具を強化するための魔法が含まれています。魔法によって自分自身の戦闘力を非常に高めることができますが、相手を直接攻撃するような呪文は含まれていません。",
        "Craft magic can strengthen the caster or his or her equipment.  These spells can greatly improve the caster's fighting ability.  Using them against opponents directly is not possible."),

    _("悪魔の魔法は暗黒と同様非常に邪悪なカテゴリーです。様々な攻撃魔法に優れ、また悪魔のごとき知覚能力を得ることができます。高レベルの呪文は悪魔を自在に操り、自分自身の肉体をも悪魔化させることができます。",
        "Demon is a very evil realm, same as Death.  It provides various attack spells and devilish detection spells.  At higher levels, Demon magic provides the ability to dominate demons and to polymorph yourself into a demon."),

    _("破邪は「正義」の魔法です。直接敵を傷つける魔法が多く含まれ、特に邪悪な敵に対する力は恐るべきものがあります。しかし、善良な敵にはあまり効果がありません。",
        "Crusade is a magic of 'Justice'.  It includes damage spells, which are greatly effective against foul and evil monsters, but have poor effects against good monsters."),

    _("歌集は、歌によって効果を発揮する魔法です。魔法と同様、使った時に効果のあるものと、歌い続けることによって持続して効果を発揮するものがあります。後者の場合は、MPの続く限り効果を発揮することができますが、同時に歌える歌は1つだけという制限もあります。",
        "Music magic works through the caster singing songs.  There are two types of songs; one which shows effects instantly and another which shows effects continuously until SP runs out.  The latter type has a limit:  only one song can be sung at a time."),

    _("武芸の書は、様々な戦闘の技について書かれています。この本は技を覚えるときに読む必要がありますが、一度覚えた技は使うのに本を持つ必要はありません。技を使うときには必ず武器を装備していなければいけません。",
        "The books of Kenjutsu describe various combat techniques.  When learning new techniques, you are required to carry the books, but once you memorize them, you don't have to carry them.  When using a technique, wielding a weapon is required."),

    _("呪術は忌むべき領域です。複数の呪いの言葉を歌のように紡ぎながら詠唱します。多くの呪文は詠唱し続けることによって効果が持続されます。呪文には相手の行動を束縛するもの、ダメージを与えるもの、攻撃に対して反撃するものが多くあります。",
        "Hex is an unsavory realm, like the death and demon realms.  Some of the spells can act continuously by stringing together curses like a song.  Spells may obstruct monsters' actions, deal damage to monsters in sight, or return damage to monsters who have damaged the caster."),
};

/*! 魔法領域の簡易解説メッセージテーブル */
const std::vector<std::string_view> realm_subinfo = {
    _("感知と防御と回復に優れています", "Good at detection and healing."),
    _("攻撃はできませんが非常に便利です", "Utility and protective spells."),
    _("感知と防御に優れています", "Good at detection and defence."),
    _("破壊的な攻撃に優れています", "Offensive and destructive."),
    _("生命のある敵への攻撃に優れています", "Ruins living creatures."),
    _("召喚とテレポートに優れています", "Good at summoning and teleportation."),
    _("やや弱いながらも非常に便利です", "Very useful but not as potent."),
    _("直接戦闘の補助に優れています", "Support for melee fighting."),
    _("攻撃と防御の両面に優れています", "Good at both offence and defence."),
    _("邪悪な怪物に対する攻撃に優れています", "Destroys evil creatures."),
    _("様々な魔法効果を持った歌を歌います", "Songs with magical effects."),
    _("打撃攻撃に特殊能力を付加します", "Special abilities for melee."),
    _("敵を邪魔しつつ復讐を狙います", "Good at obstacle and revenge."),
};
