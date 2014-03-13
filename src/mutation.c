/*!
 * @file mutation.c
 * @brief 突然変異ルールの実装 / Mutation effects (and racial powers)
 * @date 2014/01/11
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 *\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 */

#include "angband.h"

/*!
 * @brief プレイヤーに突然変異を与える
 * @param choose_mut 与えたい突然変異のID、0ならばランダムに選択
 * @return なし
 */
bool gain_random_mutation(int choose_mut)
{
	int     attempts_left = 20;
	cptr    muta_desc = "";
	bool    muta_chosen = FALSE;
	u32b    muta_which = 0;
	u32b    *muta_class = NULL;

	if (choose_mut) attempts_left = 1;

	while (attempts_left--)
	{
		switch (choose_mut ? choose_mut : (p_ptr->pclass == CLASS_BERSERKER ? 74+randint1(119) : randint1(193)))
		{
		case 1: case 2: case 3: case 4:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_SPIT_ACID;
			muta_desc = _("酸を吐く能力を得た。", "You gain the ability to spit acid.");
			break;
			
		case 5: case 6: case 7:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_BR_FIRE;
			muta_desc = _("火を吐く能力を得た。", "You gain the ability to breathe fire.");
			break;
			
		case 8: case 9:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_HYPN_GAZE;
			muta_desc = _("催眠眼の能力を得た。", "Your eyes look mesmerizing...");
			break;
			
		case 10: case 11:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_TELEKINES;
			muta_desc = _("物体を念動力で動かす能力を得た。", "You gain the ability to move objects telekinetically.");
			break;
			
		case 12: case 13: case 14:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_VTELEPORT;
			muta_desc = _("自分の意思でテレポートする能力を得た。", "You gain the power of teleportation at will.");
			break;
			
		case 15: case 16:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_MIND_BLST;
			muta_desc = _("精神攻撃の能力を得た。", "You gain the power of Mind Blast.");
			break;
			
		case 17: case 18:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_RADIATION;
			muta_desc = _("あなたは強い放射線を発生し始めた。", "You start emitting hard radiation.");
			break;
			
		case 19: case 20:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_VAMPIRISM;
			muta_desc = _("生命力を吸収できるようになった。", "You become vampiric.");
			break;
			
		case 21: case 22: case 23:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_SMELL_MET;
			muta_desc = _("金属の匂いを嗅ぎ分けられるようになった。", "You smell a metallic odor.");
			break;
			
		case 24: case 25: case 26: case 27:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_SMELL_MON;
			muta_desc = _("モンスターの臭いを嗅ぎ分けられるようになった。", "You smell filthy monsters.");
			break;
			
		case 28: case 29: case 30:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_BLINK;
			muta_desc = _("近距離テレポートの能力を得た。", "You gain the power of minor teleportation.");
			break;
			
		case 31: case 32:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_EAT_ROCK;
			muta_desc = _("壁が美味しそうに見える。", "The walls look delicious.");
			break;
			
		case 33: case 34:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_SWAP_POS;
			muta_desc = _("他人の靴で一マイル歩くような気分がする。", "You feel like walking a mile in someone else's shoes.");
			break;
			
		case 35: case 36: case 37:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_SHRIEK;
			muta_desc = _("あなたの声は相当強くなった。", "Your vocal cords get much tougher.");
			break;
			
		case 38: case 39: case 40:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_ILLUMINE;
			muta_desc = _("あなたは光り輝いて部屋を明るくするようになった。", "You can light up rooms with your presence.");
			break;
			
		case 41: case 42:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_DET_CURSE;
			muta_desc = _("邪悪な魔法を感知できるようになった。", "You can feel evil magics.");
			break;
			
		case 43: case 44: case 45:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_BERSERK;
			muta_desc = _("制御できる激情を感じる。", "You feel a controlled rage.");
			break;
			
		case 46:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_POLYMORPH;
			muta_desc = _("体が変異しやすくなった。", "Your body seems mutable.");
			break;
			
		case 47: case 48:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_MIDAS_TCH;
			muta_desc = _("「ミダス王の手」の能力を得た。", "You gain the Midas touch.");/*トゥームレイダースにありましたね。 */
			break;
			
		case 49:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_GROW_MOLD;
			muta_desc = _("突然カビに親しみを覚えた。", "You feel a sudden affinity for mold.");
			break;
			
		case 50: case 51: case 52:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_RESIST;
			muta_desc = _("あなたは自分自身を守れる気がする。", "You feel like you can protect yourself.");
			break;
			
		case 53: case 54: case 55:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_EARTHQUAKE;
			muta_desc = _("ダンジョンを破壊する能力を得た。", "You gain the ability to wreck the dungeon.");
			break;
			
		case 56:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_EAT_MAGIC;
			muta_desc = _("魔法のアイテムが美味そうに見える。", "Your magic items look delicious.");
			break;
			
		case 57: case 58:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_WEIGH_MAG;
			muta_desc = _("あなたは周囲にある魔法をより良く理解できる気がする。", "You feel you can better understand the magic around you.");
			break;
			
		case 59:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_STERILITY;
			muta_desc = _("周りの全ての者に頭痛を起こすことができる。", "You can give everything around you a headache.");
			break;
		case 60: case 61:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_PANIC_HIT;
			muta_desc = _("突然、泥棒の気分が分かるようになった。", "You suddenly understand how thieves feel.");
			break;
			
		case 62: case 63: case 64:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_DAZZLE;
			muta_desc = _("眩い閃光を発する能力を得た。", "You gain the ability to emit dazzling lights.");
			break;
			
		case 65: case 66: case 67:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_LASER_EYE;
			muta_desc = _("あなたの目は一瞬焼け付いた。", "Your eyes burn for a moment.");
			break;
			
		case 68: case 69:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_RECALL;
			muta_desc = _("少しだけホームシックになったが、すぐ直った。", "You feel briefly homesick, but it passes.");
			break;
			
		case 70:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_BANISH;
			muta_desc = _("神聖な怒りの力に満たされた。", "You feel a holy wrath fill you.");
			break;
			
		case 71: case 72:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_COLD_TOUCH;
			muta_desc = _("あなたの両手はとても冷たくなった。", "Your hands get very cold.");
			break;
			
		case 73: case 74:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_LAUNCHER;
			muta_desc = _("あなたの物を投げる手はかなり強くなった気がする。", "Your throwing arm feels much stronger.");
			break;
			
		case 75:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_BERS_RAGE;
			muta_desc = _("あなたは狂暴化の発作を起こすようになった！", "You become subject to fits of berserk rage!");
			break;
			
		case 76:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_COWARDICE;
			muta_desc = _("信じられないくらい臆病になった！", "You become an incredible coward!");
			break;
			
		case 77:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_RTELEPORT;
			muta_desc = _("あなたの位置は非常に不確定になった。", "Your position seems very uncertain...");
			break;
			
		case 78:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_ALCOHOL;
			muta_desc = _("あなたはアルコールを分泌するようになった。", "Your body starts producing alcohol!");
			break;
			
		case 79:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_HALLU;
			muta_desc = _("あなたは幻覚を引き起こす精神錯乱に侵された。", "You are afflicted by a hallucinatory insanity!");
			break;
			
		case 80:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_FLATULENT;
			muta_desc = _( "あなたは制御不能な強烈な屁をこくようになった。",  "You become subject to uncontrollable flatulence.");

			break;
		case 81: case 82:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_SCOR_TAIL;
			muta_desc = _( "サソリの尻尾が生えてきた！",  "You grow a scorpion tail!");

			break;
		case 83: case 84:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_HORNS;
			muta_desc = _( "額に角が生えた！",  "Horns pop forth into your forehead!");

			break;
		case 85: case 86:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_BEAK;
			muta_desc = _( "口が鋭く強いクチバシに変化した！",  "Your mouth turns into a sharp, powerful beak!");

			break;
		case 87: case 88:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_ATT_DEMON;
			muta_desc = _( "悪魔を引き付けるようになった。",  "You start attracting demons.");

			break;
		case 89:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_PROD_MANA;
			muta_desc = _( "あなたは制御不能な魔法のエネルギーを発生するようになった。",  "You start producing magical energy uncontrollably.");

			break;
		case 90: case 91:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_SPEED_FLUX;
			muta_desc = _( "あなたは躁鬱質になった。",  "You become manic-depressive.");

			break;
		case 92: case 93:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_BANISH_ALL;
			muta_desc = _( "恐ろしい力があなたの背後に潜んでいる気がする。",  "You feel a terrifying power lurking behind you.");

			break;
		case 94:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_EAT_LIGHT;
			muta_desc = _( "あなたはウンゴリアントに奇妙な親しみを覚えるようになった。",  "You feel a strange kinship with Ungoliant.");

			break;
		case 95: case 96:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_TRUNK;
			muta_desc = _( "あなたの鼻は伸びて象の鼻のようになった。",  "Your nose grows into an elephant-like trunk.");

			break;
		case 97:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_ATT_ANIMAL;
			muta_desc = _( "動物を引き付けるようになった。",  "You start attracting animals.");

			break;
		case 98:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_TENTACLES;
			muta_desc = _( "邪悪な触手が体の両側に生えてきた。",  "Evil-looking tentacles sprout from your sides.");

			break;
		case 99:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_RAW_CHAOS;
			muta_desc = _( "周囲の空間が不安定になった気がする。",  "You feel the universe is less stable around you.");

			break;
		case 100: case 101: case 102:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_NORMALITY;
			muta_desc = _( "あなたは奇妙なほど普通になった気がする。",  "You feel strangely normal.");

			break;
		case 103:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WRAITH;
			muta_desc = _( "あなたは幽体化したり実体化したりするようになった。",  "You start to fade in and out of the physical world.");

			break;
		case 104:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_POLY_WOUND;
			muta_desc = _( "あなたはカオスの力が古い傷に入り込んでくるのを感じた。",  "You feel forces of chaos entering your old scars.");

			break;
		case 105:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WASTING;
			muta_desc = _( "あなたは突然おぞましい衰弱病にかかった。",  "You suddenly contract a horrible wasting disease.");

			break;
		case 106:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_ATT_DRAGON;
			muta_desc = _( "あなたはドラゴンを引きつけるようになった。",  "You start attracting dragons.");

			break;
		case 107: case 108:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WEIRD_MIND;
			muta_desc = _( "あなたの思考は突然おかしな方向に向き始めた。",  "Your thoughts suddenly take off in strange directions.");

			break;
		case 109:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_NAUSEA;
			muta_desc = _( "胃袋がピクピクしはじめた。",  "Your stomach starts to roil nauseously.");

			break;
		case 110: case 111:
			/* Chaos warriors already have a chaos deity */
			if (p_ptr->pclass != CLASS_CHAOS_WARRIOR)
			{
				muta_class = &(p_ptr->muta2);
				muta_which = MUT2_CHAOS_GIFT;
			muta_desc = _( "あなたはカオスの守護悪魔の注意を惹くようになった。",  "You attract the notice of a chaos deity!");

			}
			break;
		case 112:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WALK_SHAD;
			muta_desc = _( "あなたは現実が紙のように薄いと感じるようになった。",  "You feel like reality is as thin as paper.");

			break;
		case 113: case 114:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WARNING;
			muta_desc = _( "あなたは突然パラノイアになった気がする。",  "You suddenly feel paranoid.");

			break;
		case 115:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_INVULN;
			muta_desc = _( "あなたは祝福され、無敵状態になる発作を起こすようになった。",  "You are blessed with fits of invulnerability.");

			break;
		case 116: case 117:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_SP_TO_HP;
			muta_desc = _( "魔法の治癒の発作を起こすようになった。",  "You are subject to fits of magical healing.");

			break;
		case 118:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_HP_TO_SP;
			muta_desc = _( "痛みを伴う精神明瞭化の発作を起こすようになった。",  "You are subject to fits of painful clarity.");

			break;
		case 119:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_DISARM;
			muta_desc = _( "あなたの脚は長さが四倍になった。",  "Your feet grow to four times their former size.");

			break;
		case 120: case 121: case 122:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_HYPER_STR;
			muta_desc = _( "超人的に強くなった！",  "You turn into a superhuman he-man!");

			break;
		case 123: case 124: case 125:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_PUNY;
			muta_desc = _( "筋肉が弱ってしまった...",  "Your muscles wither away...");

			break;
		case 126: case 127: case 128:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_HYPER_INT;
			muta_desc = _( "あなたの脳は生体コンピュータに進化した！",  "Your brain evolves into a living computer!");

			break;
		case 129: case 130: case 131:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_MORONIC;
			muta_desc = _( "脳が萎縮してしまった...",  "Your brain withers away...");

			break;
		case 132: case 133:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_RESILIENT;
			muta_desc = _( "並外れてタフになった。",  "You become extraordinarily resilient.");

			break;
		case 134: case 135:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_XTRA_FAT;
			muta_desc = _( "あなたは気持ち悪いくらい太った！",  "You become sickeningly fat!");

			break;
		case 136: case 137:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ALBINO;
			muta_desc = _( "アルビノになった！弱くなった気がする...",  "You turn into an albino! You feel frail...");

			break;
		case 138: case 139: case 140:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_FLESH_ROT;
			muta_desc = _( "あなたの肉体は腐敗する病気に侵された！",  "Your flesh is afflicted by a rotting disease!");

			break;
		case 141: case 142:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_SILLY_VOI;
			muta_desc = _( "声が間抜けなキーキー声になった！",  "Your voice turns into a ridiculous squeak!");

			break;
		case 143: case 144:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_BLANK_FAC;
			muta_desc = _( "のっぺらぼうになった！",  "Your face becomes completely featureless!");

			break;
		case 145:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ILL_NORM;
			muta_desc = _( "心の安らぐ幻影を映し出すようになった。",  "You start projecting a reassuring image.");

			break;
		case 146: case 147: case 148:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_XTRA_EYES;
			muta_desc = _( "新たに二つの目が出来た！",  "You grow an extra pair of eyes!");

			break;
		case 149: case 150:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_MAGIC_RES;
			muta_desc = _( "魔法への耐性がついた。",  "You become resistant to magic.");

			break;
		case 151: case 152: case 153:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_XTRA_NOIS;
			muta_desc = _( "あなたは奇妙な音を立て始めた！",  "You start making strange noise!");

			break;
		case 154: case 155: case 156:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_INFRAVIS;
			muta_desc = _( "赤外線視力が増した。",  "Your infravision is improved.");

			break;
		case 157: case 158:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_XTRA_LEGS;
			muta_desc = _( "新たに二本の足が生えてきた！",  "You grow an extra pair of legs!");

			break;
		case 159: case 160:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_SHORT_LEG;
			muta_desc = _( "足が短い突起になってしまった！",  "Your legs turn into short stubs!");

			break;
		case 161: case 162:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ELEC_TOUC;
			muta_desc = _( "血管を電流が流れ始めた！",  "Electricity starts running through you!");

			break;
		case 163: case 164:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_FIRE_BODY;
			muta_desc = _( "あなたの体は炎につつまれている。",  "Your body is enveloped in flames!");

			break;
		case 165: case 166: case 167:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_WART_SKIN;
			muta_desc = _( "気持ち悪いイボイボが体中にできた！",  "Disgusting warts appear everywhere on you!");

			break;
		case 168: case 169: case 170:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_SCALES;
			muta_desc = _( "肌が黒い鱗に変わった！",  "Your skin turns into black scales!");

			break;
		case 171: case 172:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_IRON_SKIN;
			muta_desc = _( "あなたの肌は鉄になった！",  "Your skin turns to steel!");

			break;
		case 173: case 174:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_WINGS;
			muta_desc = _( "背中に羽が生えた。",  "You grow a pair of wings.");

			break;
		case 175: case 176: case 177:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_FEARLESS;
			muta_desc = _( "完全に怖れ知らずになった。",  "You become completely fearless.");

			break;
		case 178: case 179:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_REGEN;
			muta_desc = _( "急速に回復し始めた。",  "You start regenerating.");

			break;
		case 180: case 181:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ESP;
			muta_desc = _( "テレパシーの能力を得た！",  "You develop a telepathic ability!");

			break;
		case 182: case 183: case 184:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_LIMBER;
			muta_desc = _( "筋肉がしなやかになった。",  "Your muscles become limber.");

			break;
		case 185: case 186: case 187:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ARTHRITIS;
			muta_desc = _( "関節が突然痛み出した。",  "Your joints suddenly hurt.");

			break;
		case 188:
			if (p_ptr->pseikaku == SEIKAKU_LUCKY) break;
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_BAD_LUCK;
			muta_desc = _( "悪意に満ちた黒いオーラがあなたをとりまいた...",  "There is a malignant black aura surrounding you...");

			break;
		case 189:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_VULN_ELEM;
			muta_desc = _( "妙に無防備になった気がする。",  "You feel strangely exposed.");

			break;
		case 190: case 191: case 192:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_MOTION;
			muta_desc = _( "体の動作がより正確になった。",  "You move with new assurance.");

			break;
		case 193:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_GOOD_LUCK;
			muta_desc = _( "慈悲深い白いオーラがあなたをとりまいた...",  "There is a benevolent white aura surrounding you...");

			break;
		default:
			muta_class = NULL;
			muta_which = 0;
		}

		if (muta_class && muta_which)
		{
			if (!(*muta_class & muta_which))
			{
				muta_chosen = TRUE;
			}
		}
		if (muta_chosen == TRUE) break;
	}

	if (!muta_chosen)
	{
		msg_print(_("普通になった気がする。", "You feel normal."));

		return FALSE;
	}
	else
	{
		chg_virtue(V_CHANCE, 1);

		/*
		  some races are apt to gain specified mutations
		  This should be allowed only if "choose_mut" is 0.
							--- henkma
		*/
		if(!choose_mut){
			if (p_ptr->prace == RACE_VAMPIRE &&
			  !(p_ptr->muta1 & MUT1_HYPN_GAZE) &&
			   (randint1(10) < 7))
			{
				muta_class = &(p_ptr->muta1);
				muta_which = MUT1_HYPN_GAZE;
			muta_desc = _( "眼が幻惑的になった...",  "Your eyes look mesmerizing...");

			}

			else if (p_ptr->prace == RACE_IMP &&
				 !(p_ptr->muta2 & MUT2_HORNS) &&
				 (randint1(10) < 7))
			  {
				muta_class = &(p_ptr->muta2);
				muta_which = MUT2_HORNS;
			muta_desc = _( "角が額から生えてきた！",  "Horns pop forth into your forehead!");

			}

			else if (p_ptr->prace == RACE_YEEK &&
				!(p_ptr->muta1 & MUT1_SHRIEK) &&
				(randint1(10) < 7))
			{
				muta_class = &(p_ptr->muta1);
				muta_which = MUT1_SHRIEK;
			muta_desc = _( "声質がかなり強くなった。",  "Your vocal cords get much tougher.");

			}

			else if (p_ptr->prace == RACE_BEASTMAN &&
				!(p_ptr->muta1 & MUT1_POLYMORPH) &&
				(randint1(10) < 2))
			{
				muta_class = &(p_ptr->muta1);
				muta_which = MUT1_POLYMORPH;
			muta_desc = _( "あなたの肉体は変化できるようになった、",  "Your body seems mutable.");

			}

			else if (p_ptr->prace == RACE_MIND_FLAYER &&
				!(p_ptr->muta2 & MUT2_TENTACLES) &&
				(randint1(10) < 7))
			{
				muta_class = &(p_ptr->muta2);
				muta_which = MUT2_TENTACLES;
			muta_desc = _( "邪悪な触手が口の周りに生えた。",  "Evil-looking tentacles sprout from your mouth.");

			}
		}
		msg_print(_("突然変異した！", "You mutate!"));

		msg_print(muta_desc);
		*muta_class |= muta_which;

		if (muta_class == &(p_ptr->muta3))
		{
			if (muta_which == MUT3_PUNY)
			{
				if (p_ptr->muta3 & MUT3_HYPER_STR)
				{
					msg_print(_("あなたはもう超人的に強くはない！", "You no longer feel super-strong!"));

					p_ptr->muta3 &= ~(MUT3_HYPER_STR);
				}
			}
			else if (muta_which == MUT3_HYPER_STR)
			{
				if (p_ptr->muta3 & MUT3_PUNY)
				{
					msg_print(_("あなたはもう虚弱ではない！", "You no longer feel puny!"));

					p_ptr->muta3 &= ~(MUT3_PUNY);
				}
			}
			else if (muta_which == MUT3_MORONIC)
			{
				if (p_ptr->muta3 & MUT3_HYPER_INT)
				{
					msg_print(_("あなたの脳はもう生体コンピュータではない。", "Your brain is no longer a living computer."));

					p_ptr->muta3 &= ~(MUT3_HYPER_INT);
				}
			}
			else if (muta_which == MUT3_HYPER_INT)
			{
				if (p_ptr->muta3 & MUT3_MORONIC)
				{
					msg_print(_("あなたはもう精神薄弱ではない。", "You are no longer moronic."));

					p_ptr->muta3 &= ~(MUT3_MORONIC);
				}
			}
			else if (muta_which == MUT3_IRON_SKIN)
			{
				if (p_ptr->muta3 & MUT3_SCALES)
				{
					msg_print(_("鱗がなくなった。", "You lose your scales."));

					p_ptr->muta3 &= ~(MUT3_SCALES);
				}
				if (p_ptr->muta3 & MUT3_FLESH_ROT)
				{
					msg_print(_("肉体が腐乱しなくなった。", "Your flesh rots no longer."));

					p_ptr->muta3 &= ~(MUT3_FLESH_ROT);
				}
				if (p_ptr->muta3 & MUT3_WART_SKIN)
				{
					msg_print(_("肌のイボイボがなくなった。", "You lose your warts."));

					p_ptr->muta3 &= ~(MUT3_WART_SKIN);
				}
			}
			else if (muta_which == MUT3_WART_SKIN || muta_which == MUT3_SCALES
				|| muta_which == MUT3_FLESH_ROT)
			{
				if (p_ptr->muta3 & MUT3_IRON_SKIN)
				{
					msg_print(_("あなたの肌はもう鉄ではない。", "Your skin is no longer made of steel."));

					p_ptr->muta3 &= ~(MUT3_IRON_SKIN);
				}
			}
			else if (muta_which == MUT3_FEARLESS)
			{
				if (p_ptr->muta2 & MUT2_COWARDICE)
				{
					msg_print(_("臆病でなくなった。", "You are no longer cowardly."));

					p_ptr->muta2 &= ~(MUT2_COWARDICE);
				}
			}
			else if (muta_which == MUT3_FLESH_ROT)
			{
				if (p_ptr->muta3 & MUT3_REGEN)
				{
					msg_print(_("急速に回復しなくなった。", "You stop regenerating."));

					p_ptr->muta3 &= ~(MUT3_REGEN);
				}
			}
			else if (muta_which == MUT3_REGEN)
			{
				if (p_ptr->muta3 & MUT3_FLESH_ROT)
				{
					msg_print(_("肉体が腐乱しなくなった。", "Your flesh stops rotting."));

					p_ptr->muta3 &= ~(MUT3_FLESH_ROT);
				}
			}
			else if (muta_which == MUT3_LIMBER)
			{
				if (p_ptr->muta3 & MUT3_ARTHRITIS)
				{
					msg_print(_("関節が痛くなくなった。", "Your joints stop hurting."));

					p_ptr->muta3 &= ~(MUT3_ARTHRITIS);
				}
			}
			else if (muta_which == MUT3_ARTHRITIS)
			{
				if (p_ptr->muta3 & MUT3_LIMBER)
				{
					msg_print(_("あなたはしなやかでなくなった。", "You no longer feel limber."));

					p_ptr->muta3 &= ~(MUT3_LIMBER);
				}
			}
		}
		else if (muta_class == &(p_ptr->muta2))
		{
			if (muta_which == MUT2_COWARDICE)
			{
				if (p_ptr->muta3 & MUT3_FEARLESS)
				{
					msg_print(_("恐れ知らずでなくなった。", "You no longer feel fearless."));

					p_ptr->muta3 &= ~(MUT3_FEARLESS);
				}
			}
			if (muta_which == MUT2_BEAK)
			{
				if (p_ptr->muta2 & MUT2_TRUNK)
				{
					msg_print(_("あなたの鼻はもう象の鼻のようではなくなった。", "Your nose is no longer elephantine."));

					p_ptr->muta2 &= ~(MUT2_TRUNK);
				}
			}
			if (muta_which == MUT2_TRUNK)
			{
				if (p_ptr->muta2 & MUT2_BEAK)
				{
					msg_print(_("硬いクチバシがなくなった。", "You no longer have a hard beak."));

					p_ptr->muta2 &= ~(MUT2_BEAK);
				}
			}
		}

		mutant_regenerate_mod = calc_mutant_regenerate_mod();
		p_ptr->update |= PU_BONUS;
		handle_stuff();
		return TRUE;
	}
}

