#include "player-info/mutation-info.h"
#include "mutation/mutation-flag-types.h"
#include "player-info/self-info-util.h"
#include "player/player-status-flags.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!< @todo FEAELESS フラグも記述して問題ないと思われる */
void set_mutation_info(player_type *player_ptr, self_info_type *self_ptr)
{
    if (player_ptr->muta.none())
        return;

    if (player_ptr->muta.has(MUTA::SPIT_ACID))
        self_ptr->info[self_ptr->line++] = _("あなたは酸を吹きかけることができる。(ダメージ レベルX1)", "You can spit acid (dam lvl).");

    if (player_ptr->muta.has(MUTA::BR_FIRE))
        self_ptr->info[self_ptr->line++] = _("あなたは炎のブレスを吐くことができる。(ダメージ レベルX2)", "You can breathe fire (dam lvl * 2).");

    if (player_ptr->muta.has(MUTA::HYPN_GAZE))
        self_ptr->info[self_ptr->line++] = _("あなたの睨みは催眠効果をもつ。", "Your gaze is hypnotic.");

    if (player_ptr->muta.has(MUTA::TELEKINES))
        self_ptr->info[self_ptr->line++] = _("あなたは念動力をもっている。", "You are telekinetic.");

    if (player_ptr->muta.has(MUTA::VTELEPORT))
        self_ptr->info[self_ptr->line++] = _("あなたは自分の意思でテレポートできる。", "You can teleport at will.");

    if (player_ptr->muta.has(MUTA::MIND_BLST))
        self_ptr->info[self_ptr->line++] = _("あなたは精神攻撃を行える。(ダメージ 3～12d3)", "You can Mind Blast your enemies (3 to 12d3 dam).");

    if (player_ptr->muta.has(MUTA::RADIATION))
        self_ptr->info[self_ptr->line++]
            = _("あなたは自分の意思で強い放射線を発生することができる。(ダメージ レベルX2)", "You can emit hard radiation at will (dam lvl * 2).");

    if (player_ptr->muta.has(MUTA::VAMPIRISM))
        self_ptr->info[self_ptr->line++] = _(
            "あなたは吸血鬼のように敵から生命力を吸収することができる。(ダメージ レベルX2)", "Like a vampire, you can drain life from a foe (dam lvl * 2).");

    if (player_ptr->muta.has(MUTA::SMELL_MET))
        self_ptr->info[self_ptr->line++] = _("あなたは近くにある貴金属をかぎ分けることができる。", "You can smell nearby precious metal.");

    if (player_ptr->muta.has(MUTA::SMELL_MON))
        self_ptr->info[self_ptr->line++] = _("あなたは近くのモンスターの存在をかぎ分けることができる。", "You can smell nearby monsters.");

    if (player_ptr->muta.has(MUTA::BLINK))
        self_ptr->info[self_ptr->line++] = _("あなたは短い距離をテレポートできる。", "You can teleport yourself short distances.");

    if (player_ptr->muta.has(MUTA::EAT_ROCK))
        self_ptr->info[self_ptr->line++] = _("あなたは硬い岩を食べることができる。", "You can consume solid rock.");

    if (player_ptr->muta.has(MUTA::SWAP_POS))
        self_ptr->info[self_ptr->line++] = _("あなたは他の者と場所を入れ替わることができる。", "You can switch locations with another being.");

    if (player_ptr->muta.has(MUTA::SHRIEK))
        self_ptr->info[self_ptr->line++]
            = _("あなたは身の毛もよだつ叫び声を発することができる。(ダメージ レベルX2)", "You can emit a horrible shriek (dam 2 * lvl).");

    if (player_ptr->muta.has(MUTA::ILLUMINE))
        self_ptr->info[self_ptr->line++] = _("あなたは明るい光を放つことができる。", "You can emit bright light.");

    if (player_ptr->muta.has(MUTA::DET_CURSE))
        self_ptr->info[self_ptr->line++] = _("あなたは邪悪な魔法の危険を感じとることができる。", "You can feel the danger of evil magic.");

    if (player_ptr->muta.has(MUTA::BERSERK))
        self_ptr->info[self_ptr->line++] = _("あなたは自分の意思で狂乱戦闘状態になることができる。", "You can drive yourself into a berserk frenzy.");

    if (player_ptr->muta.has(MUTA::POLYMORPH))
        self_ptr->info[self_ptr->line++] = _("あなたは自分の意志で変化できる。", "You can polymorph yourself at will.");

    if (player_ptr->muta.has(MUTA::MIDAS_TCH))
        self_ptr->info[self_ptr->line++] = _("あなたは通常アイテムを金に変えることができる。", "You can turn ordinary items to gold.");

    if (player_ptr->muta.has(MUTA::GROW_MOLD))
        self_ptr->info[self_ptr->line++] = _("あなたは周囲にキノコを生やすことができる。", "You can cause mold to grow near you.");

    if (player_ptr->muta.has(MUTA::RESIST))
        self_ptr->info[self_ptr->line++] = _("あなたは元素の攻撃に対して身を硬くすることができる。", "You can harden yourself to the ravages of the elements.");

    if (player_ptr->muta.has(MUTA::EARTHQUAKE))
        self_ptr->info[self_ptr->line++] = _("あなたは周囲のダンジョンを崩壊させることができる。", "You can bring down the dungeon around your ears.");

    if (player_ptr->muta.has(MUTA::EAT_MAGIC))
        self_ptr->info[self_ptr->line++] = _("あなたは魔法のエネルギーを自分の物として使用できる。", "You can consume magic energy for your own use.");

    if (player_ptr->muta.has(MUTA::WEIGH_MAG))
        self_ptr->info[self_ptr->line++]
            = _("あなたは自分に影響を与える魔法の力を感じることができる。", "You can feel the strength of the magics affecting you.");

    if (player_ptr->muta.has(MUTA::STERILITY))
        self_ptr->info[self_ptr->line++] = _("あなたは集団的生殖不能を起こすことができる。", "You can cause mass impotence.");

    if (player_ptr->muta.has(MUTA::HIT_AND_AWAY))
        self_ptr->info[self_ptr->line++] = _("あなたは攻撃した後身を守るため逃げることができる。", "You can run for your life after hitting something.");

    if (player_ptr->muta.has(MUTA::DAZZLE))
        self_ptr->info[self_ptr->line++] = _("あなたは混乱と盲目を引き起こす放射能を発生することができる。 ", "You can emit confusing, blinding radiation.");

    if (player_ptr->muta.has(MUTA::LASER_EYE))
        self_ptr->info[self_ptr->line++]
            = _("あなたは目からレーザー光線を発することができる。(ダメージ レベルX2)", "Your eyes can fire laser beams (dam 2 * lvl).");

    if (player_ptr->muta.has(MUTA::RECALL))
        self_ptr->info[self_ptr->line++] = _("あなたは街とダンジョンの間を行き来することができる。", "You can travel between town and the depths.");

    if (player_ptr->muta.has(MUTA::BANISH))
        self_ptr->info[self_ptr->line++] = _("あなたは邪悪なモンスターを地獄に落とすことができる。", "You can send evil creatures directly to Hell.");

    if (player_ptr->muta.has(MUTA::COLD_TOUCH))
        self_ptr->info[self_ptr->line++]
            = _("あなたは敵を触って凍らせることができる。(ダメージ レベルX3)", "You can freeze things with a touch (dam 3 * lvl).");

    if (player_ptr->muta.has(MUTA::LAUNCHER))
        self_ptr->info[self_ptr->line++] = _("あなたはアイテムを力強く投げることができる。", "You can hurl objects with great force.");

    if (player_ptr->muta.has(MUTA::BERS_RAGE))
        self_ptr->info[self_ptr->line++] = _("あなたは狂戦士化の発作を起こす。", "You are subject to berserker fits.");

    if (player_ptr->muta.has(MUTA::COWARDICE))
        self_ptr->info[self_ptr->line++] = _("あなたは時々臆病になる。", "You are subject to cowardice.");

    if (player_ptr->muta.has(MUTA::RTELEPORT))
        self_ptr->info[self_ptr->line++] = _("あなたはランダムにテレポートする。", "You may randomly teleport.");

    if (player_ptr->muta.has(MUTA::ALCOHOL))
        self_ptr->info[self_ptr->line++] = _("あなたの体はアルコールを分泌する。", "Your body produces alcohol.");

    if (player_ptr->muta.has(MUTA::HALLU))
        self_ptr->info[self_ptr->line++] = _("あなたは幻覚を引き起こす精神錯乱に侵されている。", "You have a hallucinatory insanity.");

    if (player_ptr->muta.has(MUTA::FLATULENT))
        self_ptr->info[self_ptr->line++] = _("あなたは制御できない強烈な屁をこく。", "You are subject to uncontrollable flatulence.");

    if (player_ptr->muta.has(MUTA::PROD_MANA))
        self_ptr->info[self_ptr->line++] = _("あなたは制御不能な魔法のエネルギーを発している。", "You produce magical energy uncontrollably.");

    if (player_ptr->muta.has(MUTA::ATT_DEMON))
        self_ptr->info[self_ptr->line++] = _("あなたはデーモンを引きつける。", "You attract demons.");

    if (player_ptr->muta.has(MUTA::SCOR_TAIL))
        self_ptr->info[self_ptr->line++] = _("あなたはサソリの尻尾が生えている。(毒、ダメージ 3d7)", "You have a scorpion tail (poison, 3d7).");

    if (player_ptr->muta.has(MUTA::HORNS))
        self_ptr->info[self_ptr->line++] = _("あなたは角が生えている。(ダメージ 2d6)", "You have horns (dam. 2d6).");

    if (player_ptr->muta.has(MUTA::BEAK))
        self_ptr->info[self_ptr->line++] = _("あなたはクチバシが生えている。(ダメージ 2d4)", "You have a beak (dam. 2d4).");

    if (player_ptr->muta.has(MUTA::SPEED_FLUX))
        self_ptr->info[self_ptr->line++] = _("あなたはランダムに早く動いたり遅く動いたりする。", "You move faster or slower randomly.");

    if (player_ptr->muta.has(MUTA::BANISH_ALL))
        self_ptr->info[self_ptr->line++] = _("あなたは時々近くのモンスターを消滅させる。", "You sometimes cause nearby creatures to vanish.");

    if (player_ptr->muta.has(MUTA::EAT_LIGHT))
        self_ptr->info[self_ptr->line++] = _("あなたは時々周囲の光を吸収して栄養にする。", "You sometimes feed off of the light around you.");

    if (player_ptr->muta.has(MUTA::TRUNK))
        self_ptr->info[self_ptr->line++] = _("あなたは象のような鼻を持っている。(ダメージ 1d4)", "You have an elephantine trunk (dam 1d4).");

    if (player_ptr->muta.has(MUTA::ATT_ANIMAL))
        self_ptr->info[self_ptr->line++] = _("あなたは動物を引きつける。", "You attract animals.");

    if (player_ptr->muta.has(MUTA::TENTACLES))
        self_ptr->info[self_ptr->line++] = _("あなたは邪悪な触手を持っている。(ダメージ 2d5)", "You have evil looking tentacles (dam 2d5).");

    if (player_ptr->muta.has(MUTA::RAW_CHAOS))
        self_ptr->info[self_ptr->line++] = _("あなたはしばしば純カオスに包まれる。", "You occasionally are surrounded with raw chaos.");

    if (player_ptr->muta.has(MUTA::NORMALITY))
        self_ptr->info[self_ptr->line++] = _("あなたは変異していたが、回復してきている。", "You may be mutated, but you're recovering.");

    if (player_ptr->muta.has(MUTA::WRAITH))
        self_ptr->info[self_ptr->line++] = _("あなたの肉体は幽体化したり実体化したりする。", "You fade in and out of physical reality.");

    if (player_ptr->muta.has(MUTA::POLY_WOUND))
        self_ptr->info[self_ptr->line++] = _("あなたの健康はカオスの力に影響を受ける。", "Your health is subject to chaotic forces.");

    if (player_ptr->muta.has(MUTA::WASTING))
        self_ptr->info[self_ptr->line++] = _("あなたは衰弱する恐ろしい病気にかかっている。", "You have a horrible wasting disease.");

    if (player_ptr->muta.has(MUTA::ATT_DRAGON))
        self_ptr->info[self_ptr->line++] = _("あなたはドラゴンを引きつける。", "You attract dragons.");

    if (player_ptr->muta.has(MUTA::WEIRD_MIND))
        self_ptr->info[self_ptr->line++] = _("あなたの精神はランダムに拡大したり縮小したりしている。", "Your mind randomly expands and contracts.");

    if (player_ptr->muta.has(MUTA::NAUSEA))
        self_ptr->info[self_ptr->line++] = _("あなたの胃は非常に落ち着きがない。", "You have a seriously upset stomach.");

    if (player_ptr->muta.has(MUTA::CHAOS_GIFT))
        self_ptr->info[self_ptr->line++] = _("あなたはカオスの守護悪魔から褒美をうけとる。", "Chaos deities give you gifts.");

    if (player_ptr->muta.has(MUTA::WALK_SHAD))
        self_ptr->info[self_ptr->line++] = _("あなたはしばしば他の「影」に迷い込む。", "You occasionally stumble into other shadows.");

    if (player_ptr->muta.has(MUTA::WARNING))
        self_ptr->info[self_ptr->line++] = _("あなたは敵に関する警告を感じる。", "You receive warnings about your foes.");

    if (player_ptr->muta.has(MUTA::INVULN))
        self_ptr->info[self_ptr->line++] = _("あなたは時々負け知らずな気分になる。", "You occasionally feel invincible.");

    if (player_ptr->muta.has(MUTA::SP_TO_HP))
        self_ptr->info[self_ptr->line++] = _("あなたは時々血が筋肉にどっと流れる。", "Your blood sometimes rushes to your muscles.");

    if (player_ptr->muta.has(MUTA::HP_TO_SP))
        self_ptr->info[self_ptr->line++] = _("あなたは時々頭に血がどっと流れる。", "Your blood sometimes rushes to your head.");

    if (player_ptr->muta.has(MUTA::DISARM))
        self_ptr->info[self_ptr->line++] = _("あなたはよくつまづいて物を落とす。", "You occasionally stumble and drop things.");

    if (player_ptr->muta.has(MUTA::HYPER_STR))
        self_ptr->info[self_ptr->line++] = _("あなたは超人的に強い。(腕力+4)", "You are superhumanly strong (+4 STR).");

    if (player_ptr->muta.has(MUTA::PUNY))
        self_ptr->info[self_ptr->line++] = _("あなたは虚弱だ。(腕力-4)", "You are puny (-4 STR).");

    if (player_ptr->muta.has(MUTA::HYPER_INT))
        self_ptr->info[self_ptr->line++] = _("あなたの脳は生体コンピュータだ。(知能＆賢さ+4)", "Your brain is a living computer (+4 INT/WIS).");

    if (player_ptr->muta.has(MUTA::MORONIC))
        self_ptr->info[self_ptr->line++] = _("あなたは精神薄弱だ。(知能＆賢さ-4)", "You are moronic (-4 INT/WIS).");

    if (player_ptr->muta.has(MUTA::RESILIENT))
        self_ptr->info[self_ptr->line++] = _("あなたは非常にタフだ。(耐久+4)", "You are very resilient (+4 CON).");

    if (player_ptr->muta.has(MUTA::XTRA_FAT))
        self_ptr->info[self_ptr->line++] = _("あなたは極端に太っている。(耐久+2,スピード-2)", "You are extremely fat (+2 CON, -2 speed).");

    if (player_ptr->muta.has(MUTA::ALBINO))
        self_ptr->info[self_ptr->line++] = _("あなたはアルビノだ。(耐久-4)", "You are an albino (-4 CON).");

    if (player_ptr->muta.has(MUTA::FLESH_ROT))
        self_ptr->info[self_ptr->line++] = _("あなたの肉体は腐敗している。(耐久-2,魅力-1)", "Your flesh is rotting (-2 CON, -1 CHR).");

    if (player_ptr->muta.has(MUTA::SILLY_VOI))
        self_ptr->info[self_ptr->line++] = _("あなたの声は間抜けなキーキー声だ。(魅力-4)", "Your voice is a silly squeak (-4 CHR).");

    if (player_ptr->muta.has(MUTA::BLANK_FAC))
        self_ptr->info[self_ptr->line++] = _("あなたはのっぺらぼうだ。(魅力-1)", "Your face is featureless (-1 CHR).");

    if (player_ptr->muta.has(MUTA::ILL_NORM))
        self_ptr->info[self_ptr->line++] = _("あなたは幻影に覆われている。", "Your appearance is masked with illusion.");

    if (player_ptr->muta.has(MUTA::XTRA_EYES))
        self_ptr->info[self_ptr->line++] = _("あなたは余分に二つの目を持っている。(探索+15)", "You have an extra pair of eyes (+15 search).");

    if (player_ptr->muta.has(MUTA::MAGIC_RES))
        self_ptr->info[self_ptr->line++] = _("あなたは魔法への耐性をもっている。", "You are resistant to magic.");

    if (player_ptr->muta.has(MUTA::XTRA_NOIS))
        self_ptr->info[self_ptr->line++] = _("あなたは変な音を発している。(隠密-3)", "You make a lot of strange noise (-3 stealth).");

    if (player_ptr->muta.has(MUTA::INFRAVIS))
        self_ptr->info[self_ptr->line++] = _("あなたは素晴らしい赤外線視力を持っている。(+3)", "You have remarkable infravision (+3).");

    if (player_ptr->muta.has(MUTA::XTRA_LEGS))
        self_ptr->info[self_ptr->line++] = _("あなたは余分に二本の足が生えている。(加速+3)", "You have an extra pair of legs (+3 speed).");

    if (player_ptr->muta.has(MUTA::SHORT_LEG))
        self_ptr->info[self_ptr->line++] = _("あなたの足は短い突起だ。(加速-3)", "Your legs are short stubs (-3 speed).");

    if (player_ptr->muta.has(MUTA::ELEC_TOUC))
        self_ptr->info[self_ptr->line++] = _("あなたの血管には電流が流れている。", "Electricity is running through your veins.");

    if (player_ptr->muta.has(MUTA::FIRE_BODY))
        self_ptr->info[self_ptr->line++] = _("あなたの体は炎につつまれている。", "Your body is enveloped in flames.");

    if (player_ptr->muta.has(MUTA::WART_SKIN))
        self_ptr->info[self_ptr->line++] = _("あなたの肌はイボに被われている。(魅力-2, AC+5)", "Your skin is covered with warts (-2 CHR, +5 AC).");

    if (player_ptr->muta.has(MUTA::SCALES))
        self_ptr->info[self_ptr->line++] = _("あなたの肌は鱗になっている。(魅力-1, AC+10)", "Your skin has turned into scales (-1 CHR, +10 AC).");

    if (player_ptr->muta.has(MUTA::IRON_SKIN))
        self_ptr->info[self_ptr->line++] = _("あなたの肌は鉄でできている。(器用-1, AC+25)", "Your skin is made of steel (-1 DEX, +25 AC).");

    if (player_ptr->muta.has(MUTA::WINGS))
        self_ptr->info[self_ptr->line++] = _("あなたは羽を持っている。", "You have wings.");

    if (player_ptr->muta.has(MUTA::FEARLESS)) {
        /* Unnecessary */
    }

    if (player_ptr->muta.has(MUTA::REGEN)) {
        /* Unnecessary */
    }

    if (player_ptr->muta.has(MUTA::ESP)) {
        /* Unnecessary */
    }

    if (player_ptr->muta.has(MUTA::LIMBER))
        self_ptr->info[self_ptr->line++] = _("あなたの体は非常にしなやかだ。(器用+3)", "Your body is very limber (+3 DEX).");

    if (player_ptr->muta.has(MUTA::ARTHRITIS))
        self_ptr->info[self_ptr->line++] = _("あなたはいつも関節に痛みを感じている。(器用-3)", "Your joints ache constantly (-3 DEX).");

    if (player_ptr->muta.has(MUTA::VULN_ELEM))
        self_ptr->info[self_ptr->line++] = _("あなたは元素の攻撃に弱い。", "You are susceptible to damage from the elements.");

    if (player_ptr->muta.has(MUTA::MOTION))
        self_ptr->info[self_ptr->line++] = _("あなたの動作は正確で力強い。(隠密+1)", "Your movements are precise and forceful (+1 STL).");

    if (has_good_luck(player_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは白いオーラにつつまれている。", "There is a white aura surrounding you.");

    if (player_ptr->muta.has(MUTA::BAD_LUCK))
        self_ptr->info[self_ptr->line++] = _("あなたは黒いオーラにつつまれている。", "There is a black aura surrounding you.");
}
