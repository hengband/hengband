/* File: mutation.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

/* Purpose: Mutation effects (and racial powers) */

#include "angband.h"


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
#ifdef JP
muta_desc = "酸を吐く能力を得た。";
#else
			muta_desc = "You gain the ability to spit acid.";
#endif

			break;
		case 5: case 6: case 7:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_BR_FIRE;
#ifdef JP
muta_desc = "火を吐く能力を得た。";
#else
			muta_desc = "You gain the ability to breathe fire.";
#endif

			break;
		case 8: case 9:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_HYPN_GAZE;
#ifdef JP
muta_desc = "催眠眼の能力を得た。";
#else
			muta_desc = "Your eyes look mesmerizing...";
#endif

			break;
		case 10: case 11:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_TELEKINES;
#ifdef JP
muta_desc = "物体を念動力で動かす能力を得た。";
#else
			muta_desc = "You gain the ability to move objects telekinetically.";
#endif

			break;
		case 12: case 13: case 14:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_VTELEPORT;
#ifdef JP
muta_desc = "自分の意思でテレポートする能力を得た。";
#else
			muta_desc = "You gain the power of teleportation at will.";
#endif

			break;
		case 15: case 16:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_MIND_BLST;
#ifdef JP
muta_desc = "精神攻撃の能力を得た。";
#else
			muta_desc = "You gain the power of Mind Blast.";
#endif

			break;
		case 17: case 18:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_RADIATION;
#ifdef JP
muta_desc = "あなたは強い放射線を発生し始めた。";
#else
			muta_desc = "You start emitting hard radiation.";
#endif

			break;
		case 19: case 20:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_VAMPIRISM;
#ifdef JP
muta_desc = "生命力を吸収できるようになった。";
#else
			muta_desc = "You become vampiric.";
#endif

			break;
		case 21: case 22: case 23:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_SMELL_MET;
#ifdef JP
muta_desc = "金属の匂いを嗅ぎ分けられるようになった。";
#else
			muta_desc = "You smell a metallic odor.";
#endif

			break;
		case 24: case 25: case 26: case 27:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_SMELL_MON;
#ifdef JP
muta_desc = "モンスターの臭いを嗅ぎ分けられるようになった。";
#else
			muta_desc = "You smell filthy monsters.";
#endif

			break;
		case 28: case 29: case 30:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_BLINK;
#ifdef JP
muta_desc = "近距離テレポートの能力を得た。";
#else
			muta_desc = "You gain the power of minor teleportation.";
#endif

			break;
		case 31: case 32:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_EAT_ROCK;
#ifdef JP
muta_desc = "壁が美味しそうに見える。";
#else
			muta_desc = "The walls look delicious.";
#endif

			break;
		case 33: case 34:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_SWAP_POS;
#ifdef JP
muta_desc = "他人の靴で一マイル歩くような気分がする。";
#else
			muta_desc = "You feel like walking a mile in someone else's shoes.";
#endif

			break;
		case 35: case 36: case 37:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_SHRIEK;
#ifdef JP
muta_desc = "あなたの声は相当強くなった。";
#else
			muta_desc = "Your vocal cords get much tougher.";
#endif

			break;
		case 38: case 39: case 40:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_ILLUMINE;
#ifdef JP
muta_desc = "あなたは光り輝いて部屋を明るくするようになった。";
#else
			muta_desc = "You can light up rooms with your presence.";
#endif

			break;
		case 41: case 42:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_DET_CURSE;
#ifdef JP
muta_desc = "邪悪な魔法を感知できるようになった。";
#else
			muta_desc = "You can feel evil magics.";
#endif

			break;
		case 43: case 44: case 45:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_BERSERK;
#ifdef JP
muta_desc = "制御できる激情を感じる。";
#else
			muta_desc = "You feel a controlled rage.";
#endif

			break;
		case 46:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_POLYMORPH;
#ifdef JP
muta_desc = "体が変異しやすくなった。";
#else
			muta_desc = "Your body seems mutable.";
#endif

			break;
		case 47: case 48:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_MIDAS_TCH;
#ifdef JP
muta_desc = "「ミダス王の手」の能力を得た。";/*トゥームレイダースにありましたね。 */
#else
			muta_desc = "You gain the Midas touch.";
#endif

			break;
		case 49:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_GROW_MOLD;
#ifdef JP
muta_desc = "突然カビに親しみを覚えた。";
#else
			muta_desc = "You feel a sudden affinity for mold.";
#endif

			break;
		case 50: case 51: case 52:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_RESIST;
#ifdef JP
muta_desc = "あなたは自分自身を守れる気がする。";
#else
			muta_desc = "You feel like you can protect yourself.";
#endif

			break;
		case 53: case 54: case 55:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_EARTHQUAKE;
#ifdef JP
muta_desc = "ダンジョンを破壊する能力を得た。";
#else
			muta_desc = "You gain the ability to wreck the dungeon.";
#endif

			break;
		case 56:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_EAT_MAGIC;
#ifdef JP
muta_desc = "魔法のアイテムが美味そうに見える。";
#else
			muta_desc = "Your magic items look delicious.";
#endif

			break;
		case 57: case 58:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_WEIGH_MAG;
#ifdef JP
muta_desc = "あなたは周囲にある魔法をより良く理解できる気がする。";
#else
			muta_desc = "You feel you can better understand the magic around you.";
#endif

			break;
		case 59:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_STERILITY;
#ifdef JP
muta_desc = "周りの全ての者に頭痛を起こすことができる。";
#else
			muta_desc = "You can give everything around you a headache.";
#endif

			break;
		case 60: case 61:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_PANIC_HIT;
#ifdef JP
muta_desc = "突然、泥棒の気分が分かるようになった。";
#else
			muta_desc = "You suddenly understand how thieves feel.";
#endif

			break;
		case 62: case 63: case 64:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_DAZZLE;
#ifdef JP
muta_desc = "眩い閃光を発する能力を得た。";
#else
			muta_desc = "You gain the ability to emit dazzling lights.";
#endif

			break;
		case 65: case 66: case 67:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_LASER_EYE;
#ifdef JP
muta_desc = "あなたの目は一瞬焼け付いた。";
#else
			muta_desc = "Your eyes burn for a moment.";
#endif

			break;
		case 68: case 69:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_RECALL;
#ifdef JP
muta_desc = "少しだけホームシックになったが、すぐ直った。";
#else
			muta_desc = "You feel briefly homesick, but it passes.";
#endif

			break;
		case 70:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_BANISH;
#ifdef JP
muta_desc = "神聖な怒りの力に満たされた。";
#else
			muta_desc = "You feel a holy wrath fill you.";
#endif

			break;
		case 71: case 72:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_COLD_TOUCH;
#ifdef JP
muta_desc = "あなたの両手はとても冷たくなった。";
#else
			muta_desc = "Your hands get very cold.";
#endif

			break;
		case 73: case 74:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_LAUNCHER;
#ifdef JP
muta_desc = "あなたの物を投げる手はかなり強くなった気がする。";
#else
			muta_desc = "Your throwing arm feels much stronger.";
#endif

			break;
		case 75:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_BERS_RAGE;
#ifdef JP
muta_desc = "あなたは狂暴化の発作を起こすようになった！";
#else
			muta_desc = "You become subject to fits of berserk rage!";
#endif

			break;
		case 76:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_COWARDICE;
#ifdef JP
muta_desc = "信じられないくらい臆病になった！";
#else
			muta_desc = "You become an incredible coward!";
#endif

			break;
		case 77:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_RTELEPORT;
#ifdef JP
muta_desc = "あなたの位置は非常に不確定になった。";
#else
			muta_desc = "Your position seems very uncertain...";
#endif

			break;
		case 78:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_ALCOHOL;
#ifdef JP
muta_desc = "あなたはアルコールを分泌するようになった。";
#else
			muta_desc = "Your body starts producing alcohol!";
#endif

			break;
		case 79:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_HALLU;
#ifdef JP
muta_desc = "あなたは幻覚を引き起こす精神錯乱に侵された。";
#else
			muta_desc = "You are afflicted by a hallucinatory insanity!";
#endif

			break;
		case 80:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_FLATULENT;
#ifdef JP
muta_desc = "あなたは制御不能な強烈な屁をこくようになった。";
#else
			muta_desc = "You become subject to uncontrollable flatulence.";
#endif

			break;
		case 81: case 82:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_SCOR_TAIL;
#ifdef JP
muta_desc = "サソリの尻尾が生えてきた！";
#else
			muta_desc = "You grow a scorpion tail!";
#endif

			break;
		case 83: case 84:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_HORNS;
#ifdef JP
muta_desc = "額に角が生えた！";
#else
			muta_desc = "Horns pop forth into your forehead!";
#endif

			break;
		case 85: case 86:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_BEAK;
#ifdef JP
muta_desc = "口が鋭く強いクチバシに変化した！";
#else
			muta_desc = "Your mouth turns into a sharp, powerful beak!";
#endif

			break;
		case 87: case 88:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_ATT_DEMON;
#ifdef JP
muta_desc = "悪魔を引き付けるようになった。";
#else
			muta_desc = "You start attracting demons.";
#endif

			break;
		case 89:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_PROD_MANA;
#ifdef JP
muta_desc = "あなたは制御不能な魔法のエネルギーを発生するようになった。";
#else
			muta_desc = "You start producing magical energy uncontrollably.";
#endif

			break;
		case 90: case 91:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_SPEED_FLUX;
#ifdef JP
muta_desc = "あなたは躁鬱質になった。";
#else
			muta_desc = "You become manic-depressive.";
#endif

			break;
		case 92: case 93:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_BANISH_ALL;
