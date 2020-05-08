#include "angband.h"
#include "birth/birth-explanations-table.h"

/*! 種族の解説メッセージテーブル */
concptr race_explanations[MAX_RACES] = {
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
        "The Amberites are a reputedly immortal race, who are endowed with numerous advantages in addition to their longevity.  They are very tough and their constitution cannot be reduced.  Their ability to heal wounds far surpasses that of any other race.  Having seen virtually everything, very little is new to them, and they gain levels much slower than the other races."),

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
        "Balrogs are a higher class of demons.  They are strong, intelligent and tough.  They do not believe in gods, and are not suitable for priest at all.  Balrog are resistant to fire and nether, and have a firm hold on their experience.  They also eventually learn to see invisible things.  They are good at almost all skills except stealth.  They gain very little nutrition from the food of mortals, and need human corpses as sacrifices to regain their vitality."),

    _("ドゥナダンは西方から来た屈強な種族です。このいにしえの種族は全ての領域において人間の能力を凌駕し、特に耐久力に関してはそれが顕著です。しかしながらこの種族は全てに卓越していることが災いして、この世界には新しい経験といったものがほとんどなく、レベルを上げることが非常に困難です。彼らはとてもタフで頑強であり、彼らの耐久力が下がることはありません。",
        "Dunedain are a race of hardy men from the West.  This elder race surpasses human abilities in every field, especially constitution.  However, being men of the world, very little is new to them, and levels are very hard for them to gain.  Their constitution cannot be reduced."),

    _("影フェアリーは人間よりやや大きい妖精族で、翼を持ち、罠や危険な地形を飛び越えることができます。しかし、彼らは日光を嫌い、閃光によって通常よりも大きなダメージを受けてしまいます。肉体的には非常に貧弱ですが、魔法の面では優れた能力を持っています。彼らにはすばらしい長所が一つあり、モンスターの反感をかうような強力なアイテムを装備してもモンスターを怒らせることがありません。ただしその場合でも隠密行動能力が下がり、また、自分自身の性格によって反感をかっている場合には効果がありません。",
        "Shadow Fairies are one of several fairy races.  They have wings, and can fly over traps that may open up beneath them.  Shadow Fairies must beware of sunlight, as they are vulnerable to bright light.  They are physically weak, but have advantages in using magic and are amazingly stealthy.  Shadow Fairies have a wonderful advantage in that they never aggravate monsters (If their equipment normally aggravates monsters, they only suffer a penalty to stealth, but if they aggravate by their personality itself, the advantage will be lost)."),

    _("クターとしている無表情の謎の生物です。彼らは外見がかわいらしいため、魅力が高いです。彼らは混乱しません。なぜなら、混乱してもクターとしているため変わりないからです。しかも、そのクターとしている外見から敵に見つかりにくいです。しかし、彼らは注意力が少ないため探索や知覚能力は悪いです。彼らはレベルが上がると横に伸びてACを上げる技を覚えますが、伸びている間は魔法防御能力は低くなってしまいます。",
        "A Kutar is an expressionless animal-like living creature.  The word 'kuta' means 'absentmindedly' or 'vacantly'.  Their absentmindedness hurts their searching and perception skills, but renders them incapable of being confused.  Their unearthly calmness and serenity make them among the most stealthy of any race.  Kutars, although expressionless, are beautiful and so have a high charisma.  Members of this race can learn to expand their body horizontally.  This increases armour class, but renders them vulnerable to magical attacks."),

    _("アンドロイドは機械の身体を持つ人工的な存在です。魔法をうまく使うことはできませんが、戦士としては非常に優れています。彼らは他の種族のように経験値を得て成長するということはありません。身体に身につける装備によって成長します。ただし、指輪、アミュレット、光源は成長に影響しません。彼らは毒の耐性を持ち、麻痺知らずで、生命力を吸収されることがありません。また、身体が頑丈なのでACにボーナスを得ます。しかし身体のいたるところに電子回路が組み込まれているため、電撃によって通常よりも大きなダメージを受けてしまいます。彼らは食物からほとんど動力を得られませんが、油を補給する事で動力源を得る事ができます。",
        "An android is a artificial creation with a body of machinery.  They are poor at spell casting, but they make excellent warriors.  They don't acquire experience like other races, but rather gain in power as they attach new equipment to their frame.  Rings, amulets, and lights do not influence growth.  Androids are resistant to poison, can move freely, and are immune to exp-draining attacks.  Moreover, because of their hard metallic bodies, they get a bonus to AC.  Androids have electronic circuits throughout their body and must beware of electric shocks.  They gain very little nutrition from the food of mortals, but they can use flasks of oil as their energy source."),

    _("マーフォークは人型生物と水棲生物の合いの子たちの総称です。彼らは知的であり、陸上よりも水中での営みに適応している点で共通しています。彼らの真価は水中でこそ発揮されますが、彼らの文明は大抵陸上に長く滞在する何らかの術も有しています。それを失わない限り、陸上でも不自由はないでしょう。また、彼らは致命的な水難にも抗える能力を生まれつき持っています。",
        "Merfolks are general terms for people of mixed races between humanoids and aquatic habitats. They have in common that they are intelligent and adapt to working underwater rather than on land. Their true value comes in water, but their civilization also has some form of staying longer on land. As long as you do not lose it, there will be no inconvenience on land. They also have the ability to withstand disaster by water."),
};
