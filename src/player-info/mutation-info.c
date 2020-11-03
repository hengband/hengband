#include "player-info/mutation-info.h"
#include "mutation/mutation-flag-types.h"
#include "player-info/self-info-util.h"
#include "player/player-status-flags.h"

void set_mutation_info_1(player_type *creature_ptr, self_info_type *self_ptr)
{
    if (creature_ptr->muta1 == 0)
        return;

    if (creature_ptr->muta1 & MUT1_SPIT_ACID)
        self_ptr->info[self_ptr->line++] = _("あなたは酸を吹きかけることができる。(ダメージ レベルX1)", "You can spit acid (dam lvl).");

    if (creature_ptr->muta1 & MUT1_BR_FIRE)
        self_ptr->info[self_ptr->line++] = _("あなたは炎のブレスを吐くことができる。(ダメージ レベルX2)", "You can breathe fire (dam lvl * 2).");

    if (creature_ptr->muta1 & MUT1_HYPN_GAZE)
        self_ptr->info[self_ptr->line++] = _("あなたの睨みは催眠効果をもつ。", "Your gaze is hypnotic.");

    if (creature_ptr->muta1 & MUT1_TELEKINES)
        self_ptr->info[self_ptr->line++] = _("あなたは念動力をもっている。", "You are telekinetic.");

    if (creature_ptr->muta1 & MUT1_VTELEPORT)
        self_ptr->info[self_ptr->line++] = _("あなたは自分の意思でテレポートできる。", "You can teleport at will.");

    if (creature_ptr->muta1 & MUT1_MIND_BLST)
        self_ptr->info[self_ptr->line++] = _("あなたは精神攻撃を行える。(ダメージ 3～12d3)", "You can Mind Blast your enemies (3 to 12d3 dam).");

    if (creature_ptr->muta1 & MUT1_RADIATION)
        self_ptr->info[self_ptr->line++]
            = _("あなたは自分の意思で強い放射線を発生することができる。(ダメージ レベルX2)", "You can emit hard radiation at will (dam lvl * 2).");

    if (creature_ptr->muta1 & MUT1_VAMPIRISM)
        self_ptr->info[self_ptr->line++] = _(
            "あなたは吸血鬼のように敵から生命力を吸収することができる。(ダメージ レベルX2)", "Like a vampire, you can drain life from a foe (dam lvl * 2).");

    if (creature_ptr->muta1 & MUT1_SMELL_MET)
        self_ptr->info[self_ptr->line++] = _("あなたは近くにある貴金属をかぎ分けることができる。", "You can smell nearby precious metal.");

    if (creature_ptr->muta1 & MUT1_SMELL_MON)
        self_ptr->info[self_ptr->line++] = _("あなたは近くのモンスターの存在をかぎ分けることができる。", "You can smell nearby monsters.");

    if (creature_ptr->muta1 & MUT1_BLINK)
        self_ptr->info[self_ptr->line++] = _("あなたは短い距離をテレポートできる。", "You can teleport yourself short distances.");

    if (creature_ptr->muta1 & MUT1_EAT_ROCK)
        self_ptr->info[self_ptr->line++] = _("あなたは硬い岩を食べることができる。", "You can consume solid rock.");

    if (creature_ptr->muta1 & MUT1_SWAP_POS)
        self_ptr->info[self_ptr->line++] = _("あなたは他の者と場所を入れ替わることができる。", "You can switch locations with another being.");

    if (creature_ptr->muta1 & MUT1_SHRIEK)
        self_ptr->info[self_ptr->line++]
            = _("あなたは身の毛もよだつ叫び声を発することができる。(ダメージ レベルX2)", "You can emit a horrible shriek (dam 2 * lvl).");

    if (creature_ptr->muta1 & MUT1_ILLUMINE)
        self_ptr->info[self_ptr->line++] = _("あなたは明るい光を放つことができる。", "You can emit bright light.");

    if (creature_ptr->muta1 & MUT1_DET_CURSE)
        self_ptr->info[self_ptr->line++] = _("あなたは邪悪な魔法の危険を感じとることができる。", "You can feel the danger of evil magic.");

    if (creature_ptr->muta1 & MUT1_BERSERK)
        self_ptr->info[self_ptr->line++] = _("あなたは自分の意思で狂乱戦闘状態になることができる。", "You can drive yourself into a berserk frenzy.");

    if (creature_ptr->muta1 & MUT1_POLYMORPH)
        self_ptr->info[self_ptr->line++] = _("あなたは自分の意志で変化できる。", "You can polymorph yourself at will.");

    if (creature_ptr->muta1 & MUT1_MIDAS_TCH)
        self_ptr->info[self_ptr->line++] = _("あなたは通常アイテムを金に変えることができる。", "You can turn ordinary items to gold.");

    if (creature_ptr->muta1 & MUT1_GROW_MOLD)
        self_ptr->info[self_ptr->line++] = _("あなたは周囲にキノコを生やすことができる。", "You can cause mold to grow near you.");

    if (creature_ptr->muta1 & MUT1_RESIST)
        self_ptr->info[self_ptr->line++] = _("あなたは元素の攻撃に対して身を硬くすることができる。", "You can harden yourself to the ravages of the elements.");

    if (creature_ptr->muta1 & MUT1_EARTHQUAKE)
        self_ptr->info[self_ptr->line++] = _("あなたは周囲のダンジョンを崩壊させることができる。", "You can bring down the dungeon around your ears.");

    if (creature_ptr->muta1 & MUT1_EAT_MAGIC)
        self_ptr->info[self_ptr->line++] = _("あなたは魔法のエネルギーを自分の物として使用できる。", "You can consume magic energy for your own use.");

    if (creature_ptr->muta1 & MUT1_WEIGH_MAG)
        self_ptr->info[self_ptr->line++] = _("あなたは自分に影響を与える魔法の力を感じることができる。", "You can feel the strength of the magics affecting you.");

    if (creature_ptr->muta1 & MUT1_STERILITY)
        self_ptr->info[self_ptr->line++] = _("あなたは集団的生殖不能を起こすことができる。", "You can cause mass impotence.");

    if (creature_ptr->muta1 & MUT1_HIT_AND_AWAY)
        self_ptr->info[self_ptr->line++] = _("あなたは攻撃した後身を守るため逃げることができる。", "You can run for your life after hitting something.");

    if (creature_ptr->muta1 & MUT1_DAZZLE)
        self_ptr->info[self_ptr->line++] = _("あなたは混乱と盲目を引き起こす放射能を発生することができる。 ", "You can emit confusing, blinding radiation.");

    if (creature_ptr->muta1 & MUT1_LASER_EYE)
        self_ptr->info[self_ptr->line++]
            = _("あなたは目からレーザー光線を発することができる。(ダメージ レベルX2)", "Your eyes can fire laser beams (dam 2 * lvl).");

    if (creature_ptr->muta1 & MUT1_RECALL)
        self_ptr->info[self_ptr->line++] = _("あなたは街とダンジョンの間を行き来することができる。", "You can travel between town and the depths.");

    if (creature_ptr->muta1 & MUT1_BANISH)
        self_ptr->info[self_ptr->line++] = _("あなたは邪悪なモンスターを地獄に落とすことができる。", "You can send evil creatures directly to Hell.");

    if (creature_ptr->muta1 & MUT1_COLD_TOUCH)
        self_ptr->info[self_ptr->line++] = _("あなたは敵を触って凍らせることができる。(ダメージ レベルX3)", "You can freeze things with a touch (dam 3 * lvl).");

    if (creature_ptr->muta1 & MUT1_LAUNCHER)
        self_ptr->info[self_ptr->line++] = _("あなたはアイテムを力強く投げることができる。", "You can hurl objects with great force.");
}