#ifdef JP
muta_desc = "恐ろしい力があなたの背後に潜んでいる気がする。";
#else
			muta_desc = "You feel a terrifying power lurking behind you.";
#endif

			break;
		case 94:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_EAT_LIGHT;
#ifdef JP
muta_desc = "あなたはウンゴリアントに奇妙な親しみを覚えるようになった。";
#else
			muta_desc = "You feel a strange kinship with Ungoliant.";
#endif

			break;
		case 95: case 96:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_TRUNK;
#ifdef JP
muta_desc = "あなたの鼻は伸びて象の鼻のようになった。";
#else
			muta_desc = "Your nose grows into an elephant-like trunk.";
#endif

			break;
		case 97:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_ATT_ANIMAL;
#ifdef JP
muta_desc = "動物を引き付けるようになった。";
#else
			muta_desc = "You start attracting animals.";
#endif

			break;
		case 98:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_TENTACLES;
#ifdef JP
muta_desc = "邪悪な触手が体の両側に生えてきた。";
#else
			muta_desc = "Evil-looking tentacles sprout from your sides.";
#endif

			break;
		case 99:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_RAW_CHAOS;
#ifdef JP
muta_desc = "周囲の空間が不安定になった気がする。";
#else
			muta_desc = "You feel the universe is less stable around you.";
#endif

			break;
		case 100: case 101: case 102:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_NORMALITY;
#ifdef JP
muta_desc = "あなたは奇妙なほど普通になった気がする。";
#else
			muta_desc = "You feel strangely normal.";
#endif

			break;
		case 103:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WRAITH;
#ifdef JP
muta_desc = "あなたは幽体化したり実体化したりするようになった。";
#else
			muta_desc = "You start to fade in and out of the physical world.";
#endif

			break;
		case 104:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_POLY_WOUND;
#ifdef JP
muta_desc = "あなたはカオスの力が古い傷に入り込んでくるのを感じた。";
#else
			muta_desc = "You feel forces of chaos entering your old scars.";
#endif

			break;
		case 105:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WASTING;
#ifdef JP
muta_desc = "あなたは突然おぞましい衰弱病にかかった。";
#else
			muta_desc = "You suddenly contract a horrible wasting disease.";
#endif

			break;
		case 106:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_ATT_DRAGON;
#ifdef JP
muta_desc = "あなたはドラゴンを引きつけるようになった。";
#else
			muta_desc = "You start attracting dragons.";
#endif

			break;
		case 107: case 108:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WEIRD_MIND;
#ifdef JP
muta_desc = "あなたの思考は突然おかしな方向に向き始めた。";
#else
			muta_desc = "Your thoughts suddenly take off in strange directions.";
#endif

			break;
		case 109:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_NAUSEA;
#ifdef JP
muta_desc = "胃袋がピクピクしはじめた。";
#else
			muta_desc = "Your stomach starts to roil nauseously.";
#endif

			break;
		case 110: case 111:
			/* Chaos warriors already have a chaos deity */
			if (p_ptr->pclass != CLASS_CHAOS_WARRIOR)
			{
				muta_class = &(p_ptr->muta2);
				muta_which = MUT2_CHAOS_GIFT;
#ifdef JP
muta_desc = "あなたはカオスの守護悪魔の注意を惹くようになった。";
#else
				muta_desc = "You attract the notice of a chaos deity!";
#endif

			}
			break;
		case 112:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WALK_SHAD;
#ifdef JP
muta_desc = "あなたは現実が紙のように薄いと感じるようになった。";
#else
			muta_desc = "You feel like reality is as thin as paper.";
#endif

			break;
		case 113: case 114:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WARNING;
#ifdef JP
muta_desc = "あなたは突然パラノイアになった気がする。";
#else
			muta_desc = "You suddenly feel paranoid.";
#endif

			break;
		case 115:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_INVULN;
#ifdef JP
muta_desc = "あなたは祝福され、無敵状態になる発作を起こすようになった。";
#else
			muta_desc = "You are blessed with fits of invulnerability.";
#endif

			break;
		case 116: case 117:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_SP_TO_HP;
#ifdef JP
muta_desc = "魔法の治癒の発作を起こすようになった。";
#else
			muta_desc = "You are subject to fits of magical healing.";
#endif

			break;
		case 118:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_HP_TO_SP;
#ifdef JP
muta_desc = "痛みを伴う精神明瞭化の発作を起こすようになった。";
#else
			muta_desc = "You are subject to fits of painful clarity.";
#endif

			break;
		case 119:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_DISARM;
#ifdef JP
muta_desc = "あなたの脚は長さが四倍になった。";
#else
			muta_desc = "Your feet grow to four times their former size.";
#endif

			break;
		case 120: case 121: case 122:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_HYPER_STR;
#ifdef JP
muta_desc = "超人的に強くなった！";
#else
			muta_desc = "You turn into a superhuman he-man!";
#endif

			break;
		case 123: case 124: case 125:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_PUNY;
#ifdef JP
muta_desc = "筋肉が弱ってしまった...";
#else
			muta_desc = "Your muscles wither away...";
#endif

			break;
		case 126: case 127: case 128:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_HYPER_INT;
#ifdef JP
muta_desc = "あなたの脳は生体コンピュータに進化した！";
#else
			muta_desc = "Your brain evolves into a living computer!";
#endif

			break;
		case 129: case 130: case 131:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_MORONIC;
#ifdef JP
muta_desc = "脳が萎縮してしまった...";
#else
			muta_desc = "Your brain withers away...";
#endif

			break;
		case 132: case 133:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_RESILIENT;
#ifdef JP
muta_desc = "並外れてタフになった。";
#else
			muta_desc = "You become extraordinarily resilient.";
#endif

			break;
		case 134: case 135:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_XTRA_FAT;
#ifdef JP
muta_desc = "あなたは気持ち悪いくらい太った！";
#else
			muta_desc = "You become sickeningly fat!";
#endif

			break;
		case 136: case 137:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ALBINO;
#ifdef JP
muta_desc = "アルビノになった！弱くなった気がする...";
#else
			muta_desc = "You turn into an albino! You feel frail...";
#endif

			break;
		case 138: case 139: case 140:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_FLESH_ROT;
#ifdef JP
muta_desc = "あなたの肉体は腐敗する病気に侵された！";
#else
			muta_desc = "Your flesh is afflicted by a rotting disease!";
#endif

			break;
		case 141: case 142:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_SILLY_VOI;
#ifdef JP
muta_desc = "声が間抜けなキーキー声になった！";
#else
			muta_desc = "Your voice turns into a ridiculous squeak!";
#endif

			break;
		case 143: case 144:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_BLANK_FAC;
#ifdef JP
muta_desc = "のっぺらぼうになった！";
#else
			muta_desc = "Your face becomes completely featureless!";
#endif

			break;
		case 145:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ILL_NORM;
#ifdef JP
muta_desc = "心の安らぐ幻影を映し出すようになった。";
#else
			muta_desc = "You start projecting a reassuring image.";
#endif

			break;
		case 146: case 147: case 148:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_XTRA_EYES;
#ifdef JP
muta_desc = "新たに二つの目が出来た！";
#else
			muta_desc = "You grow an extra pair of eyes!";
#endif

			break;
		case 149: case 150:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_MAGIC_RES;
#ifdef JP
muta_desc = "魔法への耐性がついた。";
#else
			muta_desc = "You become resistant to magic.";
#endif

			break;
		case 151: case 152: case 153:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_XTRA_NOIS;
#ifdef JP
muta_desc = "あなたは奇妙な音を立て始めた！";
#else
			muta_desc = "You start making strange noise!";
#endif

			break;
		case 154: case 155: case 156:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_INFRAVIS;
#ifdef JP
muta_desc = "赤外線視力が増した。";
#else
			muta_desc = "Your infravision is improved.";
#endif

			break;
		case 157: case 158:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_XTRA_LEGS;
#ifdef JP
muta_desc = "新たに二本の足が生えてきた！";
#else
			muta_desc = "You grow an extra pair of legs!";
#endif

			break;
		case 159: case 160:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_SHORT_LEG;
#ifdef JP
muta_desc = "足が短い突起になってしまった！";
#else
			muta_desc = "Your legs turn into short stubs!";
#endif

			break;
		case 161: case 162:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ELEC_TOUC;
#ifdef JP
muta_desc = "血管を電流が流れ始めた！";
#else
			muta_desc = "Electricity starts running through you!";
#endif

			break;
		case 163: case 164:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_FIRE_BODY;
#ifdef JP
muta_desc = "あなたの体は炎につつまれている。";
#else
			muta_desc = "Your body is enveloped in flames!";
#endif

			break;
		case 165: case 166: case 167:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_WART_SKIN;
#ifdef JP
muta_desc = "気持ち悪いイボイボが体中にできた！";
#else
			muta_desc = "Disgusting warts appear everywhere on you!";
#endif

			break;
		case 168: case 169: case 170:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_SCALES;
#ifdef JP
muta_desc = "肌が黒い鱗に変わった！";
#else
			muta_desc = "Your skin turns into black scales!";
#endif

			break;
		case 171: case 172:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_IRON_SKIN;
#ifdef JP
muta_desc = "あなたの肌は鉄になった！";
#else
			muta_desc = "Your skin turns to steel!";
#endif

			break;
		case 173: case 174:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_WINGS;
#ifdef JP
muta_desc = "背中に羽が生えた。";
#else
			muta_desc = "You grow a pair of wings.";
#endif

			break;
		case 175: case 176: case 177:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_FEARLESS;
#ifdef JP
muta_desc = "完全に怖れ知らずになった。";
#else
			muta_desc = "You become completely fearless.";
