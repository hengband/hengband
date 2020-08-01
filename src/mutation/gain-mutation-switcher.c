#include "mutation/gain-mutation-switcher.h"
#include "mutation/gain-mutation-util.h"
#include "mutation/mutation-flag-types.h"

void switch_gain_mutation(player_type *creature_ptr, gm_type *gm_ptr)
{
    switch (gm_ptr->choose_mut ? gm_ptr->choose_mut : (creature_ptr->pclass == CLASS_BERSERKER ? 74 + randint1(119) : randint1(193))) {
    case 1:
    case 2:
    case 3:
    case 4:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_SPIT_ACID;
        gm_ptr->muta_desc = _("酸を吐く能力を得た。", "You gain the ability to spit acid.");
        break;
    case 5:
    case 6:
    case 7:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_BR_FIRE;
        gm_ptr->muta_desc = _("火を吐く能力を得た。", "You gain the ability to breathe fire.");
        break;
    case 8:
    case 9:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_HYPN_GAZE;
        gm_ptr->muta_desc = _("催眠眼の能力を得た。", "Your eyes look mesmerizing...");
        break;
    case 10:
    case 11:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_TELEKINES;
        gm_ptr->muta_desc = _("物体を念動力で動かす能力を得た。", "You gain the ability to move objects telekinetically.");
        break;
    case 12:
    case 13:
    case 14:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_VTELEPORT;
        gm_ptr->muta_desc = _("自分の意思でテレポートする能力を得た。", "You gain the power of teleportation at will.");
        break;
    case 15:
    case 16:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_MIND_BLST;
        gm_ptr->muta_desc = _("精神攻撃の能力を得た。", "You gain the power of Mind Blast.");
        break;
    case 17:
    case 18:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_RADIATION;
        gm_ptr->muta_desc = _("あなたは強い放射線を発生し始めた。", "You start emitting hard radiation.");
        break;
    case 19:
    case 20:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_VAMPIRISM;
        gm_ptr->muta_desc = _("生命力を吸収できるようになった。", "You become vampiric.");
        break;
    case 21:
    case 22:
    case 23:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_SMELL_MET;
        gm_ptr->muta_desc = _("金属の匂いを嗅ぎ分けられるようになった。", "You smell a metallic odor.");
        break;
    case 24:
    case 25:
    case 26:
    case 27:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_SMELL_MON;
        gm_ptr->muta_desc = _("モンスターの臭いを嗅ぎ分けられるようになった。", "You smell filthy monsters.");
        break;
    case 28:
    case 29:
    case 30:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_BLINK;
        gm_ptr->muta_desc = _("近距離テレポートの能力を得た。", "You gain the power of minor teleportation.");
        break;
    case 31:
    case 32:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_EAT_ROCK;
        gm_ptr->muta_desc = _("壁が美味しそうに見える。", "The walls look delicious.");
        break;
    case 33:
    case 34:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_SWAP_POS;
        gm_ptr->muta_desc = _("他人の靴で一マイル歩くような気分がする。", "You feel like walking a mile in someone else's shoes.");
        break;
    case 35:
    case 36:
    case 37:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_SHRIEK;
        gm_ptr->muta_desc = _("あなたの声は相当強くなった。", "Your vocal cords get much tougher.");
        break;
    case 38:
    case 39:
    case 40:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_ILLUMINE;
        gm_ptr->muta_desc = _("あなたは光り輝いて部屋を明るくするようになった。", "You can light up rooms with your presence.");
        break;
    case 41:
    case 42:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_DET_CURSE;
        gm_ptr->muta_desc = _("邪悪な魔法を感知できるようになった。", "You can feel evil magics.");
        break;
    case 43:
    case 44:
    case 45:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_BERSERK;
        gm_ptr->muta_desc = _("制御できる激情を感じる。", "You feel a controlled rage.");
        break;
    case 46:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_POLYMORPH;
        gm_ptr->muta_desc = _("体が変異しやすくなった。", "Your body seems mutable.");
        break;
    case 47:
    case 48:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_MIDAS_TCH;
        gm_ptr->muta_desc = _("「ミダス王の手」の能力を得た。", "You gain the Midas touch."); /*トゥームレイダースにありましたね。 */
        break;
    case 49:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_GROW_MOLD;
        gm_ptr->muta_desc = _("突然カビに親しみを覚えた。", "You feel a sudden affinity for mold.");
        break;
    case 50:
    case 51:
    case 52:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_RESIST;
        gm_ptr->muta_desc = _("あなたは自分自身を守れる気がする。", "You feel like you can protect yourself.");
        break;
    case 53:
    case 54:
    case 55:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_EARTHQUAKE;
        gm_ptr->muta_desc = _("ダンジョンを破壊する能力を得た。", "You gain the ability to wreck the dungeon.");
        break;
    case 56:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_EAT_MAGIC;
        gm_ptr->muta_desc = _("魔法のアイテムが美味そうに見える。", "Your magic items look delicious.");
        break;
    case 57:
    case 58:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_WEIGH_MAG;
        gm_ptr->muta_desc = _("あなたは周囲にある魔法をより良く理解できる気がする。", "You feel you can better understand the magic around you.");
        break;
    case 59:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_STERILITY;
        gm_ptr->muta_desc = _("周りの全ての者に頭痛を起こすことができる。", "You can give everything around you a headache.");
        break;
    case 60:
    case 61:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_HIT_AND_AWAY;
        gm_ptr->muta_desc = _("突然、泥棒の気分が分かるようになった。", "You suddenly understand how thieves feel.");
        break;
    case 62:
    case 63:
    case 64:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_DAZZLE;
        gm_ptr->muta_desc = _("眩い閃光を発する能力を得た。", "You gain the ability to emit dazzling lights.");
        break;
    case 65:
    case 66:
    case 67:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_LASER_EYE;
        gm_ptr->muta_desc = _("あなたの目は一瞬焼け付いた。", "Your eyes burn for a moment.");
        break;
    case 68:
    case 69:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_RECALL;
        gm_ptr->muta_desc = _("少しだけホームシックになったが、すぐ直った。", "You feel briefly homesick, but it passes.");
        break;
    case 70:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_BANISH;
        gm_ptr->muta_desc = _("神聖な怒りの力に満たされた。", "You feel a holy wrath fill you.");
        break;
    case 71:
    case 72:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_COLD_TOUCH;
        gm_ptr->muta_desc = _("あなたの両手はとても冷たくなった。", "Your hands get very cold.");
        break;
    case 73:
    case 74:
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_LAUNCHER;
        gm_ptr->muta_desc = _("あなたの物を投げる手はかなり強くなった気がする。", "Your throwing arm feels much stronger.");
        break;
    case 75:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_BERS_RAGE;
        gm_ptr->muta_desc = _("あなたは狂暴化の発作を起こすようになった！", "You become subject to fits of berserk rage!");
        break;
    case 76:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_COWARDICE;
        gm_ptr->muta_desc = _("信じられないくらい臆病になった！", "You become an incredible coward!");
        break;
    case 77:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_RTELEPORT;
        gm_ptr->muta_desc = _("あなたの位置は非常に不確定になった。", "Your position seems very uncertain...");
        break;
    case 78:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_ALCOHOL;
        gm_ptr->muta_desc = _("あなたはアルコールを分泌するようになった。", "Your body starts producing alcohol!");
        break;
    case 79:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_HALLU;
        gm_ptr->muta_desc = _("あなたは幻覚を引き起こす精神錯乱に侵された。", "You are afflicted by a hallucinatory insanity!");
        break;
    case 80:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_FLATULENT;
        gm_ptr->muta_desc = _("あなたは制御不能な強烈な屁をこくようになった。", "You become subject to uncontrollable flatulence.");
        break;
    case 81:
    case 82:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_SCOR_TAIL;
        gm_ptr->muta_desc = _("サソリの尻尾が生えてきた！", "You grow a scorpion tail!");
        break;
    case 83:
    case 84:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_HORNS;
        gm_ptr->muta_desc = _("額に角が生えた！", "Horns pop forth into your forehead!");
        break;
    case 85:
    case 86:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_BEAK;
        gm_ptr->muta_desc = _("口が鋭く強いクチバシに変化した！", "Your mouth turns into a sharp, powerful beak!");
        break;
    case 87:
    case 88:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_ATT_DEMON;
        gm_ptr->muta_desc = _("悪魔を引き付けるようになった。", "You start attracting demons.");
        break;
    case 89:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_PROD_MANA;
        gm_ptr->muta_desc = _("あなたは制御不能な魔法のエネルギーを発生するようになった。", "You start producing magical energy uncontrollably.");
        break;
    case 90:
    case 91:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_SPEED_FLUX;
        gm_ptr->muta_desc = _("あなたは躁鬱質になった。", "You become manic-depressive.");
        break;
    case 92:
    case 93:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_BANISH_ALL;
        gm_ptr->muta_desc = _("恐ろしい力があなたの背後に潜んでいる気がする。", "You feel a terrifying power lurking behind you.");
        break;
    case 94:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_EAT_LIGHT;
        gm_ptr->muta_desc = _("あなたはウンゴリアントに奇妙な親しみを覚えるようになった。", "You feel a strange kinship with Ungoliant.");
        break;
    case 95:
    case 96:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_TRUNK;
        gm_ptr->muta_desc = _("あなたの鼻は伸びて象の鼻のようになった。", "Your nose grows into an elephant-like trunk.");
        break;
    case 97:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_ATT_ANIMAL;
        gm_ptr->muta_desc = _("動物を引き付けるようになった。", "You start attracting animals.");
        break;
    case 98:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_TENTACLES;
        gm_ptr->muta_desc = _("邪悪な触手が体の両側に生えてきた。", "Evil-looking tentacles sprout from your sides.");
        break;
    case 99:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_RAW_CHAOS;
        gm_ptr->muta_desc = _("周囲の空間が不安定になった気がする。", "You feel the universe is less stable around you.");
        break;
    case 100:
    case 101:
    case 102:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_NORMALITY;
        gm_ptr->muta_desc = _("あなたは奇妙なほど普通になった気がする。", "You feel strangely normal.");
        break;
    case 103:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_WRAITH;
        gm_ptr->muta_desc = _("あなたは幽体化したり実体化したりするようになった。", "You start to fade in and out of the physical world.");
        break;
    case 104:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_POLY_WOUND;
        gm_ptr->muta_desc = _("あなたはカオスの力が古い傷に入り込んでくるのを感じた。", "You feel forces of chaos entering your old scars.");
        break;
    case 105:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_WASTING;
        gm_ptr->muta_desc = _("あなたは突然おぞましい衰弱病にかかった。", "You suddenly contract a horrible wasting disease.");
        break;
    case 106:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_ATT_DRAGON;
        gm_ptr->muta_desc = _("あなたはドラゴンを引きつけるようになった。", "You start attracting dragons.");
        break;
    case 107:
    case 108:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_WEIRD_MIND;
        gm_ptr->muta_desc = _("あなたの思考は突然おかしな方向に向き始めた。", "Your thoughts suddenly take off in strange directions.");
        break;
    case 109:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_NAUSEA;
        gm_ptr->muta_desc = _("胃袋がピクピクしはじめた。", "Your stomach starts to roil nauseously.");
        break;
    case 110:
    case 111:
        if (creature_ptr->pclass == CLASS_CHAOS_WARRIOR)
            break;

        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_CHAOS_GIFT;
        gm_ptr->muta_desc = _("あなたはカオスの守護悪魔の注意を惹くようになった。", "You attract the notice of a chaos deity!");
        break;
    case 112:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_WALK_SHAD;
        gm_ptr->muta_desc = _("あなたは現実が紙のように薄いと感じるようになった。", "You feel like reality is as thin as paper.");
        break;
    case 113:
    case 114:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_WARNING;
        gm_ptr->muta_desc = _("あなたは突然パラノイアになった気がする。", "You suddenly feel paranoid.");
        break;
    case 115:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_INVULN;
        gm_ptr->muta_desc = _("あなたは祝福され、無敵状態になる発作を起こすようになった。", "You are blessed with fits of invulnerability.");
        break;
    case 116:
    case 117:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_SP_TO_HP;
        gm_ptr->muta_desc = _("魔法の治癒の発作を起こすようになった。", "You are subject to fits of magical healing.");
        break;
    case 118:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_HP_TO_SP;
        gm_ptr->muta_desc = _("痛みを伴う精神明瞭化の発作を起こすようになった。", "You are subject to fits of painful clarity.");
        break;
    case 119:
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_DISARM;
        gm_ptr->muta_desc = _("あなたの脚は長さが四倍になった。", "Your feet grow to four times their former size.");
        break;
    case 120:
    case 121:
    case 122:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_HYPER_STR;
        gm_ptr->muta_desc = _("超人的に強くなった！", "You turn into a superhuman he-man!");
        break;
    case 123:
    case 124:
    case 125:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_PUNY;
        gm_ptr->muta_desc = _("筋肉が弱ってしまった...", "Your muscles wither away...");
        break;
    case 126:
    case 127:
    case 128:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_HYPER_INT;
        gm_ptr->muta_desc = _("あなたの脳は生体コンピュータに進化した！", "Your brain evolves into a living computer!");
        break;
    case 129:
    case 130:
    case 131:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_MORONIC;
        gm_ptr->muta_desc = _("脳が萎縮してしまった...", "Your brain withers away...");
        break;
    case 132:
    case 133:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_RESILIENT;
        gm_ptr->muta_desc = _("並外れてタフになった。", "You become extraordinarily resilient.");
        break;
    case 134:
    case 135:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_XTRA_FAT;
        gm_ptr->muta_desc = _("あなたは気持ち悪いくらい太った！", "You become sickeningly fat!");
        break;
    case 136:
    case 137:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_ALBINO;
        gm_ptr->muta_desc = _("アルビノになった！弱くなった気がする...", "You turn into an albino! You feel frail...");
        break;
    case 138:
    case 139:
    case 140:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_FLESH_ROT;
        gm_ptr->muta_desc = _("あなたの肉体は腐敗する病気に侵された！", "Your flesh is afflicted by a rotting disease!");
        break;
    case 141:
    case 142:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_SILLY_VOI;
        gm_ptr->muta_desc = _("声が間抜けなキーキー声になった！", "Your voice turns into a ridiculous squeak!");
        break;
    case 143:
    case 144:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_BLANK_FAC;
        gm_ptr->muta_desc = _("のっぺらぼうになった！", "Your face becomes completely featureless!");
        break;
    case 145:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_ILL_NORM;
        gm_ptr->muta_desc = _("心の安らぐ幻影を映し出すようになった。", "You start projecting a reassuring image.");
        break;
    case 146:
    case 147:
    case 148:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_XTRA_EYES;
        gm_ptr->muta_desc = _("新たに二つの目が出来た！", "You grow an extra pair of eyes!");
        break;
    case 149:
    case 150:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_MAGIC_RES;
        gm_ptr->muta_desc = _("魔法への耐性がついた。", "You become resistant to magic.");
        break;
    case 151:
    case 152:
    case 153:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_XTRA_NOIS;
        gm_ptr->muta_desc = _("あなたは奇妙な音を立て始めた！", "You start making strange noise!");
        break;
    case 154:
    case 155:
    case 156:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_INFRAVIS;
        gm_ptr->muta_desc = _("赤外線視力が増した。", "Your infravision is improved.");
        break;
    case 157:
    case 158:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_XTRA_LEGS;
        gm_ptr->muta_desc = _("新たに二本の足が生えてきた！", "You grow an extra pair of legs!");
        break;
    case 159:
    case 160:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_SHORT_LEG;
        gm_ptr->muta_desc = _("足が短い突起になってしまった！", "Your legs turn into short stubs!");
        break;
    case 161:
    case 162:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_ELEC_TOUC;
        gm_ptr->muta_desc = _("血管を電流が流れ始めた！", "Electricity starts running through you!");
        break;
    case 163:
    case 164:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_FIRE_BODY;
        gm_ptr->muta_desc = _("あなたの体は炎につつまれている。", "Your body is enveloped in flames!");
        break;
    case 165:
    case 166:
    case 167:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_WART_SKIN;
        gm_ptr->muta_desc = _("気持ち悪いイボイボが体中にできた！", "Disgusting warts appear everywhere on you!");
        break;
    case 168:
    case 169:
    case 170:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_SCALES;
        gm_ptr->muta_desc = _("肌が黒い鱗に変わった！", "Your skin turns into black scales!");
        break;
    case 171:
    case 172:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_IRON_SKIN;
        gm_ptr->muta_desc = _("あなたの肌は鉄になった！", "Your skin turns to steel!");
        break;
    case 173:
    case 174:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_WINGS;
        gm_ptr->muta_desc = _("背中に羽が生えた。", "You grow a pair of wings.");
        break;
    case 175:
    case 176:
    case 177:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_FEARLESS;
        gm_ptr->muta_desc = _("完全に怖れ知らずになった。", "You become completely fearless.");
        break;
    case 178:
    case 179:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_REGEN;
        gm_ptr->muta_desc = _("急速に回復し始めた。", "You start regenerating.");
        break;
    case 180:
    case 181:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_ESP;
        gm_ptr->muta_desc = _("テレパシーの能力を得た！", "You develop a telepathic ability!");
        break;
    case 182:
    case 183:
    case 184:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_LIMBER;
        gm_ptr->muta_desc = _("筋肉がしなやかになった。", "Your muscles become limber.");
        break;
    case 185:
    case 186:
    case 187:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_ARTHRITIS;
        gm_ptr->muta_desc = _("関節が突然痛み出した。", "Your joints suddenly hurt.");
        break;
    case 188:
        if (creature_ptr->pseikaku == PERSONALITY_LUCKY)
            break;

        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_BAD_LUCK;
        gm_ptr->muta_desc = _("悪意に満ちた黒いオーラがあなたをとりまいた...", "There is a malignant black aura surrounding you...");
        break;
    case 189:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_VULN_ELEM;
        gm_ptr->muta_desc = _("妙に無防備になった気がする。", "You feel strangely exposed.");
        break;
    case 190:
    case 191:
    case 192:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_MOTION;
        gm_ptr->muta_desc = _("体の動作がより正確になった。", "You move with new assurance.");
        break;
    case 193:
        gm_ptr->muta_class = &(creature_ptr->muta3);
        gm_ptr->muta_which = MUT3_GOOD_LUCK;
        gm_ptr->muta_desc = _("慈悲深い白いオーラがあなたをとりまいた...", "There is a benevolent white aura surrounding you...");
        break;
    default:
        gm_ptr->muta_class = NULL;
        gm_ptr->muta_which = 0;
        break;
    }
}