void set_mutation_info_2(player_type *creature_ptr, self_info_type *self_ptr)
{
    if (creature_ptr->muta2 == 0)
        return;

    if (creature_ptr->muta2 & MUT2_BERS_RAGE)
        self_ptr->info[self_ptr->line++] = _("あなたは狂戦士化の発作を起こす。", "You are subject to berserker fits.");

    if (creature_ptr->muta2 & MUT2_COWARDICE)
        self_ptr->info[self_ptr->line++] = _("あなたは時々臆病になる。", "You are subject to cowardice.");

    if (creature_ptr->muta2 & MUT2_RTELEPORT)
        self_ptr->info[self_ptr->line++] = _("あなたはランダムにテレポートする。", "You may randomly teleport.");

    if (creature_ptr->muta2 & MUT2_ALCOHOL)
        self_ptr->info[self_ptr->line++] = _("あなたの体はアルコールを分泌する。", "Your body produces alcohol.");

    if (creature_ptr->muta2 & MUT2_HALLU)
        self_ptr->info[self_ptr->line++] = _("あなたは幻覚を引き起こす精神錯乱に侵されている。", "You have a hallucinatory insanity.");

    if (creature_ptr->muta2 & MUT2_FLATULENT)
        self_ptr->info[self_ptr->line++] = _("あなたは制御できない強烈な屁をこく。", "You are subject to uncontrollable flatulence.");

    if (creature_ptr->muta2 & MUT2_PROD_MANA)
        self_ptr->info[self_ptr->line++] = _("あなたは制御不能な魔法のエネルギーを発している。", "You produce magical energy uncontrollably.");

    if (creature_ptr->muta2 & MUT2_ATT_DEMON)
        self_ptr->info[self_ptr->line++] = _("あなたはデーモンを引きつける。", "You attract demons.");

    if (creature_ptr->muta2 & MUT2_SCOR_TAIL)
        self_ptr->info[self_ptr->line++] = _("あなたはサソリの尻尾が生えている。(毒、ダメージ 3d7)", "You have a scorpion tail (poison, 3d7).");

    if (creature_ptr->muta2 & MUT2_HORNS)
        self_ptr->info[self_ptr->line++] = _("あなたは角が生えている。(ダメージ 2d6)", "You have horns (dam. 2d6).");

    if (creature_ptr->muta2 & MUT2_BEAK)
        self_ptr->info[self_ptr->line++] = _("あなたはクチバシが生えている。(ダメージ 2d4)", "You have a beak (dam. 2d4).");

    if (creature_ptr->muta2 & MUT2_SPEED_FLUX)
        self_ptr->info[self_ptr->line++] = _("あなたはランダムに早く動いたり遅く動いたりする。", "You move faster or slower randomly.");

    if (creature_ptr->muta2 & MUT2_BANISH_ALL)
        self_ptr->info[self_ptr->line++] = _("あなたは時々近くのモンスターを消滅させる。", "You sometimes cause nearby creatures to vanish.");

    if (creature_ptr->muta2 & MUT2_EAT_LIGHT)
        self_ptr->info[self_ptr->line++] = _("あなたは時々周囲の光を吸収して栄養にする。", "You sometimes feed off of the light around you.");

    if (creature_ptr->muta2 & MUT2_TRUNK)
        self_ptr->info[self_ptr->line++] = _("あなたは象のような鼻を持っている。(ダメージ 1d4)", "You have an elephantine trunk (dam 1d4).");

    if (creature_ptr->muta2 & MUT2_ATT_ANIMAL)
        self_ptr->info[self_ptr->line++] = _("あなたは動物を引きつける。", "You attract animals.");

    if (creature_ptr->muta2 & MUT2_TENTACLES)
        self_ptr->info[self_ptr->line++] = _("あなたは邪悪な触手を持っている。(ダメージ 2d5)", "You have evil looking tentacles (dam 2d5).");

    if (creature_ptr->muta2 & MUT2_RAW_CHAOS)
        self_ptr->info[self_ptr->line++] = _("あなたはしばしば純カオスに包まれる。", "You occasionally are surrounded with raw chaos.");

    if (creature_ptr->muta2 & MUT2_NORMALITY)
        self_ptr->info[self_ptr->line++] = _("あなたは変異していたが、回復してきている。", "You may be mutated, but you're recovering.");

    if (creature_ptr->muta2 & MUT2_WRAITH)
        self_ptr->info[self_ptr->line++] = _("あなたの肉体は幽体化したり実体化したりする。", "You fade in and out of physical reality.");

    if (creature_ptr->muta2 & MUT2_POLY_WOUND)
        self_ptr->info[self_ptr->line++] = _("あなたの健康はカオスの力に影響を受ける。", "Your health is subject to chaotic forces.");

    if (creature_ptr->muta2 & MUT2_WASTING)
        self_ptr->info[self_ptr->line++] = _("あなたは衰弱する恐ろしい病気にかかっている。", "You have a horrible wasting disease.");

    if (creature_ptr->muta2 & MUT2_ATT_DRAGON)
        self_ptr->info[self_ptr->line++] = _("あなたはドラゴンを引きつける。", "You attract dragons.");

    if (creature_ptr->muta2 & MUT2_WEIRD_MIND)
        self_ptr->info[self_ptr->line++] = _("あなたの精神はランダムに拡大したり縮小したりしている。", "Your mind randomly expands and contracts.");

    if (creature_ptr->muta2 & MUT2_NAUSEA)
        self_ptr->info[self_ptr->line++] = _("あなたの胃は非常に落ち着きがない。", "You have a seriously upset stomach.");

    if (creature_ptr->muta2 & MUT2_CHAOS_GIFT)
        self_ptr->info[self_ptr->line++] = _("あなたはカオスの守護悪魔から褒美をうけとる。", "Chaos deities give you gifts.");

    if (creature_ptr->muta2 & MUT2_WALK_SHAD)
        self_ptr->info[self_ptr->line++] = _("あなたはしばしば他の「影」に迷い込む。", "You occasionally stumble into other shadows.");

    if (creature_ptr->muta2 & MUT2_WARNING)
        self_ptr->info[self_ptr->line++] = _("あなたは敵に関する警告を感じる。", "You receive warnings about your foes.");

    if (creature_ptr->muta2 & MUT2_INVULN)
        self_ptr->info[self_ptr->line++] = _("あなたは時々負け知らずな気分になる。", "You occasionally feel invincible.");

    if (creature_ptr->muta2 & MUT2_SP_TO_HP)
        self_ptr->info[self_ptr->line++] = _("あなたは時々血が筋肉にどっと流れる。", "Your blood sometimes rushes to your muscles.");

    if (creature_ptr->muta2 & MUT2_HP_TO_SP)
        self_ptr->info[self_ptr->line++] = _("あなたは時々頭に血がどっと流れる。", "Your blood sometimes rushes to your head.");

    if (creature_ptr->muta2 & MUT2_DISARM)
        self_ptr->info[self_ptr->line++] = _("あなたはよくつまづいて物を落とす。", "You occasionally stumble and drop things.");
}