/*!
 * @brief プレイヤーから突然変異を取り除く
 * @param choose_mut 取り除きたい突然変異のID、0ならばランダムに消去
 * @return なし
 */
bool lose_mutation(int choose_mut)
{
	int attempts_left = 20;
	cptr muta_desc = "";
	bool muta_chosen = FALSE;
	u32b muta_which = 0;
	u32b *muta_class = NULL;

	if (choose_mut) attempts_left = 1;

	while (attempts_left--)
	{
		switch (choose_mut ? choose_mut : randint1(193))
		{
		case 1: case 2: case 3: case 4:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_SPIT_ACID;
			muta_desc = _( "酸を吹きかける能力を失った。",  "You lose the ability to spit acid.");

			break;
		case 5: case 6: case 7:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_BR_FIRE;
			muta_desc = _( "炎のブレスを吐く能力を失った。",  "You lose the ability to breathe fire.");

			break;
		case 8: case 9:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_HYPN_GAZE;
			muta_desc = _( "あなたの目はつまらない目になった。",  "Your eyes look uninteresting.");

			break;
		case 10: case 11:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_TELEKINES;
			muta_desc = _( "念動力で物を動かす能力を失った。",  "You lose the ability to move objects telekinetically.");

			break;
		case 12: case 13: case 14:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_VTELEPORT;
			muta_desc = _( "自分の意思でテレポートする能力を失った。",  "You lose the power of teleportation at will.");

			break;
		case 15: case 16:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_MIND_BLST;
			muta_desc = _( "精神攻撃の能力を失った。",  "You lose the power of Mind Blast.");

			break;
		case 17: case 18:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_RADIATION;
			muta_desc = _( "あなたは放射能を発生しなくなった。",  "You stop emitting hard radiation.");

			break;
		case 19: case 20:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_VAMPIRISM;
			muta_desc = _( "吸血の能力を失った。",  "You are no longer vampiric.");

			break;
		case 21: case 22: case 23:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_SMELL_MET;
			muta_desc = _( "金属の臭いを嗅げなくなった。",  "You no longer smell a metallic odor.");

			break;
		case 24: case 25: case 26: case 27:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_SMELL_MON;
			muta_desc = _( "不潔なモンスターの臭いを嗅げなくなった。",  "You no longer smell filthy monsters.");

			break;
		case 28: case 29: case 30:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_BLINK;
			muta_desc = _( "近距離テレポートの能力を失った。",  "You lose the power of minor teleportation.");

			break;
		case 31: case 32:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_EAT_ROCK;
			muta_desc = _( "壁は美味しそうに見えなくなった。",  "The walls look unappetizing.");

			break;
		case 33: case 34:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_SWAP_POS;
			muta_desc = _( "あなたは自分の靴に留まる感じがする。",  "You feel like staying in your own shoes.");

			break;
		case 35: case 36: case 37:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_SHRIEK;
			muta_desc = _( "あなたの声質は弱くなった。",  "Your vocal cords get much weaker.");

			break;
		case 38: case 39: case 40:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_ILLUMINE;
			muta_desc = _( "部屋を明るく照らすことが出来なくなった。",  "You can no longer light up rooms with your presence.");

			break;
		case 41: case 42:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_DET_CURSE;
			muta_desc = _( "邪悪な魔法を感じられなくなった。",  "You can no longer feel evil magics.");

			break;
		case 43: case 44: case 45:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_BERSERK;
			muta_desc = _( "制御できる激情を感じなくなった。",  "You no longer feel a controlled rage.");

			break;
		case 46:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_POLYMORPH;
			muta_desc = _( "あなたの体は安定したように見える。",  "Your body seems stable.");

			break;
		case 47: case 48:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_MIDAS_TCH;
			muta_desc = _( "ミダスの手の能力を失った。",  "You lose the Midas touch.");

			break;
		case 49:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_GROW_MOLD;
			muta_desc = _( "突然カビが嫌いになった。",  "You feel a sudden dislike for mold.");

			break;
		case 50: case 51: case 52:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_RESIST;
			muta_desc = _( "傷つき易くなった気がする。",  "You feel like you might be vulnerable.");

			break;
		case 53: case 54: case 55:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_EARTHQUAKE;
			muta_desc = _( "ダンジョンを壊す能力を失った。",  "You lose the ability to wreck the dungeon.");

			break;
		case 56:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_EAT_MAGIC;
			muta_desc = _( "魔法のアイテムはもう美味しそうに見えなくなった。",  "Your magic items no longer look delicious.");

			break;
		case 57: case 58:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_WEIGH_MAG;
			muta_desc = _( "魔力を感じられなくなった。",  "You no longer sense magic.");

			break;
		case 59:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_STERILITY;
			muta_desc = _( "たくさんの安堵の吐息が聞こえた。",  "You hear a massed sigh of relief.");

			break;
		case 60: case 61:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_PANIC_HIT;
			muta_desc = _( "あちこちへ跳べる気分がなくなった。",  "You no longer feel jumpy.");

			break;
		case 62: case 63: case 64:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_DAZZLE;
			muta_desc = _( "まばゆい閃光を発する能力を失った。",  "You lose the ability to emit dazzling lights.");

			break;
		case 65: case 66: case 67:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_LASER_EYE;
			muta_desc = _( "眼が少しの間焼き付いて、痛みが和らいだ。",  "Your eyes burn for a moment, then feel soothed.");

			break;
		case 68: case 69:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_RECALL;
			muta_desc = _( "少しの間ホームシックになった。",  "You feel briefly homesick.");

			break;
		case 70:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_BANISH;
			muta_desc = _( "神聖な怒りの力を感じなくなった。",  "You no longer feel a holy wrath.");

			break;
		case 71: case 72:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_COLD_TOUCH;
			muta_desc = _( "手が暖かくなった。",  "Your hands warm up.");

			break;
		case 73: case 74:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_LAUNCHER;
			muta_desc = _( "物を投げる手が弱くなった気がする。",  "Your throwing arm feels much weaker.");

			break;
		case 75:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_BERS_RAGE;
			muta_desc = _( "凶暴化の発作にさらされなくなった！",  "You are no longer subject to fits of berserk rage!");

			break;
		case 76:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_COWARDICE;
			muta_desc = _( "もう信じがたいほど臆病ではなくなった！",  "You are no longer an incredible coward!");

			break;
		case 77:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_RTELEPORT;
			muta_desc = _( "あなたの位置はより確定的になった。",  "Your position seems more certain.");

			break;
		case 78:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_ALCOHOL;
			muta_desc = _( "あなたはアルコールを分泌しなくなった！",  "Your body stops producing alcohol!");

			break;
		case 79:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_HALLU;
			muta_desc = _( "幻覚をひき起こす精神障害を起こさなくなった！",  "You are no longer afflicted by a hallucinatory insanity!");

			break;
		case 80:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_FLATULENT;
			muta_desc = _( "もう強烈な屁はこかなくなった。",  "You are no longer subject to uncontrollable flatulence.");

			break;
		case 81: case 82:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_SCOR_TAIL;
			muta_desc = _( "サソリの尻尾がなくなった！",  "You lose your scorpion tail!");

			break;
		case 83: case 84:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_HORNS;
			muta_desc = _( "額から角が消えた！",  "Your horns vanish from your forehead!");

			break;
		case 85: case 86:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_BEAK;
			muta_desc = _( "口が普通に戻った！",  "Your mouth reverts to normal!");

			break;
		case 87: case 88:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_ATT_DEMON;
			muta_desc = _( "デーモンを引き寄せなくなった。",  "You stop attracting demons.");

			break;
		case 89:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_PROD_MANA;
			muta_desc = _( "制御不能な魔法のエネルギーを発生しなくなった。",  "You stop producing magical energy uncontrollably.");

			break;
		case 90: case 91:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_SPEED_FLUX;
			muta_desc = _( "躁鬱質でなくなった。",  "You are no longer manic-depressive.");

			break;
		case 92: case 93:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_BANISH_ALL;
			muta_desc = _( "背後に恐ろしい力を感じなくなった。",  "You no longer feel a terrifying power lurking behind you.");

			break;
		case 94:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_EAT_LIGHT;
			muta_desc = _( "世界が明るいと感じる。",  "You feel the world's a brighter place.");

			break;
		case 95: case 96:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_TRUNK;
			muta_desc = _( "鼻が普通の長さに戻った。",  "Your nose returns to a normal length.");

			break;
		case 97:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_ATT_ANIMAL;
			muta_desc = _( "動物を引き寄せなくなった。",  "You stop attracting animals.");

			break;
		case 98:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_TENTACLES;
			muta_desc = _( "触手が消えた。",  "Your tentacles vanish from your sides.");

			break;
		case 99:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_RAW_CHAOS;
			muta_desc = _( "周囲の空間が安定した気がする。",  "You feel the universe is more stable around you.");

			break;
		case 100: case 101: case 102:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_NORMALITY;
			muta_desc = _( "普通に奇妙な感じがする。",  "You feel normally strange.");

			break;
		case 103:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WRAITH;
			muta_desc = _( "あなたは物質世界にしっかり存在している。",  "You are firmly in the physical world.");

			break;
		case 104:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_POLY_WOUND;
			muta_desc = _( "古い傷からカオスの力が去っていった。",  "You feel forces of chaos departing your old scars.");

			break;
		case 105:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WASTING;
			muta_desc = _( "おぞましい衰弱病が治った！",  "You are cured of the horrible wasting disease!");

			break;
		case 106:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_ATT_DRAGON;
			muta_desc = _( "ドラゴンを引き寄せなくなった。",  "You stop attracting dragons.");

			break;
		case 107: case 108:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WEIRD_MIND;
			muta_desc = _( "思考が退屈な方向に戻った。",  "Your thoughts return to boring paths.");

			break;
		case 109:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_NAUSEA;
			muta_desc = _( "胃が痙攣しなくなった。",  "Your stomach stops roiling.");

			break;
		case 110: case 111:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_CHAOS_GIFT;
			muta_desc = _( "混沌の神々の興味を惹かなくなった。",  "You lose the attention of the chaos deities.");

			break;
		case 112:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WALK_SHAD;
			muta_desc = _( "物質世界に捕らわれている気がする。",  "You feel like you're trapped in reality.");

			break;
		case 113: case 114:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WARNING;
			muta_desc = _( "パラノイアでなくなった。",  "You no longer feel paranoid.");

			break;
		case 115:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_INVULN;
			muta_desc = _( "無敵状態の発作を起こさなくなった。",  "You are no longer blessed with fits of invulnerability.");

			break;
		case 116: case 117:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_SP_TO_HP;
			muta_desc = _( "魔法の治癒の発作に襲われなくなった。",  "You are no longer subject to fits of magical healing.");

			break;
		case 118:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_HP_TO_SP;
			muta_desc = _( "痛みを伴う精神明瞭化の発作に襲われなくなった。",  "You are no longer subject to fits of painful clarity.");

			break;
		case 119:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_DISARM;
			muta_desc = _( "脚が元の大きさに戻った。",  "Your feet shrink to their former size.");

			break;
		case 120: case 121: case 122:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_HYPER_STR;
			muta_desc = _( "筋肉が普通に戻った。",  "Your muscles revert to normal.");

			break;
		case 123: case 124: case 125:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_PUNY;
			muta_desc = _( "筋肉が普通に戻った。",  "Your muscles revert to normal.");

			break;
		case 126: case 127: case 128:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_HYPER_INT;
			muta_desc = _( "脳が普通に戻った。",  "Your brain reverts to normal.");

			break;
		case 129: case 130: case 131:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_MORONIC;
			muta_desc = _( "脳が普通に戻った。",  "Your brain reverts to normal.");

			break;
		case 132: case 133:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_RESILIENT;
			muta_desc = _( "普通の丈夫さに戻った。",  "You become ordinarily resilient again.");

			break;
		case 134: case 135:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_XTRA_FAT;
			muta_desc = _( "奇跡的なダイエットに成功した！",  "You benefit from a miracle diet!");

			break;
		case 136: case 137:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ALBINO;
			muta_desc = _( "アルビノでなくなった！",  "You are no longer an albino!");

			break;
		case 138: case 139: case 140:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_FLESH_ROT;
			muta_desc = _( "肉体を腐敗させる病気が治った！",  "Your flesh is no longer afflicted by a rotting disease!");

			break;
		case 141: case 142:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_SILLY_VOI;
			muta_desc = _( "声質が普通に戻った。",  "Your voice returns to normal.");

			break;
		case 143: case 144:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_BLANK_FAC;
			muta_desc = _( "顔に目鼻が戻った。",  "Your facial features return.");

			break;
		case 145:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ILL_NORM;
			muta_desc = _( "心が安らぐ幻影を映し出さなくなった。",  "You stop projecting a reassuring image.");

			break;
		case 146: case 147: case 148:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_XTRA_EYES;
			muta_desc = _( "余分な目が消えてしまった！",  "Your extra eyes vanish!");

			break;
		case 149: case 150:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_MAGIC_RES;
			muta_desc = _( "魔法に弱くなった。",  "You become susceptible to magic again.");

			break;
		case 151: case 152: case 153:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_XTRA_NOIS;
			muta_desc = _( "奇妙な音を立てなくなった！",  "You stop making strange noise!");

			break;
		case 154: case 155: case 156:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_INFRAVIS;
			muta_desc = _( "赤外線視力が落ちた。",  "Your infravision is degraded.");

			break;
		case 157: case 158:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_XTRA_LEGS;
			muta_desc = _( "余分な脚が消えてしまった！",  "Your extra legs disappear!");

			break;
		case 159: case 160:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_SHORT_LEG;
			muta_desc = _( "脚の長さが普通に戻った。",  "Your legs lengthen to normal.");

			break;
		case 161: case 162:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ELEC_TOUC;
			muta_desc = _( "体を電流が流れなくなった。",  "Electricity stops running through you.");

			break;
		case 163: case 164:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_FIRE_BODY;
			muta_desc = _( "体が炎に包まれなくなった。",  "Your body is no longer enveloped in flames.");

			break;
		case 165: case 166: case 167:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_WART_SKIN;
			muta_desc = _( "イボイボが消えた！",  "Your warts disappear!");

			break;
		case 168: case 169: case 170:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_SCALES;
			muta_desc = _( "鱗が消えた！",  "Your scales vanish!");

			break;
		case 171: case 172:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_IRON_SKIN;
			muta_desc = _( "肌が肉にもどった！",  "Your skin reverts to flesh!");

			break;
		case 173: case 174:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_WINGS;
			muta_desc = _( "背中の羽根が取れ落ちた。",  "Your wings fall off.");

			break;
		case 175: case 176: case 177:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_FEARLESS;
			muta_desc = _( "再び恐怖を感じるようになった。",  "You begin to feel fear again.");

			break;
		case 178: case 179:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_REGEN;
			muta_desc = _( "急速回復しなくなった。",  "You stop regenerating.");

			break;
		case 180: case 181:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ESP;
			muta_desc = _( "テレパシーの能力を失った！",  "You lose your telepathic ability!");

			break;
		case 182: case 183: case 184:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_LIMBER;
			muta_desc = _( "筋肉が硬くなった。",  "Your muscles stiffen.");

			break;
		case 185: case 186: case 187:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ARTHRITIS;
			muta_desc = _( "関節が痛くなくなった。",  "Your joints stop hurting.");

			break;
		case 188:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_BAD_LUCK;
			muta_desc = _( "黒いオーラは渦巻いて消えた。",  "Your black aura swirls and fades.");

			break;
		case 189:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_VULN_ELEM;
			muta_desc = _( "無防備な感じはなくなった。",  "You feel less exposed.");

			break;
		case 190: case 191: case 192:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_MOTION;
			muta_desc = _( "動作の正確さがなくなった。",  "You move with less assurance.");

			break;
		case 193:
			if (p_ptr->pseikaku == SEIKAKU_LUCKY) break;
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_GOOD_LUCK;
			muta_desc = _( "白いオーラは輝いて消えた。",  "Your white aura shimmers and fades.");

			break;
		default:
			muta_class = NULL;
			muta_which = 0;
		}

		if (muta_class && muta_which)
		{
			if (*(muta_class) & muta_which)
			{
				muta_chosen = TRUE;
			}
		}
		if (muta_chosen == TRUE) break;
	}

	if (!muta_chosen)
	{
		return FALSE;
	}
	else
	{
		msg_print(muta_desc);
		*(muta_class) &= ~(muta_which);

		p_ptr->update |= PU_BONUS;
		handle_stuff();
		mutant_regenerate_mod = calc_mutant_regenerate_mod();
		return TRUE;
	}
}

