#include "mutation/gain-mutation-switcher.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-util.h"
#include "system/player-type-definition.h"

void switch_gain_mutation(player_type *player_ptr, glm_type *glm_ptr)
{
    switch (glm_ptr->choose_mut ? glm_ptr->choose_mut : (player_ptr->pclass == PlayerClassType::BERSERKER ? 74 + randint1(119) : randint1(193))) {
    case 1:
    case 2:
    case 3:
    case 4:
        glm_ptr->muta_which = MUTA::SPIT_ACID;
        glm_ptr->muta_desc = _("酸を吐く能力を得た。", "You gain the ability to spit acid.");
        break;
    case 5:
    case 6:
    case 7:
        glm_ptr->muta_which = MUTA::BR_FIRE;
        glm_ptr->muta_desc = _("火を吐く能力を得た。", "You gain the ability to breathe fire.");
        break;
    case 8:
    case 9:
        glm_ptr->muta_which = MUTA::HYPN_GAZE;
        glm_ptr->muta_desc = _("催眠眼の能力を得た。", "Your eyes look mesmerizing...");
        break;
    case 10:
    case 11:
        glm_ptr->muta_which = MUTA::TELEKINES;
        glm_ptr->muta_desc = _("物体を念動力で動かす能力を得た。", "You gain the ability to move objects telekinetically.");
        break;
    case 12:
    case 13:
    case 14:
        glm_ptr->muta_which = MUTA::VTELEPORT;
        glm_ptr->muta_desc = _("自分の意思でテレポートする能力を得た。", "You gain the power of teleportation at will.");
        break;
    case 15:
    case 16:
        glm_ptr->muta_which = MUTA::MIND_BLST;
        glm_ptr->muta_desc = _("精神攻撃の能力を得た。", "You gain the power of Mind Blast.");
        break;
    case 17:
    case 18:
        glm_ptr->muta_which = MUTA::RADIATION;
        glm_ptr->muta_desc = _("あなたは強い放射線を発生し始めた。", "You start emitting hard radiation.");
        break;
    case 19:
    case 20:
        glm_ptr->muta_which = MUTA::VAMPIRISM;
        glm_ptr->muta_desc = _("生命力を吸収できるようになった。", "You become vampiric.");
        break;
    case 21:
    case 22:
    case 23:
        glm_ptr->muta_which = MUTA::SMELL_MET;
        glm_ptr->muta_desc = _("金属の匂いを嗅ぎ分けられるようになった。", "You smell a metallic odor.");
        break;
    case 24:
    case 25:
    case 26:
    case 27:
        glm_ptr->muta_which = MUTA::SMELL_MON;
        glm_ptr->muta_desc = _("モンスターの臭いを嗅ぎ分けられるようになった。", "You smell filthy monsters.");
        break;
    case 28:
    case 29:
    case 30:
        glm_ptr->muta_which = MUTA::BLINK;
        glm_ptr->muta_desc = _("近距離テレポートの能力を得た。", "You gain the power of minor teleportation.");
        break;
    case 31:
    case 32:
        glm_ptr->muta_which = MUTA::EAT_ROCK;
        glm_ptr->muta_desc = _("壁が美味しそうに見える。", "The walls look delicious.");
        break;
    case 33:
    case 34:
        glm_ptr->muta_which = MUTA::SWAP_POS;
        glm_ptr->muta_desc = _("他人の靴で一マイル歩くような気分がする。", "You feel like walking a mile in someone else's shoes.");
        break;
    case 35:
    case 36:
    case 37:
        glm_ptr->muta_which = MUTA::SHRIEK;
        glm_ptr->muta_desc = _("あなたの声は相当強くなった。", "Your vocal cords get much tougher.");
        break;
    case 38:
    case 39:
    case 40:
        glm_ptr->muta_which = MUTA::ILLUMINE;
        glm_ptr->muta_desc = _("あなたは光り輝いて部屋を明るくするようになった。", "You can light up rooms with your presence.");
        break;
    case 41:
    case 42:
        glm_ptr->muta_which = MUTA::DET_CURSE;
        glm_ptr->muta_desc = _("邪悪な魔法を感知できるようになった。", "You can feel evil magics.");
        break;
    case 43:
    case 44:
    case 45:
        glm_ptr->muta_which = MUTA::BERSERK;
        glm_ptr->muta_desc = _("制御できる激情を感じる。", "You feel a controlled rage.");
        break;
    case 46:
        glm_ptr->muta_which = MUTA::POLYMORPH;
        glm_ptr->muta_desc = _("体が変異しやすくなった。", "Your body seems mutable.");
        break;
    case 47:
    case 48:
        glm_ptr->muta_which = MUTA::MIDAS_TCH;
        glm_ptr->muta_desc = _("「ミダス王の手」の能力を得た。", "You gain the Midas touch."); /*トゥームレイダースにありましたね。 */
        break;
    case 49:
        glm_ptr->muta_which = MUTA::GROW_MOLD;
        glm_ptr->muta_desc = _("突然カビに親しみを覚えた。", "You feel a sudden affinity for mold.");
        break;
    case 50:
    case 51:
    case 52:
        glm_ptr->muta_which = MUTA::RESIST;
        glm_ptr->muta_desc = _("あなたは自分自身を守れる気がする。", "You feel like you can protect yourself.");
        break;
    case 53:
    case 54:
    case 55:
        glm_ptr->muta_which = MUTA::EARTHQUAKE;
        glm_ptr->muta_desc = _("ダンジョンを破壊する能力を得た。", "You gain the ability to wreck the dungeon.");
        break;
    case 56:
        glm_ptr->muta_which = MUTA::EAT_MAGIC;
        glm_ptr->muta_desc = _("魔法のアイテムが美味そうに見える。", "Your magic items look delicious.");
        break;
    case 57:
    case 58:
        glm_ptr->muta_which = MUTA::WEIGH_MAG;
        glm_ptr->muta_desc = _("あなたは周囲にある魔法をより良く理解できる気がする。", "You feel you can better understand the magic around you.");
        break;
    case 59:
        glm_ptr->muta_which = MUTA::STERILITY;
        glm_ptr->muta_desc = _("周りの全ての者に頭痛を起こすことができる。", "You can give everything around you a headache.");
        break;
    case 60:
    case 61:
        glm_ptr->muta_which = MUTA::HIT_AND_AWAY;
        glm_ptr->muta_desc = _("突然、泥棒の気分が分かるようになった。", "You suddenly understand how thieves feel.");
        break;
    case 62:
    case 63:
    case 64:
        glm_ptr->muta_which = MUTA::DAZZLE;
        glm_ptr->muta_desc = _("眩い閃光を発する能力を得た。", "You gain the ability to emit dazzling lights.");
        break;
    case 65:
    case 66:
    case 67:
        glm_ptr->muta_which = MUTA::LASER_EYE;
        glm_ptr->muta_desc = _("あなたの目は一瞬焼け付いた。", "Your eyes burn for a moment.");
        break;
    case 68:
    case 69:
        glm_ptr->muta_which = MUTA::RECALL;
        glm_ptr->muta_desc = _("少しだけホームシックになったが、すぐ直った。", "You feel briefly homesick, but it passes.");
        break;
    case 70:
        glm_ptr->muta_which = MUTA::BANISH;
        glm_ptr->muta_desc = _("神聖な怒りの力に満たされた。", "You feel a holy wrath fill you.");
        break;
    case 71:
    case 72:
        glm_ptr->muta_which = MUTA::COLD_TOUCH;
        glm_ptr->muta_desc = _("あなたの両手はとても冷たくなった。", "Your hands get very cold.");
        break;
    case 73:
    case 74:
        glm_ptr->muta_which = MUTA::LAUNCHER;
        glm_ptr->muta_desc = _("あなたの物を投げる手はかなり強くなった気がする。", "Your throwing arm feels much stronger.");
        break;
    case 75:
        glm_ptr->muta_which = MUTA::BERS_RAGE;
        glm_ptr->muta_desc = _("あなたは狂暴化の発作を起こすようになった！", "You become subject to fits of berserk rage!");
        break;
    case 76:
        glm_ptr->muta_which = MUTA::COWARDICE;
        glm_ptr->muta_desc = _("信じられないくらい臆病になった！", "You become an incredible coward!");
        break;
    case 77:
        glm_ptr->muta_which = MUTA::RTELEPORT;
        glm_ptr->muta_desc = _("あなたの位置は非常に不確定になった。", "Your position seems very uncertain...");
        break;
    case 78:
        glm_ptr->muta_which = MUTA::ALCOHOL;
        glm_ptr->muta_desc = _("あなたはアルコールを分泌するようになった。", "Your body starts producing alcohol!");
        break;
    case 79:
        glm_ptr->muta_which = MUTA::HALLU;
        glm_ptr->muta_desc = _("あなたは幻覚を引き起こす精神錯乱に侵された。", "You are afflicted by a hallucinatory insanity!");
        break;
    case 80:
        glm_ptr->muta_which = MUTA::FLATULENT;
        glm_ptr->muta_desc = _("あなたは制御不能な強烈な屁をこくようになった。", "You become subject to uncontrollable flatulence.");
        break;
    case 81:
    case 82:
        glm_ptr->muta_which = MUTA::SCOR_TAIL;
        glm_ptr->muta_desc = _("サソリの尻尾が生えてきた！", "You grow a scorpion tail!");
        break;
    case 83:
    case 84:
        glm_ptr->muta_which = MUTA::HORNS;
        glm_ptr->muta_desc = _("額に角が生えた！", "Horns pop forth into your forehead!");
        break;
    case 85:
    case 86:
        glm_ptr->muta_which = MUTA::BEAK;
        glm_ptr->muta_desc = _("口が鋭く強いクチバシに変化した！", "Your mouth turns into a sharp, powerful beak!");
        break;
    case 87:
    case 88:
        glm_ptr->muta_which = MUTA::ATT_DEMON;
        glm_ptr->muta_desc = _("悪魔を引き付けるようになった。", "You start attracting demons.");
        break;
    case 89:
        glm_ptr->muta_which = MUTA::PROD_MANA;
        glm_ptr->muta_desc = _("あなたは制御不能な魔法のエネルギーを発生するようになった。", "You start producing magical energy uncontrollably.");
        break;
    case 90:
    case 91:
        glm_ptr->muta_which = MUTA::SPEED_FLUX;
        glm_ptr->muta_desc = _("あなたは躁鬱質になった。", "You become manic-depressive.");
        break;
    case 92:
    case 93:
        glm_ptr->muta_which = MUTA::BANISH_ALL;
        glm_ptr->muta_desc = _("恐ろしい力があなたの背後に潜んでいる気がする。", "You feel a terrifying power lurking behind you.");
        break;
    case 94:
        glm_ptr->muta_which = MUTA::EAT_LIGHT;
        glm_ptr->muta_desc = _("あなたはウンゴリアントに奇妙な親しみを覚えるようになった。", "You feel a strange kinship with Ungoliant.");
        break;
    case 95:
    case 96:
        glm_ptr->muta_which = MUTA::TRUNK;
        glm_ptr->muta_desc = _("あなたの鼻は伸びて象の鼻のようになった。", "Your nose grows into an elephant-like trunk.");
        break;
    case 97:
        glm_ptr->muta_which = MUTA::ATT_ANIMAL;
        glm_ptr->muta_desc = _("動物を引き付けるようになった。", "You start attracting animals.");
        break;
    case 98:
        glm_ptr->muta_which = MUTA::TENTACLES;
        glm_ptr->muta_desc = _("邪悪な触手が体の両側に生えてきた。", "Evil-looking tentacles sprout from your sides.");
        break;
    case 99:
        glm_ptr->muta_which = MUTA::RAW_CHAOS;
        glm_ptr->muta_desc = _("周囲の空間が不安定になった気がする。", "You feel the universe is less stable around you.");
        break;
    case 100:
    case 101:
    case 102:
        glm_ptr->muta_which = MUTA::NORMALITY;
        glm_ptr->muta_desc = _("あなたは奇妙なほど普通になった気がする。", "You feel strangely normal.");
        break;
    case 103:
        glm_ptr->muta_which = MUTA::WRAITH;
        glm_ptr->muta_desc = _("あなたは幽体化したり実体化したりするようになった。", "You start to fade in and out of the physical world.");
        break;
    case 104:
        glm_ptr->muta_which = MUTA::POLY_WOUND;
        glm_ptr->muta_desc = _("あなたはカオスの力が古い傷に入り込んでくるのを感じた。", "You feel forces of chaos entering your old scars.");
        break;
    case 105:
        glm_ptr->muta_which = MUTA::WASTING;
        glm_ptr->muta_desc = _("あなたは突然おぞましい衰弱病にかかった。", "You suddenly contract a horrible wasting disease.");
        break;
    case 106:
        glm_ptr->muta_which = MUTA::ATT_DRAGON;
        glm_ptr->muta_desc = _("あなたはドラゴンを引きつけるようになった。", "You start attracting dragons.");
        break;
    case 107:
    case 108:
        glm_ptr->muta_which = MUTA::WEIRD_MIND;
        glm_ptr->muta_desc = _("あなたの思考は突然おかしな方向に向き始めた。", "Your thoughts suddenly take off in strange directions.");
        break;
    case 109:
        glm_ptr->muta_which = MUTA::NAUSEA;
        glm_ptr->muta_desc = _("胃袋がピクピクしはじめた。", "Your stomach starts to roil nauseously.");
        break;
    case 110:
    case 111:
        if (player_ptr->pclass == PlayerClassType::CHAOS_WARRIOR)
            break;

        glm_ptr->muta_which = MUTA::CHAOS_GIFT;
        glm_ptr->muta_desc = _("あなたはカオスの守護悪魔の注意を惹くようになった。", "You attract the notice of a chaos deity!");
        break;
    case 112:
        glm_ptr->muta_which = MUTA::WALK_SHAD;
        glm_ptr->muta_desc = _("あなたは現実が紙のように薄いと感じるようになった。", "You feel like reality is as thin as paper.");
        break;
    case 113:
    case 114:
        glm_ptr->muta_which = MUTA::WARNING;
        glm_ptr->muta_desc = _("あなたは突然パラノイアになった気がする。", "You suddenly feel paranoid.");
        break;
    case 115:
        glm_ptr->muta_which = MUTA::INVULN;
        glm_ptr->muta_desc = _("あなたは祝福され、無敵状態になる発作を起こすようになった。", "You are blessed with fits of invulnerability.");
        break;
    case 116:
    case 117:
        glm_ptr->muta_which = MUTA::SP_TO_HP;
        glm_ptr->muta_desc = _("魔法の治癒の発作を起こすようになった。", "You are subject to fits of magical healing.");
        break;
    case 118:
        glm_ptr->muta_which = MUTA::HP_TO_SP;
        glm_ptr->muta_desc = _("痛みを伴う精神明瞭化の発作を起こすようになった。", "You are subject to fits of painful clarity.");
        break;
    case 119:
        glm_ptr->muta_which = MUTA::DISARM;
        glm_ptr->muta_desc = _("あなたの脚は長さが四倍になった。", "Your feet grow to four times their former size.");
        break;
    case 120:
    case 121:
    case 122:
        glm_ptr->muta_which = MUTA::HYPER_STR;
        glm_ptr->muta_desc = _("超人的に強くなった！", "You turn into a superhuman he-man!");
        break;
    case 123:
    case 124:
    case 125:
        glm_ptr->muta_which = MUTA::PUNY;
        glm_ptr->muta_desc = _("筋肉が弱ってしまった...", "Your muscles wither away...");
        break;
    case 126:
    case 127:
    case 128:
        glm_ptr->muta_which = MUTA::HYPER_INT;
        glm_ptr->muta_desc = _("あなたの脳は生体コンピュータに進化した！", "Your brain evolves into a living computer!");
        break;
    case 129:
    case 130:
    case 131:
        glm_ptr->muta_which = MUTA::MORONIC;
        glm_ptr->muta_desc = _("脳が萎縮してしまった...", "Your brain withers away...");
        break;
    case 132:
    case 133:
        glm_ptr->muta_which = MUTA::RESILIENT;
        glm_ptr->muta_desc = _("並外れてタフになった。", "You become extraordinarily resilient.");
        break;
    case 134:
    case 135:
        glm_ptr->muta_which = MUTA::XTRA_FAT;
        glm_ptr->muta_desc = _("あなたは気持ち悪いくらい太った！", "You become sickeningly fat!");
        break;
    case 136:
    case 137:
        glm_ptr->muta_which = MUTA::ALBINO;
        glm_ptr->muta_desc = _("アルビノになった！弱くなった気がする...", "You turn into an albino! You feel frail...");
        break;
    case 138:
    case 139:
    case 140:
        glm_ptr->muta_which = MUTA::FLESH_ROT;
        glm_ptr->muta_desc = _("あなたの肉体は腐敗する病気に侵された！", "Your flesh is afflicted by a rotting disease!");
        break;
    case 141:
    case 142:
        glm_ptr->muta_which = MUTA::SILLY_VOI;
        glm_ptr->muta_desc = _("声が間抜けなキーキー声になった！", "Your voice turns into a ridiculous squeak!");
        break;
    case 143:
    case 144:
        glm_ptr->muta_which = MUTA::BLANK_FAC;
        glm_ptr->muta_desc = _("のっぺらぼうになった！", "Your face becomes completely featureless!");
        break;
    case 145:
        glm_ptr->muta_which = MUTA::ILL_NORM;
        glm_ptr->muta_desc = _("心の安らぐ幻影を映し出すようになった。", "You start projecting a reassuring image.");
        break;
    case 146:
    case 147:
    case 148:
        glm_ptr->muta_which = MUTA::XTRA_EYES;
        glm_ptr->muta_desc = _("新たに二つの目が出来た！", "You grow an extra pair of eyes!");
        break;
    case 149:
    case 150:
        glm_ptr->muta_which = MUTA::MAGIC_RES;
        glm_ptr->muta_desc = _("魔法への耐性がついた。", "You become resistant to magic.");
        break;
    case 151:
    case 152:
    case 153:
        glm_ptr->muta_which = MUTA::XTRA_NOIS;
        glm_ptr->muta_desc = _("あなたは奇妙な音を立て始めた！", "You start making strange noise!");
        break;
    case 154:
    case 155:
    case 156:
        glm_ptr->muta_which = MUTA::INFRAVIS;
        glm_ptr->muta_desc = _("赤外線視力が増した。", "Your infravision is improved.");
        break;
    case 157:
    case 158:
        glm_ptr->muta_which = MUTA::XTRA_LEGS;
        glm_ptr->muta_desc = _("新たに二本の足が生えてきた！", "You grow an extra pair of legs!");
        break;
    case 159:
    case 160:
        glm_ptr->muta_which = MUTA::SHORT_LEG;
        glm_ptr->muta_desc = _("足が短い突起になってしまった！", "Your legs turn into short stubs!");
        break;
    case 161:
    case 162:
        glm_ptr->muta_which = MUTA::ELEC_TOUC;
        glm_ptr->muta_desc = _("血管を電流が流れ始めた！", "Electricity starts running through you!");
        break;
    case 163:
    case 164:
        glm_ptr->muta_which = MUTA::FIRE_BODY;
        glm_ptr->muta_desc = _("あなたの体は炎につつまれている。", "Your body is enveloped in flames!");
        break;
    case 165:
    case 166:
    case 167:
        glm_ptr->muta_which = MUTA::WART_SKIN;
        glm_ptr->muta_desc = _("気持ち悪いイボイボが体中にできた！", "Disgusting warts appear everywhere on you!");
        break;
    case 168:
    case 169:
    case 170:
        glm_ptr->muta_which = MUTA::SCALES;
        glm_ptr->muta_desc = _("肌が黒い鱗に変わった！", "Your skin turns into black scales!");
        break;
    case 171:
    case 172:
        glm_ptr->muta_which = MUTA::IRON_SKIN;
        glm_ptr->muta_desc = _("あなたの肌は鉄になった！", "Your skin turns to steel!");
        break;
    case 173:
    case 174:
        glm_ptr->muta_which = MUTA::WINGS;
        glm_ptr->muta_desc = _("背中に羽が生えた。", "You grow a pair of wings.");
        break;
    case 175:
    case 176:
    case 177:
        glm_ptr->muta_which = MUTA::FEARLESS;
        glm_ptr->muta_desc = _("完全に怖れ知らずになった。", "You become completely fearless.");
        break;
    case 178:
    case 179:
        glm_ptr->muta_which = MUTA::REGEN;
        glm_ptr->muta_desc = _("急速に回復し始めた。", "You start regenerating.");
        break;
    case 180:
    case 181:
        glm_ptr->muta_which = MUTA::ESP;
        glm_ptr->muta_desc = _("テレパシーの能力を得た！", "You develop a telepathic ability!");
        break;
    case 182:
    case 183:
    case 184:
        glm_ptr->muta_which = MUTA::LIMBER;
        glm_ptr->muta_desc = _("筋肉がしなやかになった。", "Your muscles become limber.");
        break;
    case 185:
    case 186:
    case 187:
        glm_ptr->muta_which = MUTA::ARTHRITIS;
        glm_ptr->muta_desc = _("関節が突然痛み出した。", "Your joints suddenly hurt.");
        break;
    case 188:
        if (player_ptr->ppersonality == PERSONALITY_LUCKY)
            break;

        glm_ptr->muta_which = MUTA::BAD_LUCK;
        glm_ptr->muta_desc = _("悪意に満ちた黒いオーラがあなたをとりまいた...", "There is a malignant black aura surrounding you...");
        break;
    case 189:
        glm_ptr->muta_which = MUTA::VULN_ELEM;
        glm_ptr->muta_desc = _("妙に無防備になった気がする。", "You feel strangely exposed.");
        break;
    case 190:
    case 191:
    case 192:
        glm_ptr->muta_which = MUTA::MOTION;
        glm_ptr->muta_desc = _("体の動作がより正確になった。", "You move with new assurance.");
        break;
    case 193:
        glm_ptr->muta_which = MUTA::GOOD_LUCK;
        glm_ptr->muta_desc = _("慈悲深い白いオーラがあなたをとりまいた...", "There is a benevolent white aura surrounding you...");
        break;
    default:
        glm_ptr->muta_which = MUTA::MAX;
        break;
    }
}
