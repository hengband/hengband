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

#include "mutation/mutation.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-item/cmd-throw.h"
#include "core/asking-player.h"
#include "player/player-race-types.h"
#include "core/show-file.h"
#include "core/stuff-handler.h"
#include "effect/spells-effect-util.h"
#include "game-option/play-record-options.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "io/targeting.h"
#include "io/write-diary.h"
#include "mind/mind-mage.h"
#include "mind/mind-warrior.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/smart-learn-types.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-techniques.h"
#include "object-enchant/item-feeling.h"
#include "object-hook/hook-checker.h"
#include "player/avatar.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-personalities-types.h"
#include "core/player-update-types.h"
#include "player/selfinfo.h"
#include "racial/racial-vampire.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-fetcher.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-sorcery.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "spell/spell-types.h"
#include "status/element-resistance.h"
#include "status/shape-changer.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤーに突然変異を与える
 * @param choose_mut 与えたい突然変異のID、0ならばランダムに選択
 * @return なし
 */
bool gain_mutation(player_type *creature_ptr, MUTATION_IDX choose_mut)
{
	int attempts_left = 20;
	concptr muta_desc = "";
	bool muta_chosen = FALSE;
	int muta_which = 0; // mutation_flag_type_1 とmutation_flag_type_2 の両対応とするため、敢えてint型で定義する
	BIT_FLAGS *muta_class = NULL;

	if (choose_mut) attempts_left = 1;

	while (attempts_left--)
	{
		switch (choose_mut ? choose_mut : (creature_ptr->pclass == CLASS_BERSERKER ? 74+randint1(119) : randint1(193)))
		{
		case 1: case 2: case 3: case 4:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_SPIT_ACID;
			muta_desc = _("酸を吐く能力を得た。", "You gain the ability to spit acid.");
			break;
			
		case 5: case 6: case 7:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_BR_FIRE;
			muta_desc = _("火を吐く能力を得た。", "You gain the ability to breathe fire.");
			break;
			
		case 8: case 9:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_HYPN_GAZE;
			muta_desc = _("催眠眼の能力を得た。", "Your eyes look mesmerizing...");
			break;
			
		case 10: case 11:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_TELEKINES;
			muta_desc = _("物体を念動力で動かす能力を得た。", "You gain the ability to move objects telekinetically.");
			break;
			
		case 12: case 13: case 14:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_VTELEPORT;
			muta_desc = _("自分の意思でテレポートする能力を得た。", "You gain the power of teleportation at will.");
			break;
			
		case 15: case 16:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_MIND_BLST;
			muta_desc = _("精神攻撃の能力を得た。", "You gain the power of Mind Blast.");
			break;
			
		case 17: case 18:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_RADIATION;
			muta_desc = _("あなたは強い放射線を発生し始めた。", "You start emitting hard radiation.");
			break;
			
		case 19: case 20:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_VAMPIRISM;
			muta_desc = _("生命力を吸収できるようになった。", "You become vampiric.");
			break;
			
		case 21: case 22: case 23:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_SMELL_MET;
			muta_desc = _("金属の匂いを嗅ぎ分けられるようになった。", "You smell a metallic odor.");
			break;
			
		case 24: case 25: case 26: case 27:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_SMELL_MON;
			muta_desc = _("モンスターの臭いを嗅ぎ分けられるようになった。", "You smell filthy monsters.");
			break;
			
		case 28: case 29: case 30:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_BLINK;
			muta_desc = _("近距離テレポートの能力を得た。", "You gain the power of minor teleportation.");
			break;
			
		case 31: case 32:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_EAT_ROCK;
			muta_desc = _("壁が美味しそうに見える。", "The walls look delicious.");
			break;
			
		case 33: case 34:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_SWAP_POS;
			muta_desc = _("他人の靴で一マイル歩くような気分がする。", "You feel like walking a mile in someone else's shoes.");
			break;
			
		case 35: case 36: case 37:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_SHRIEK;
			muta_desc = _("あなたの声は相当強くなった。", "Your vocal cords get much tougher.");
			break;
			
		case 38: case 39: case 40:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_ILLUMINE;
			muta_desc = _("あなたは光り輝いて部屋を明るくするようになった。", "You can light up rooms with your presence.");
			break;
			
		case 41: case 42:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_DET_CURSE;
			muta_desc = _("邪悪な魔法を感知できるようになった。", "You can feel evil magics.");
			break;
			
		case 43: case 44: case 45:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_BERSERK;
			muta_desc = _("制御できる激情を感じる。", "You feel a controlled rage.");
			break;
			
		case 46:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_POLYMORPH;
			muta_desc = _("体が変異しやすくなった。", "Your body seems mutable.");
			break;
			
		case 47: case 48:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_MIDAS_TCH;
			muta_desc = _("「ミダス王の手」の能力を得た。", "You gain the Midas touch.");/*トゥームレイダースにありましたね。 */
			break;
			
		case 49:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_GROW_MOLD;
			muta_desc = _("突然カビに親しみを覚えた。", "You feel a sudden affinity for mold.");
			break;
			
		case 50: case 51: case 52:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_RESIST;
			muta_desc = _("あなたは自分自身を守れる気がする。", "You feel like you can protect yourself.");
			break;
			
		case 53: case 54: case 55:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_EARTHQUAKE;
			muta_desc = _("ダンジョンを破壊する能力を得た。", "You gain the ability to wreck the dungeon.");
			break;
			
		case 56:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_EAT_MAGIC;
			muta_desc = _("魔法のアイテムが美味そうに見える。", "Your magic items look delicious.");
			break;
			
		case 57: case 58:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_WEIGH_MAG;
			muta_desc = _("あなたは周囲にある魔法をより良く理解できる気がする。", "You feel you can better understand the magic around you.");
			break;
			
		case 59:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_STERILITY;
			muta_desc = _("周りの全ての者に頭痛を起こすことができる。", "You can give everything around you a headache.");
			break;
		case 60: case 61:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_HIT_AND_AWAY;
			muta_desc = _("突然、泥棒の気分が分かるようになった。", "You suddenly understand how thieves feel.");
			break;
			
		case 62: case 63: case 64:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_DAZZLE;
			muta_desc = _("眩い閃光を発する能力を得た。", "You gain the ability to emit dazzling lights.");
			break;
			
		case 65: case 66: case 67:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_LASER_EYE;
			muta_desc = _("あなたの目は一瞬焼け付いた。", "Your eyes burn for a moment.");
			break;
			
		case 68: case 69:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_RECALL;
			muta_desc = _("少しだけホームシックになったが、すぐ直った。", "You feel briefly homesick, but it passes.");
			break;
			
		case 70:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_BANISH;
			muta_desc = _("神聖な怒りの力に満たされた。", "You feel a holy wrath fill you.");
			break;
			
		case 71: case 72:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_COLD_TOUCH;
			muta_desc = _("あなたの両手はとても冷たくなった。", "Your hands get very cold.");
			break;
			
		case 73: case 74:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_LAUNCHER;
			muta_desc = _("あなたの物を投げる手はかなり強くなった気がする。", "Your throwing arm feels much stronger.");
			break;
			
		case 75:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_BERS_RAGE;
			muta_desc = _("あなたは狂暴化の発作を起こすようになった！", "You become subject to fits of berserk rage!");
			break;
			
		case 76:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_COWARDICE;
			muta_desc = _("信じられないくらい臆病になった！", "You become an incredible coward!");
			break;
			
		case 77:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_RTELEPORT;
			muta_desc = _("あなたの位置は非常に不確定になった。", "Your position seems very uncertain...");
			break;
			
		case 78:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_ALCOHOL;
			muta_desc = _("あなたはアルコールを分泌するようになった。", "Your body starts producing alcohol!");
			break;
			
		case 79:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_HALLU;
			muta_desc = _("あなたは幻覚を引き起こす精神錯乱に侵された。", "You are afflicted by a hallucinatory insanity!");
			break;
			
		case 80:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_FLATULENT;
			muta_desc = _( "あなたは制御不能な強烈な屁をこくようになった。",  "You become subject to uncontrollable flatulence.");

			break;
		case 81: case 82:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_SCOR_TAIL;
			muta_desc = _( "サソリの尻尾が生えてきた！",  "You grow a scorpion tail!");

			break;
		case 83: case 84:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_HORNS;
			muta_desc = _( "額に角が生えた！",  "Horns pop forth into your forehead!");

			break;
		case 85: case 86:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_BEAK;
			muta_desc = _( "口が鋭く強いクチバシに変化した！",  "Your mouth turns into a sharp, powerful beak!");

			break;
		case 87: case 88:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_ATT_DEMON;
			muta_desc = _( "悪魔を引き付けるようになった。",  "You start attracting demons.");

			break;
		case 89:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_PROD_MANA;
			muta_desc = _( "あなたは制御不能な魔法のエネルギーを発生するようになった。",  "You start producing magical energy uncontrollably.");

			break;
		case 90: case 91:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_SPEED_FLUX;
			muta_desc = _( "あなたは躁鬱質になった。",  "You become manic-depressive.");

			break;
		case 92: case 93:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_BANISH_ALL;
			muta_desc = _( "恐ろしい力があなたの背後に潜んでいる気がする。",  "You feel a terrifying power lurking behind you.");

			break;
		case 94:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_EAT_LIGHT;
			muta_desc = _( "あなたはウンゴリアントに奇妙な親しみを覚えるようになった。",  "You feel a strange kinship with Ungoliant.");

			break;
		case 95: case 96:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_TRUNK;
			muta_desc = _( "あなたの鼻は伸びて象の鼻のようになった。",  "Your nose grows into an elephant-like trunk.");

			break;
		case 97:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_ATT_ANIMAL;
			muta_desc = _( "動物を引き付けるようになった。",  "You start attracting animals.");

			break;
		case 98:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_TENTACLES;
			muta_desc = _( "邪悪な触手が体の両側に生えてきた。",  "Evil-looking tentacles sprout from your sides.");

			break;
		case 99:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_RAW_CHAOS;
			muta_desc = _( "周囲の空間が不安定になった気がする。",  "You feel the universe is less stable around you.");

			break;
		case 100: case 101: case 102:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_NORMALITY;
			muta_desc = _( "あなたは奇妙なほど普通になった気がする。",  "You feel strangely normal.");

			break;
		case 103:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_WRAITH;
			muta_desc = _( "あなたは幽体化したり実体化したりするようになった。",  "You start to fade in and out of the physical world.");

			break;
		case 104:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_POLY_WOUND;
			muta_desc = _( "あなたはカオスの力が古い傷に入り込んでくるのを感じた。",  "You feel forces of chaos entering your old scars.");

			break;
		case 105:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_WASTING;
			muta_desc = _( "あなたは突然おぞましい衰弱病にかかった。",  "You suddenly contract a horrible wasting disease.");

			break;
		case 106:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_ATT_DRAGON;
			muta_desc = _( "あなたはドラゴンを引きつけるようになった。",  "You start attracting dragons.");

			break;
		case 107: case 108:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_WEIRD_MIND;
			muta_desc = _( "あなたの思考は突然おかしな方向に向き始めた。",  "Your thoughts suddenly take off in strange directions.");

			break;
		case 109:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_NAUSEA;
			muta_desc = _( "胃袋がピクピクしはじめた。",  "Your stomach starts to roil nauseously.");

			break;
		case 110: case 111:
			/* Chaos warriors already have a chaos deity */
			if (creature_ptr->pclass != CLASS_CHAOS_WARRIOR)
			{
				muta_class = &(creature_ptr->muta2);
				muta_which = MUT2_CHAOS_GIFT;
			muta_desc = _( "あなたはカオスの守護悪魔の注意を惹くようになった。",  "You attract the notice of a chaos deity!");

			}
			break;
		case 112:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_WALK_SHAD;
			muta_desc = _( "あなたは現実が紙のように薄いと感じるようになった。",  "You feel like reality is as thin as paper.");

			break;
		case 113: case 114:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_WARNING;
			muta_desc = _( "あなたは突然パラノイアになった気がする。",  "You suddenly feel paranoid.");

			break;
		case 115:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_INVULN;
			muta_desc = _( "あなたは祝福され、無敵状態になる発作を起こすようになった。",  "You are blessed with fits of invulnerability.");

			break;
		case 116: case 117:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_SP_TO_HP;
			muta_desc = _( "魔法の治癒の発作を起こすようになった。",  "You are subject to fits of magical healing.");

			break;
		case 118:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_HP_TO_SP;
			muta_desc = _( "痛みを伴う精神明瞭化の発作を起こすようになった。",  "You are subject to fits of painful clarity.");

			break;
		case 119:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_DISARM;
			muta_desc = _( "あなたの脚は長さが四倍になった。",  "Your feet grow to four times their former size.");

			break;
		case 120: case 121: case 122:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_HYPER_STR;
			muta_desc = _( "超人的に強くなった！",  "You turn into a superhuman he-man!");

			break;
		case 123: case 124: case 125:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_PUNY;
			muta_desc = _( "筋肉が弱ってしまった...",  "Your muscles wither away...");

			break;
		case 126: case 127: case 128:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_HYPER_INT;
			muta_desc = _( "あなたの脳は生体コンピュータに進化した！",  "Your brain evolves into a living computer!");

			break;
		case 129: case 130: case 131:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_MORONIC;
			muta_desc = _( "脳が萎縮してしまった...",  "Your brain withers away...");

			break;
		case 132: case 133:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_RESILIENT;
			muta_desc = _( "並外れてタフになった。",  "You become extraordinarily resilient.");

			break;
		case 134: case 135:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_XTRA_FAT;
			muta_desc = _( "あなたは気持ち悪いくらい太った！",  "You become sickeningly fat!");

			break;
		case 136: case 137:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_ALBINO;
			muta_desc = _( "アルビノになった！弱くなった気がする...",  "You turn into an albino! You feel frail...");

			break;
		case 138: case 139: case 140:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_FLESH_ROT;
			muta_desc = _( "あなたの肉体は腐敗する病気に侵された！",  "Your flesh is afflicted by a rotting disease!");

			break;
		case 141: case 142:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_SILLY_VOI;
			muta_desc = _( "声が間抜けなキーキー声になった！",  "Your voice turns into a ridiculous squeak!");

			break;
		case 143: case 144:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_BLANK_FAC;
			muta_desc = _( "のっぺらぼうになった！",  "Your face becomes completely featureless!");

			break;
		case 145:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_ILL_NORM;
			muta_desc = _( "心の安らぐ幻影を映し出すようになった。",  "You start projecting a reassuring image.");

			break;
		case 146: case 147: case 148:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_XTRA_EYES;
			muta_desc = _( "新たに二つの目が出来た！",  "You grow an extra pair of eyes!");

			break;
		case 149: case 150:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_MAGIC_RES;
			muta_desc = _( "魔法への耐性がついた。",  "You become resistant to magic.");

			break;
		case 151: case 152: case 153:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_XTRA_NOIS;
			muta_desc = _( "あなたは奇妙な音を立て始めた！",  "You start making strange noise!");

			break;
		case 154: case 155: case 156:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_INFRAVIS;
			muta_desc = _( "赤外線視力が増した。",  "Your infravision is improved.");

			break;
		case 157: case 158:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_XTRA_LEGS;
			muta_desc = _( "新たに二本の足が生えてきた！",  "You grow an extra pair of legs!");

			break;
		case 159: case 160:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_SHORT_LEG;
			muta_desc = _( "足が短い突起になってしまった！",  "Your legs turn into short stubs!");

			break;
		case 161: case 162:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_ELEC_TOUC;
			muta_desc = _( "血管を電流が流れ始めた！",  "Electricity starts running through you!");

			break;
		case 163: case 164:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_FIRE_BODY;
			muta_desc = _( "あなたの体は炎につつまれている。",  "Your body is enveloped in flames!");

			break;
		case 165: case 166: case 167:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_WART_SKIN;
			muta_desc = _( "気持ち悪いイボイボが体中にできた！",  "Disgusting warts appear everywhere on you!");

			break;
		case 168: case 169: case 170:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_SCALES;
			muta_desc = _( "肌が黒い鱗に変わった！",  "Your skin turns into black scales!");

			break;
		case 171: case 172:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_IRON_SKIN;
			muta_desc = _( "あなたの肌は鉄になった！",  "Your skin turns to steel!");

			break;
		case 173: case 174:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_WINGS;
			muta_desc = _( "背中に羽が生えた。",  "You grow a pair of wings.");

			break;
		case 175: case 176: case 177:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_FEARLESS;
			muta_desc = _( "完全に怖れ知らずになった。",  "You become completely fearless.");

			break;
		case 178: case 179:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_REGEN;
			muta_desc = _( "急速に回復し始めた。",  "You start regenerating.");

			break;
		case 180: case 181:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_ESP;
			muta_desc = _( "テレパシーの能力を得た！",  "You develop a telepathic ability!");

			break;
		case 182: case 183: case 184:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_LIMBER;
			muta_desc = _( "筋肉がしなやかになった。",  "Your muscles become limber.");

			break;
		case 185: case 186: case 187:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_ARTHRITIS;
			muta_desc = _( "関節が突然痛み出した。",  "Your joints suddenly hurt.");

			break;
		case 188:
			if (creature_ptr->pseikaku == PERSONALITY_LUCKY) break;
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_BAD_LUCK;
			muta_desc = _( "悪意に満ちた黒いオーラがあなたをとりまいた...",  "There is a malignant black aura surrounding you...");

			break;
		case 189:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_VULN_ELEM;
			muta_desc = _( "妙に無防備になった気がする。",  "You feel strangely exposed.");

			break;
		case 190: case 191: case 192:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_MOTION;
			muta_desc = _( "体の動作がより正確になった。",  "You move with new assurance.");

			break;
		case 193:
			muta_class = &(creature_ptr->muta3);
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

	chg_virtue(creature_ptr, V_CHANCE, 1);

	/*
	  some races are apt to gain specified mutations
	  This should be allowed only if "choose_mut" is 0.
						--- henkma
	*/
	if (!choose_mut)
	{
		if (creature_ptr->prace == RACE_VAMPIRE &&
			!(creature_ptr->muta1 & MUT1_HYPN_GAZE) &&
			(randint1(10) < 7))
		{
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_HYPN_GAZE;
			muta_desc = _("眼が幻惑的になった...", "Your eyes look mesmerizing...");

		}

		else if (creature_ptr->prace == RACE_IMP &&
			!(creature_ptr->muta2 & MUT2_HORNS) &&
			(randint1(10) < 7))
		{
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_HORNS;
			muta_desc = _("角が額から生えてきた！", "Horns pop forth into your forehead!");

		}

		else if (creature_ptr->prace == RACE_YEEK &&
			!(creature_ptr->muta1 & MUT1_SHRIEK) &&
			(randint1(10) < 7))
		{
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_SHRIEK;
			muta_desc = _("声質がかなり強くなった。", "Your vocal cords get much tougher.");

		}

		else if (creature_ptr->prace == RACE_BEASTMAN &&
			!(creature_ptr->muta1 & MUT1_POLYMORPH) &&
			(randint1(10) < 2))
		{
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_POLYMORPH;
			muta_desc = _("あなたの肉体は変化できるようになった、", "Your body seems mutable.");

		}

		else if (creature_ptr->prace == RACE_MIND_FLAYER &&
			!(creature_ptr->muta2 & MUT2_TENTACLES) &&
			(randint1(10) < 7))
		{
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_TENTACLES;
			muta_desc = _("邪悪な触手が口の周りに生えた。", "Evil-looking tentacles sprout from your mouth.");

		}
	}

	msg_print(_("突然変異した！", "You mutate!"));

	msg_print(muta_desc);
	*muta_class |= muta_which;

	if (muta_class == &(creature_ptr->muta3))
	{
		if (muta_which == MUT3_PUNY)
		{
			if (creature_ptr->muta3 & MUT3_HYPER_STR)
			{
				msg_print(_("あなたはもう超人的に強くはない！", "You no longer feel super-strong!"));

				creature_ptr->muta3 &= ~(MUT3_HYPER_STR);
			}
		}
		else if (muta_which == MUT3_HYPER_STR)
		{
			if (creature_ptr->muta3 & MUT3_PUNY)
			{
				msg_print(_("あなたはもう虚弱ではない！", "You no longer feel puny!"));

				creature_ptr->muta3 &= ~(MUT3_PUNY);
			}
		}
		else if (muta_which == MUT3_MORONIC)
		{
			if (creature_ptr->muta3 & MUT3_HYPER_INT)
			{
				msg_print(_("あなたの脳はもう生体コンピュータではない。", "Your brain is no longer a living computer."));

				creature_ptr->muta3 &= ~(MUT3_HYPER_INT);
			}
		}
		else if (muta_which == MUT3_HYPER_INT)
		{
			if (creature_ptr->muta3 & MUT3_MORONIC)
			{
				msg_print(_("あなたはもう精神薄弱ではない。", "You are no longer moronic."));

				creature_ptr->muta3 &= ~(MUT3_MORONIC);
			}
		}
		else if (muta_which == MUT3_IRON_SKIN)
		{
			if (creature_ptr->muta3 & MUT3_SCALES)
			{
				msg_print(_("鱗がなくなった。", "You lose your scales."));

				creature_ptr->muta3 &= ~(MUT3_SCALES);
			}
			if (creature_ptr->muta3 & MUT3_FLESH_ROT)
			{
				msg_print(_("肉体が腐乱しなくなった。", "Your flesh rots no longer."));

				creature_ptr->muta3 &= ~(MUT3_FLESH_ROT);
			}
			if (creature_ptr->muta3 & MUT3_WART_SKIN)
			{
				msg_print(_("肌のイボイボがなくなった。", "You lose your warts."));

				creature_ptr->muta3 &= ~(MUT3_WART_SKIN);
			}
		}
		else if (muta_which == MUT3_WART_SKIN || muta_which == MUT3_SCALES
			|| muta_which == MUT3_FLESH_ROT)
		{
			if (creature_ptr->muta3 & MUT3_IRON_SKIN)
			{
				msg_print(_("あなたの肌はもう鉄ではない。", "Your skin is no longer made of steel."));

				creature_ptr->muta3 &= ~(MUT3_IRON_SKIN);
			}
		}
		else if (muta_which == MUT3_FEARLESS)
		{
			if (creature_ptr->muta2 & MUT2_COWARDICE)
			{
				msg_print(_("臆病でなくなった。", "You are no longer cowardly."));

				creature_ptr->muta2 &= ~(MUT2_COWARDICE);
			}
		}
		else if (muta_which == MUT3_FLESH_ROT)
		{
			if (creature_ptr->muta3 & MUT3_REGEN)
			{
				msg_print(_("急速に回復しなくなった。", "You stop regenerating."));

				creature_ptr->muta3 &= ~(MUT3_REGEN);
			}
		}
		else if (muta_which == MUT3_REGEN)
		{
			if (creature_ptr->muta3 & MUT3_FLESH_ROT)
			{
				msg_print(_("肉体が腐乱しなくなった。", "Your flesh stops rotting."));

				creature_ptr->muta3 &= ~(MUT3_FLESH_ROT);
			}
		}
		else if (muta_which == MUT3_LIMBER)
		{
			if (creature_ptr->muta3 & MUT3_ARTHRITIS)
			{
				msg_print(_("関節が痛くなくなった。", "Your joints stop hurting."));

				creature_ptr->muta3 &= ~(MUT3_ARTHRITIS);
			}
		}
		else if (muta_which == MUT3_ARTHRITIS)
		{
			if (creature_ptr->muta3 & MUT3_LIMBER)
			{
				msg_print(_("あなたはしなやかでなくなった。", "You no longer feel limber."));

				creature_ptr->muta3 &= ~(MUT3_LIMBER);
			}
		}
	}
	else if (muta_class == &(creature_ptr->muta2))
	{
		if (muta_which == MUT2_COWARDICE)
		{
			if (creature_ptr->muta3 & MUT3_FEARLESS)
			{
				msg_print(_("恐れ知らずでなくなった。", "You no longer feel fearless."));

				creature_ptr->muta3 &= ~(MUT3_FEARLESS);
			}
		}
		if (muta_which == MUT2_BEAK)
		{
			if (creature_ptr->muta2 & MUT2_TRUNK)
			{
				msg_print(_("あなたの鼻はもう象の鼻のようではなくなった。", "Your nose is no longer elephantine."));

				creature_ptr->muta2 &= ~(MUT2_TRUNK);
			}
		}
		if (muta_which == MUT2_TRUNK)
		{
			if (creature_ptr->muta2 & MUT2_BEAK)
			{
				msg_print(_("硬いクチバシがなくなった。", "You no longer have a hard beak."));

				creature_ptr->muta2 &= ~(MUT2_BEAK);
			}
		}
	}

	creature_ptr->mutant_regenerate_mod = calc_mutant_regenerate_mod(creature_ptr);
	creature_ptr->update |= PU_BONUS;
	handle_stuff(creature_ptr);
	return TRUE;
}

/*!
 * @brief プレイヤーから突然変異を取り除く
 * @param choose_mut 取り除きたい突然変異のID、0ならばランダムに消去
 * @return なし
 */
bool lose_mutation(player_type *creature_ptr, MUTATION_IDX choose_mut)
{
	int attempts_left = 20;
	concptr muta_desc = "";
	bool muta_chosen = FALSE;
	int muta_which = 0; // mutation_flag_type_1 とmutation_flag_type_2 の両対応とするため、敢えてint型で定義する
	BIT_FLAGS *muta_class = NULL;

	if (choose_mut) attempts_left = 1;

	while (attempts_left--)
	{
		switch (choose_mut ? choose_mut : randint1(193))
		{
		case 1: case 2: case 3: case 4:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_SPIT_ACID;
			muta_desc = _( "酸を吹きかける能力を失った。",  "You lose the ability to spit acid.");

			break;
		case 5: case 6: case 7:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_BR_FIRE;
			muta_desc = _( "炎のブレスを吐く能力を失った。",  "You lose the ability to breathe fire.");

			break;
		case 8: case 9:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_HYPN_GAZE;
			muta_desc = _( "あなたの目はつまらない目になった。",  "Your eyes look uninteresting.");

			break;
		case 10: case 11:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_TELEKINES;
			muta_desc = _( "念動力で物を動かす能力を失った。",  "You lose the ability to move objects telekinetically.");

			break;
		case 12: case 13: case 14:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_VTELEPORT;
			muta_desc = _( "自分の意思でテレポートする能力を失った。",  "You lose the power of teleportation at will.");

			break;
		case 15: case 16:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_MIND_BLST;
			muta_desc = _( "精神攻撃の能力を失った。",  "You lose the power of Mind Blast.");

			break;
		case 17: case 18:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_RADIATION;
			muta_desc = _( "あなたは放射能を発生しなくなった。",  "You stop emitting hard radiation.");

			break;
		case 19: case 20:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_VAMPIRISM;
			muta_desc = _( "吸血の能力を失った。",  "You are no longer vampiric.");

			break;
		case 21: case 22: case 23:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_SMELL_MET;
			muta_desc = _( "金属の臭いを嗅げなくなった。",  "You no longer smell a metallic odor.");

			break;
		case 24: case 25: case 26: case 27:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_SMELL_MON;
			muta_desc = _( "不潔なモンスターの臭いを嗅げなくなった。",  "You no longer smell filthy monsters.");

			break;
		case 28: case 29: case 30:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_BLINK;
			muta_desc = _( "近距離テレポートの能力を失った。",  "You lose the power of minor teleportation.");

			break;
		case 31: case 32:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_EAT_ROCK;
			muta_desc = _( "壁は美味しそうに見えなくなった。",  "The walls look unappetizing.");

			break;
		case 33: case 34:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_SWAP_POS;
			muta_desc = _( "あなたは自分の靴に留まる感じがする。",  "You feel like staying in your own shoes.");

			break;
		case 35: case 36: case 37:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_SHRIEK;
			muta_desc = _( "あなたの声質は弱くなった。",  "Your vocal cords get much weaker.");

			break;
		case 38: case 39: case 40:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_ILLUMINE;
			muta_desc = _( "部屋を明るく照らすことが出来なくなった。",  "You can no longer light up rooms with your presence.");

			break;
		case 41: case 42:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_DET_CURSE;
			muta_desc = _( "邪悪な魔法を感じられなくなった。",  "You can no longer feel evil magics.");

			break;
		case 43: case 44: case 45:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_BERSERK;
			muta_desc = _( "制御できる激情を感じなくなった。",  "You no longer feel a controlled rage.");

			break;
		case 46:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_POLYMORPH;
			muta_desc = _( "あなたの体は安定したように見える。",  "Your body seems stable.");

			break;
		case 47: case 48:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_MIDAS_TCH;
			muta_desc = _( "ミダスの手の能力を失った。",  "You lose the Midas touch.");

			break;
		case 49:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_GROW_MOLD;
			muta_desc = _( "突然カビが嫌いになった。",  "You feel a sudden dislike for mold.");

			break;
		case 50: case 51: case 52:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_RESIST;
			muta_desc = _( "傷つき易くなった気がする。",  "You feel like you might be vulnerable.");

			break;
		case 53: case 54: case 55:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_EARTHQUAKE;
			muta_desc = _( "ダンジョンを壊す能力を失った。",  "You lose the ability to wreck the dungeon.");

			break;
		case 56:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_EAT_MAGIC;
			muta_desc = _( "魔法のアイテムはもう美味しそうに見えなくなった。",  "Your magic items no longer look delicious.");

			break;
		case 57: case 58:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_WEIGH_MAG;
			muta_desc = _( "魔力を感じられなくなった。",  "You no longer sense magic.");

			break;
		case 59:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_STERILITY;
			muta_desc = _( "たくさんの安堵の吐息が聞こえた。",  "You hear a massed sigh of relief.");

			break;
		case 60: case 61:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_HIT_AND_AWAY;
			muta_desc = _( "あちこちへ跳べる気分がなくなった。",  "You no longer feel jumpy.");

			break;
		case 62: case 63: case 64:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_DAZZLE;
			muta_desc = _( "まばゆい閃光を発する能力を失った。",  "You lose the ability to emit dazzling lights.");

			break;
		case 65: case 66: case 67:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_LASER_EYE;
			muta_desc = _( "眼が少しの間焼き付いて、痛みが和らいだ。",  "Your eyes burn for a moment, then feel soothed.");

			break;
		case 68: case 69:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_RECALL;
			muta_desc = _( "少しの間ホームシックになった。",  "You feel briefly homesick.");

			break;
		case 70:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_BANISH;
			muta_desc = _( "神聖な怒りの力を感じなくなった。",  "You no longer feel a holy wrath.");

			break;
		case 71: case 72:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_COLD_TOUCH;
			muta_desc = _( "手が暖かくなった。",  "Your hands warm up.");

			break;
		case 73: case 74:
			muta_class = &(creature_ptr->muta1);
			muta_which = MUT1_LAUNCHER;
			muta_desc = _( "物を投げる手が弱くなった気がする。",  "Your throwing arm feels much weaker.");

			break;
		case 75:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_BERS_RAGE;
			muta_desc = _( "凶暴化の発作にさらされなくなった！",  "You are no longer subject to fits of berserk rage!");

			break;
		case 76:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_COWARDICE;
			muta_desc = _( "もう信じがたいほど臆病ではなくなった！",  "You are no longer an incredible coward!");

			break;
		case 77:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_RTELEPORT;
			muta_desc = _( "あなたの位置はより確定的になった。",  "Your position seems more certain.");

			break;
		case 78:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_ALCOHOL;
			muta_desc = _( "あなたはアルコールを分泌しなくなった！",  "Your body stops producing alcohol!");

			break;
		case 79:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_HALLU;
			muta_desc = _( "幻覚をひき起こす精神障害を起こさなくなった！",  "You are no longer afflicted by a hallucinatory insanity!");

			break;
		case 80:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_FLATULENT;
			muta_desc = _( "もう強烈な屁はこかなくなった。",  "You are no longer subject to uncontrollable flatulence.");

			break;
		case 81: case 82:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_SCOR_TAIL;
			muta_desc = _( "サソリの尻尾がなくなった！",  "You lose your scorpion tail!");

			break;
		case 83: case 84:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_HORNS;
			muta_desc = _( "額から角が消えた！",  "Your horns vanish from your forehead!");

			break;
		case 85: case 86:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_BEAK;
			muta_desc = _( "口が普通に戻った！",  "Your mouth reverts to normal!");

			break;
		case 87: case 88:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_ATT_DEMON;
			muta_desc = _( "デーモンを引き寄せなくなった。",  "You stop attracting demons.");

			break;
		case 89:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_PROD_MANA;
			muta_desc = _( "制御不能な魔法のエネルギーを発生しなくなった。",  "You stop producing magical energy uncontrollably.");

			break;
		case 90: case 91:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_SPEED_FLUX;
			muta_desc = _( "躁鬱質でなくなった。",  "You are no longer manic-depressive.");

			break;
		case 92: case 93:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_BANISH_ALL;
			muta_desc = _( "背後に恐ろしい力を感じなくなった。",  "You no longer feel a terrifying power lurking behind you.");

			break;
		case 94:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_EAT_LIGHT;
			muta_desc = _( "世界が明るいと感じる。",  "You feel the world's a brighter place.");

			break;
		case 95: case 96:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_TRUNK;
			muta_desc = _( "鼻が普通の長さに戻った。",  "Your nose returns to a normal length.");

			break;
		case 97:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_ATT_ANIMAL;
			muta_desc = _( "動物を引き寄せなくなった。",  "You stop attracting animals.");

			break;
		case 98:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_TENTACLES;
			muta_desc = _( "触手が消えた。",  "Your tentacles vanish from your sides.");

			break;
		case 99:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_RAW_CHAOS;
			muta_desc = _( "周囲の空間が安定した気がする。",  "You feel the universe is more stable around you.");

			break;
		case 100: case 101: case 102:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_NORMALITY;
			muta_desc = _( "普通に奇妙な感じがする。",  "You feel normally strange.");

			break;
		case 103:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_WRAITH;
			muta_desc = _( "あなたは物質世界にしっかり存在している。",  "You are firmly in the physical world.");

			break;
		case 104:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_POLY_WOUND;
			muta_desc = _( "古い傷からカオスの力が去っていった。",  "You feel forces of chaos departing your old scars.");

			break;
		case 105:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_WASTING;
			muta_desc = _( "おぞましい衰弱病が治った！",  "You are cured of the horrible wasting disease!");

			break;
		case 106:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_ATT_DRAGON;
			muta_desc = _( "ドラゴンを引き寄せなくなった。",  "You stop attracting dragons.");

			break;
		case 107: case 108:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_WEIRD_MIND;
			muta_desc = _( "思考が退屈な方向に戻った。",  "Your thoughts return to boring paths.");

			break;
		case 109:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_NAUSEA;
			muta_desc = _( "胃が痙攣しなくなった。",  "Your stomach stops roiling.");

			break;
		case 110: case 111:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_CHAOS_GIFT;
			muta_desc = _( "混沌の神々の興味を惹かなくなった。",  "You lose the attention of the chaos deities.");

			break;
		case 112:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_WALK_SHAD;
			muta_desc = _( "物質世界に捕らわれている気がする。",  "You feel like you're trapped in reality.");

			break;
		case 113: case 114:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_WARNING;
			muta_desc = _( "パラノイアでなくなった。",  "You no longer feel paranoid.");

			break;
		case 115:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_INVULN;
			muta_desc = _( "無敵状態の発作を起こさなくなった。",  "You are no longer blessed with fits of invulnerability.");

			break;
		case 116: case 117:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_SP_TO_HP;
			muta_desc = _( "魔法の治癒の発作に襲われなくなった。",  "You are no longer subject to fits of magical healing.");

			break;
		case 118:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_HP_TO_SP;
			muta_desc = _( "痛みを伴う精神明瞭化の発作に襲われなくなった。",  "You are no longer subject to fits of painful clarity.");

			break;
		case 119:
			muta_class = &(creature_ptr->muta2);
			muta_which = MUT2_DISARM;
			muta_desc = _( "脚が元の大きさに戻った。",  "Your feet shrink to their former size.");

			break;
		case 120: case 121: case 122:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_HYPER_STR;
			muta_desc = _( "筋肉が普通に戻った。",  "Your muscles revert to normal.");

			break;
		case 123: case 124: case 125:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_PUNY;
			muta_desc = _( "筋肉が普通に戻った。",  "Your muscles revert to normal.");

			break;
		case 126: case 127: case 128:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_HYPER_INT;
			muta_desc = _( "脳が普通に戻った。",  "Your brain reverts to normal.");

			break;
		case 129: case 130: case 131:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_MORONIC;
			muta_desc = _( "脳が普通に戻った。",  "Your brain reverts to normal.");

			break;
		case 132: case 133:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_RESILIENT;
			muta_desc = _( "普通の丈夫さに戻った。",  "You become ordinarily resilient again.");

			break;
		case 134: case 135:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_XTRA_FAT;
			muta_desc = _( "奇跡的なダイエットに成功した！",  "You benefit from a miracle diet!");

			break;
		case 136: case 137:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_ALBINO;
			muta_desc = _( "アルビノでなくなった！",  "You are no longer an albino!");

			break;
		case 138: case 139: case 140:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_FLESH_ROT;
			muta_desc = _( "肉体を腐敗させる病気が治った！",  "Your flesh is no longer afflicted by a rotting disease!");

			break;
		case 141: case 142:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_SILLY_VOI;
			muta_desc = _( "声質が普通に戻った。",  "Your voice returns to normal.");

			break;
		case 143: case 144:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_BLANK_FAC;
			muta_desc = _( "顔に目鼻が戻った。",  "Your facial features return.");

			break;
		case 145:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_ILL_NORM;
			muta_desc = _( "心が安らぐ幻影を映し出さなくなった。",  "You stop projecting a reassuring image.");

			break;
		case 146: case 147: case 148:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_XTRA_EYES;
			muta_desc = _( "余分な目が消えてしまった！",  "Your extra eyes vanish!");

			break;
		case 149: case 150:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_MAGIC_RES;
			muta_desc = _( "魔法に弱くなった。",  "You become susceptible to magic again.");

			break;
		case 151: case 152: case 153:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_XTRA_NOIS;
			muta_desc = _( "奇妙な音を立てなくなった！",  "You stop making strange noise!");

			break;
		case 154: case 155: case 156:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_INFRAVIS;
			muta_desc = _( "赤外線視力が落ちた。",  "Your infravision is degraded.");

			break;
		case 157: case 158:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_XTRA_LEGS;
			muta_desc = _( "余分な脚が消えてしまった！",  "Your extra legs disappear!");

			break;
		case 159: case 160:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_SHORT_LEG;
			muta_desc = _( "脚の長さが普通に戻った。",  "Your legs lengthen to normal.");

			break;
		case 161: case 162:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_ELEC_TOUC;
			muta_desc = _( "体を電流が流れなくなった。",  "Electricity stops running through you.");

			break;
		case 163: case 164:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_FIRE_BODY;
			muta_desc = _( "体が炎に包まれなくなった。",  "Your body is no longer enveloped in flames.");

			break;
		case 165: case 166: case 167:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_WART_SKIN;
			muta_desc = _( "イボイボが消えた！",  "Your warts disappear!");

			break;
		case 168: case 169: case 170:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_SCALES;
			muta_desc = _( "鱗が消えた！",  "Your scales vanish!");

			break;
		case 171: case 172:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_IRON_SKIN;
			muta_desc = _( "肌が肉にもどった！",  "Your skin reverts to flesh!");

			break;
		case 173: case 174:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_WINGS;
			muta_desc = _( "背中の羽根が取れ落ちた。",  "Your wings fall off.");

			break;
		case 175: case 176: case 177:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_FEARLESS;
			muta_desc = _( "再び恐怖を感じるようになった。",  "You begin to feel fear again.");

			break;
		case 178: case 179:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_REGEN;
			muta_desc = _( "急速回復しなくなった。",  "You stop regenerating.");

			break;
		case 180: case 181:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_ESP;
			muta_desc = _( "テレパシーの能力を失った！",  "You lose your telepathic ability!");

			break;
		case 182: case 183: case 184:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_LIMBER;
			muta_desc = _( "筋肉が硬くなった。",  "Your muscles stiffen.");

			break;
		case 185: case 186: case 187:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_ARTHRITIS;
			muta_desc = _( "関節が痛くなくなった。",  "Your joints stop hurting.");

			break;
		case 188:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_BAD_LUCK;
			muta_desc = _( "黒いオーラは渦巻いて消えた。",  "Your black aura swirls and fades.");

			break;
		case 189:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_VULN_ELEM;
			muta_desc = _( "無防備な感じはなくなった。",  "You feel less exposed.");

			break;
		case 190: case 191: case 192:
			muta_class = &(creature_ptr->muta3);
			muta_which = MUT3_MOTION;
			muta_desc = _( "動作の正確さがなくなった。",  "You move with less assurance.");

			break;
		case 193:
			if (creature_ptr->pseikaku == PERSONALITY_LUCKY) break;
			muta_class = &(creature_ptr->muta3);
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

	msg_print(muta_desc);
	*(muta_class) &= ~(muta_which);

	creature_ptr->update |= PU_BONUS;
	handle_stuff(creature_ptr);
	creature_ptr->mutant_regenerate_mod = calc_mutant_regenerate_mod(creature_ptr);
	return TRUE;
}


void lose_all_mutations(player_type *creature_ptr)
{
	if (creature_ptr->muta1 || creature_ptr->muta2 || creature_ptr->muta3)
	{
		chg_virtue(creature_ptr, V_CHANCE, -5);
		msg_print(_("全ての突然変異が治った。", "You are cured of all mutations."));
		creature_ptr->muta1 = creature_ptr->muta2 = creature_ptr->muta3 = 0;
		creature_ptr->update |= PU_BONUS;
		handle_stuff(creature_ptr);
		creature_ptr->mutant_regenerate_mod = calc_mutant_regenerate_mod(creature_ptr);
	}
}


/*!
 * @brief 現在プレイヤー得ている突然変異の数を返す。
 * @return 現在得ている突然変異の数
 */
static int count_mutations(player_type *creature_ptr)
{
	return (count_bits(creature_ptr->muta1) +
		count_bits(creature_ptr->muta2) +
		count_bits(creature_ptr->muta3));
}


/*!
 * @brief 突然変異による自然回復ペナルティをパーセント値で返す /
 * Return the modifier to the regeneration rate (in percent)
 * @return ペナルティ修正(%)
 */
int calc_mutant_regenerate_mod(player_type *creature_ptr)
{
	int regen;
	int mod = 10;
	int count = count_mutations(creature_ptr);

	/*
	 * Beastman get 10 "free" mutations and
	 * only 5% decrease per additional mutation
	 */

	if (creature_ptr->pseikaku == PERSONALITY_LUCKY) count--;
	if (creature_ptr->prace == RACE_BEASTMAN)
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
 * @brief 突然変異のレイシャル効果実装
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param power 発動させる突然変異レイシャルのID
 * @return レイシャルを実行した場合TRUE、キャンセルした場合FALSEを返す
 */
bool exe_mutation_power(player_type *creature_ptr, int power)
{
	DIRECTION dir = 0;
	PLAYER_LEVEL lvl = creature_ptr->lev;

	switch (power)
	{
		case MUT1_SPIT_ACID:
			if (!get_aim_dir(creature_ptr, &dir)) return FALSE;
			stop_mouth(creature_ptr);
			msg_print(_("酸を吐きかけた...", "You spit acid..."));
			fire_ball(creature_ptr, GF_ACID, dir, lvl, 1 + (lvl / 30));
			break;

		case MUT1_BR_FIRE:
			if (!get_aim_dir(creature_ptr, &dir)) return FALSE;
			stop_mouth(creature_ptr);
			msg_print(_("あなたは火炎のブレスを吐いた...", "You breathe fire..."));
			fire_breath(creature_ptr, GF_FIRE, dir, lvl * 2, 1 + (lvl / 20));
			break;

		case MUT1_HYPN_GAZE:
			if (!get_aim_dir(creature_ptr, &dir)) return FALSE;
			msg_print(_("あなたの目は幻惑的になった...", "Your eyes look mesmerizing..."));
			(void)charm_monster(creature_ptr, dir, lvl);
			break;

		case MUT1_TELEKINES:
			if (!get_aim_dir(creature_ptr, &dir)) return FALSE;
			msg_print(_("集中している...", "You concentrate..."));
			fetch_item(creature_ptr, dir, lvl * 10, TRUE);
			break;

		case MUT1_VTELEPORT:
			msg_print(_("集中している...", "You concentrate..."));
			teleport_player(creature_ptr, 10 + 4 * lvl, TELEPORT_SPONTANEOUS);
			break;

		case MUT1_MIND_BLST:
			if (!get_aim_dir(creature_ptr, &dir)) return FALSE;
			msg_print(_("集中している...", "You concentrate..."));
			fire_bolt(creature_ptr, GF_PSI, dir, damroll(3 + ((lvl - 1) / 5), 3));
			break;

		case MUT1_RADIATION:
			msg_print(_("体から放射能が発生した！", "Radiation flows from your body!"));
			fire_ball(creature_ptr, GF_NUKE, 0, (lvl * 2), 3 + (lvl / 20));
			break;

		case MUT1_VAMPIRISM:
			vampirism(creature_ptr);
			break;

		case MUT1_SMELL_MET:
			stop_mouth(creature_ptr);
			(void)detect_treasure(creature_ptr, DETECT_RAD_DEFAULT);
			break;

		case MUT1_SMELL_MON:
			stop_mouth(creature_ptr);
			(void)detect_monsters_normal(creature_ptr, DETECT_RAD_DEFAULT);
			break;

		case MUT1_BLINK:
			teleport_player(creature_ptr, 10, TELEPORT_SPONTANEOUS);
			break;

		case MUT1_EAT_ROCK:
			return eat_rock(creature_ptr);
			break;

		case MUT1_SWAP_POS:
			project_length = -1;
			if (!get_aim_dir(creature_ptr, &dir))
			{
				project_length = 0;
				return FALSE;
			}
			(void)teleport_swap(creature_ptr, dir);
			project_length = 0;
			break;

		case MUT1_SHRIEK:
			stop_mouth(creature_ptr);
			(void)fire_ball(creature_ptr, GF_SOUND, 0, 2 * lvl, 8);
			(void)aggravate_monsters(creature_ptr, 0);
			break;

		case MUT1_ILLUMINE:
			(void)lite_area(creature_ptr, damroll(2, (lvl / 2)), (lvl / 10) + 1);
			break;

		case MUT1_DET_CURSE:
			{
				int i;

				for (i = 0; i < INVEN_TOTAL; i++)
				{
					object_type *o_ptr = &creature_ptr->inventory_list[i];

					if (!o_ptr->k_idx) continue;
					if (!object_is_cursed(o_ptr)) continue;

					o_ptr->feeling = FEEL_CURSED;
				}
			}
			break;

		case MUT1_BERSERK:
			(void)berserk(creature_ptr, randint1(25) + 25);
			break;

		case MUT1_POLYMORPH:
			if (!get_check(_("変身します。よろしいですか？", "You will polymorph your self. Are you sure? "))) return FALSE;
			do_poly_self(creature_ptr);
			break;

		case MUT1_MIDAS_TCH:
			if (!alchemy(creature_ptr)) return FALSE;
			break;

		/* Summon pet molds around the player */
		case MUT1_GROW_MOLD:
			{
				DIRECTION i;
				for (i = 0; i < 8; i++)
				{
					summon_specific(creature_ptr, -1, creature_ptr->y, creature_ptr->x, lvl, SUMMON_MOLD, PM_FORCE_PET);
				}
			}
			break;

		case MUT1_RESIST:
			{
				int num = lvl / 10;
				TIME_EFFECT dur = randint1(20) + 20;

				if (randint0(5) < num)
				{
					(void)set_oppose_acid(creature_ptr, dur, FALSE);
					num--;
				}
				if (randint0(4) < num)
				{
					(void)set_oppose_elec(creature_ptr, dur, FALSE);
					num--;
				}
				if (randint0(3) < num)
				{
					(void)set_oppose_fire(creature_ptr, dur, FALSE);
					num--;
				}
				if (randint0(2) < num)
				{
					(void)set_oppose_cold(creature_ptr, dur, FALSE);
					num--;
				}
				if (num)
				{
					(void)set_oppose_pois(creature_ptr, dur, FALSE);
					num--;
				}
			}
			break;

		case MUT1_EARTHQUAKE:
			(void)earthquake(creature_ptr, creature_ptr->y, creature_ptr->x, 10, 0);
			break;

		case MUT1_EAT_MAGIC:
			if (!eat_magic(creature_ptr, creature_ptr->lev * 2)) return FALSE;
			break;

		case MUT1_WEIGH_MAG:
			report_magics(creature_ptr);
			break;

		case MUT1_STERILITY:
			msg_print(_("突然頭が痛くなった！", "You suddenly have a headache!"));
			take_hit(creature_ptr, DAMAGE_LOSELIFE, randint1(17) + 17, _("禁欲を強いた疲労", "the strain of forcing abstinence"), -1);
			creature_ptr->current_floor_ptr->num_repro += MAX_REPRO;
			break;

		case MUT1_HIT_AND_AWAY:
			if(!hit_and_away(creature_ptr)) return FALSE;
			break;

		case MUT1_DAZZLE:
			stun_monsters(creature_ptr, lvl * 4);
			confuse_monsters(creature_ptr, lvl * 4);
			turn_monsters(creature_ptr, lvl * 4);
			break;

		case MUT1_LASER_EYE:
			if (!get_aim_dir(creature_ptr, &dir)) return FALSE;
			fire_beam(creature_ptr, GF_LITE, dir, 2 * lvl);
			break;

		case MUT1_RECALL:
			if (!recall_player(creature_ptr, randint0(21) + 15)) return FALSE;
			break;

		case MUT1_BANISH:
			{
				POSITION x, y;
				grid_type *g_ptr;
				monster_type *m_ptr;
				monster_race *r_ptr;
				if (!get_direction(creature_ptr, &dir, FALSE, FALSE)) return FALSE;
				y = creature_ptr->y + ddy[dir];
				x = creature_ptr->x + ddx[dir];
				g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

				if (!g_ptr->m_idx)
				{
					msg_print(_("邪悪な存在を感じとれません！", "You sense no evil there!"));

					break;
				}

				m_ptr = &creature_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
				r_ptr = &r_info[m_ptr->r_idx];

				if ((r_ptr->flags3 & RF3_EVIL) &&
				    !(r_ptr->flags1 & RF1_QUESTOR) &&
				    !(r_ptr->flags1 & RF1_UNIQUE) &&
				    !creature_ptr->current_floor_ptr->inside_arena && !creature_ptr->current_floor_ptr->inside_quest &&
					(r_ptr->level < randint1(creature_ptr->lev+50)) &&
					!(m_ptr->mflag2 & MFLAG2_NOGENO))
				{
					if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname)
					{
						GAME_TEXT m_name[MAX_NLEN];
						monster_desc(creature_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
						exe_write_diary(creature_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_GENOCIDE, m_name);
					}
					/* Delete the monster, rather than killing it. */
					delete_monster_idx(creature_ptr, g_ptr->m_idx);
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
				POSITION x, y;
				grid_type *g_ptr;
				if (!get_direction(creature_ptr, &dir, FALSE, FALSE)) return FALSE;
				y = creature_ptr->y + ddy[dir];
				x = creature_ptr->x + ddx[dir];
				g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
				if (!g_ptr->m_idx)
				{
					msg_print(_("あなたは何もない場所で手を振った。", "You wave your hands in the air."));

					break;
				}
				fire_bolt(creature_ptr, GF_COLD, dir, 2 * lvl);
			}
			break;

		/* XXX_XXX_XXX Hack! MUT1_LAUNCHER is negative, see above */
		case 3: /* MUT1_LAUNCHER */
			/* Gives a multiplier of 2 at first, up to 3 at 40th */
			if (!do_cmd_throw(creature_ptr, 2 + lvl / 40, FALSE, -1)) return FALSE;
			break;

		default:
			free_turn(creature_ptr);
			msg_format(_("能力 %s は実装されていません。", "Power %s not implemented. Oops."), power);
	}

	return TRUE;
}

void become_living_trump(player_type *creature_ptr)
{
	MUTATION_IDX mutation;

	if (one_in_(7))
		mutation = 12; /* Teleport control */
	else
		mutation = 77; /* Random teleportation (uncontrolled) */

	/* Gain the mutation */
	if (gain_mutation(creature_ptr, mutation))
	{
		msg_print(_("あなたは生きているカードに変わった。", "You have turned into a Living Trump."));
	}
}

void set_mutation_flags(player_type *creature_ptr)
{
    if (creature_ptr->muta3) {

        if (creature_ptr->muta3 & MUT3_FLESH_ROT) {
            creature_ptr->regenerate = FALSE;
        }

        if (creature_ptr->muta3 & MUT3_ELEC_TOUC) {
            creature_ptr->sh_elec = TRUE;
        }

        if (creature_ptr->muta3 & MUT3_FIRE_BODY) {
            creature_ptr->sh_fire = TRUE;
            creature_ptr->lite = TRUE;
        }

        if (creature_ptr->muta3 & MUT3_WINGS) {
            creature_ptr->levitation = TRUE;
        }

        if (creature_ptr->muta3 & MUT3_FEARLESS) {
            creature_ptr->resist_fear = TRUE;
        }

        if (creature_ptr->muta3 & MUT3_REGEN) {
            creature_ptr->regenerate = TRUE;
        }

        if (creature_ptr->muta3 & MUT3_ESP) {
            creature_ptr->telepathy = TRUE;
        }

        if (creature_ptr->muta3 & MUT3_MOTION) {
            creature_ptr->free_act = TRUE;
        }
    }
}