/*!
 * @brief ファイルポインタを通じて突然変異の一覧を出力する
 * @param OutFile 出力先ファイルポインタ
 * @return なし
 */
void dump_mutations(FILE *OutFile)
{
	if (!OutFile) return;

	if (p_ptr->muta1)
	{
		if (p_ptr->muta1 & MUT1_SPIT_ACID)
		{
			fprintf(OutFile, _(" あなたは酸を吹きかけることができる。(ダメージ レベルX1)\n", " You can spit acid (dam lvl).\n"));

		}
		if (p_ptr->muta1 & MUT1_BR_FIRE)
		{
			fprintf(OutFile, _(" あなたは炎のブレスを吐くことができる。(ダメージ レベルX2)\n", " You can breathe fire (dam lvl * 2).\n"));

		}
		if (p_ptr->muta1 & MUT1_HYPN_GAZE)
		{
			fprintf(OutFile, _(" あなたの睨みは催眠効果をもつ。\n", " Your gaze is hypnotic.\n"));

		}
		if (p_ptr->muta1 & MUT1_TELEKINES)
		{
			fprintf(OutFile, _(" あなたは念動力をもっている。\n", " You are telekinetic.\n"));

		}
		if (p_ptr->muta1 & MUT1_VTELEPORT)
		{
			fprintf(OutFile, _(" あなたは自分の意思でテレポートできる。\n", " You can teleport at will.\n"));

		}
		if (p_ptr->muta1 & MUT1_MIND_BLST)
		{
			fprintf(OutFile, _(" あなたは敵を精神攻撃できる。\n", " You can Mind Blast your enemies.\n"));

		}
		if (p_ptr->muta1 & MUT1_RADIATION)
		{
			fprintf(OutFile, _(" あなたは自分の意思で放射能を発生することができる。\n", " You can emit hard radiation at will.\n"));

		}
		if (p_ptr->muta1 & MUT1_VAMPIRISM)
		{
			fprintf(OutFile, _(" あなたは吸血鬼のように敵から生命力を吸収することができる。\n", " You can drain life from a foe like a vampire.\n"));

		}
		if (p_ptr->muta1 & MUT1_SMELL_MET)
		{
			fprintf(OutFile, _(" あなたは近くにある貴金属をかぎ分けることができる。\n", " You can smell nearby precious metal.\n"));

		}
		if (p_ptr->muta1 & MUT1_SMELL_MON)
		{
			fprintf(OutFile, _(" あなたは近くのモンスターの存在をかぎ分けることができる。\n", " You can smell nearby monsters.\n"));

		}
		if (p_ptr->muta1 & MUT1_BLINK)
		{
			fprintf(OutFile, _(" あなたは短い距離をテレポートできる。\n", " You can teleport yourself short distances.\n"));

		}
		if (p_ptr->muta1 & MUT1_EAT_ROCK)
		{
			fprintf(OutFile, _(" あなたは硬い岩を食べることができる。\n", " You can consume solid rock.\n"));

		}
		if (p_ptr->muta1 & MUT1_SWAP_POS)
		{
			fprintf(OutFile, _(" あなたは他の者と場所を入れ替わることができる。\n", " You can switch locations with another being.\n"));

		}
		if (p_ptr->muta1 & MUT1_SHRIEK)
		{
			fprintf(OutFile, _(" あなたは身の毛もよだつ叫び声を発することができる。\n", " You can emit a horrible shriek.\n"));

		}
		if (p_ptr->muta1 & MUT1_ILLUMINE)
		{
			fprintf(OutFile, _(" あなたは明るい光を放つことができる。\n", " You can emit bright light.\n"));

		}
		if (p_ptr->muta1 & MUT1_DET_CURSE)
		{
			fprintf(OutFile, _(" あなたは邪悪な魔法の危険を感じとることができる。\n", " You can feel the danger of evil magic.\n"));

		}
		if (p_ptr->muta1 & MUT1_BERSERK)
		{
			fprintf(OutFile, _(" あなたは自分の意思で狂乱戦闘状態になることができる。\n", " You can drive yourself into a berserk frenzy.\n"));

		}
		if (p_ptr->muta1 & MUT1_POLYMORPH)
		{
			fprintf(OutFile, _(" あなたは自分の意志で変化できる。\n", " You can polymorph yourself at will.\n"));

		}
		if (p_ptr->muta1 & MUT1_MIDAS_TCH)
		{
			fprintf(OutFile, _(" あなたは通常アイテムを金に変えることができる。\n", " You can turn ordinary items to gold.\n"));

		}
		if (p_ptr->muta1 & MUT1_GROW_MOLD)
		{
			fprintf(OutFile, _(" あなたは周囲にキノコを生やすことができる。\n", " You can cause mold to grow near you.\n"));

		}
		if (p_ptr->muta1 & MUT1_RESIST)
		{
			fprintf(OutFile, _(" あなたは元素の攻撃に対して身を硬くすることができる。\n", " You can harden yourself to the ravages of the elements.\n"));

		}
		if (p_ptr->muta1 & MUT1_EARTHQUAKE)
		{
			fprintf(OutFile, _(" あなたは周囲のダンジョンを崩壊させることができる。\n", " You can bring down the dungeon around your ears.\n"));

		}
		if (p_ptr->muta1 & MUT1_EAT_MAGIC)
		{
			fprintf(OutFile, _(" あなたは魔法のエネルギーを自分の物として使用できる。\n", " You can consume magic energy for your own use.\n"));

		}
		if (p_ptr->muta1 & MUT1_WEIGH_MAG)
		{
			fprintf(OutFile, _(" あなたは自分に影響を与える魔法の力を感じることができる。\n", " You can feel the strength of the magics affecting you.\n"));

		}
		if (p_ptr->muta1 & MUT1_STERILITY)
		{
			fprintf(OutFile, _(" あなたは集団的生殖不能を起こすことができる。\n", " You can cause mass impotence.\n"));

		}
		if (p_ptr->muta1 & MUT1_PANIC_HIT)
		{
			fprintf(OutFile, _(" あなたは攻撃した後身を守るため逃げることができる。\n", " You can run for your life after hitting something.\n"));

		}
		if (p_ptr->muta1 & MUT1_DAZZLE)
		{
			fprintf(OutFile, _(" あなたは混乱と盲目を引き起こす放射能を発生することができる。 \n", " You can emit confusing, blinding radiation.\n"));

		}
		if (p_ptr->muta1 & MUT1_LASER_EYE)
		{
			fprintf(OutFile, _(" あなたは目からレーザー光線を発射することができる。\n", " Your eyes can fire laser beams.\n"));

		}
		if (p_ptr->muta1 & MUT1_RECALL)
		{
			fprintf(OutFile, _(" あなたは街とダンジョンの間を行き来することができる。\n", " You can travel between town and the depths.\n"));

		}
		if (p_ptr->muta1 & MUT1_BANISH)
		{
			fprintf(OutFile, _(" あなたは邪悪なモンスターを地獄に落とすことができる。\n", " You can send evil creatures directly to Hell.\n"));

		}
		if (p_ptr->muta1 & MUT1_COLD_TOUCH)
		{
			fprintf(OutFile, _(" あなたは物を触って凍らせることができる。\n", " You can freeze things with a touch.\n"));

		}
		if (p_ptr->muta1 & MUT1_LAUNCHER)
		{
			fprintf(OutFile, _(" あなたはアイテムを力強く投げることができる。\n", " You can hurl objects with great force.\n"));

		}
	}

	if (p_ptr->muta2)
	{
		if (p_ptr->muta2 & MUT2_BERS_RAGE)
		{
			fprintf(OutFile, _(" あなたは狂戦士化の発作を起こす。\n", " You are subject to berserker fits.\n"));

		}
		if (p_ptr->muta2 & MUT2_COWARDICE)
		{
			fprintf(OutFile, _(" あなたは時々臆病になる。\n", " You are subject to cowardice.\n"));

		}
		if (p_ptr->muta2 & MUT2_RTELEPORT)
		{
			fprintf(OutFile, _(" あなたはランダムにテレポートする。\n", " You are teleporting randomly.\n"));

		}
		if (p_ptr->muta2 & MUT2_ALCOHOL)
		{
			fprintf(OutFile, _(" あなたの体はアルコールを分泌する。\n", " Your body produces alcohol.\n"));

		}
		if (p_ptr->muta2 & MUT2_HALLU)
		{
			fprintf(OutFile, _(" あなたは幻覚を引き起こす精神錯乱に侵されている。\n", " You have a hallucinatory insanity.\n"));

		}
		if (p_ptr->muta2 & MUT2_FLATULENT)
		{
			fprintf(OutFile, _(" あなたは制御できない強烈な屁をこく。\n", " You are subject to uncontrollable flatulence.\n"));

		}
		if (p_ptr->muta2 & MUT2_PROD_MANA)
		{
			fprintf(OutFile, _(" あなたは制御不能な魔法のエネルギーを発している。\n", " You are producing magical energy uncontrollably.\n"));

		}
		if (p_ptr->muta2 & MUT2_ATT_DEMON)
		{
			fprintf(OutFile, _(" あなたはデーモンを引きつける。\n", " You attract demons.\n"));

		}
		if (p_ptr->muta2 & MUT2_SCOR_TAIL)
		{
			fprintf(OutFile, _(" あなたはサソリの尻尾が生えている。(毒、ダメージ 3d7)\n", " You have a scorpion tail (poison, 3d7).\n"));

		}
		if (p_ptr->muta2 & MUT2_HORNS)
		{
			fprintf(OutFile, _(" あなたは角が生えている。(ダメージ 2d6)\n", " You have horns (dam. 2d6).\n"));

		}
		if (p_ptr->muta2 & MUT2_BEAK)
		{
			fprintf(OutFile, _(" あなたはクチバシが生えている。(ダメージ 2d4)\n", " You have a beak (dam. 2d4).\n"));

		}
		if (p_ptr->muta2 & MUT2_SPEED_FLUX)
		{
			fprintf(OutFile, _(" あなたはランダムに早く動いたり遅く動いたりする。\n", " You move faster or slower randomly.\n"));

		}
		if (p_ptr->muta2 & MUT2_BANISH_ALL)
		{
			fprintf(OutFile, _(" あなたは時々近くのモンスターを消滅させる。\n", " You sometimes cause nearby creatures to vanish.\n"));

		}
		if (p_ptr->muta2 & MUT2_EAT_LIGHT)
		{
			fprintf(OutFile, _(" あなたは時々周囲の光を吸収して栄養にする。\n", " You sometimes feed off of the light around you.\n"));

		}
		if (p_ptr->muta2 & MUT2_TRUNK)
		{
			fprintf(OutFile, _(" あなたは象のような鼻を持っている。(ダメージ 1d4)\n", " You have an elephantine trunk (dam 1d4).\n"));

		}
		if (p_ptr->muta2 & MUT2_ATT_ANIMAL)
		{
			fprintf(OutFile, _(" あなたは動物を引きつける。\n", " You attract animals.\n"));

		}
		if (p_ptr->muta2 & MUT2_TENTACLES)
		{
			fprintf(OutFile, _(" あなたは邪悪な触手を持っている。(ダメージ 2d5)\n", " You have evil looking tentacles (dam 2d5).\n"));

		}
		if (p_ptr->muta2 & MUT2_RAW_CHAOS)
		{
			fprintf(OutFile, _(" あなたはしばしば純カオスに包まれる。\n", " You occasionally are surrounded with raw chaos.\n"));

		}
		if (p_ptr->muta2 & MUT2_NORMALITY)
		{
			fprintf(OutFile, _(" あなたは変異していたが、回復してきている。\n", " You may be mutated, but you're recovering.\n"));

		}
		if (p_ptr->muta2 & MUT2_WRAITH)
		{
			fprintf(OutFile, _(" あなたの肉体は幽体化したり実体化したりする。\n", " You fade in and out of physical reality.\n"));

		}
		if (p_ptr->muta2 & MUT2_POLY_WOUND)
		{
			fprintf(OutFile, _(" あなたの健康はカオスの力に影響を受ける。\n", " Your health is subject to chaotic forces.\n"));

		}
		if (p_ptr->muta2 & MUT2_WASTING)
		{
			fprintf(OutFile, _(" あなたは衰弱する恐ろしい病気にかかっている。\n", " You have a horrible wasting disease.\n"));

		}
		if (p_ptr->muta2 & MUT2_ATT_DRAGON)
		{
			fprintf(OutFile, _(" あなたはドラゴンを引きつける。\n", " You attract dragons.\n"));

		}
		if (p_ptr->muta2 & MUT2_WEIRD_MIND)
		{
			fprintf(OutFile, _(" あなたの精神はランダムに拡大したり縮小したりしている。\n", " Your mind randomly expands and contracts.\n"));

		}
		if (p_ptr->muta2 & MUT2_NAUSEA)
		{
			fprintf(OutFile, _(" あなたの胃は非常に落ち着きがない。\n", " You have a seriously upset stomach.\n"));

		}
		if (p_ptr->muta2 & MUT2_CHAOS_GIFT)
		{
			fprintf(OutFile, _(" あなたはカオスの守護悪魔から褒美をうけとる。\n", " Chaos deities give you gifts.\n"));

		}
		if (p_ptr->muta2 & MUT2_WALK_SHAD)
		{
			fprintf(OutFile, _(" あなたはしばしば他の「影」に迷い込む。\n", " You occasionally stumble into other shadows.\n"));

		}
		if (p_ptr->muta2 & MUT2_WARNING)
		{
			fprintf(OutFile, _(" あなたは敵に関する警告を感じる。\n", " You receive warnings about your foes.\n"));

		}
		if (p_ptr->muta2 & MUT2_INVULN)
		{
			fprintf(OutFile, _(" あなたは時々負け知らずな気分になる。\n", " You occasionally feel invincible.\n"));

		}
		if (p_ptr->muta2 & MUT2_SP_TO_HP)
		{
			fprintf(OutFile, _(" あなたは時々血が筋肉にどっと流れる。\n", " Your blood sometimes rushes to your muscles.\n"));

		}
		if (p_ptr->muta2 & MUT2_HP_TO_SP)
		{
			fprintf(OutFile, _(" あなたは時々頭に血がどっと流れる。\n", " Your blood sometimes rushes to your head.\n"));

		}
		if (p_ptr->muta2 & MUT2_DISARM)
		{
			fprintf(OutFile, _(" あなたはよくつまづいて物を落とす。\n", " You occasionally stumble and drop things.\n"));

		}
	}

	if (p_ptr->muta3)
	{
		if (p_ptr->muta3 & MUT3_HYPER_STR)
		{
			fprintf(OutFile, _(" あなたは超人的に強い。(腕力+4)\n", " You are superhumanly strong (+4 STR).\n"));

		}
		if (p_ptr->muta3 & MUT3_PUNY)
		{
			fprintf(OutFile, _(" あなたは虚弱だ。(腕力-4)\n", " You are puny (-4 STR).\n"));

		}
		if (p_ptr->muta3 & MUT3_HYPER_INT)
		{
			fprintf(OutFile, _(" あなたの脳は生体コンピュータだ。(知能＆賢さ+4)\n", " Your brain is a living computer (+4 INT/WIS).\n"));

		}
		if (p_ptr->muta3 & MUT3_MORONIC)
		{
			fprintf(OutFile, _(" あなたは精神薄弱だ。(知能＆賢さ-4)\n", " You are moronic (-4 INT/WIS).\n"));

		}
		if (p_ptr->muta3 & MUT3_RESILIENT)
		{
			fprintf(OutFile, _(" あなたの体は弾力性に富んでいる。(耐久+4)\n", " You are very resilient (+4 CON).\n"));

		}
		if (p_ptr->muta3 & MUT3_XTRA_FAT)
		{
			fprintf(OutFile, _(" あなたは極端に太っている。(耐久+2,スピード-2)\n", " You are extremely fat (+2 CON, -2 speed).\n"));

		}
		if (p_ptr->muta3 & MUT3_ALBINO)
		{
			fprintf(OutFile, _(" あなたはアルビノだ。(耐久-4)\n", " You are albino (-4 CON).\n"));

		}
		if (p_ptr->muta3 & MUT3_FLESH_ROT)
		{
			fprintf(OutFile, _(" あなたの肉体は腐敗している。(耐久-2,魅力-1)\n", " Your flesh is rotting (-2 CON, -1 CHR).\n"));

		}
		if (p_ptr->muta3 & MUT3_SILLY_VOI)
		{
			fprintf(OutFile, _(" あなたの声は間抜けなキーキー声だ。(魅力-4)\n", " Your voice is a silly squeak (-4 CHR).\n"));

		}
		if (p_ptr->muta3 & MUT3_BLANK_FAC)
		{
			fprintf(OutFile, _(" あなたはのっぺらぼうだ。(魅力-1)\n", " Your face is featureless (-1 CHR).\n"));

		}
		if (p_ptr->muta3 & MUT3_ILL_NORM)
		{
			fprintf(OutFile, _(" あなたは幻影に覆われている。\n", " Your appearance is masked with illusion.\n"));

		}
		if (p_ptr->muta3 & MUT3_XTRA_EYES)
		{
			fprintf(OutFile, _(" あなたは余分に二つの目を持っている。(探索+15)\n", " You have an extra pair of eyes (+15 search).\n"));

		}
		if (p_ptr->muta3 & MUT3_MAGIC_RES)
		{
			fprintf(OutFile, _(" あなたは魔法への耐性をもっている。\n", " You are resistant to magic.\n"));

		}
		if (p_ptr->muta3 & MUT3_XTRA_NOIS)
		{
			fprintf(OutFile, _(" あなたは変な音を発している。(隠密-3)\n", " You make a lot of strange noise (-3 stealth).\n"));

		}
		if (p_ptr->muta3 & MUT3_INFRAVIS)
		{
			fprintf(OutFile, _(" あなたは素晴らしい赤外線視力を持っている。(+3)\n", " You have remarkable infravision (+3).\n"));

		}
		if (p_ptr->muta3 & MUT3_XTRA_LEGS)
		{
			fprintf(OutFile, _(" あなたは余分に二本の足が生えている。(加速+3)\n", " You have an extra pair of legs (+3 speed).\n"));

		}
		if (p_ptr->muta3 & MUT3_SHORT_LEG)
		{
			fprintf(OutFile, _(" あなたの足は短い突起だ。(加速-3)\n", " Your legs are short stubs (-3 speed).\n"));

		}
		if (p_ptr->muta3 & MUT3_ELEC_TOUC)
		{
			fprintf(OutFile, _(" あなたの血管には電流が流れている。\n", " Electricity is running through your veins.\n"));

		}
		if (p_ptr->muta3 & MUT3_FIRE_BODY)
		{
			fprintf(OutFile, _(" あなたの体は炎につつまれている。\n", " Your body is enveloped in flames.\n"));

		}
		if (p_ptr->muta3 & MUT3_WART_SKIN)
		{
			fprintf(OutFile, _(" あなたの肌はイボに被われている。(魅力-2, AC+5)\n", " Your skin is covered with warts (-2 CHR, +5 AC).\n"));

		}
		if (p_ptr->muta3 & MUT3_SCALES)
		{
			fprintf(OutFile, _(" あなたの肌は鱗になっている。(魅力-1, AC+10)\n", " Your skin has turned into scales (-1 CHR, +10 AC).\n"));

		}
		if (p_ptr->muta3 & MUT3_IRON_SKIN)
		{
			fprintf(OutFile, _(" あなたの肌は鉄でできている。(器用-1, AC+25)\n", " Your skin is made of steel (-1 DEX, +25 AC).\n"));

		}
		if (p_ptr->muta3 & MUT3_WINGS)
		{
			fprintf(OutFile, _(" あなたは羽を持っている。\n", " You have wings.\n"));

		}
		if (p_ptr->muta3 & MUT3_FEARLESS)
		{
			fprintf(OutFile, _(" あなたは全く恐怖を感じない。\n", " You are completely fearless.\n"));

		}
		if (p_ptr->muta3 & MUT3_REGEN)
		{
			fprintf(OutFile, _(" あなたは急速に回復する。\n", " You are regenerating.\n"));

		}
		if (p_ptr->muta3 & MUT3_ESP)
		{
			fprintf(OutFile, _(" あなたはテレパシーを持っている。\n", " You are telepathic.\n"));

		}
		if (p_ptr->muta3 & MUT3_LIMBER)
		{
			fprintf(OutFile, _(" あなたの体は非常にしなやかだ。(器用+3)\n", " Your body is very limber (+3 DEX).\n"));

		}
		if (p_ptr->muta3 & MUT3_ARTHRITIS)
		{
			fprintf(OutFile, _(" あなたはいつも関節に痛みを感じている。(器用-3)\n", " Your joints ache constantly (-3 DEX).\n"));

		}
		if (p_ptr->muta3 & MUT3_VULN_ELEM)
		{
			fprintf(OutFile, _(" あなたは元素の攻撃に弱い。\n", " You are susceptible to damage from the elements.\n"));

		}
		if (p_ptr->muta3 & MUT3_MOTION)
		{
			fprintf(OutFile, _(" あなたの動作は正確で力強い。(隠密+1)\n", " Your movements are precise and forceful (+1 STL).\n"));

		}
		if (p_ptr->muta3 & MUT3_GOOD_LUCK)
		{
			fprintf(OutFile, _(" あなたは白いオーラにつつまれている。\n", " There is a white aura surrounding you.\n"));

		}
		if (p_ptr->muta3 & MUT3_BAD_LUCK)
		{
			fprintf(OutFile, _(" あなたは黒いオーラにつつまれている。\n", " There is a black aura surrounding you.\n"));

		}
	}
}