/* todo FEAELESS フラグも記述して問題ないと思われる */
void set_mutation_info_3(player_type *creature_ptr, self_info_type *self_ptr)
{
    if (creature_ptr->muta3 == 0)
        return;

    if (creature_ptr->muta3 & MUT3_HYPER_STR)
        self_ptr->info[self_ptr->line++] = _("あなたは超人的に強い。(腕力+4)", "You are superhumanly strong (+4 STR).");

    if (creature_ptr->muta3 & MUT3_PUNY)
        self_ptr->info[self_ptr->line++] = _("あなたは虚弱だ。(腕力-4)", "You are puny (-4 STR).");

    if (creature_ptr->muta3 & MUT3_HYPER_INT)
        self_ptr->info[self_ptr->line++] = _("あなたの脳は生体コンピュータだ。(知能＆賢さ+4)", "Your brain is a living computer (+4 INT/WIS).");

    if (creature_ptr->muta3 & MUT3_MORONIC)
        self_ptr->info[self_ptr->line++] = _("あなたは精神薄弱だ。(知能＆賢さ-4)", "You are moronic (-4 INT/WIS).");

    if (creature_ptr->muta3 & MUT3_RESILIENT)
        self_ptr->info[self_ptr->line++] = _("あなたは非常にタフだ。(耐久+4)", "You are very resilient (+4 CON).");

    if (creature_ptr->muta3 & MUT3_XTRA_FAT)
        self_ptr->info[self_ptr->line++] = _("あなたは極端に太っている。(耐久+2,スピード-2)", "You are extremely fat (+2 CON, -2 speed).");

    if (creature_ptr->muta3 & MUT3_ALBINO)
        self_ptr->info[self_ptr->line++] = _("あなたはアルビノだ。(耐久-4)", "You are an albino (-4 CON).");

    if (creature_ptr->muta3 & MUT3_FLESH_ROT)
        self_ptr->info[self_ptr->line++] = _("あなたの肉体は腐敗している。(耐久-2,魅力-1)", "Your flesh is rotting (-2 CON, -1 CHR).");

    if (creature_ptr->muta3 & MUT3_SILLY_VOI)
        self_ptr->info[self_ptr->line++] = _("あなたの声は間抜けなキーキー声だ。(魅力-4)", "Your voice is a silly squeak (-4 CHR).");

    if (creature_ptr->muta3 & MUT3_BLANK_FAC)
        self_ptr->info[self_ptr->line++] = _("あなたはのっぺらぼうだ。(魅力-1)", "Your face is featureless (-1 CHR).");

    if (creature_ptr->muta3 & MUT3_ILL_NORM)
        self_ptr->info[self_ptr->line++] = _("あなたは幻影に覆われている。", "Your appearance is masked with illusion.");

    if (creature_ptr->muta3 & MUT3_XTRA_EYES)
        self_ptr->info[self_ptr->line++] = _("あなたは余分に二つの目を持っている。(探索+15)", "You have an extra pair of eyes (+15 search).");

    if (creature_ptr->muta3 & MUT3_MAGIC_RES)
        self_ptr->info[self_ptr->line++] = _("あなたは魔法への耐性をもっている。", "You are resistant to magic.");

    if (creature_ptr->muta3 & MUT3_XTRA_NOIS)
        self_ptr->info[self_ptr->line++] = _("あなたは変な音を発している。(隠密-3)", "You make a lot of strange noise (-3 stealth).");

    if (creature_ptr->muta3 & MUT3_INFRAVIS)
        self_ptr->info[self_ptr->line++] = _("あなたは素晴らしい赤外線視力を持っている。(+3)", "You have remarkable infravision (+3).");

    if (creature_ptr->muta3 & MUT3_XTRA_LEGS)
        self_ptr->info[self_ptr->line++] = _("あなたは余分に二本の足が生えている。(加速+3)", "You have an extra pair of legs (+3 speed).");

    if (creature_ptr->muta3 & MUT3_SHORT_LEG)
        self_ptr->info[self_ptr->line++] = _("あなたの足は短い突起だ。(加速-3)", "Your legs are short stubs (-3 speed).");

    if (creature_ptr->muta3 & MUT3_ELEC_TOUC)
        self_ptr->info[self_ptr->line++] = _("あなたの血管には電流が流れている。", "Electricity is running through your veins.");

    if (creature_ptr->muta3 & MUT3_FIRE_BODY)
        self_ptr->info[self_ptr->line++] = _("あなたの体は炎につつまれている。", "Your body is enveloped in flames.");

    if (creature_ptr->muta3 & MUT3_WART_SKIN)
        self_ptr->info[self_ptr->line++] = _("あなたの肌はイボに被われている。(魅力-2, AC+5)", "Your skin is covered with warts (-2 CHR, +5 AC).");

    if (creature_ptr->muta3 & MUT3_SCALES)
        self_ptr->info[self_ptr->line++] = _("あなたの肌は鱗になっている。(魅力-1, AC+10)", "Your skin has turned into scales (-1 CHR, +10 AC).");

    if (creature_ptr->muta3 & MUT3_IRON_SKIN)
        self_ptr->info[self_ptr->line++] = _("あなたの肌は鉄でできている。(器用-1, AC+25)", "Your skin is made of steel (-1 DEX, +25 AC).");

    if (creature_ptr->muta3 & MUT3_WINGS)
        self_ptr->info[self_ptr->line++] = _("あなたは羽を持っている。", "You have wings.");

    if (creature_ptr->muta3 & MUT3_FEARLESS) {
        /* Unnecessary */
    }

    if (creature_ptr->muta3 & MUT3_REGEN) {
        /* Unnecessary */
    }

    if (creature_ptr->muta3 & MUT3_ESP) {
        /* Unnecessary */
    }

    if (creature_ptr->muta3 & MUT3_LIMBER)
        self_ptr->info[self_ptr->line++] = _("あなたの体は非常にしなやかだ。(器用+3)", "Your body is very limber (+3 DEX).");

    if (creature_ptr->muta3 & MUT3_ARTHRITIS)
        self_ptr->info[self_ptr->line++] = _("あなたはいつも関節に痛みを感じている。(器用-3)", "Your joints ache constantly (-3 DEX).");

    if (creature_ptr->muta3 & MUT3_VULN_ELEM)
        self_ptr->info[self_ptr->line++] = _("あなたは元素の攻撃に弱い。", "You are susceptible to damage from the elements.");

    if (creature_ptr->muta3 & MUT3_MOTION)
        self_ptr->info[self_ptr->line++] = _("あなたの動作は正確で力強い。(隠密+1)", "Your movements are precise and forceful (+1 STL).");

    if (has_good_luck(creature_ptr))
        self_ptr->info[self_ptr->line++] = _("あなたは白いオーラにつつまれている。", "There is a white aura surrounding you.");

    if (creature_ptr->muta3 & MUT3_BAD_LUCK)
        self_ptr->info[self_ptr->line++] = _("あなたは黒いオーラにつつまれている。", "There is a black aura surrounding you.");
}