#endif

			break;
		case 178: case 179:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_REGEN;
#ifdef JP
muta_desc = "急速に回復し始めた。";
#else
			muta_desc = "You start regenerating.";
#endif

			break;
		case 180: case 181:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ESP;
#ifdef JP
muta_desc = "テレパシーの能力を得た！";
#else
			muta_desc = "You develop a telepathic ability!";
#endif

			break;
		case 182: case 183: case 184:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_LIMBER;
#ifdef JP
muta_desc = "筋肉がしなやかになった。";
#else
			muta_desc = "Your muscles become limber.";
#endif

			break;
		case 185: case 186: case 187:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ARTHRITIS;
#ifdef JP
muta_desc = "関節が突然痛み出した。";
#else
			muta_desc = "Your joints suddenly hurt.";
#endif

			break;
		case 188:
			if (p_ptr->pseikaku == SEIKAKU_LUCKY) break;
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_BAD_LUCK;
#ifdef JP
muta_desc = "悪意に満ちた黒いオーラがあなたをとりまいた...";
#else
			muta_desc = "There is a malignant black aura surrounding you...";
#endif

			break;
		case 189:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_VULN_ELEM;
#ifdef JP
muta_desc = "妙に無防備になった気がする。";
#else
			muta_desc = "You feel strangely exposed.";
#endif

			break;
		case 190: case 191: case 192:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_MOTION;
#ifdef JP
muta_desc = "体の動作がより正確になった。";
#else
			muta_desc = "You move with new assurance.";
#endif

			break;
		case 193:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_GOOD_LUCK;
#ifdef JP
muta_desc = "慈悲深い白いオーラがあなたをとりまいた...";
#else
			muta_desc = "There is a benevolent white aura surrounding you...";
#endif

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
#ifdef JP
msg_print("普通になった気がする。");
#else
		msg_print("You feel normal.");
#endif

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
#ifdef JP
muta_desc = "眼が幻惑的になった...";
#else
				muta_desc = "Your eyes look mesmerizing...";
#endif

			}

			else if (p_ptr->prace == RACE_IMP &&
				 !(p_ptr->muta2 & MUT2_HORNS) &&
				 (randint1(10) < 7))
			  {
				muta_class = &(p_ptr->muta2);
				muta_which = MUT2_HORNS;
#ifdef JP
muta_desc = "角が額から生えてきた！";
#else
				muta_desc = "Horns pop forth into your forehead!";
#endif

			}

			else if (p_ptr->prace == RACE_YEEK &&
				!(p_ptr->muta1 & MUT1_SHRIEK) &&
				(randint1(10) < 7))
			{
				muta_class = &(p_ptr->muta1);
				muta_which = MUT1_SHRIEK;
#ifdef JP
muta_desc = "声質がかなり強くなった。";
#else
				muta_desc = "Your vocal cords get much tougher.";
#endif

			}

			else if (p_ptr->prace == RACE_BEASTMAN &&
				!(p_ptr->muta1 & MUT1_POLYMORPH) &&
				(randint1(10) < 2))
			{
				muta_class = &(p_ptr->muta1);
				muta_which = MUT1_POLYMORPH;
#ifdef JP
muta_desc = "あなたの肉体は変化できるようになった、";
#else
				muta_desc = "Your body seems mutable.";
#endif

			}

			else if (p_ptr->prace == RACE_MIND_FLAYER &&
				!(p_ptr->muta2 & MUT2_TENTACLES) &&
				(randint1(10) < 7))
			{
				muta_class = &(p_ptr->muta2);
				muta_which = MUT2_TENTACLES;
#ifdef JP
muta_desc = "邪悪な触手が口の周りに生えた。";
#else
				muta_desc = "Evil-looking tentacles sprout from your mouth.";
#endif

			}
		}
#ifdef JP
msg_print("突然変異した！");
#else
		msg_print("You mutate!");