/*!
 * @brief 突然変異表示コマンドの実装 / List mutations we have...
 * @return なし
 */
void do_cmd_knowledge_mutations(void)
{
	FILE *fff;
	char file_name[1024];

	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);

	/* Dump the mutations to file */
	if (fff) dump_mutations(fff);

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(TRUE, file_name, _("突然変異", "Mutations"), 0, 0);

	/* Remove the file */
	fd_kill(file_name);
}

/*!
 * @brief 符号なし32ビット整数のビット数を返す。
 * @param x ビット数を調べたい変数
 * @return ビット数
 */
int count_bits(u32b x)
{
	int n = 0;

	if (x) do
	{
		n++;
	}
	while (0 != (x = x&(x-1)));

	return (n);
}

/*!
 * @brief 現在プレイヤー得ている突然変異の数を返す。
 * @return 現在得ている突然変異の数
 */
static int count_mutations(void)
{
	return (count_bits(p_ptr->muta1) +
		count_bits(p_ptr->muta2) +
		count_bits(p_ptr->muta3));
}


/*!
 * @brief 突然変異による自然回復ペナルティをパーセント値で返す /
 * Return the modifier to the regeneration rate (in percent)
 * @return ペナルティ修正(%)
 */
int calc_mutant_regenerate_mod(void)
{
	int regen;
	int mod = 10;
	int count = count_mutations();

	/*
	 * Beastman get 10 "free" mutations and
	 * only 5% decrease per additional mutation
	 */

	if (p_ptr->pseikaku == SEIKAKU_LUCKY) count--;
	if (p_ptr->prace == RACE_BEASTMAN)
	{
		count -= 10;
		mod = 5;
	}

	/* No negative modifier */
	if (count <= 0) return 100;

	regen = 100 - count * mod;

	/* Max. 90% decrease in regeneration speed */
	if (regen < 10) regen = 10;

	return (regen);
}

