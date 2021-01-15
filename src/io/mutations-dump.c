/*!
 * @brief 突然変異の一覧を出力する
 * @date 2020/04/24
 * @author Hourier
 */

#include "io/mutations-dump.h"
#include "mutation/mutation-flag-types.h"
#include "player/player-status-flags.h"

/*!
 * @brief ファイルポインタを通じて突然変異の一覧を出力する
 * @param out_file 出力先ファイルポインタ
 * @return なし
 */
void dump_mutations(player_type *creature_ptr, FILE *out_file)
{
    if (!out_file)
        return;

    if (creature_ptr->muta1) {
        if (creature_ptr->muta1 & MUT1_SPIT_ACID)
            fprintf(out_file, _(" あなたは酸を吹きかけることができる。(ダメージ レベルX1)\n", " You can spit acid (dam lvl).\n"));

        if (creature_ptr->muta1 & MUT1_BR_FIRE)
            fprintf(out_file, _(" あなたは炎のブレスを吐くことができる。(ダメージ レベルX2)\n", " You can breathe fire (dam lvl * 2).\n"));

        if (creature_ptr->muta1 & MUT1_HYPN_GAZE)
            fprintf(out_file, _(" あなたの睨みは催眠効果をもつ。\n", " Your gaze is hypnotic.\n"));

        if (creature_ptr->muta1 & MUT1_TELEKINES)
            fprintf(out_file, _(" あなたは念動力をもっている。\n", " You are telekinetic.\n"));

        if (creature_ptr->muta1 & MUT1_VTELEPORT)
            fprintf(out_file, _(" あなたは自分の意思でテレポートできる。\n", " You can teleport at will.\n"));

        if (creature_ptr->muta1 & MUT1_MIND_BLST)
            fprintf(out_file, _(" あなたは敵を精神攻撃できる。\n", " You can Mind Blast your enemies.\n"));

        if (creature_ptr->muta1 & MUT1_RADIATION)
            fprintf(out_file, _(" あなたは自分の意思で放射能を発生することができる。\n", " You can emit hard radiation at will.\n"));

        if (creature_ptr->muta1 & MUT1_VAMPIRISM)
            fprintf(out_file, _(" あなたは吸血鬼のように敵から生命力を吸収することができる。\n", " You can drain life from a foe like a vampire.\n"));

        if (creature_ptr->muta1 & MUT1_SMELL_MET)
            fprintf(out_file, _(" あなたは近くにある貴金属をかぎ分けることができる。\n", " You can smell nearby precious metal.\n"));

        if (creature_ptr->muta1 & MUT1_SMELL_MON)
            fprintf(out_file, _(" あなたは近くのモンスターの存在をかぎ分けることができる。\n", " You can smell nearby monsters.\n"));

        if (creature_ptr->muta1 & MUT1_BLINK)
            fprintf(out_file, _(" あなたは短い距離をテレポートできる。\n", " You can teleport yourself short distances.\n"));

        if (creature_ptr->muta1 & MUT1_EAT_ROCK)
            fprintf(out_file, _(" あなたは硬い岩を食べることができる。\n", " You can consume solid rock.\n"));

        if (creature_ptr->muta1 & MUT1_SWAP_POS)
            fprintf(out_file, _(" あなたは他の者と場所を入れ替わることができる。\n", " You can switch locations with another being.\n"));

        if (creature_ptr->muta1 & MUT1_SHRIEK)
            fprintf(out_file, _(" あなたは身の毛もよだつ叫び声を発することができる。\n", " You can emit a horrible shriek.\n"));

        if (creature_ptr->muta1 & MUT1_ILLUMINE)
            fprintf(out_file, _(" あなたは明るい光を放つことができる。\n", " You can emit bright light.\n"));

        if (creature_ptr->muta1 & MUT1_DET_CURSE)
            fprintf(out_file, _(" あなたは邪悪な魔法の危険を感じとることができる。\n", " You can feel the danger of evil magic.\n"));

        if (creature_ptr->muta1 & MUT1_BERSERK)
            fprintf(out_file, _(" あなたは自分の意思で狂乱戦闘状態になることができる。\n", " You can drive yourself into a berserk frenzy.\n"));

        if (creature_ptr->muta1 & MUT1_POLYMORPH)
            fprintf(out_file, _(" あなたは自分の意志で変化できる。\n", " You can polymorph yourself at will.\n"));

        if (creature_ptr->muta1 & MUT1_MIDAS_TCH)
            fprintf(out_file, _(" あなたは通常アイテムを金に変えることができる。\n", " You can turn ordinary items to gold.\n"));

        if (creature_ptr->muta1 & MUT1_GROW_MOLD)
            fprintf(out_file, _(" あなたは周囲にキノコを生やすことができる。\n", " You can cause mold to grow near you.\n"));

        if (creature_ptr->muta1 & MUT1_RESIST)
            fprintf(out_file, _(" あなたは元素の攻撃に対して身を硬くすることができる。\n", " You can harden yourself to the ravages of the elements.\n"));

        if (creature_ptr->muta1 & MUT1_EARTHQUAKE)
            fprintf(out_file, _(" あなたは周囲のダンジョンを崩壊させることができる。\n", " You can bring down the dungeon around your ears.\n"));

        if (creature_ptr->muta1 & MUT1_EAT_MAGIC)
            fprintf(out_file, _(" あなたは魔法のエネルギーを自分の物として使用できる。\n", " You can consume magic energy for your own use.\n"));

        if (creature_ptr->muta1 & MUT1_WEIGH_MAG)
            fprintf(out_file, _(" あなたは自分に影響を与える魔法の力を感じることができる。\n", " You can feel the strength of the magics affecting you.\n"));

        if (creature_ptr->muta1 & MUT1_STERILITY)
            fprintf(out_file, _(" あなたは集団的生殖不能を起こすことができる。\n", " You can cause mass impotence.\n"));

        if (creature_ptr->muta1 & MUT1_HIT_AND_AWAY)
            fprintf(out_file, _(" あなたは攻撃した後身を守るため逃げることができる。\n", " You can run for your life after hitting something.\n"));

        if (creature_ptr->muta1 & MUT1_DAZZLE)
            fprintf(out_file, _(" あなたは混乱と盲目を引き起こす放射能を発生することができる。 \n", " You can emit confusing, blinding radiation.\n"));

        if (creature_ptr->muta1 & MUT1_LASER_EYE)
            fprintf(out_file, _(" あなたは目からレーザー光線を発射することができる。\n", " Your eyes can fire laser beams.\n"));

        if (creature_ptr->muta1 & MUT1_RECALL)
            fprintf(out_file, _(" あなたは街とダンジョンの間を行き来することができる。\n", " You can travel between town and the depths.\n"));

        if (creature_ptr->muta1 & MUT1_BANISH)
            fprintf(out_file, _(" あなたは邪悪なモンスターを地獄に落とすことができる。\n", " You can send evil creatures directly to Hell.\n"));

        if (creature_ptr->muta1 & MUT1_COLD_TOUCH)
            fprintf(out_file, _(" あなたは物を触って凍らせることができる。\n", " You can freeze things with a touch.\n"));

        if (creature_ptr->muta1 & MUT1_LAUNCHER)
            fprintf(out_file, _(" あなたはアイテムを力強く投げることができる。\n", " You can hurl objects with great force.\n"));
    }

    if (creature_ptr->muta2) {
        if (creature_ptr->muta2 & MUT2_BERS_RAGE)
            fprintf(out_file, _(" あなたは狂戦士化の発作を起こす。\n", " You are subject to berserker fits.\n"));

        if (creature_ptr->muta2 & MUT2_COWARDICE)
            fprintf(out_file, _(" あなたは時々臆病になる。\n", " You are subject to cowardice.\n"));

        if (creature_ptr->muta2 & MUT2_RTELEPORT)
            fprintf(out_file, _(" あなたはランダムにテレポートする。\n", " You sometimes randomly teleport.\n"));

        if (creature_ptr->muta2 & MUT2_ALCOHOL)
            fprintf(out_file, _(" あなたの体はアルコールを分泌する。\n", " Your body produces alcohol.\n"));

        if (creature_ptr->muta2 & MUT2_HALLU)
            fprintf(out_file, _(" あなたは幻覚を引き起こす精神錯乱に侵されている。\n", " You have a hallucinatory insanity.\n"));

        if (creature_ptr->muta2 & MUT2_FLATULENT)
            fprintf(out_file, _(" あなたは制御できない強烈な屁をこく。\n", " You are subject to uncontrollable flatulence.\n"));

        if (creature_ptr->muta2 & MUT2_PROD_MANA)
            fprintf(out_file, _(" あなたは制御不能な魔法のエネルギーを発している。\n", " You produce magical energy uncontrollably.\n"));

        if (creature_ptr->muta2 & MUT2_ATT_DEMON)
            fprintf(out_file, _(" あなたはデーモンを引きつける。\n", " You attract demons.\n"));

        if (creature_ptr->muta2 & MUT2_SCOR_TAIL)
            fprintf(out_file, _(" あなたはサソリの尻尾が生えている。(毒、ダメージ 3d7)\n", " You have a scorpion tail (poison, 3d7).\n"));

        if (creature_ptr->muta2 & MUT2_HORNS)
            fprintf(out_file, _(" あなたは角が生えている。(ダメージ 2d6)\n", " You have horns (dam. 2d6).\n"));

        if (creature_ptr->muta2 & MUT2_BEAK)
            fprintf(out_file, _(" あなたはクチバシが生えている。(ダメージ 2d4)\n", " You have a beak (dam. 2d4).\n"));

        if (creature_ptr->muta2 & MUT2_SPEED_FLUX)
            fprintf(out_file, _(" あなたはランダムに早く動いたり遅く動いたりする。\n", " You move faster or slower randomly.\n"));

        if (creature_ptr->muta2 & MUT2_BANISH_ALL)
            fprintf(out_file, _(" あなたは時々近くのモンスターを消滅させる。\n", " You sometimes cause nearby creatures to vanish.\n"));

        if (creature_ptr->muta2 & MUT2_EAT_LIGHT)
            fprintf(out_file, _(" あなたは時々周囲の光を吸収して栄養にする。\n", " You sometimes feed off of the light around you.\n"));

        if (creature_ptr->muta2 & MUT2_TRUNK)
            fprintf(out_file, _(" あなたは象のような鼻を持っている。(ダメージ 1d4)\n", " You have an elephantine trunk (dam 1d4).\n"));

        if (creature_ptr->muta2 & MUT2_ATT_ANIMAL)
            fprintf(out_file, _(" あなたは動物を引きつける。\n", " You attract animals.\n"));

        if (creature_ptr->muta2 & MUT2_TENTACLES)
            fprintf(out_file, _(" あなたは邪悪な触手を持っている。(ダメージ 2d5)\n", " You have evil looking tentacles (dam 2d5).\n"));

        if (creature_ptr->muta2 & MUT2_RAW_CHAOS)
            fprintf(out_file, _(" あなたはしばしば純カオスに包まれる。\n", " You occasionally are surrounded with raw chaos.\n"));

        if (creature_ptr->muta2 & MUT2_NORMALITY)
            fprintf(out_file, _(" あなたは変異していたが、回復してきている。\n", " You may be mutated, but you're recovering.\n"));

        if (creature_ptr->muta2 & MUT2_WRAITH)
            fprintf(out_file, _(" あなたの肉体は幽体化したり実体化したりする。\n", " You fade in and out of physical reality.\n"));

        if (creature_ptr->muta2 & MUT2_POLY_WOUND)
            fprintf(out_file, _(" あなたの健康はカオスの力に影響を受ける。\n", " Your health is subject to chaotic forces.\n"));

        if (creature_ptr->muta2 & MUT2_WASTING)
            fprintf(out_file, _(" あなたは衰弱する恐ろしい病気にかかっている。\n", " You have a horrible wasting disease.\n"));

        if (creature_ptr->muta2 & MUT2_ATT_DRAGON)
            fprintf(out_file, _(" あなたはドラゴンを引きつける。\n", " You attract dragons.\n"));

        if (creature_ptr->muta2 & MUT2_WEIRD_MIND)
            fprintf(out_file, _(" あなたの精神はランダムに拡大したり縮小したりしている。\n", " Your mind randomly expands and contracts.\n"));

        if (creature_ptr->muta2 & MUT2_NAUSEA)
            fprintf(out_file, _(" あなたの胃は非常に落ち着きがない。\n", " You have a seriously upset stomach.\n"));

        if (creature_ptr->muta2 & MUT2_CHAOS_GIFT)
            fprintf(out_file, _(" あなたはカオスの守護悪魔から褒美をうけとる。\n", " Chaos deities give you gifts.\n"));

        if (creature_ptr->muta2 & MUT2_WALK_SHAD)
            fprintf(out_file, _(" あなたはしばしば他の「影」に迷い込む。\n", " You occasionally stumble into other shadows.\n"));

        if (creature_ptr->muta2 & MUT2_WARNING)
            fprintf(out_file, _(" あなたは敵に関する警告を感じる。\n", " You receive warnings about your foes.\n"));

        if (creature_ptr->muta2 & MUT2_INVULN)
            fprintf(out_file, _(" あなたは時々負け知らずな気分になる。\n", " You occasionally feel invincible.\n"));

        if (creature_ptr->muta2 & MUT2_SP_TO_HP)
            fprintf(out_file, _(" あなたは時々血が筋肉にどっと流れる。\n", " Your blood sometimes rushes to your muscles.\n"));

        if (creature_ptr->muta2 & MUT2_HP_TO_SP)
            fprintf(out_file, _(" あなたは時々頭に血がどっと流れる。\n", " Your blood sometimes rushes to your head.\n"));

        if (creature_ptr->muta2 & MUT2_DISARM)
            fprintf(out_file, _(" あなたはよくつまづいて物を落とす。\n", " You occasionally stumble and drop things.\n"));
    }

    if (creature_ptr->muta3) {
        if (creature_ptr->muta3 & MUT3_HYPER_STR)
            fprintf(out_file, _(" あなたは超人的に強い。(腕力+4)\n", " You are superhumanly strong (+4 STR).\n"));

        if (creature_ptr->muta3 & MUT3_PUNY)
            fprintf(out_file, _(" あなたは虚弱だ。(腕力-4)\n", " You are puny (-4 STR).\n"));

        if (creature_ptr->muta3 & MUT3_HYPER_INT)
            fprintf(out_file, _(" あなたの脳は生体コンピュータだ。(知能＆賢さ+4)\n", " Your brain is a living computer (+4 INT/WIS).\n"));

        if (creature_ptr->muta3 & MUT3_MORONIC)
            fprintf(out_file, _(" あなたは精神薄弱だ。(知能＆賢さ-4)\n", " You are moronic (-4 INT/WIS).\n"));

        if (creature_ptr->muta3 & MUT3_RESILIENT)
            fprintf(out_file, _(" あなたの体は弾力性に富んでいる。(耐久+4)\n", " You are very resilient (+4 CON).\n"));

        if (creature_ptr->muta3 & MUT3_XTRA_FAT)
            fprintf(out_file, _(" あなたは極端に太っている。(耐久+2,スピード-2)\n", " You are extremely fat (+2 CON, -2 speed).\n"));

        if (creature_ptr->muta3 & MUT3_ALBINO)
            fprintf(out_file, _(" あなたはアルビノだ。(耐久-4)\n", " You are albino (-4 CON).\n"));

        if (creature_ptr->muta3 & MUT3_FLESH_ROT)
            fprintf(out_file, _(" あなたの肉体は腐敗している。(耐久-2,魅力-1)\n", " Your flesh is rotting (-2 CON, -1 CHR).\n"));

        if (creature_ptr->muta3 & MUT3_SILLY_VOI)
            fprintf(out_file, _(" あなたの声は間抜けなキーキー声だ。(魅力-4)\n", " Your voice is a silly squeak (-4 CHR).\n"));

        if (creature_ptr->muta3 & MUT3_BLANK_FAC)
            fprintf(out_file, _(" あなたはのっぺらぼうだ。(魅力-1)\n", " Your face is featureless (-1 CHR).\n"));

        if (creature_ptr->muta3 & MUT3_ILL_NORM)
            fprintf(out_file, _(" あなたは幻影に覆われている。\n", " Your appearance is masked with illusion.\n"));

        if (creature_ptr->muta3 & MUT3_XTRA_EYES)
            fprintf(out_file, _(" あなたは余分に二つの目を持っている。(探索+15)\n", " You have an extra pair of eyes (+15 search).\n"));

        if (creature_ptr->muta3 & MUT3_MAGIC_RES)
            fprintf(out_file, _(" あなたは魔法への耐性をもっている。\n", " You are resistant to magic.\n"));

        if (creature_ptr->muta3 & MUT3_XTRA_NOIS)
            fprintf(out_file, _(" あなたは変な音を発している。(隠密-3)\n", " You make a lot of strange noise (-3 stealth).\n"));

        if (creature_ptr->muta3 & MUT3_INFRAVIS)
            fprintf(out_file, _(" あなたは素晴らしい赤外線視力を持っている。(+3)\n", " You have remarkable infravision (+3).\n"));

        if (creature_ptr->muta3 & MUT3_XTRA_LEGS)
            fprintf(out_file, _(" あなたは余分に二本の足が生えている。(加速+3)\n", " You have an extra pair of legs (+3 speed).\n"));

        if (creature_ptr->muta3 & MUT3_SHORT_LEG)
            fprintf(out_file, _(" あなたの足は短い突起だ。(加速-3)\n", " Your legs are short stubs (-3 speed).\n"));

        if (creature_ptr->muta3 & MUT3_ELEC_TOUC)
            fprintf(out_file, _(" あなたの血管には電流が流れている。\n", " Electricity runs through your veins.\n"));

        if (creature_ptr->muta3 & MUT3_FIRE_BODY)
            fprintf(out_file, _(" あなたの体は炎につつまれている。\n", " Your body is enveloped in flames.\n"));

        if (creature_ptr->muta3 & MUT3_WART_SKIN)
            fprintf(out_file, _(" あなたの肌はイボに被われている。(魅力-2, AC+5)\n", " Your skin is covered with warts (-2 CHR, +5 AC).\n"));

        if (creature_ptr->muta3 & MUT3_SCALES)
            fprintf(out_file, _(" あなたの肌は鱗になっている。(魅力-1, AC+10)\n", " Your skin has turned into scales (-1 CHR, +10 AC).\n"));

        if (creature_ptr->muta3 & MUT3_IRON_SKIN)
            fprintf(out_file, _(" あなたの肌は鉄でできている。(器用-1, AC+25)\n", " Your skin is made of steel (-1 DEX, +25 AC).\n"));

        if (creature_ptr->muta3 & MUT3_WINGS)
            fprintf(out_file, _(" あなたは羽を持っている。\n", " You have wings.\n"));

        if (creature_ptr->muta3 & MUT3_FEARLESS)
            fprintf(out_file, _(" あなたは全く恐怖を感じない。\n", " You are completely fearless.\n"));

        if (creature_ptr->muta3 & MUT3_REGEN)
            fprintf(out_file, _(" あなたは急速に回復する。\n", " You are regenerating.\n"));

        if (creature_ptr->muta3 & MUT3_ESP)
            fprintf(out_file, _(" あなたはテレパシーを持っている。\n", " You are telepathic.\n"));

        if (creature_ptr->muta3 & MUT3_LIMBER)
            fprintf(out_file, _(" あなたの体は非常にしなやかだ。(器用+3)\n", " Your body is very limber (+3 DEX).\n"));

        if (creature_ptr->muta3 & MUT3_ARTHRITIS)
            fprintf(out_file, _(" あなたはいつも関節に痛みを感じている。(器用-3)\n", " Your joints ache constantly (-3 DEX).\n"));

        if (creature_ptr->muta3 & MUT3_VULN_ELEM)
            fprintf(out_file, _(" あなたは元素の攻撃に弱い。\n", " You are susceptible to damage from the elements.\n"));

        if (creature_ptr->muta3 & MUT3_MOTION)
            fprintf(out_file, _(" あなたの動作は正確で力強い。(隠密+1)\n", " Your movements are precise and forceful (+1 STL).\n"));

        if (has_good_luck(creature_ptr))
            fprintf(out_file, _(" あなたは白いオーラにつつまれている。\n", " There is a white aura surrounding you.\n"));

        if (creature_ptr->muta3 & MUT3_BAD_LUCK)
            fprintf(out_file, _(" あなたは黒いオーラにつつまれている。\n", " There is a black aura surrounding you.\n"));
    }
}