#endif

		msg_print(muta_desc);
		*muta_class |= muta_which;

		if (muta_class == &(p_ptr->muta3))
		{
			if (muta_which == MUT3_PUNY)
			{
				if (p_ptr->muta3 & MUT3_HYPER_STR)
				{
#ifdef JP
msg_print("あなたはもう超人的に強くはない！");
#else
					msg_print("You no longer feel super-strong!");
#endif

					p_ptr->muta3 &= ~(MUT3_HYPER_STR);
				}
			}
			else if (muta_which == MUT3_HYPER_STR)
			{
				if (p_ptr->muta3 & MUT3_PUNY)
				{
#ifdef JP
msg_print("あなたはもう虚弱ではない！");
#else
					msg_print("You no longer feel puny!");
#endif

					p_ptr->muta3 &= ~(MUT3_PUNY);
				}
			}
			else if (muta_which == MUT3_MORONIC)
			{
				if (p_ptr->muta3 & MUT3_HYPER_INT)
				{
#ifdef JP
msg_print("あなたの脳はもう生体コンピュータではない。");
#else
					msg_print("Your brain is no longer a living computer.");
#endif

					p_ptr->muta3 &= ~(MUT3_HYPER_INT);
				}
			}
			else if (muta_which == MUT3_HYPER_INT)
			{
				if (p_ptr->muta3 & MUT3_MORONIC)
				{
#ifdef JP
msg_print("あなたはもう精神薄弱ではない。");
#else
					msg_print("You are no longer moronic.");
#endif

					p_ptr->muta3 &= ~(MUT3_MORONIC);
				}
			}
			else if (muta_which == MUT3_IRON_SKIN)
			{
				if (p_ptr->muta3 & MUT3_SCALES)
				{
#ifdef JP
msg_print("鱗がなくなった。");
#else
					msg_print("You lose your scales.");
#endif

					p_ptr->muta3 &= ~(MUT3_SCALES);
				}
				if (p_ptr->muta3 & MUT3_FLESH_ROT)
				{
#ifdef JP
msg_print("肉体が腐乱しなくなった。");
#else
					msg_print("Your flesh rots no longer.");
#endif

					p_ptr->muta3 &= ~(MUT3_FLESH_ROT);
				}
				if (p_ptr->muta3 & MUT3_WART_SKIN)
				{
#ifdef JP
msg_print("肌のイボイボがなくなった。");
#else
					msg_print("You lose your warts.");
#endif

					p_ptr->muta3 &= ~(MUT3_WART_SKIN);
				}
			}
			else if (muta_which == MUT3_WART_SKIN || muta_which == MUT3_SCALES
				|| muta_which == MUT3_FLESH_ROT)
			{
				if (p_ptr->muta3 & MUT3_IRON_SKIN)
				{
#ifdef JP
msg_print("あなたの肌はもう鉄ではない。");
#else
					msg_print("Your skin is no longer made of steel.");
#endif

					p_ptr->muta3 &= ~(MUT3_IRON_SKIN);
				}
			}
			else if (muta_which == MUT3_FEARLESS)
			{
				if (p_ptr->muta2 & MUT2_COWARDICE)
				{
#ifdef JP
msg_print("臆病でなくなった。");
#else
					msg_print("You are no longer cowardly.");
#endif

					p_ptr->muta2 &= ~(MUT2_COWARDICE);
				}
			}
			else if (muta_which == MUT3_FLESH_ROT)
			{
				if (p_ptr->muta3 & MUT3_REGEN)
				{
#ifdef JP
msg_print("急速に回復しなくなった。");
#else
					msg_print("You stop regenerating.");
#endif

					p_ptr->muta3 &= ~(MUT3_REGEN);
				}
			}
			else if (muta_which == MUT3_REGEN)
			{
				if (p_ptr->muta3 & MUT3_FLESH_ROT)
				{
#ifdef JP
msg_print("肉体が腐乱しなくなった。");
#else
					msg_print("Your flesh stops rotting.");
#endif

					p_ptr->muta3 &= ~(MUT3_FLESH_ROT);
				}
			}
			else if (muta_which == MUT3_LIMBER)
			{
				if (p_ptr->muta3 & MUT3_ARTHRITIS)
				{
#ifdef JP
msg_print("関節が痛くなくなった。");
#else
					msg_print("Your joints stop hurting.");
#endif

					p_ptr->muta3 &= ~(MUT3_ARTHRITIS);
				}
			}
			else if (muta_which == MUT3_ARTHRITIS)
			{
				if (p_ptr->muta3 & MUT3_LIMBER)
				{
#ifdef JP
msg_print("あなたはしなやかでなくなった。");
#else
					msg_print("You no longer feel limber.");
#endif

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
#ifdef JP
msg_print("恐れ知らずでなくなった。");
#else
					msg_print("You no longer feel fearless.");
#endif

					p_ptr->muta3 &= ~(MUT3_FEARLESS);
				}
			}
			if (muta_which == MUT2_BEAK)
			{
				if (p_ptr->muta2 & MUT2_TRUNK)
				{
#ifdef JP
msg_print("あなたの鼻はもう象の鼻のようではなくなった。");
#else
					msg_print("Your nose is no longer elephantine.");
#endif

					p_ptr->muta2 &= ~(MUT2_TRUNK);
				}
			}
			if (muta_which == MUT2_TRUNK)
			{
				if (p_ptr->muta2 & MUT2_BEAK)
				{
#ifdef JP
msg_print("硬いクチバシがなくなった。");
#else
					msg_print("You no longer have a hard beak.");
#endif

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
#ifdef JP
muta_desc = "酸を吹きかける能力を失った。";
#else
			muta_desc = "You lose the ability to spit acid.";
#endif

			break;
		case 5: case 6: case 7:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_BR_FIRE;
#ifdef JP
muta_desc = "炎のブレスを吐く能力を失った。";
#else
			muta_desc = "You lose the ability to breathe fire.";
#endif

			break;
		case 8: case 9:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_HYPN_GAZE;
#ifdef JP
muta_desc = "あなたの目はつまらない目になった。";
#else
			muta_desc = "Your eyes look uninteresting.";
#endif

			break;
		case 10: case 11:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_TELEKINES;
#ifdef JP
muta_desc = "念動力で物を動かす能力を失った。";
#else
			muta_desc = "You lose the ability to move objects telekinetically.";
#endif

			break;
		case 12: case 13: case 14:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_VTELEPORT;
#ifdef JP
muta_desc = "自分の意思でテレポートする能力を失った。";
#else
			muta_desc = "You lose the power of teleportation at will.";
#endif

			break;
		case 15: case 16:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_MIND_BLST;
#ifdef JP
muta_desc = "精神攻撃の能力を失った。";
#else
			muta_desc = "You lose the power of Mind Blast.";
#endif

			break;
		case 17: case 18:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_RADIATION;
#ifdef JP
muta_desc = "あなたは放射能を発生しなくなった。";
#else
			muta_desc = "You stop emitting hard radiation.";
#endif

			break;
		case 19: case 20:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_VAMPIRISM;
#ifdef JP
muta_desc = "吸血の能力を失った。";
#else
			muta_desc = "You are no longer vampiric.";
#endif

			break;
		case 21: case 22: case 23:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_SMELL_MET;
#ifdef JP
muta_desc = "金属の臭いを嗅げなくなった。";
#else
			muta_desc = "You no longer smell a metallic odor.";
#endif

			break;
		case 24: case 25: case 26: case 27:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_SMELL_MON;
#ifdef JP
muta_desc = "不潔なモンスターの臭いを嗅げなくなった。";
#else
			muta_desc = "You no longer smell filthy monsters.";
#endif

			break;
		case 28: case 29: case 30:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_BLINK;
#ifdef JP
muta_desc = "近距離テレポートの能力を失った。";
#else
			muta_desc = "You lose the power of minor teleportation.";
#endif

			break;
		case 31: case 32:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_EAT_ROCK;
#ifdef JP
muta_desc = "壁は美味しそうに見えなくなった。";
#else
			muta_desc = "The walls look unappetizing.";
#endif

			break;
		case 33: case 34:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_SWAP_POS;
#ifdef JP
muta_desc = "あなたは自分の靴に留まる感じがする。";
#else
			muta_desc = "You feel like staying in your own shoes.";
#endif

			break;
		case 35: case 36: case 37:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_SHRIEK;
#ifdef JP
muta_desc = "あなたの声質は弱くなった。";
#else
			muta_desc = "Your vocal cords get much weaker.";
#endif

			break;
		case 38: case 39: case 40:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_ILLUMINE;
#ifdef JP
muta_desc = "部屋を明るく照らすことが出来なくなった。";
#else
			muta_desc = "You can no longer light up rooms with your presence.";
#endif

			break;
		case 41: case 42:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_DET_CURSE;
#ifdef JP
muta_desc = "邪悪な魔法を感じられなくなった。";
#else
			muta_desc = "You can no longer feel evil magics.";
#endif

			break;
		case 43: case 44: case 45:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_BERSERK;
#ifdef JP
muta_desc = "制御できる激情を感じなくなった。";
#else
			muta_desc = "You no longer feel a controlled rage.";
#endif

			break;
		case 46:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_POLYMORPH;
#ifdef JP
muta_desc = "あなたの体は安定したように見える。";
#else
			muta_desc = "Your body seems stable.";
#endif

			break;
		case 47: case 48:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_MIDAS_TCH;
#ifdef JP
muta_desc = "ミダスの手の能力を失った。";
#else
			muta_desc = "You lose the Midas touch.";
#endif

			break;
		case 49:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_GROW_MOLD;
#ifdef JP
muta_desc = "突然カビが嫌いになった。";
#else
			muta_desc = "You feel a sudden dislike for mold.";
#endif

			break;
		case 50: case 51: case 52:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_RESIST;
#ifdef JP
muta_desc = "傷つき易くなった気がする。";
#else
			muta_desc = "You feel like you might be vulnerable.";
#endif

			break;
		case 53: case 54: case 55:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_EARTHQUAKE;
#ifdef JP
muta_desc = "ダンジョンを壊す能力を失った。";
#else
			muta_desc = "You lose the ability to wreck the dungeon.";
#endif

			break;
		case 56:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_EAT_MAGIC;
#ifdef JP
muta_desc = "魔法のアイテムはもう美味しそうに見えなくなった。";
#else
			muta_desc = "Your magic items no longer look delicious.";
#endif

			break;
		case 57: case 58:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_WEIGH_MAG;
#ifdef JP
muta_desc = "魔力を感じられなくなった。";
#else
			muta_desc = "You no longer sense magic.";
#endif

			break;
		case 59:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_STERILITY;
#ifdef JP
muta_desc = "たくさんの安堵の吐息が聞こえた。";
#else
			muta_desc = "You hear a massed sigh of relief.";
#endif

			break;
		case 60: case 61:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_PANIC_HIT;
#ifdef JP
muta_desc = "あちこちへ跳べる気分がなくなった。";
#else
			muta_desc = "You no longer feel jumpy.";
#endif

			break;
		case 62: case 63: case 64:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_DAZZLE;
#ifdef JP
muta_desc = "まばゆい閃光を発する能力を失った。";
#else
			muta_desc = "You lose the ability to emit dazzling lights.";
#endif

			break;
		case 65: case 66: case 67:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_LASER_EYE;
#ifdef JP
muta_desc = "眼が少しの間焼き付いて、痛みが和らいだ。";
#else
			muta_desc = "Your eyes burn for a moment, then feel soothed.";
#endif

			break;
		case 68: case 69:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_RECALL;
#ifdef JP
muta_desc = "少しの間ホームシックになった。";
#else
			muta_desc = "You feel briefly homesick.";
#endif

			break;
		case 70:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_BANISH;
#ifdef JP
muta_desc = "神聖な怒りの力を感じなくなった。";
#else
			muta_desc = "You no longer feel a holy wrath.";
#endif

			break;
		case 71: case 72:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_COLD_TOUCH;
#ifdef JP
muta_desc = "手が暖かくなった。";
#else
			muta_desc = "Your hands warm up.";
#endif

			break;
		case 73: case 74:
			muta_class = &(p_ptr->muta1);
			muta_which = MUT1_LAUNCHER;
#ifdef JP
muta_desc = "物を投げる手が弱くなった気がする。";
#else
			muta_desc = "Your throwing arm feels much weaker.";
#endif

			break;
		case 75:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_BERS_RAGE;
#ifdef JP
muta_desc = "凶暴化の発作にさらされなくなった！";
#else
			muta_desc = "You are no longer subject to fits of berserk rage!";
#endif

			break;
		case 76:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_COWARDICE;
#ifdef JP
muta_desc = "もう信じがたいほど臆病ではなくなった！";
#else
			muta_desc = "You are no longer an incredible coward!";
#endif

			break;
		case 77:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_RTELEPORT;
#ifdef JP
muta_desc = "あなたの位置はより確定的になった。";
#else
			muta_desc = "Your position seems more certain.";
#endif

			break;
		case 78:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_ALCOHOL;
#ifdef JP
muta_desc = "あなたはアルコールを分泌しなくなった！";
#else
			muta_desc = "Your body stops producing alcohol!";
#endif

			break;
		case 79:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_HALLU;
#ifdef JP
muta_desc = "幻覚をひき起こす精神障害を起こさなくなった！";
#else
			muta_desc = "You are no longer afflicted by a hallucinatory insanity!";
#endif

			break;
		case 80:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_FLATULENT;
#ifdef JP
muta_desc = "もう強烈な屁はこかなくなった。";
#else
			muta_desc = "You are no longer subject to uncontrollable flatulence.";
#endif

			break;
		case 81: case 82:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_SCOR_TAIL;
#ifdef JP
muta_desc = "サソリの尻尾がなくなった！";
#else
			muta_desc = "You lose your scorpion tail!";
#endif

			break;
		case 83: case 84:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_HORNS;
#ifdef JP
muta_desc = "額から角が消えた！";
#else
			muta_desc = "Your horns vanish from your forehead!";
#endif

			break;
		case 85: case 86:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_BEAK;
#ifdef JP
muta_desc = "口が普通に戻った！";
#else
			muta_desc = "Your mouth reverts to normal!";
#endif

			break;
		case 87: case 88:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_ATT_DEMON;
#ifdef JP
muta_desc = "デーモンを引き寄せなくなった。";
#else
			muta_desc = "You stop attracting demons.";
#endif

			break;
		case 89:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_PROD_MANA;
#ifdef JP
muta_desc = "制御不能な魔法のエネルギーを発生しなくなった。";
#else
			muta_desc = "You stop producing magical energy uncontrollably.";
#endif

			break;
		case 90: case 91:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_SPEED_FLUX;
#ifdef JP
muta_desc = "躁鬱質でなくなった。";
#else
			muta_desc = "You are no longer manic-depressive.";
#endif

			break;
		case 92: case 93:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_BANISH_ALL;
#ifdef JP
muta_desc = "背後に恐ろしい力を感じなくなった。";
#else
			muta_desc = "You no longer feel a terrifying power lurking behind you.";
#endif

			break;
		case 94:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_EAT_LIGHT;
#ifdef JP
muta_desc = "世界が明るいと感じる。";
#else
			muta_desc = "You feel the world's a brighter place.";
#endif

			break;
		case 95: case 96:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_TRUNK;
#ifdef JP
muta_desc = "鼻が普通の長さに戻った。";
#else
			muta_desc = "Your nose returns to a normal length.";
#endif

			break;
		case 97:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_ATT_ANIMAL;
#ifdef JP
muta_desc = "動物を引き寄せなくなった。";
#else
			muta_desc = "You stop attracting animals.";
#endif

			break;
		case 98:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_TENTACLES;
#ifdef JP
muta_desc = "触手が消えた。";
#else
			muta_desc = "Your tentacles vanish from your sides.";
#endif

			break;
		case 99:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_RAW_CHAOS;
#ifdef JP
muta_desc = "周囲の空間が安定した気がする。";
#else
			muta_desc = "You feel the universe is more stable around you.";
#endif

			break;
		case 100: case 101: case 102:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_NORMALITY;
#ifdef JP
muta_desc = "普通に奇妙な感じがする。";
#else
			muta_desc = "You feel normally strange.";
#endif

			break;
		case 103:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WRAITH;
#ifdef JP
muta_desc = "あなたは物質世界にしっかり存在している。";
#else
			muta_desc = "You are firmly in the physical world.";
#endif

			break;
		case 104:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_POLY_WOUND;
#ifdef JP
muta_desc = "古い傷からカオスの力が去っていった。";
#else
			muta_desc = "You feel forces of chaos departing your old scars.";
#endif

			break;
		case 105:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WASTING;
#ifdef JP
muta_desc = "おぞましい衰弱病が治った！";
#else
			muta_desc = "You are cured of the horrible wasting disease!";
#endif

			break;
		case 106:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_ATT_DRAGON;
#ifdef JP
muta_desc = "ドラゴンを引き寄せなくなった。";
#else
			muta_desc = "You stop attracting dragons.";
#endif

			break;
		case 107: case 108:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WEIRD_MIND;
#ifdef JP
muta_desc = "思考が退屈な方向に戻った。";
#else
			muta_desc = "Your thoughts return to boring paths.";
#endif

			break;
		case 109:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_NAUSEA;
#ifdef JP
muta_desc = "胃が痙攣しなくなった。";
#else
			muta_desc = "Your stomach stops roiling.";
#endif

			break;
		case 110: case 111:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_CHAOS_GIFT;
#ifdef JP
muta_desc = "混沌の神々の興味を惹かなくなった。";
#else
			muta_desc = "You lose the attention of the chaos deities.";
#endif

			break;
		case 112:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WALK_SHAD;
#ifdef JP
muta_desc = "物質世界に捕らわれている気がする。";
#else
			muta_desc = "You feel like you're trapped in reality.";
#endif

			break;
		case 113: case 114:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_WARNING;
#ifdef JP
muta_desc = "パラノイアでなくなった。";
#else
			muta_desc = "You no longer feel paranoid.";
#endif

			break;
		case 115:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_INVULN;
#ifdef JP
muta_desc = "無敵状態の発作を起こさなくなった。";
#else
			muta_desc = "You are no longer blessed with fits of invulnerability.";
#endif

			break;
		case 116: case 117:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_SP_TO_HP;
#ifdef JP
muta_desc = "魔法の治癒の発作に襲われなくなった。";
#else
			muta_desc = "You are no longer subject to fits of magical healing.";
#endif

			break;
		case 118:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_HP_TO_SP;
#ifdef JP
muta_desc = "痛みを伴う精神明瞭化の発作に襲われなくなった。";
#else
			muta_desc = "You are no longer subject to fits of painful clarity.";
#endif

			break;
		case 119:
			muta_class = &(p_ptr->muta2);
			muta_which = MUT2_DISARM;
#ifdef JP
muta_desc = "脚が元の大きさに戻った。";
#else
			muta_desc = "Your feet shrink to their former size.";
#endif

			break;
		case 120: case 121: case 122:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_HYPER_STR;
#ifdef JP
muta_desc = "筋肉が普通に戻った。";
#else
			muta_desc = "Your muscles revert to normal.";
#endif

			break;
		case 123: case 124: case 125:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_PUNY;
#ifdef JP
muta_desc = "筋肉が普通に戻った。";
#else
			muta_desc = "Your muscles revert to normal.";
#endif

			break;
		case 126: case 127: case 128:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_HYPER_INT;
#ifdef JP
muta_desc = "脳が普通に戻った。";
#else
			muta_desc = "Your brain reverts to normal.";
#endif

			break;
		case 129: case 130: case 131:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_MORONIC;
#ifdef JP
muta_desc = "脳が普通に戻った。";
#else
			muta_desc = "Your brain reverts to normal.";
#endif

			break;
		case 132: case 133:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_RESILIENT;
#ifdef JP
muta_desc = "普通の丈夫さに戻った。";
#else
			muta_desc = "You become ordinarily resilient again.";
#endif

			break;
		case 134: case 135:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_XTRA_FAT;
#ifdef JP
muta_desc = "奇跡的なダイエットに成功した！";
#else
			muta_desc = "You benefit from a miracle diet!";
#endif

			break;
		case 136: case 137:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ALBINO;
#ifdef JP
muta_desc = "アルビノでなくなった！";
#else
			muta_desc = "You are no longer an albino!";
#endif

			break;
		case 138: case 139: case 140:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_FLESH_ROT;
#ifdef JP
muta_desc = "肉体を腐敗させる病気が治った！";
#else
			muta_desc = "Your flesh is no longer afflicted by a rotting disease!";
#endif

			break;
		case 141: case 142:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_SILLY_VOI;
#ifdef JP
muta_desc = "声質が普通に戻った。";
#else
			muta_desc = "Your voice returns to normal.";
#endif

			break;
		case 143: case 144:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_BLANK_FAC;
#ifdef JP
muta_desc = "顔に目鼻が戻った。";
#else
			muta_desc = "Your facial features return.";
#endif

			break;
		case 145:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ILL_NORM;
#ifdef JP
muta_desc = "心が安らぐ幻影を映し出さなくなった。";
#else
			muta_desc = "You stop projecting a reassuring image.";
#endif

			break;
		case 146: case 147: case 148:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_XTRA_EYES;
#ifdef JP
muta_desc = "余分な目が消えてしまった！";
#else
			muta_desc = "Your extra eyes vanish!";
#endif

			break;
		case 149: case 150:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_MAGIC_RES;
#ifdef JP
muta_desc = "魔法に弱くなった。";
#else
			muta_desc = "You become susceptible to magic again.";
#endif

			break;
		case 151: case 152: case 153:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_XTRA_NOIS;
#ifdef JP
muta_desc = "奇妙な音を立てなくなった！";
#else
			muta_desc = "You stop making strange noise!";
#endif

			break;
		case 154: case 155: case 156:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_INFRAVIS;
#ifdef JP
muta_desc = "赤外線視力が落ちた。";
#else
			muta_desc = "Your infravision is degraded.";
#endif

			break;
		case 157: case 158:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_XTRA_LEGS;
#ifdef JP
muta_desc = "余分な脚が消えてしまった！";
#else
			muta_desc = "Your extra legs disappear!";
#endif

			break;
		case 159: case 160:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_SHORT_LEG;
#ifdef JP
muta_desc = "脚の長さが普通に戻った。";
#else
			muta_desc = "Your legs lengthen to normal.";
#endif

			break;
		case 161: case 162:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ELEC_TOUC;
#ifdef JP
muta_desc = "体を電流が流れなくなった。";
#else
			muta_desc = "Electricity stops running through you.";
#endif

			break;
		case 163: case 164:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_FIRE_BODY;
#ifdef JP
muta_desc = "体が炎に包まれなくなった。";
#else
			muta_desc = "Your body is no longer enveloped in flames.";
#endif

			break;
		case 165: case 166: case 167:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_WART_SKIN;
#ifdef JP
muta_desc = "イボイボが消えた！";
#else
			muta_desc = "Your warts disappear!";
#endif

			break;
		case 168: case 169: case 170:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_SCALES;
#ifdef JP
muta_desc = "鱗が消えた！";
#else
			muta_desc = "Your scales vanish!";
#endif

			break;
		case 171: case 172:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_IRON_SKIN;
#ifdef JP
muta_desc = "肌が肉にもどった！";
#else
			muta_desc = "Your skin reverts to flesh!";
#endif

			break;
		case 173: case 174:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_WINGS;
#ifdef JP
muta_desc = "背中の羽根が取れ落ちた。";
#else
			muta_desc = "Your wings fall off.";
#endif

			break;
		case 175: case 176: case 177:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_FEARLESS;
#ifdef JP
muta_desc = "再び恐怖を感じるようになった。";
#else
			muta_desc = "You begin to feel fear again.";
#endif

			break;
		case 178: case 179:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_REGEN;
#ifdef JP
muta_desc = "急速回復しなくなった。";
#else
			muta_desc = "You stop regenerating.";
#endif

			break;
		case 180: case 181:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ESP;
#ifdef JP
muta_desc = "テレパシーの能力を失った！";
#else
			muta_desc = "You lose your telepathic ability!";
#endif

			break;
		case 182: case 183: case 184:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_LIMBER;
#ifdef JP
muta_desc = "筋肉が硬くなった。";
#else
			muta_desc = "Your muscles stiffen.";
#endif

			break;
		case 185: case 186: case 187:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_ARTHRITIS;
#ifdef JP
muta_desc = "関節が痛くなくなった。";
#else
			muta_desc = "Your joints stop hurting.";
#endif

			break;
		case 188:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_BAD_LUCK;
#ifdef JP
muta_desc = "黒いオーラは渦巻いて消えた。";
#else
			muta_desc = "Your black aura swirls and fades.";
#endif

			break;
		case 189:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_VULN_ELEM;
#ifdef JP
muta_desc = "無防備な感じはなくなった。";
#else
			muta_desc = "You feel less exposed.";
#endif

			break;
		case 190: case 191: case 192:
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_MOTION;
#ifdef JP
muta_desc = "動作の正確さがなくなった。";
#else
			muta_desc = "You move with less assurance.";
#endif

			break;
		case 193:
			if (p_ptr->pseikaku == SEIKAKU_LUCKY) break;
			muta_class = &(p_ptr->muta3);
			muta_which = MUT3_GOOD_LUCK;
#ifdef JP
muta_desc = "白いオーラは輝いて消えた。";
#else
			muta_desc = "Your white aura shimmers and fades.";
#endif

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


void dump_mutations(FILE *OutFile)
{
	if (!OutFile) return;

	if (p_ptr->muta1)
	{
		if (p_ptr->muta1 & MUT1_SPIT_ACID)
		{
#ifdef JP
fprintf(OutFile, " あなたは酸を吹きかけることができる。(ダメージ レベルX1)\n");
#else
			fprintf(OutFile, " You can spit acid (dam lvl).\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_BR_FIRE)
		{
#ifdef JP
fprintf(OutFile, " あなたは炎のブレスを吐くことができる。(ダメージ レベルX2)\n");
#else
			fprintf(OutFile, " You can breathe fire (dam lvl * 2).\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_HYPN_GAZE)
		{
#ifdef JP
fprintf(OutFile, " あなたの睨みは催眠効果をもつ。\n");
#else
			fprintf(OutFile, " Your gaze is hypnotic.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_TELEKINES)
		{
#ifdef JP
fprintf(OutFile, " あなたは念動力をもっている。\n");
#else
			fprintf(OutFile, " You are telekinetic.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_VTELEPORT)
		{
#ifdef JP
fprintf(OutFile, " あなたは自分の意思でテレポートできる。\n");
#else
			fprintf(OutFile, " You can teleport at will.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_MIND_BLST)
		{
#ifdef JP
fprintf(OutFile, " あなたは敵を精神攻撃できる。\n");
#else
			fprintf(OutFile, " You can Mind Blast your enemies.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_RADIATION)
		{
#ifdef JP
fprintf(OutFile, " あなたは自分の意思で放射能を発生することができる。\n");
#else
			fprintf(OutFile, " You can emit hard radiation at will.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_VAMPIRISM)
		{
#ifdef JP
fprintf(OutFile, " あなたは吸血鬼のように敵から生命力を吸収することができる。\n");
#else
			fprintf(OutFile, " You can drain life from a foe like a vampire.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_SMELL_MET)
		{
#ifdef JP
fprintf(OutFile, " あなたは近くにある貴金属をかぎ分けることができる。\n");
#else
			fprintf(OutFile, " You can smell nearby precious metal.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_SMELL_MON)
		{
#ifdef JP
fprintf(OutFile, " あなたは近くのモンスターの存在をかぎ分けることができる。\n");
#else
			fprintf(OutFile, " You can smell nearby monsters.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_BLINK)
		{
#ifdef JP
fprintf(OutFile, " あなたは短い距離をテレポートできる。\n");
#else
			fprintf(OutFile, " You can teleport yourself short distances.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_EAT_ROCK)
		{
#ifdef JP
fprintf(OutFile, " あなたは硬い岩を食べることができる。\n");
#else
			fprintf(OutFile, " You can consume solid rock.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_SWAP_POS)
		{
#ifdef JP
fprintf(OutFile, " あなたは他の者と場所を入れ替わることができる。\n");
#else
			fprintf(OutFile, " You can switch locations with another being.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_SHRIEK)
		{
#ifdef JP
fprintf(OutFile, " あなたは身の毛もよだつ叫び声を発することができる。\n");
#else
			fprintf(OutFile, " You can emit a horrible shriek.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_ILLUMINE)
		{
#ifdef JP
fprintf(OutFile, " あなたは明るい光を放つことができる。\n");
#else
			fprintf(OutFile, " You can emit bright light.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_DET_CURSE)
		{
#ifdef JP
fprintf(OutFile, " あなたは邪悪な魔法の危険を感じとることができる。\n");
#else
			fprintf(OutFile, " You can feel the danger of evil magic.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_BERSERK)
		{
#ifdef JP
fprintf(OutFile, " あなたは自分の意思で狂乱戦闘状態になることができる。\n");
#else
			fprintf(OutFile, " You can drive yourself into a berserk frenzy.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_POLYMORPH)
		{
#ifdef JP
fprintf(OutFile, " あなたは自分の意志で変化できる。\n");
#else
			fprintf(OutFile, " You can polymorph yourself at will.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_MIDAS_TCH)
		{
#ifdef JP
fprintf(OutFile, " あなたは通常アイテムを金に変えることができる。\n");
#else
			fprintf(OutFile, " You can turn ordinary items to gold.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_GROW_MOLD)
		{
#ifdef JP
fprintf(OutFile, " あなたは周囲にキノコを生やすことができる。\n");
#else
			fprintf(OutFile, " You can cause mold to grow near you.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_RESIST)
		{
#ifdef JP
fprintf(OutFile, " あなたは元素の攻撃に対して身を硬くすることができる。\n");
#else
			fprintf(OutFile, " You can harden yourself to the ravages of the elements.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_EARTHQUAKE)
		{
#ifdef JP
fprintf(OutFile, " あなたは周囲のダンジョンを崩壊させることができる。\n");
#else
			fprintf(OutFile, " You can bring down the dungeon around your ears.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_EAT_MAGIC)
		{
#ifdef JP
fprintf(OutFile, " あなたは魔法のエネルギーを自分の物として使用できる。\n");
#else
			fprintf(OutFile, " You can consume magic energy for your own use.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_WEIGH_MAG)
		{
#ifdef JP
fprintf(OutFile, " あなたは自分に影響を与える魔法の力を感じることができる。\n");
#else
			fprintf(OutFile, " You can feel the strength of the magics affecting you.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_STERILITY)
		{
#ifdef JP
fprintf(OutFile, " あなたは集団的生殖不能を起こすことができる。\n");
#else
			fprintf(OutFile, " You can cause mass impotence.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_PANIC_HIT)
		{
#ifdef JP
fprintf(OutFile, " あなたは攻撃した後身を守るため逃げることができる。\n");
#else
			fprintf(OutFile, " You can run for your life after hitting something.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_DAZZLE)
		{
#ifdef JP
fprintf(OutFile, " あなたは混乱と盲目を引き起こす放射能を発生することができる。 \n");
#else
			fprintf(OutFile, " You can emit confusing, blinding radiation.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_LASER_EYE)
		{
#ifdef JP
fprintf(OutFile, " あなたは目からレーザー光線を発射することができる。\n");
#else
			fprintf(OutFile, " Your eyes can fire laser beams.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_RECALL)
		{
#ifdef JP
fprintf(OutFile, " あなたは街とダンジョンの間を行き来することができる。\n");
#else
			fprintf(OutFile, " You can travel between town and the depths.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_BANISH)
		{
#ifdef JP
fprintf(OutFile, " あなたは邪悪なモンスターを地獄に落とすことができる。\n");
#else
			fprintf(OutFile, " You can send evil creatures directly to Hell.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_COLD_TOUCH)
		{
#ifdef JP
fprintf(OutFile, " あなたは物を触って凍らせることができる。\n");
#else
			fprintf(OutFile, " You can freeze things with a touch.\n");
#endif

		}
		if (p_ptr->muta1 & MUT1_LAUNCHER)
		{
#ifdef JP
fprintf(OutFile, " あなたはアイテムを力強く投げることができる。\n");
#else
			fprintf(OutFile, " You can hurl objects with great force.\n");
#endif

		}
	}

	if (p_ptr->muta2)
	{
		if (p_ptr->muta2 & MUT2_BERS_RAGE)
		{
#ifdef JP
fprintf(OutFile, " あなたは狂戦士化の発作を起こす。\n");
#else
			fprintf(OutFile, " You are subject to berserker fits.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_COWARDICE)
		{
#ifdef JP
fprintf(OutFile, " あなたは時々臆病になる。\n");
#else
			fprintf(OutFile, " You are subject to cowardice.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_RTELEPORT)
		{
#ifdef JP
fprintf(OutFile, " あなたはランダムにテレポートする。\n");
#else
			fprintf(OutFile, " You are teleporting randomly.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_ALCOHOL)
		{
#ifdef JP
fprintf(OutFile, " あなたの体はアルコールを分泌する。\n");
#else
			fprintf(OutFile, " Your body produces alcohol.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_HALLU)
		{
#ifdef JP
fprintf(OutFile, " あなたは幻覚を引き起こす精神錯乱に侵されている。\n");
#else
			fprintf(OutFile, " You have a hallucinatory insanity.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_FLATULENT)
		{
#ifdef JP
fprintf(OutFile, " あなたは制御できない強烈な屁をこく。\n");
#else
			fprintf(OutFile, " You are subject to uncontrollable flatulence.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_PROD_MANA)
		{
#ifdef JP
fprintf(OutFile, " あなたは制御不能な魔法のエネルギーを発している。\n");
#else
			fprintf(OutFile, " You are producing magical energy uncontrollably.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_ATT_DEMON)
		{
#ifdef JP
fprintf(OutFile, " あなたはデーモンを引きつける。\n");
#else
			fprintf(OutFile, " You attract demons.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_SCOR_TAIL)
		{
#ifdef JP
fprintf(OutFile, " あなたはサソリの尻尾が生えている。(毒、ダメージ 3d7)\n");
#else
			fprintf(OutFile, " You have a scorpion tail (poison, 3d7).\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_HORNS)
		{
#ifdef JP
fprintf(OutFile, " あなたは角が生えている。(ダメージ 2d6)\n");
#else
			fprintf(OutFile, " You have horns (dam. 2d6).\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_BEAK)
		{
#ifdef JP
fprintf(OutFile, " あなたはクチバシが生えている。(ダメージ 2d4)\n");
#else
			fprintf(OutFile, " You have a beak (dam. 2d4).\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_SPEED_FLUX)
		{
#ifdef JP
fprintf(OutFile, " あなたはランダムに早く動いたり遅く動いたりする。\n");
#else
			fprintf(OutFile, " You move faster or slower randomly.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_BANISH_ALL)
		{
#ifdef JP
fprintf(OutFile, " あなたは時々近くのモンスターを消滅させる。\n");
#else
			fprintf(OutFile, " You sometimes cause nearby creatures to vanish.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_EAT_LIGHT)
		{
#ifdef JP
fprintf(OutFile, " あなたは時々周囲の光を吸収して栄養にする。\n");
#else
			fprintf(OutFile, " You sometimes feed off of the light around you.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_TRUNK)
		{
#ifdef JP
fprintf(OutFile, " あなたは象のような鼻を持っている。(ダメージ 1d4)\n");
#else
			fprintf(OutFile, " You have an elephantine trunk (dam 1d4).\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_ATT_ANIMAL)
		{
#ifdef JP
fprintf(OutFile, " あなたは動物を引きつける。\n");
#else
			fprintf(OutFile, " You attract animals.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_TENTACLES)
		{
#ifdef JP
fprintf(OutFile, " あなたは邪悪な触手を持っている。(ダメージ 2d5)\n");
#else
			fprintf(OutFile, " You have evil looking tentacles (dam 2d5).\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_RAW_CHAOS)
		{
#ifdef JP
fprintf(OutFile, " あなたはしばしば純カオスに包まれる。\n");
#else
			fprintf(OutFile, " You occasionally are surrounded with raw chaos.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_NORMALITY)
		{
#ifdef JP
fprintf(OutFile, " あなたは変異していたが、回復してきている。\n");
#else
			fprintf(OutFile, " You may be mutated, but you're recovering.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_WRAITH)
		{
#ifdef JP
fprintf(OutFile, " あなたの肉体は幽体化したり実体化したりする。\n");
#else
			fprintf(OutFile, " You fade in and out of physical reality.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_POLY_WOUND)
		{
#ifdef JP
fprintf(OutFile, " あなたの健康はカオスの力に影響を受ける。\n");
#else
			fprintf(OutFile, " Your health is subject to chaotic forces.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_WASTING)
		{
#ifdef JP
fprintf(OutFile, " あなたは衰弱する恐ろしい病気にかかっている。\n");
#else
			fprintf(OutFile, " You have a horrible wasting disease.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_ATT_DRAGON)
		{
#ifdef JP
fprintf(OutFile, " あなたはドラゴンを引きつける。\n");
#else
			fprintf(OutFile, " You attract dragons.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_WEIRD_MIND)
		{
#ifdef JP
fprintf(OutFile, " あなたの精神はランダムに拡大したり縮小したりしている。\n");
#else
			fprintf(OutFile, " Your mind randomly expands and contracts.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_NAUSEA)
		{
#ifdef JP
fprintf(OutFile, " あなたの胃は非常に落ち着きがない。\n");
#else
			fprintf(OutFile, " You have a seriously upset stomach.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_CHAOS_GIFT)
		{
#ifdef JP
fprintf(OutFile, " あなたはカオスの守護悪魔から褒美をうけとる。\n");
#else
			fprintf(OutFile, " Chaos deities give you gifts.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_WALK_SHAD)
		{
#ifdef JP
fprintf(OutFile, " あなたはしばしば他の「影」に迷い込む。\n");
#else
			fprintf(OutFile, " You occasionally stumble into other shadows.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_WARNING)
		{
#ifdef JP
fprintf(OutFile, " あなたは敵に関する警告を感じる。\n");
#else
			fprintf(OutFile, " You receive warnings about your foes.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_INVULN)
		{
#ifdef JP
fprintf(OutFile, " あなたは時々負け知らずな気分になる。\n");
#else
			fprintf(OutFile, " You occasionally feel invincible.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_SP_TO_HP)
		{
#ifdef JP
fprintf(OutFile, " あなたは時々血が筋肉にどっと流れる。\n");
#else
			fprintf(OutFile, " Your blood sometimes rushes to your muscles.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_HP_TO_SP)
		{
#ifdef JP
fprintf(OutFile, " あなたは時々頭に血がどっと流れる。\n");
#else
			fprintf(OutFile, " Your blood sometimes rushes to your head.\n");
#endif

		}
		if (p_ptr->muta2 & MUT2_DISARM)
		{
#ifdef JP
fprintf(OutFile, " あなたはよくつまづいて物を落とす。\n");
#else
			fprintf(OutFile, " You occasionally stumble and drop things.\n");
#endif

		}
	}

	if (p_ptr->muta3)
	{
		if (p_ptr->muta3 & MUT3_HYPER_STR)
		{
#ifdef JP
fprintf(OutFile, " あなたは超人的に強い。(腕力+4)\n");
#else
			fprintf(OutFile, " You are superhumanly strong (+4 STR).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_PUNY)
		{
#ifdef JP
fprintf(OutFile, " あなたは虚弱だ。(腕力-4)\n");
#else
			fprintf(OutFile, " You are puny (-4 STR).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_HYPER_INT)
		{
#ifdef JP
fprintf(OutFile, " あなたの脳は生体コンピュータだ。(知能＆賢さ+4)\n");
#else
			fprintf(OutFile, " Your brain is a living computer (+4 INT/WIS).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_MORONIC)
		{
#ifdef JP
fprintf(OutFile, " あなたは精神薄弱だ。(知能＆賢さ-4)\n");
#else
			fprintf(OutFile, " You are moronic (-4 INT/WIS).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_RESILIENT)
		{
#ifdef JP
fprintf(OutFile, " あなたの体は弾力性に富んでいる。(耐久+4)\n");
#else
			fprintf(OutFile, " You are very resilient (+4 CON).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_XTRA_FAT)
		{
#ifdef JP
fprintf(OutFile, " あなたは極端に太っている。(耐久+2,スピード-2)\n");
#else
			fprintf(OutFile, " You are extremely fat (+2 CON, -2 speed).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_ALBINO)
		{
#ifdef JP
fprintf(OutFile, " あなたはアルビノだ。(耐久-4)\n");
#else
			fprintf(OutFile, " You are albino (-4 CON).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_FLESH_ROT)
		{
#ifdef JP
fprintf(OutFile, " あなたの肉体は腐敗している。(耐久-2,魅力-1)\n");
#else
			fprintf(OutFile, " Your flesh is rotting (-2 CON, -1 CHR).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_SILLY_VOI)
		{
#ifdef JP
fprintf(OutFile, " あなたの声は間抜けなキーキー声だ。(魅力-4)\n");
#else
			fprintf(OutFile, " Your voice is a silly squeak (-4 CHR).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_BLANK_FAC)
		{
#ifdef JP
fprintf(OutFile, " あなたはのっぺらぼうだ。(魅力-1)\n");
#else
			fprintf(OutFile, " Your face is featureless (-1 CHR).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_ILL_NORM)
		{
#ifdef JP
fprintf(OutFile, " あなたは幻影に覆われている。\n");
#else
			fprintf(OutFile, " Your appearance is masked with illusion.\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_XTRA_EYES)
		{
#ifdef JP
fprintf(OutFile, " あなたは余分に二つの目を持っている。(探索+15)\n");
#else
			fprintf(OutFile, " You have an extra pair of eyes (+15 search).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_MAGIC_RES)
		{
#ifdef JP
fprintf(OutFile, " あなたは魔法への耐性をもっている。\n");
#else
			fprintf(OutFile, " You are resistant to magic.\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_XTRA_NOIS)
		{
#ifdef JP
fprintf(OutFile, " あなたは変な音を発している。(隠密-3)\n");
#else
			fprintf(OutFile, " You make a lot of strange noise (-3 stealth).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_INFRAVIS)
		{
#ifdef JP
fprintf(OutFile, " あなたは素晴らしい赤外線視力を持っている。(+3)\n");
#else
			fprintf(OutFile, " You have remarkable infravision (+3).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_XTRA_LEGS)
		{
#ifdef JP
fprintf(OutFile, " あなたは余分に二本の足が生えている。(加速+3)\n");
#else
			fprintf(OutFile, " You have an extra pair of legs (+3 speed).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_SHORT_LEG)
		{
#ifdef JP
fprintf(OutFile, " あなたの足は短い突起だ。(加速-3)\n");
#else
			fprintf(OutFile, " Your legs are short stubs (-3 speed).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_ELEC_TOUC)
		{
#ifdef JP
fprintf(OutFile, " あなたの血管には電流が流れている。\n");
#else
			fprintf(OutFile, " Electricity is running through your veins.\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_FIRE_BODY)
		{
#ifdef JP
fprintf(OutFile, " あなたの体は炎につつまれている。\n");
#else
			fprintf(OutFile, " Your body is enveloped in flames.\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_WART_SKIN)
		{
#ifdef JP
fprintf(OutFile, " あなたの肌はイボに被われている。(魅力-2, AC+5)\n");
#else
			fprintf(OutFile, " Your skin is covered with warts (-2 CHR, +5 AC).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_SCALES)
		{
#ifdef JP
fprintf(OutFile, " あなたの肌は鱗になっている。(魅力-1, AC+10)\n");
#else
			fprintf(OutFile, " Your skin has turned into scales (-1 CHR, +10 AC).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_IRON_SKIN)
		{
#ifdef JP
fprintf(OutFile, " あなたの肌は鉄でできている。(器用-1, AC+25)\n");
#else
			fprintf(OutFile, " Your skin is made of steel (-1 DEX, +25 AC).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_WINGS)
		{
#ifdef JP
fprintf(OutFile, " あなたは羽を持っている。\n");
#else
			fprintf(OutFile, " You have wings.\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_FEARLESS)
		{
#ifdef JP
fprintf(OutFile, " あなたは全く恐怖を感じない。\n");
#else
			fprintf(OutFile, " You are completely fearless.\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_REGEN)
		{
#ifdef JP
fprintf(OutFile, " あなたは急速に回復する。\n");
#else
			fprintf(OutFile, " You are regenerating.\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_ESP)
		{
#ifdef JP
fprintf(OutFile, " あなたはテレパシーを持っている。\n");
#else
			fprintf(OutFile, " You are telepathic.\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_LIMBER)
		{
#ifdef JP
fprintf(OutFile, " あなたの体は非常にしなやかだ。(器用+3)\n");
#else
			fprintf(OutFile, " Your body is very limber (+3 DEX).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_ARTHRITIS)
		{
#ifdef JP
fprintf(OutFile, " あなたはいつも関節に痛みを感じている。(器用-3)\n");
#else
			fprintf(OutFile, " Your joints ache constantly (-3 DEX).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_VULN_ELEM)
		{
#ifdef JP
fprintf(OutFile, " あなたは元素の攻撃に弱い。\n");
#else
			fprintf(OutFile, " You are susceptible to damage from the elements.\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_MOTION)
		{
#ifdef JP
fprintf(OutFile, " あなたの動作は正確で力強い。(隠密+1)\n");
#else
			fprintf(OutFile, " Your movements are precise and forceful (+1 STL).\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_GOOD_LUCK)
		{
#ifdef JP
fprintf(OutFile, " あなたは白いオーラにつつまれている。\n");
#else
			fprintf(OutFile, " There is a white aura surrounding you.\n");
#endif

		}
		if (p_ptr->muta3 & MUT3_BAD_LUCK)
		{
#ifdef JP
fprintf(OutFile, " あなたは黒いオーラにつつまれている。\n");
#else
			fprintf(OutFile, " There is a black aura surrounding you.\n");
#endif

		}
	}
}


/*
 * List mutations we have...
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
#ifdef JP
show_file(TRUE, file_name, "突然変異", 0, 0);
#else
	show_file(TRUE, file_name, "Mutations", 0, 0);
#endif


	/* Remove the file */
	fd_kill(file_name);
}


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


static int count_mutations(void)
{
	return (count_bits(p_ptr->muta1) +
		count_bits(p_ptr->muta2) +
		count_bits(p_ptr->muta3));
}


/*
 * Return the modifier to the regeneration rate
 * (in percent)
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


void mutation_stop_mouth()
{
	if (music_singing_any()) stop_singing();
	if (hex_spelling_any()) stop_hex_spell_all();
}


bool mutation_power_aux(u32b power)
{
	int     dir = 0;
	int     lvl = p_ptr->lev;


	switch (power)
	{
		case MUT1_SPIT_ACID:
			if (!get_aim_dir(&dir)) return FALSE;
			mutation_stop_mouth();
#ifdef JP
			msg_print("酸を吐きかけた...");
#else
			msg_print("You spit acid...");
#endif

			fire_ball(GF_ACID, dir, lvl, 1 + (lvl / 30));
			break;

		case MUT1_BR_FIRE:
			if (!get_aim_dir(&dir)) return FALSE;
			mutation_stop_mouth();
#ifdef JP
			msg_print("あなたは火炎のブレスを吐いた...");
#else
			msg_print("You breathe fire...");
#endif

			fire_ball(GF_FIRE, dir, lvl * 2, 1 + (lvl / 20));
			break;

		case MUT1_HYPN_GAZE:
			if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
			msg_print("あなたの目は幻惑的になった...");
#else
			msg_print("Your eyes look mesmerizing...");
#endif

			(void)charm_monster(dir, lvl);
			break;

		case MUT1_TELEKINES:
			if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
			msg_print("集中している...");
#else
			msg_print("You concentrate...");
#endif

			fetch(dir, lvl * 10, TRUE);
			break;

		case MUT1_VTELEPORT:
#ifdef JP
			msg_print("集中している...");
#else
			msg_print("You concentrate...");
#endif

			teleport_player(10 + 4 * lvl, 0L);
			break;

		case MUT1_MIND_BLST:
			if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
			msg_print("集中している...");
#else
			msg_print("You concentrate...");
#endif

			fire_bolt(GF_PSI, dir, damroll(3 + ((lvl - 1) / 5), 3));
			break;

		case MUT1_RADIATION:
#ifdef JP
			msg_print("体から放射能が発生した！");
#else
			msg_print("Radiation flows from your body!");
#endif

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
#ifdef JP
					msg_print("何もない場所に噛みついた！");
#else
					msg_print("You bite into thin air!");
#endif

					break;
				}

#ifdef JP
				msg_print("あなたはニヤリとして牙をむいた...");
#else
				msg_print("You grin and bare your fangs...");
#endif


				dummy = lvl * 2;

				if (drain_life(dir, dummy))
				{
					if (p_ptr->food < PY_FOOD_FULL)
						/* No heal if we are "full" */
						(void)hp_player(dummy);
					else
#ifdef JP
						msg_print("あなたは空腹ではありません。");
#else
						msg_print("You were not hungry.");
#endif

					/* Gain nutritional sustenance: 150/hp drained */
					/* A Food ration gives 5000 food points (by contrast) */
					/* Don't ever get more than "Full" this way */
					/* But if we ARE Gorged,  it won't cure us */
					dummy = p_ptr->food + MIN(5000, 100 * dummy);
					if (p_ptr->food < PY_FOOD_MAX)   /* Not gorged already */
						(void)set_food(dummy >= PY_FOOD_MAX ? PY_FOOD_MAX-1 : dummy);
				}
				else
#ifdef JP
					msg_print("げぇ！ひどい味だ。");
#else
					msg_print("Yechh. That tastes foul.");
#endif

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
#ifdef JP
					msg_print("この地形は食べられない。");
#else
					msg_print("You cannot eat this feature.");
#endif
					break;
				}
				else if (have_flag(f_ptr->flags, FF_PERMANENT))
				{
#ifdef JP
					msg_format("いてっ！この%sはあなたの歯より硬い！", f_name + mimic_f_ptr->name);
#else
					msg_format("Ouch!  This %s is harder than your teeth!", f_name + mimic_f_ptr->name);
#endif
					break;
				}
				else if (c_ptr->m_idx)
				{
					monster_type *m_ptr = &m_list[c_ptr->m_idx];
#ifdef JP
					msg_print("何かが邪魔しています！");
#else
					msg_print("There's something in the way!");
#endif

					if (!m_ptr->ml || !is_pet(m_ptr)) py_attack(y, x, 0);
					break;
				}
				else if (have_flag(f_ptr->flags, FF_TREE))
				{
#ifdef JP
					msg_print("木の味は好きじゃない！");
#else
					msg_print("You don't like the woody taste!");
#endif
					break;
				}
				else if (have_flag(f_ptr->flags, FF_GLASS))
				{
#ifdef JP
					msg_print("ガラスの味は好きじゃない！");
#else
					msg_print("You don't like the glassy taste!");
#endif
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
#ifdef JP
					msg_format("この%sはとてもおいしい！", f_name + mimic_f_ptr->name);
#else
					msg_format("This %s is very filling!", f_name + mimic_f_ptr->name);
#endif
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
#ifdef JP
			if (!get_check("変身します。よろしいですか？")) return FALSE;
#else
			if (!get_check("You will polymorph your self. Are you sure? ")) return FALSE;
#endif
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
#ifdef JP
						msg_print("うまく逃げられなかった。");
#else
						msg_print("You failed to teleport.");
#endif
					else teleport_player(30, 0L);
				}
				else
				{
#ifdef JP
					msg_print("その方向にはモンスターはいません。");
#else
					msg_print("You don't see any monster in this direction");
#endif

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
#ifdef JP
					msg_print("邪悪な存在を感じとれません！");
#else
					msg_print("You sense no evil there!");
#endif

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
#ifdef JP
					msg_print("その邪悪なモンスターは硫黄臭い煙とともに消え去った！");
#else
					msg_print("The evil creature vanishes in a puff of sulfurous smoke!");
#endif

				}
				else
				{
#ifdef JP
					msg_print("祈りは効果がなかった！");
#else
					msg_print("Your invocation is ineffectual!");
#endif

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
#ifdef JP
					msg_print("あなたは何もない場所で手を振った。");
#else
					msg_print("You wave your hands in the air.");
#endif

					break;
				}
				fire_bolt(GF_COLD, dir, 2 * lvl);
			}
			break;

		/* XXX_XXX_XXX Hack!  MUT1_LAUNCHER is negative, see above */
		case 3: /* MUT1_LAUNCHER */
			/* Gives a multiplier of 2 at first, up to 3 at 40th */
			if (!do_cmd_throw_aux(2 + lvl / 40, FALSE, 0)) return FALSE;
			break;

		default:
			energy_use = 0;
#ifdef JP
			msg_format("能力 %s は実装されていません。", power);
#else
			msg_format("Power %s not implemented. Oops.", power);
#endif

	}

	return TRUE;
}