/*!
 * @brief 突然変異レイシャル上で口を使うよりを行った際に歌や呪術を停止する /
 * @return なし
 */
void mutation_stop_mouth()
{
	if (music_singing_any()) stop_singing();
	if (hex_spelling_any()) stop_hex_spell_all();
}


/*!
 * @brief 突然変異のレイシャル効果実装
 * @return 発動させる突然変異レイシャルのID
 * @return レイシャルを実行した場合TRUE、キャンセルした場合FALSEを返す
 */
bool mutation_power_aux(u32b power)
{
	int     dir = 0;
	int     lvl = p_ptr->lev;


	switch (power)
	{
		case MUT1_SPIT_ACID:
			if (!get_aim_dir(&dir)) return FALSE;
			mutation_stop_mouth();
			msg_print(_("酸を吐きかけた...", "You spit acid..."));

			fire_ball(GF_ACID, dir, lvl, 1 + (lvl / 30));
			break;

		case MUT1_BR_FIRE:
			if (!get_aim_dir(&dir)) return FALSE;
			mutation_stop_mouth();
			msg_print(_("あなたは火炎のブレスを吐いた...", "You breathe fire..."));

			fire_ball(GF_FIRE, dir, lvl * 2, 1 + (lvl / 20));
			break;

		case MUT1_HYPN_GAZE:
			if (!get_aim_dir(&dir)) return FALSE;
			msg_print(_("あなたの目は幻惑的になった...", "Your eyes look mesmerizing..."));

			(void)charm_monster(dir, lvl);
			break;

		case MUT1_TELEKINES:
			if (!get_aim_dir(&dir)) return FALSE;
			msg_print(_("集中している...", "You concentrate..."));

			fetch(dir, lvl * 10, TRUE);
			break;

		case MUT1_VTELEPORT:
			msg_print(_("集中している...", "You concentrate..."));

			teleport_player(10 + 4 * lvl, 0L);
			break;

		case MUT1_MIND_BLST:
			if (!get_aim_dir(&dir)) return FALSE;
			msg_print(_("集中している...", "You concentrate..."));

			fire_bolt(GF_PSI, dir, damroll(3 + ((lvl - 1) / 5), 3));
			break;

		case MUT1_RADIATION:
			msg_print(_("体から放射能が発生した！", "Radiation flows from your body!"));

			fire_ball(GF_NUKE, 0, (lvl * 2), 3 + (lvl / 20));
			break;

		case MUT1_VAMPIRISM:
			{
				int x, y, dummy;
				cave_type *c_ptr;

				/* Only works on adjacent monsters */
				if (!get_rep_dir2(&dir)) return FALSE;
				y = py + ddy[dir];
				x = px + ddx[dir];
				c_ptr = &cave[y][x];

				mutation_stop_mouth();

				if (!(c_ptr->m_idx))
				{
					msg_print(_("何もない場所に噛みついた！", "You bite into thin air!"));

					break;
				}

				msg_print(_("あなたはニヤリとして牙をむいた...", "You grin and bare your fangs..."));


				dummy = lvl * 2;

				if (drain_life(dir, dummy))
				{
					if (p_ptr->food < PY_FOOD_FULL)
						/* No heal if we are "full" */
						(void)hp_player(dummy);
					else
						msg_print(_("あなたは空腹ではありません。", "You were not hungry."));

					/* Gain nutritional sustenance: 150/hp drained */
					/* A Food ration gives 5000 food points (by contrast) */
					/* Don't ever get more than "Full" this way */
					/* But if we ARE Gorged,  it won't cure us */
					dummy = p_ptr->food + MIN(5000, 100 * dummy);
					if (p_ptr->food < PY_FOOD_MAX)   /* Not gorged already */
						(void)set_food(dummy >= PY_FOOD_MAX ? PY_FOOD_MAX-1 : dummy);
				}
				else
					msg_print(_("げぇ！ひどい味だ。", "Yechh. That tastes foul."));

			}
			break;

		case MUT1_SMELL_MET:
			mutation_stop_mouth();
			(void)detect_treasure(DETECT_RAD_DEFAULT);
			break;

		case MUT1_SMELL_MON:
			mutation_stop_mouth();
			(void)detect_monsters_normal(DETECT_RAD_DEFAULT);
			break;

		case MUT1_BLINK:
			teleport_player(10, 0L);
			break;

		case MUT1_EAT_ROCK:
			{
				int x, y;
				cave_type *c_ptr;
				feature_type *f_ptr, *mimic_f_ptr;

				if (!get_rep_dir2(&dir)) return FALSE;
				y = py + ddy[dir];
				x = px + ddx[dir];
				c_ptr = &cave[y][x];
				f_ptr = &f_info[c_ptr->feat];
				mimic_f_ptr = &f_info[get_feat_mimic(c_ptr)];

				mutation_stop_mouth();

				if (!have_flag(mimic_f_ptr->flags, FF_HURT_ROCK))
				{
					msg_print(_("この地形は食べられない。", "You cannot eat this feature."));
					break;
				}
				else if (have_flag(f_ptr->flags, FF_PERMANENT))
				{
					msg_format(_("いてっ！この%sはあなたの歯より硬い！", "Ouch!  This %s is harder than your teeth!"), f_name + mimic_f_ptr->name);
					break;
				}
				else if (c_ptr->m_idx)
				{
					monster_type *m_ptr = &m_list[c_ptr->m_idx];
					msg_print(_("何かが邪魔しています！", "There's something in the way!"));

					if (!m_ptr->ml || !is_pet(m_ptr)) py_attack(y, x, 0);
					break;
				}
				else if (have_flag(f_ptr->flags, FF_TREE))
				{
					msg_print(_("木の味は好きじゃない！", "You don't like the woody taste!"));
					break;
				}
				else if (have_flag(f_ptr->flags, FF_GLASS))
				{
					msg_print(_("ガラスの味は好きじゃない！", "You don't like the glassy taste!"));
					break;
				}
				else if (have_flag(f_ptr->flags, FF_DOOR) || have_flag(f_ptr->flags, FF_CAN_DIG))
				{
					(void)set_food(p_ptr->food + 3000);
				}
				else if (have_flag(f_ptr->flags, FF_MAY_HAVE_GOLD) || have_flag(f_ptr->flags, FF_HAS_GOLD))
				{
					(void)set_food(p_ptr->food + 5000);
				}
				else
				{
					msg_format(_("この%sはとてもおいしい！", "This %s is very filling!"), f_name + mimic_f_ptr->name);
					(void)set_food(p_ptr->food + 10000);
				}

				/* Destroy the wall */
				cave_alter_feat(y, x, FF_HURT_ROCK);

				/* Move the player */
				(void)move_player_effect(y, x, MPE_DONT_PICKUP);
			}
			break;

		case MUT1_SWAP_POS:
			project_length = -1;
			if (!get_aim_dir(&dir))
			{
				project_length = 0;
				return FALSE;
			}
			(void)teleport_swap(dir);
			project_length = 0;
			break;

		case MUT1_SHRIEK:
			mutation_stop_mouth();
			(void)fire_ball(GF_SOUND, 0, 2 * lvl, 8);
			(void)aggravate_monsters(0);
			break;

		case MUT1_ILLUMINE:
			(void)lite_area(damroll(2, (lvl / 2)), (lvl / 10) + 1);
			break;

		case MUT1_DET_CURSE:
			{
				int i;

				for (i = 0; i < INVEN_TOTAL; i++)
				{
					object_type *o_ptr = &inventory[i];

					if (!o_ptr->k_idx) continue;
					if (!object_is_cursed(o_ptr)) continue;

					o_ptr->feeling = FEEL_CURSED;
				}
			}
			break;

		case MUT1_BERSERK:
			(void)set_shero(randint1(25) + 25, FALSE);
			(void)hp_player(30);
			(void)set_afraid(0);
			break;

		case MUT1_POLYMORPH:
			if (!get_check(_("変身します。よろしいですか？", "You will polymorph your self. Are you sure? "))) return FALSE;
			do_poly_self();
			break;

		case MUT1_MIDAS_TCH:
			if (!alchemy()) return FALSE;
			break;

		/* Summon pet molds around the player */
		case MUT1_GROW_MOLD:
			{
				int i;
				for (i = 0; i < 8; i++)
				{
					summon_specific(-1, py, px, lvl, SUMMON_BIZARRE1, PM_FORCE_PET);
				}
			}
			break;

		case MUT1_RESIST:
			{
				int num = lvl / 10;
				int dur = randint1(20) + 20;

				if (randint0(5) < num)
				{
					(void)set_oppose_acid(dur, FALSE);
					num--;
				}
				if (randint0(4) < num)
				{
					(void)set_oppose_elec(dur, FALSE);
					num--;
				}
				if (randint0(3) < num)
				{
					(void)set_oppose_fire(dur, FALSE);
					num--;
				}
				if (randint0(2) < num)
				{
					(void)set_oppose_cold(dur, FALSE);
					num--;
				}
				if (num)
				{
					(void)set_oppose_pois(dur, FALSE);
					num--;
				}
			}
			break;

		case MUT1_EARTHQUAKE:
			(void)earthquake(py, px, 10);
			break;

		case MUT1_EAT_MAGIC:
			if (!eat_magic(p_ptr->lev * 2)) return FALSE;
			break;

		case MUT1_WEIGH_MAG:
			report_magics();
			break;

		case MUT1_STERILITY:
			/* Fake a population explosion. */
#ifdef JP
			msg_print("突然頭が痛くなった！");
			take_hit(DAMAGE_LOSELIFE, randint1(17) + 17, "禁欲を強いた疲労", -1);
#else
			msg_print("You suddenly have a headache!");
			take_hit(DAMAGE_LOSELIFE, randint1(17) + 17, "the strain of forcing abstinence", -1);
#endif

			num_repro += MAX_REPRO;
			break;

		case MUT1_PANIC_HIT:
			{
				int x, y;

				if (!get_rep_dir2(&dir)) return FALSE;
				y = py + ddy[dir];
				x = px + ddx[dir];
				if (cave[y][x].m_idx)
				{
					py_attack(y, x, 0);
					if (randint0(p_ptr->skill_dis) < 7)
						msg_print(_("うまく逃げられなかった。", "You failed to teleport."));
					else teleport_player(30, 0L);
				}
				else
				{
					msg_print(_("その方向にはモンスターはいません。", "You don't see any monster in this direction"));

					msg_print(NULL);
				}
			}
			break;

		case MUT1_DAZZLE:
			stun_monsters(lvl * 4);
			confuse_monsters(lvl * 4);
			turn_monsters(lvl * 4);
			break;

		case MUT1_LASER_EYE:
			if (!get_aim_dir(&dir)) return FALSE;
			fire_beam(GF_LITE, dir, 2 * lvl);
			break;

		case MUT1_RECALL:
			if (!word_of_recall()) return FALSE;
			break;

		case MUT1_BANISH:
			{
				int x, y;
				cave_type *c_ptr;
				monster_type *m_ptr;
				monster_race *r_ptr;

				if (!get_rep_dir2(&dir)) return FALSE;
				y = py + ddy[dir];
				x = px + ddx[dir];
				c_ptr = &cave[y][x];

				if (!c_ptr->m_idx)
				{
					msg_print(_("邪悪な存在を感じとれません！", "You sense no evil there!"));

					break;
				}

				m_ptr = &m_list[c_ptr->m_idx];
				r_ptr = &r_info[m_ptr->r_idx];

				if ((r_ptr->flags3 & RF3_EVIL) &&
				    !(r_ptr->flags1 & RF1_QUESTOR) &&
				    !(r_ptr->flags1 & RF1_UNIQUE) &&
				    !p_ptr->inside_arena && !p_ptr->inside_quest &&
					(r_ptr->level < randint1(p_ptr->lev+50)) &&
					!(m_ptr->mflag2 & MFLAG2_NOGENO))
				{
					if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname)
					{
						char m_name[80];

						monster_desc(m_name, m_ptr, MD_INDEF_VISIBLE);
						do_cmd_write_nikki(NIKKI_NAMED_PET, RECORD_NAMED_PET_GENOCIDE, m_name);
					}

					/* Delete the monster, rather than killing it. */
					delete_monster_idx(c_ptr->m_idx);
					msg_print(_("その邪悪なモンスターは硫黄臭い煙とともに消え去った！", "The evil creature vanishes in a puff of sulfurous smoke!"));

				}
				else
				{
					msg_print(_("祈りは効果がなかった！", "Your invocation is ineffectual!"));

					if (one_in_(13)) m_ptr->mflag2 |= MFLAG2_NOGENO;
				}
			}
			break;

		case MUT1_COLD_TOUCH:
			{
				int x, y;
				cave_type *c_ptr;

				if (!get_rep_dir2(&dir)) return FALSE;
				y = py + ddy[dir];
				x = px + ddx[dir];
				c_ptr = &cave[y][x];

				if (!c_ptr->m_idx)
				{
					msg_print(_("あなたは何もない場所で手を振った。", "You wave your hands in the air."));

					break;
				}
				fire_bolt(GF_COLD, dir, 2 * lvl);
			}
			break;

		/* XXX_XXX_XXX Hack!  MUT1_LAUNCHER is negative, see above */
		case 3: /* MUT1_LAUNCHER */
			/* Gives a multiplier of 2 at first, up to 3 at 40th */
			if (!do_cmd_throw_aux(2 + lvl / 40, FALSE, -1)) return FALSE;
			break;

		default:
			energy_use = 0;
			msg_format(_("能力 %s は実装されていません。", "Power %s not implemented. Oops."), power);
	}

	return TRUE;
}
