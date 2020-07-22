/*!
 * @file mind.c
 * @brief 各職業の特殊技能実装 / Special magics
 * @date 2014/01/15
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2005 henkma \n
 * 2014 Deskull rearranged comment for Doxygen.\n
 * @details
 * mind.cとあるが実際には超能力者、練気術師、狂戦士、鏡使い、忍者までの
 * 特殊技能を揃えて実装している。
 */

#include "mind/mind-switcher.h"
#include "action/action-limited.h"
#include "action/movement-execution.h"
#include "cmd-action/cmd-attack.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-item/cmd-throw.h"
#include "core/asking-player.h"
#include "core/hp-mp-processor.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "effect/effect-characteristics.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "floor/floor.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "game-option/text-display-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "io/targeting.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-magic-resistance.h"
#include "mind/mind-mindcrafter.h"
#include "mind/mind-mirror-master.h"
#include "mind/mind-ninja.h"
#include "mind/mind-warrior.h"
#include "monster-floor/monster-summon.h"
#include "monster/monster-status.h"
#include "monster-floor/place-monster-types.h"
#include "player-attack/player-attack.h"
#include "player/avatar.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "player/special-defense-types.h"
#include "spell/process-effect.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-fetcher.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-world.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell/spell-types.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/sight-setter.h"
#include "status/temporary-resistance.h"
#include "system/floor-type-definition.h"
#include "term/screen-processor.h"
#include "util/buffer-shaper.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

/*! 特殊技能の一覧テーブル */
mind_power const mind_powers[5] =
{
  {
    {
      /* Level gained,  cost,  %fail,  name */
#ifdef JP
      { 1,   1,  15, "霊視"},
      { 2,   1,  20, "神経攻撃"},
      { 3,   2,  25, "次元の瞬き"},
      { 7,   6,  35, "虚空の幻影"},
      { 9,   7,  50, "精神支配"},
      { 11,  7,  30, "念動衝撃弾"},
      { 13, 12,  50, "鎧化"},
      { 15, 12,  60, "サイコメトリー"},
      { 18, 10,  45, "精神波動"},
      { 23, 15,  50, "アドレナリン・ドーピング"},
      { 26, 28,  60, "テレキネシス"},
      { 28, 10,  40, "サイキック・ドレイン"},
      { 35, 35,  75, "光の剣"},
      { 45,150,  85, "完全な世界"},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
#else
      { 1,   1,  15, "Precognition"},
      { 2,   1,  20, "Neural Blast"},
      { 3,   2,  25, "Minor Displacement"},
      { 7,   6,  35, "Major Displacement"},
      { 9,   7,  50, "Domination"},
      { 11,  7,  30, "Pulverise"},
      { 13, 12,  50, "Character Armour"},
      { 15, 12,  60, "Psychometry" },
      { 18, 10,  45, "Mind Wave" },
      { 23, 15,  50, "Adrenaline Channeling"},
      { 26, 28,  60, "Telekinesis"},
      { 28, 10,  40, "Psychic Drain"},
      { 35, 35,  75, "Psycho-Spear"},
      { 45,150,  85, "The World"},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
#endif
      
    }
  },
  
  {
    {
      /* Level gained,  cost,  %fail,  name */
#ifdef JP
      { 1,   1,  15, "小龍"},
      { 3,   3,  30, "閃光"},
      { 5,   6,  35, "舞空術"},
      { 8,   5,  40, "カメハメ波"},
      { 10,  7,  45, "対魔法防御"},
      { 13,  5,  60, "練気"},
      { 17, 17,  50, "纏闘気"},
      { 20, 20,  50, "衝波"},
      { 23, 18,  55, "彗龍"},
      { 25, 30,  70, "いてつく波動"},
      { 28, 26,  50, "幻霊召喚"},
      { 32, 35,  65, "煉獄火炎"},
      { 38, 42,  75, "超カメハメ波"},
      { 44, 50,  80, "光速移動"},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
#else
      { 1,   1,  15, "Small Force Ball"},
      { 3,   3,  30, "Flash Light"},
      { 5,   6,  35, "Flying Technique"},
      { 8,   5,  40, "Kamehameha"},
      { 10,  7,  45, "Magic Resistance"},
      { 13,  5,  60, "Improve Force"},
      { 17, 17,  50, "Aura of Force"},
      { 20, 20,  50, "Shock Power"},
      { 23, 18,  55, "Large Force Ball"},
      { 25, 30,  70, "Dispel Magic"},
      { 28, 26,  50, "Summon Ghost"},
      { 32, 35,  65, "Exploding Frame"},
      { 38, 42,  75, "Super Kamehameha"},
      { 44, 50,  80, "Light Speed"},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
#endif
      
    }
  },
  
  {
    {
      /* Level gained,  cost,  %fail,  name */
#ifdef JP
      {  8,  5,  40, "殺気感知"},
      { 15, 20,   0, "突撃"},
      { 20, 15,   0, "トラップ粉砕"},
      { 25, 20,  60, "地震"},
      { 30, 80,  75, "皆殺し"},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
#else
      {  8,  5,  40, "Detect Atmosphere of Menace"},
      { 15, 20,   0, "Charge"},
      { 20, 15,   0, "Smash a Trap"},
      { 25, 20,  60, "Quake"},
      { 30, 80,  75, "Massacre"},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
      { 99,  0,   0, ""},
#endif
      
    }
  },

  {
    {
      /* Level gained,  cost,  %fail,  name */
#ifdef JP
      { 1,   1,  15, "真見の鏡"},
      { 1,   2,  40, "鏡生成"},
      { 2,   2,  20, "光のしずく"},
      { 3,   2,  20, "歪んだ鏡"},
      { 5,   3,  35, "閃光鏡"},
      { 6,   5,  35, "彷える鏡"},

      { 10,  5,  30, "微塵隠れ"},
      { 12, 12,  30, "追放の鏡"},
      { 15, 15,  30, "鏡砕き"},
      { 19, 13,  30, "催眠鏡"},
      { 23, 18,  50, "シーカーレイ"},

      { 25, 20,  40, "鏡の封印"},
      { 27, 30,  60, "水鏡の盾"},
      { 29, 30,  60, "スーパーレイ"},
      { 31, 35,  60, "幻惑の光"},
      { 33, 50,  80, "鏡の国"},

      { 36, 30,  80, "鏡抜け"},
      { 38, 40,  70, "帰還の鏡"},
      { 40, 50,  55, "影分身"},
      { 43, 55,  70, "封魔結界"},
      { 46, 70,  75, "ラフノールの鏡"},
#else
      { 1,   1,  15, "Mirror of Seeing"},
      { 1,   2,  40, "Making a Mirror"},
      { 2,   2,  20, "Drip of Light"},
      { 3,   2,  20, "Warped Mirror"},
      { 5,   3,  35, "Mirror of Light"},
      { 6,   5,  35, "Mirror of Wandering"},

      { 10,  5,  30, "Robe of Dust"},
      { 12, 12,  30, "Banishing Mirror"},
      { 15, 15,  30, "Mirror Clashing"},
      { 19, 13,  30, "Mirror Sleeping"},
      { 23, 18,  50, "Seeker Ray"},

      { 25, 20,  40, "Seal of Mirror"},
      { 27, 30,  60, "Shield of Water"},
      { 29, 30,  60, "Super Ray"},
      { 31, 35,  60, "Illusion Light"},
      { 33, 50,  80, "Mirror Shift"},

      { 36, 30,  80, "Mirror Tunnel"},
      { 38, 40,  70, "Mirror of Recall"},
      { 40, 50,  55, "Multi-Shadow"},
      { 43, 55,  70, "Binding Field"},
      { 46, 70,  75, "Mirror of Ruffnor"},
#endif
      
    }
  },
  
  {
    {
      /* Level gained,  cost,  %fail,  name */
#ifdef JP
      {  1,  1,  20, "暗闇生成"},
      {  2,  2,  25, "周辺調査"},
      {  3,  3,  25, "葉隠れ"},
      {  5,  3,  30, "変わり身"},
      {  7,  8,  35, "高飛び"},
      {  8, 10,  35, "一撃離脱"},
      { 10, 10,  40, "金縛り"},
      { 12, 12,  70, "古の口伝"},
      { 15, 10,  50, "浮雲"},
      { 17, 12,  45, "火遁"},
      { 18, 20,  40, "入身"},
      { 20,  5,  50, "八方手裏剣"},
      { 22, 15,  55, "鎖鎌"},
      { 25, 32,  60, "煙玉"},
      { 28, 32,  60, "転身"},
      { 30, 30,  70, "爆発の紋章"},
      { 32, 40,  40, "土遁"},
      { 34, 35,  50, "霧隠れ"},
      { 38, 40,  60, "煉獄火炎"},
      { 41, 50,  55, "分身"},
      { 99,  0,   0, ""},
#else
      {  1,  1,  20, "Create Darkness"},
      {  2,  2,  25, "Detect Near"},
      {  3,  3,  25, "Hide in Leafs"},
      {  5,  3,  30, "Kawarimi"},
      {  7,  8,  35, "Absconding"},
      {  8, 10,  35, "Hit and Away"},
      { 10, 10,  40, "Bind Monster"},
      { 12, 12,  70, "Ancient Knowledge"},
      { 15, 10,  50, "Floating"},
      { 17, 12,  45, "Hide in Flame"},
      { 18, 20,  40, "Nyusin"},
      { 20,  5,  50, "Syuriken Spreading"},
      { 22, 15,  55, "Chain Hook"},
      { 25, 32,  60, "Smoke Ball"},
      { 28, 32,  60, "Swap Position"},
      { 30, 30,  70, "Glyph of Explosion"},
      { 32, 40,  40, "Hide in Mud"},
      { 34, 35,  50, "Hide in Mist"},
      { 38, 40,  60, "Rengoku-Kaen"},
      { 41, 50,  55, "Bunshin"},
      { 99,  0,   0, ""},
#endif
      
    }
  },
};

/*! 特殊能力の解説文字列 */
static concptr const mind_tips[5][MAX_MIND_POWERS] =
{
#ifdef JP
{
	"近くの全ての見えるモンスターを感知する。レベル5で罠/扉、15で透明なモンスター、30で財宝とアイテムを感知できるようになる。レベル20で周辺の地形を感知し、45でその階全体を永久に照らし、ダンジョン内のすべてのアイテムを感知する。レベル25で一定時間テレパシーを得る。",
	"精神攻撃のビームまたは球を放つ。",
	"近距離のテレポートをする。",
	"遠距離のテレポートをする。",
	"レベル30未満で、モンスターを朦朧か混乱か恐怖させる球を放つ。レベル30以上で視界内の全てのモンスターを魅了する。抵抗されると無効。",
	"テレキネシスの球を放つ。",
	"一定時間、ACを上昇させる。レベルが上がると、酸、炎、冷気、電撃、毒の耐性も得られる。",
	"レベル25未満で、アイテムの雰囲気を知る。レベル25以上で、アイテムを鑑定する。",
	"レベル25未満で、自分を中心とした精神攻撃の球を発生させる。レベル25以上で、視界内の全てのモンスターに対して精神攻撃を行う。",
	"恐怖と朦朧から回復し、ヒーロー気分かつ加速状態でなければHPが少し回復する。さらに、一定時間ヒーロー気分になり、加速する。",
	"アイテムを自分の足元へ移動させる。",
	"精神攻撃の球を放つ。モンスターに命中すると、0～1.5ターン消費する。抵抗されなければ、MPが回復する。",
	"無傷球をも切り裂く純粋なエネルギーのビームを放つ。",
	"時を止める。全MPを消費し、消費したMPに応じて長く時を止めていられる。",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
},
{
	"ごく小さい気の球を放つ。",
	"光源が照らしている範囲か部屋全体を永久に明るくする。",
	"一定時間、空中に浮けるようになる。",
	"射程の短い気のビームを放つ。",
	"一定時間、魔法防御能力を上昇させる。",
	"気を練る。気を練ると術の威力は上がり、持続時間は長くなる。練った気は時間とともに拡散する。練りすぎると暴走する危険がある。",
	"一定時間、攻撃してきた全てのモンスターを傷つけるオーラを纏う。",
	"隣りのモンスターに対して気をぶつけ、吹きとばす。",
	"大きな気の球を放つ。",
	"モンスター1体にかかった魔法を解除する。",
	"1体の幽霊を召喚する。",
	"自分を中心とした超巨大な炎の球を発生させる。",
	"射程の長い、強力な気のビームを放つ。",
	"しばらくの間、非常に速く動くことができる。",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
},
{
	"近くの思考することができるモンスターを感知する。",
	"攻撃した後、反対側に抜ける。",
	"トラップにかかるが、そのトラップを破壊する。",
	"周囲のダンジョンを揺らし、壁と床をランダムに入れ変える。",
	"全方向に向かって攻撃する。",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
},
{
	"近くの全てのモンスターを感知する。レベル15で透明なモンスターを感知する。レベル25で一定時間テレパシーを得る。レベル35で周辺の地形を感知する。全ての効果は、鏡の上でないとレベル4だけ余計に必要になる。",
	"自分のいる床の上に鏡を生成する。",
	"閃光の矢を放つ。レベル10以上では鏡の上で使うとビームになる。",
	"近距離のテレポートをする。",
	"自分の周囲や、 自分のいる部屋全体を明るくする。",
	"遠距離のテレポートをする。",
	"一定時間、鏡のオーラが付く。攻撃を受けると破片のダメージで反撃し、さらに鏡の上にいた場合近距離のテレポートをする。",
	"モンスターをテレポートさせるビームを放つ。抵抗されると無効。",
	"破片の球を放つ。",
	"全ての鏡の周りに眠りの球を発生させる。",
	"ターゲットに向かって魔力のビームを放つ。鏡に命中すると、その鏡を破壊し、別の鏡に向かって反射する。",
	"鏡の上のモンスターを消し去る。",
	"一定時間、ACを上昇させる。レベル32で反射が付く。レベル40で魔法防御が上がる。",
	"ターゲットに向かって強力な魔力のビームを放つ。鏡に命中すると、その鏡を破壊し、8方向に魔力のビームを発生させる。",
	"視界内のモンスターを減速させ、朦朧とさせ、混乱させ、恐怖させ、麻痺させる。鏡の上で使うと威力が高い。",
	"フロアを作り変える。鏡の上でしか使えない。",
	"短距離内の指定した場所にテレポートする。",
	"地上にいるときはダンジョンの最深階へ、ダンジョンにいるときは地上へと移動する。",
	"全ての攻撃が、1/2の確率で無効になる。",
	"視界内の2つの鏡とプレイヤーを頂点とする三角形の領域に、魔力の結界を発生させる。",
	"一定時間、ダメージを受けなくなるバリアを張る。切れた瞬間に少しターンを消費するので注意。",
},
{
	"半径3以内かその部屋を暗くする。",
	"近くの全ての見えるモンスターを感知する。レベル5で罠/扉/階段、レベル15でアイテムを感知できるようになる。レベル45でその階全体の地形と全てのアイテムを感知する。",
	"近距離のテレポートをする。",
	"攻撃を受けた瞬間にテレポートをするようになる。失敗するとその攻撃のダメージを受ける。テレポートに失敗することもある。",
	"遠距離のテレポートをする。",
	"攻撃してすぐにテレポートする。",
	"敵1体の動きを封じる。ユニークモンスター相手の場合又は抵抗された場合には無効。",
	"アイテムを識別する。",
	"一定時間、浮遊能力を得る。",
	"自分を中心とした火の球を発生させ、テレポートする。さらに、一定時間炎に対する耐性を得る。装備による耐性に累積する。",
	"素早く相手に近寄り攻撃する。",
	"ランダムな方向に8回くさびを投げる。",
	"敵を1体自分の近くに引き寄せる。",
	"ダメージのない混乱の球を放つ。",
	"1体のモンスターと位置を交換する。",
	"自分のいる床の上に、モンスターが通ると爆発してダメージを与えるルーンを描く。",
	"一定時間、半物質化し壁を通り抜けられるようになる。さらに、一定時間酸への耐性を得る。装備による耐性に累積する。",
	"自分を中心とした超巨大な毒、衰弱、混乱の球を発生させ、テレポートする。",
	"ランダムな方向に何回か炎か地獄かプラズマのビームを放つ。",
	"全ての攻撃が、1/2の確率で無効になる。",
	"",
},
#else
{
	"Detects visible monsters in your vicinity. Detects traps and doors at level 5, invisible monsters at level 15, and items at level 30. Gives telepathy at level 25. Magically maps the surroundings at level 20. Lights and reveals the whole level at level 45.",
	"Fires a beam or ball which inflicts PSI damage.",
	"Teleports you a short distance.",
	"Teleports you a long distance.",
	"Stuns, confuses or scares a monster. Or attempts to charm all monsters in sight at level 30.",
	"Fires a ball which hurts monsters with telekinesis.",
	"Gives stone skin and some resistance to elements for a while. As your level increases, more resistances are given.",
	"Gives feeling of an item. Or identifies an item at level 25.",
	"Generates a ball centered on you which inflicts PSI damage on a monster or, at level 25 and higher, inflicts PSI damage on all monsters.",
	"Removes fear and being stunned. Gives heroism and speed. Heals HP a little unless you already have heroism and a temporary speed boost.",
	"Pulls a distant item close to you.",
	"Fires a ball which damages. When not resisted, you gain SP. You will be occupied for 0 to 1.5 turns after casting as your mind recovers.",
	"Fires a beam of pure energy which penetrates invulnerability barriers.",
	"Stops time. Consumes all of your SP. The more SP consumed, the longer the duration of the spell.",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
},
{
	"Fires a very small energy ball.",
	"Lights up nearby area and the inside of a room permanently.",
	"Gives levitation a while.",
	"Fires a short energy beam.",
	"Gives magic resistance for a while.",
	"Increases your spirit energy temporarily. More spirit energy will boost the effect or duration of your force abilities. Too much spirit energy can result in an explosion.",
	"Envelops you with a temporary aura that damages any monster which hits you in melee.",
	"Damages an adjacent monster and blows it away.",
	"Fires a large energy ball.",
	"Dispels all magics which are affecting a monster.",
	"Summons ghosts.",
	"Generates a huge ball of flame centered on you.",
	"Fires a long, powerful energy beam.",
	"Gives extremely fast speed.",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
},
{
	"Detects all monsters except the mindless in your vicinity.",
	"In one action, attacks a monster with your weapons normally and then moves to the space beyond the monster if that space is not blocked.",
	"Sets off a trap, then destroys that trap.",
	"Shakes dungeon structure, and results in random swapping of floors and walls.",
	"Attacks all adjacent monsters.",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
},
{
	"Detects visible monsters in your vicinity. Detects invisible monsters at level 15. Gives telepathy at level 25. Magically maps the surroundings at level 35. All of the effects need 4 more levels unless on a mirror.",
	"Makes a mirror under you.",
	"Fires bolt of light. At level ten or higher, the bolt will be a beam of light if you are on a mirror.",
	"Teleports you a short distance.",
	"Lights up nearby area and the inside of a room permanently.",
	"Teleports you a long distance.",
	"Gives a temporary aura of mirror shards. The aura damages any monster that hits you in melee. If you are on a mirror, the aura will teleport you a short distance if a monster hits you in melee.",
	"Teleports all monsters on the line away unless resisted.",
	"Fires a ball of shards.",
	"Causes any mirror to lull to sleep monsters close to the mirror.",
	"Fires a beam of mana. If the beam hits a mirror, it breaks that mirror and bounces toward another mirror.",
	"Eliminates a monster on a mirror from current dungeon level.",
	"Gives a bonus to AC. Gives reflection at level 32. Gives magic resistance at level 40.",
	"Fires a powerful beam of mana. If the beam hits a mirror, it breaks that mirror and fires 8 beams of mana to 8 different directions from that point.",
	"Attempts to slow, stun, confuse, scare, freeze all monsters in sight. Gets more power on a mirror.",
	"Recreates current dungeon level. Can only be used on a mirror.",
	"Teleports you to a given location.",
	"Recalls player from dungeon to town or from town to the deepest level of dungeon.",
	"Completely protects you from any attacks at one in two chance.",
	"Generates a magical triangle which damages all monsters in the area. The vertices of the triangle are you and two mirrors in sight.",
	"Generates a barrier which completely protects you from almost all damage. Takes a few of your turns when the barrier breaks or duration time is exceeded.",
},

{
	"Darkens nearby area and inside of a room.",
	"Detects visible monsters in your vicinity. Detects traps, doors and stairs at level 5. Detects items at level 15. Lights and reveals the whole level at level 45.",
	"Teleports you a short distance.",
	"Teleports you as you receive an attack. Might be able to teleport just before receiving damage at higher levels.",
	"Teleports you a long distance.",
	"Attacks an adjacent monster and teleports you away immediately after the attack.",
	"Attempts to freeze a monster.",
	"Identifies an item.",
	"Gives levitation for a while.",
	"Generates a fire ball and immediately teleports you away. Gives resistance to fire for a while. This resistance can be added to that from equipment for more powerful resistance.",
	"Steps close to a monster and attacks at a time.",
	"Shoots 8 iron Spikes in 8 random directions.",
	"Teleports a monster to a place adjacent to you.",
	"Releases a confusion ball which doesn't inflict any damage.",
	"Causes you and a targeted monster to exchange positions.",
	"Sets a glyph under you. The glyph will explode when a monster moves on it.",
	"Makes you ethereal for a period of time. While ethereal, you can pass through walls and are resistant to acid. The resistance can be added to that from equipment for more powerful resistance.",
	"Generates huge balls of poison, drain life and confusion. Then immediately teleports you away.",
	"Fires some number of beams of fire, nether or plasma in random directions.",
	"Creates shadows of yourself which gives you the ability to completely evade any attacks at one in two chance for a while.",
	"",
},
#endif
};

/*!
 * @brief 特殊技能の効果情報をまとめたフォーマットを返す
 * @param p 情報を返す文字列参照ポインタ
 * @param use_mind 職業毎の特殊技能ID
 * @param power モンスター魔法のID
 * @return なし
 */
void mindcraft_info(player_type *caster_ptr, char *p, int use_mind, int power)
{
	PLAYER_LEVEL plev = caster_ptr->lev;

	strcpy(p, "");

	switch (use_mind)
	{
	case MIND_MINDCRAFTER:
		switch (power)
		{
		case 0:  break;
		case 1:  sprintf(p, " %s%dd%d", KWD_DAM, 3 + ((plev - 1) / 4), 3 + plev/15); break;
		case 2:  sprintf(p, " %s10", KWD_SPHERE); break;
		case 3:  sprintf(p, " %s%d", KWD_SPHERE, plev * 5);  break;
		case 4:  break;
		case 5: sprintf(p, " %s%dd8", KWD_DAM, 8 + ((plev - 5) / 4));  break;
		case 6:  sprintf(p, " %s%d", KWD_DURATION, plev);  break;
		case 7:  break;
		case 8:  sprintf(p, (plev < 25 ? " %s%d" : " %sd%d"), KWD_DAM, (plev < 25 ? plev * 3 / 2 : plev * ((plev - 5) / 10 + 1))); break;
		case 9:  sprintf(p, " %s10+d%d", KWD_DURATION, plev * 3 / 2);  break;
#ifdef JP
		case 10: sprintf(p, " 最大重量:%d.%dkg", lbtokg1(plev * 15),lbtokg2(plev * 15));  break;
#else
		case 10: sprintf(p, " max wgt %d", plev * 15);  break;
#endif
		case 11: sprintf(p, " %s%dd6", KWD_DAM, plev / 2);  break;
		case 12: sprintf(p, " %sd%d+%d", KWD_DAM, plev * 3, plev * 3); break;
		case 13: sprintf(p, _(" 行動:%ld回", " %ld acts."), (long int)(caster_ptr->csp + 100-caster_ptr->energy_need - 50)/100); break;
		}
		break;
	case MIND_KI:
	{
		int boost = get_current_ki(caster_ptr);

		if (heavy_armor(caster_ptr)) boost /= 2;

		switch (power)
		{
		case 0:  sprintf(p, " %s%dd4", KWD_DAM, 3 + ((plev - 1) / 5) + boost / 12); break;
		case 1:  break;
		case 2:  sprintf(p, " %s%d+d30", KWD_DURATION, 30 + boost / 5); break;
		case 3:  sprintf(p, " %s%dd5", KWD_DAM, 5 + ((plev - 1) / 5) + boost / 10); break;
		case 4:  sprintf(p, " %s%d+d20", KWD_DURATION, 20 + boost / 5); break;
		case 5:  break;
		case 6:  sprintf(p, " %s%d+d%d", KWD_DURATION, 15 + boost / 7, plev / 2); break;
		case 7:  sprintf(p, " %s%dd8", KWD_DAM, 8 + ((plev - 5) / 4) + boost / 12); break;
		case 8:  sprintf(p, " %s10d6+%d", KWD_DAM, plev * 3 / 2 + boost * 3 / 5); break;
		case 9:  break;
		case 10: sprintf(p, _(" 最大%d体", " max %d"), 1+boost/100); break;
		case 11: sprintf(p, " %s%d", KWD_DAM, 100 + plev + boost); break;
		case 12: sprintf(p, " %s%dd15", KWD_DAM, 10 + plev / 2 + boost * 3 / 10); break;
		case 13: sprintf(p, _(" 行動:%d+d16回", " %d+d16 acts"), 16+boost/20); break;
		}
		break;
	}
	case MIND_MIRROR_MASTER:
	{
		switch (power)
		{
		case 0:  break;
		case 1:  break;
		case 2:  sprintf(p, " %s%dd4", KWD_DAM,  3 + ((plev - 1) / 5) ); break;
		case 3:  sprintf(p, " %s10", KWD_SPHERE); break;
		case 4:  break;
		case 5:  sprintf(p, " %s%d", KWD_SPHERE, plev *5); break;
		case 6:  sprintf(p, " %s20+d20", KWD_DURATION);  break;
		case 7:  break;
		case 8:  sprintf(p, " %s%dd8", KWD_DAM, 8+((plev -5)/4) ); break;
		case 9:  break;
		case 10: sprintf(p, " %s%dd8", KWD_DAM, 11+(plev-5)/4 ); break;
		case 11: break;
		case 12: sprintf(p, " %s20+d20", KWD_DURATION);  break;
		case 13: sprintf(p, " %s150+d%d", KWD_DAM, plev*2 ); break;
		case 14: break;
		case 15: break;
		case 16: sprintf(p, " %s%d", KWD_SPHERE, plev/2 +10); break;
		case 17: break;
		case 18: sprintf(p, " %s6+d6", KWD_DURATION);  break;
		case 19: sprintf(p, " %s%d", KWD_DAM, plev*11+5 ); break;
		case 20: sprintf(p, " %s4+d4", KWD_DURATION);  break;
		}
		break;
	}
	case MIND_NINJUTSU:
	{
		switch (power)
		{
		case 0:  break;
		case 1:  break;
		case 2:  sprintf(p, " %s10", KWD_SPHERE); break;
		case 3:  break;
		case 4:  sprintf(p, " %s%d", KWD_SPHERE , plev *5); break;
		case 5:  sprintf(p, " %s30", KWD_SPHERE); break;
		case 6:  break;
		case 7:  break;
		case 8:  sprintf(p, " %s20+d20", KWD_DURATION);  break;
		case 9:  sprintf(p, " %s%d", KWD_DAM, (50+plev)/2 ); break;
		case 10: break;
		case 11: break;
		case 12: break;
		case 13: break;
		case 14: break;
		case 15: break;
		case 16: sprintf(p, " %s%d+d%d", KWD_DURATION, plev/2, plev/2);  break;
		case 17: sprintf(p, " %s%d*3", KWD_DAM, (75+plev*2/3)/2 ); break;
		case 18: sprintf(p, " %s%dd10", KWD_DAM, 6+plev/8 ); break;
		case 19: sprintf(p, " %s6+d6", KWD_DURATION);  break;
		}
		break;
	}
	}
}

/*!
 * @brief 使用可能な特殊技能を選択する /
 * Allow user to choose a mindcrafter power.
 * @param sn 選択した特殊技能ID、キャンセルの場合-1、不正な選択の場合-2を返す
 * @param only_browse 一覧を見るだけの場合TRUEを返す
 * @return 発動可能な魔法を選択した場合TRUE、キャンセル処理か不正な選択が行われた場合FALSEを返す。
 * @details
 * If a valid spell is chosen, saves it in '*sn' and returns TRUE\n
 * If the user hits escape, returns FALSE, and set '*sn' to -1\n
 * If there are no legal choices, returns FALSE, and sets '*sn' to -2\n
 *\n
 * The "prompt" should be "cast", "recite", or "study"\n
 * The "known" should be TRUE for cast/pray, FALSE for study\n
 *\n
 * nb: This function has a (trivial) display bug which will be obvious\n
 * when you run it. It's probably easy to fix but I haven't tried,\n
 * sorry.\n
 */
static bool get_mind_power(player_type *caster_ptr, SPELL_IDX *sn, bool only_browse)
{
	SPELL_IDX i;
	int             num = 0;
	TERM_LEN y = 1;
	TERM_LEN x = 10;
	PERCENTAGE minfail = 0;
	PLAYER_LEVEL plev = caster_ptr->lev;
	PERCENTAGE chance = 0;
	int             ask = TRUE;
	char            choice;
	char            out_val[160];
	char            comment[80];
	concptr            p;
	COMMAND_CODE code;
	mind_type       spell;
	const mind_power      *mind_ptr;
	bool            flag, redraw;
	int             use_mind;
	int menu_line = (use_menu ? 1 : 0);

	switch (caster_ptr->pclass)
	{
	case CLASS_MINDCRAFTER:
	{
		use_mind = MIND_MINDCRAFTER;
		p = _("超能力", "mindcraft");
		break;
	}
	case CLASS_FORCETRAINER:
	{
		use_mind = MIND_KI;
		p = _("練気術", "Force");
		break;
	}
	case CLASS_BERSERKER:
	{
		use_mind = MIND_BERSERKER;
		p = _("技", "brutal power");
		break;
	}
	case CLASS_MIRROR_MASTER:
	{
		use_mind = MIND_MIRROR_MASTER;
		p = _("鏡魔法", "magic");
		break;
	}
	case CLASS_NINJA:
	{
		use_mind = MIND_NINJUTSU;
		p = _("忍術", "ninjutsu");
		break;
	}
	default:
	{
		use_mind = 0;
		p = _("超能力", "mindcraft");
		break;
	}
	}
	mind_ptr = &mind_powers[use_mind];

	/* Assume cancelled */
	*sn = (-1);

	/* Get the spell, if available */

	if (repeat_pull(&code))
	{
		*sn = (SPELL_IDX)code;
		/* Hack -- If requested INVEN_FORCE(1111), pull again */
		if (*sn == INVEN_FORCE) repeat_pull(&code);
		*sn = (SPELL_IDX)code;

		/* Verify the spell */
		if (mind_ptr->info[*sn].min_lev <= plev)
		{
			/* Success */
			return TRUE;
		}
	}

	flag = FALSE;
	redraw = FALSE;

	for (i = 0; i < MAX_MIND_POWERS; i++)
	{
		if (mind_ptr->info[i].min_lev <= plev)
		{
			num++;
		}
	}

	/* Build a prompt (accept all spells) */
	if (only_browse)
	{
		(void)strnfmt(out_val, 78,
			_("(%^s %c-%c, '*'で一覧, ESC) どの%sについて知りますか？", "(%^ss %c-%c, *=List, ESC=exit) Use which %s? "),
			p, I2A(0), I2A(num - 1), p);
	}
	else
	{
		(void)strnfmt(out_val, 78,
			_("(%^s %c-%c, '*'で一覧, ESC) どの%sを使いますか？", "(%^ss %c-%c, *=List, ESC=exit) Use which %s? "),
			p, I2A(0), I2A(num - 1), p);
	}

	if (use_menu && !only_browse) screen_save();

	choice = (always_show_list || use_menu) ? ESCAPE : 1;

	while (!flag)
	{
		if(choice==ESCAPE) choice = ' '; 
		else if( !get_com(out_val, &choice, TRUE) )break;

		if (use_menu && choice != ' ')
		{
			switch(choice)
			{
				case '0':
				{
					if (!only_browse) screen_load();
					return FALSE;
				}

				case '8':
				case 'k':
				case 'K':
				{
					menu_line += (num - 1);
					break;
				}

				case '2':
				case 'j':
				case 'J':
				{
					menu_line++;
					break;
				}

				case 'x':
				case 'X':
				case '\r':
				case '\n':
				{
					i = menu_line - 1;
					ask = FALSE;
					break;
				}
			}
			if (menu_line > num) menu_line -= num;
		}
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?') || (use_menu && ask))
		{
			/* Show the list */
			if (!redraw || use_menu)
			{
				char psi_desc[80];
				bool has_weapon[2];
				redraw = TRUE;
				if (!only_browse && !use_menu) screen_save();

				/* Display a list of spells */
				prt("", y, x);
				put_str(_("名前", "Name"), y, x + 5);

				put_str(format(_("Lv   %s   失率 効果", "Lv   %s   Fail Info"),
					((use_mind == MIND_BERSERKER) || (use_mind == MIND_NINJUTSU)) ? "HP" : "MP"), y, x + 35);

				has_weapon[0] = has_melee_weapon(caster_ptr, INVEN_RARM);
				has_weapon[1] = has_melee_weapon(caster_ptr, INVEN_LARM);

				/* Dump the spells */
				for (i = 0; i < MAX_MIND_POWERS; i++)
				{
					int mana_cost;

					/* Access the spell */
					spell = mind_ptr->info[i];

					if (spell.min_lev > plev)   break;

					chance = spell.fail;

					mana_cost = spell.mana_cost;
					if (chance)
					{

						/* Reduce failure rate by "effective" level adjustment */
						chance -= 3 * (plev - spell.min_lev);

						/* Reduce failure rate by INT/WIS adjustment */
						chance -= 3 * (adj_mag_stat[caster_ptr->stat_ind[mp_ptr->spell_stat]] - 1);

						if (use_mind == MIND_KI)
						{
							if (heavy_armor(caster_ptr)) chance += 20;
							if (caster_ptr->icky_wield[0]) chance += 20;
							else if (has_weapon[0]) chance += 10;
							if (caster_ptr->icky_wield[1]) chance += 20;
							else if (has_weapon[1]) chance += 10;
							if (i == 5)
							{
								int j;
								for (j = 0; j < get_current_ki(caster_ptr) / 50; j++)
									mana_cost += (j+1) * 3 / 2;
							}
						}

						/* Not enough mana to cast */
						if ((use_mind != MIND_BERSERKER) && (use_mind != MIND_NINJUTSU) && (mana_cost > caster_ptr->csp))
						{
							chance += 5 * (mana_cost - caster_ptr->csp);
						}

						chance += caster_ptr->to_m_chance;

						/* Extract the minimum failure rate */
						minfail = adj_mag_fail[caster_ptr->stat_ind[mp_ptr->spell_stat]];

						/* Minimum failure rate */
						if (chance < minfail) chance = minfail;

						/* Stunning makes spells harder */
						if (caster_ptr->stun > 50) chance += 25;
						else if (caster_ptr->stun) chance += 15;

						if (use_mind == MIND_KI)
						{
							if (heavy_armor(caster_ptr)) chance += 5;
							if (caster_ptr->icky_wield[0]) chance += 5;
							if (caster_ptr->icky_wield[1]) chance += 5;
						}
						/* Always a 5 percent chance of working */
						if (chance > 95) chance = 95;
					}

					/* Get info */
					mindcraft_info(caster_ptr, comment, use_mind, i);

					if (use_menu)
					{
						if (i == (menu_line-1)) strcpy(psi_desc, _("  》 ", "  >  "));
						else strcpy(psi_desc, "     ");
					}
					else
						sprintf(psi_desc, "  %c) ", I2A(i));
					/* Dump the spell --(-- */
					strcat(psi_desc,
					       format("%-30s%2d %4d%s %3d%%%s",
						      spell.name, spell.min_lev, mana_cost,
						      (((use_mind == MIND_MINDCRAFTER) && (i == 13)) ? _("～", "~ ") : "  "),
						      chance, comment));
					prt(psi_desc, y + i + 1, x);
				}

				/* Clear the bottom line */
				prt("", y + i + 1, x);
			}

			/* Hide the list */
			else if (!only_browse)
			{
				/* Hide list */
				redraw = FALSE;
				screen_load();
			}

			/* Redo asking */
			continue;
		}

		if (!use_menu)
		{
			/* Note verify */
			ask = isupper(choice);

			/* Lowercase */
			if (ask) choice = (char)tolower(choice);

			/* Extract request */
			i = (islower(choice) ? A2I(choice) : -1);
		}

		/* Totally Illegal */
		if ((i < 0) || (i >= num))
		{
			bell();
			continue;
		}

		/* Save the spell index */
		spell = mind_ptr->info[i];

		/* Verify it */
		if (ask)
		{
			char tmp_val[160];

			/* Prompt */
			(void) strnfmt(tmp_val, 78, _("%sを使いますか？", "Use %s? "), spell.name);

			/* Belay that order */
			if (!get_check(tmp_val)) continue;
		}

		/* Stop the loop */
		flag = TRUE;
	}
	if (redraw && !only_browse) screen_load();

	caster_ptr->window |= (PW_SPELL);
	handle_stuff(caster_ptr);

	/* Abort if needed */
	if (!flag) return FALSE;

	/* Save the choice */
	(*sn) = i;

	repeat_push((COMMAND_CODE)i);

	/* Success */
	return TRUE;
}

/*!
 * @brief 超能力の発動 /
 * do_cmd_cast calls this function if the player's class is 'mindcrafter'.
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
static bool cast_mindcrafter_spell(player_type *caster_ptr, int spell)
{
	int b = 0;
	DIRECTION dir;
	TIME_EFFECT t;
	PLAYER_LEVEL plev = caster_ptr->lev;

	/* spell code */
	switch (spell)
	{
	case 0:   /* Precog */
		if (plev > 44)
		{
			chg_virtue(caster_ptr, V_KNOWLEDGE, 1);
			chg_virtue(caster_ptr, V_ENLIGHTEN, 1);
			wiz_lite(caster_ptr, FALSE);
		}
		else if (plev > 19)
			map_area(caster_ptr, DETECT_RAD_MAP);

		if (plev < 30)
		{
			b = detect_monsters_normal(caster_ptr, DETECT_RAD_DEFAULT);
			if (plev > 14) b |= detect_monsters_invis(caster_ptr, DETECT_RAD_DEFAULT);
			if (plev > 4)  {
				b |= detect_traps(caster_ptr, DETECT_RAD_DEFAULT, TRUE);
				b |= detect_doors(caster_ptr, DETECT_RAD_DEFAULT);
			}
		}
		else
		{
			b = detect_all(caster_ptr, DETECT_RAD_DEFAULT);
		}

		if ((plev > 24) && (plev < 40))
			set_tim_esp(caster_ptr, (TIME_EFFECT)plev, FALSE);

		if (!b) msg_print(_("安全な気がする。", "You feel safe."));

		break;
	case 1:
		/* Mindblast */
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

		if (randint1(100) < plev * 2)
			fire_beam(caster_ptr, GF_PSI, dir, damroll(3 + ((plev - 1) / 4), (3 + plev / 15)));
		else
			fire_ball(caster_ptr, GF_PSI, dir, damroll(3 + ((plev - 1) / 4), (3 + plev / 15)), 0);
		break;
	case 2:
		/* Minor displace */
		teleport_player(caster_ptr, 10, TELEPORT_SPONTANEOUS);
		break;
	case 3:
		/* Major displace */
		teleport_player(caster_ptr, plev * 5, TELEPORT_SPONTANEOUS);
		break;
	case 4:
		/* Domination */
		if (plev < 30)
		{
			if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

			fire_ball(caster_ptr, GF_DOMINATION, dir, plev, 0);
		}
		else
		{
			charm_monsters(caster_ptr, plev * 2);
		}
		break;
	case 5:
		/* Fist of Force  ---  not 'true' TK  */
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

		fire_ball(caster_ptr, GF_TELEKINESIS, dir, damroll(8 + ((plev - 5) / 4), 8),
			(plev > 20 ? (plev - 20) / 8 + 1 : 0));
		break;
	case 6:
		/* Character Armour */
		set_shield(caster_ptr, (TIME_EFFECT)plev, FALSE);
		if (plev > 14) set_oppose_acid(caster_ptr, (TIME_EFFECT)plev, FALSE);
		if (plev > 19) set_oppose_fire(caster_ptr, (TIME_EFFECT)plev, FALSE);
		if (plev > 24) set_oppose_cold(caster_ptr, (TIME_EFFECT)plev, FALSE);
		if (plev > 29) set_oppose_elec(caster_ptr, (TIME_EFFECT)plev, FALSE);
		if (plev > 34) set_oppose_pois(caster_ptr, (TIME_EFFECT)plev, FALSE);
		break;
	case 7:
		/* Psychometry */
		if (plev < 25)
			return psychometry(caster_ptr);
		else
			return ident_spell(caster_ptr, FALSE, 0);
	case 8:
		/* Mindwave */
		msg_print(_("精神を捻じ曲げる波動を発生させた！", "Mind-warping forces emanate from your brain!"));

		if (plev < 25)
			project(caster_ptr, 0, 2 + plev / 10, caster_ptr->y, caster_ptr->x,
			(plev * 3), GF_PSI, PROJECT_KILL, -1);
		else
			(void)mindblast_monsters(caster_ptr, randint1(plev * ((plev - 5) / 10 + 1)));
		break;
	case 9:
		/* Adrenaline */
		set_afraid(caster_ptr, 0);
		set_stun(caster_ptr, 0);

		/*
		 * Only heal when Adrenalin Channeling is not active. We check
		 * that by checking if the player isn't fast and 'heroed' atm.
		 */
		if (!is_fast(caster_ptr) || !is_hero(caster_ptr))
		{
			hp_player(caster_ptr, plev);
		}

		t = 10 + randint1((plev * 3) / 2);
		set_hero(caster_ptr, t, FALSE);
		/* Haste */
		(void)set_fast(caster_ptr, t, FALSE);
		break;
	case 10:
		/* Telekinesis */
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

		fetch_item(caster_ptr, dir, plev * 15, FALSE);

		break;
	case 11:
		/* Psychic Drain */
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

		b = damroll(plev / 2, 6);

		/* This is always a radius-0 ball now */
		if (fire_ball(caster_ptr, GF_PSI_DRAIN, dir, b, 0))
			caster_ptr->energy_need += randint1(150);
		break;
	case 12:
		/* psycho-spear */
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

		fire_beam(caster_ptr, GF_PSY_SPEAR, dir, randint1(plev*3)+plev*3);
		break;
	case 13:
	{
		time_walk(caster_ptr);
		break;
	}
	default:
		msg_print(_("なに？", "Zap?"));
	}

	return TRUE;
}

/*!
 * @brief 練気術の発動 /
 * do_cmd_cast calls this function if the player's class is 'ForceTrainer'.
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
static bool cast_force_spell(player_type *caster_ptr, int spell)
{
	DIRECTION dir;
	PLAYER_LEVEL plev = caster_ptr->lev;
	int boost = get_current_ki(caster_ptr);

	if (heavy_armor(caster_ptr)) boost /= 2;

	/* spell code */
	switch (spell)
	{
	case 0:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;
		fire_ball(caster_ptr, GF_MISSILE, dir, damroll(3 + ((plev - 1) / 5) + boost / 12, 4), 0);
		break;
	case 1:
		(void)lite_area(caster_ptr, damroll(2, (plev / 2)), (plev / 10) + 1);
		break;
	case 2:
		set_tim_levitation(caster_ptr, randint1(30) + 30 + boost / 5, FALSE);
		break;
	case 3:
		project_length = plev / 8 + 3;
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

		fire_beam(caster_ptr, GF_MISSILE, dir, damroll(5 + ((plev - 1) / 5) + boost / 10, 5));
		break;
	case 4:
		set_resist_magic(caster_ptr, randint1(20) + 20 + boost / 5, FALSE);
		break;
	case 5:
		msg_print(_("気を練った。", "You improved the Force."));
		set_current_ki(caster_ptr, FALSE, 70 + plev);
		caster_ptr->update |= (PU_BONUS);
		if (randint1(get_current_ki(caster_ptr)) > (plev * 4 + 120))
		{
			msg_print(_("気が暴走した！", "The Force exploded!"));
			fire_ball(caster_ptr, GF_MANA, 0, get_current_ki(caster_ptr) / 2, 10);
			take_hit(caster_ptr, DAMAGE_LOSELIFE, caster_ptr->magic_num1[0] / 2, _("気の暴走", "Explosion of the Force"), -1);
		}
		else return TRUE;
		break;
	case 6:
		set_tim_sh_force(caster_ptr, randint1(plev / 2) + 15 + boost / 7, FALSE);
		break;
	case 7:
		return shock_power(caster_ptr);
		break;
	case 8:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;
		fire_ball(caster_ptr, GF_MISSILE, dir, damroll(10, 6) + plev * 3 / 2 + boost * 3 / 5, (plev < 30) ? 2 : 3);
		break;
	case 9:
	{
		MONSTER_IDX m_idx;

		if (!target_set(caster_ptr, TARGET_KILL)) return FALSE;
		m_idx = caster_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx;
		if (!m_idx) break;
		if (!player_has_los_bold(caster_ptr, target_row, target_col)) break;
		if (!projectable(caster_ptr, caster_ptr->y, caster_ptr->x, target_row, target_col)) break;
		dispel_monster_status(caster_ptr, m_idx);
		break;
	}
	case 10:
	{
		int i;
		bool success = FALSE;

		for (i = 0; i < 1 + boost/100; i++)
			if (summon_specific(caster_ptr, -1, caster_ptr->y, caster_ptr->x, plev, SUMMON_PHANTOM, PM_FORCE_PET))
				success = TRUE;
		if (success)
		{
			msg_print(_("御用でございますが、御主人様？", "'Your wish, master?'"));
		}
		else
		{
			msg_print(_("何も現れなかった。", "Nothing happen."));
		}
		break;
	}
	case 11:
		fire_ball(caster_ptr, GF_FIRE, 0, 200 + (2 * plev) + boost * 2, 10);
		break;
	case 12:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;

		fire_beam(caster_ptr, GF_MANA, dir, damroll(10 + (plev / 2) + boost * 3 / 10, 15));
		break;
	case 13:
		set_lightspeed(caster_ptr, randint1(16) + 16 + boost / 20, FALSE);
		break;
	default:
		msg_print(_("なに？", "Zap?"));
	}

	set_current_ki(caster_ptr, TRUE, 0);
	caster_ptr->update |= (PU_BONUS);

	return TRUE;
}


/*!
 * @brief 現在フロアに存在している鏡の数を数える / calculate mirrors
 * @return 鏡の枚数
 */
static int number_of_mirrors(floor_type *floor_ptr)
{
	int val = 0;
	for (POSITION x = 0; x < floor_ptr->width; x++) {
		for (POSITION y = 0; y < floor_ptr->height; y++) {
			if (is_mirror_grid(&floor_ptr->grid_array[y][x])) val++;
		}
	}

	return val;
}


/*!
 * @brief 鏡魔法の発動 /
 * do_cmd_cast calls this function if the player's class is 'Mirror magic'.
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
static bool cast_mirror_spell(player_type *caster_ptr, int spell)
{
	DIRECTION dir;
	PLAYER_LEVEL plev = caster_ptr->lev;
	int tmp;
	TIME_EFFECT t;
	POSITION x, y;

	/* spell code */
	switch (spell)
	{
		/* mirror of seeing */
	case 0:
		tmp = is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x]) ? 4 : 0;
		if (plev + tmp > 4)detect_monsters_normal(caster_ptr, DETECT_RAD_DEFAULT);
		if (plev + tmp > 18)detect_monsters_invis(caster_ptr, DETECT_RAD_DEFAULT);
		if (plev + tmp > 28)set_tim_esp(caster_ptr, (TIME_EFFECT)plev, FALSE);
		if (plev + tmp > 38)map_area(caster_ptr, DETECT_RAD_MAP);
		if (tmp == 0 && plev < 5) {
			msg_print(_("鏡がなくて集中できなかった！", "You need a mirror to concentrate!"));
		}
		break;
		/* drip of light */
	case 1:
		if (number_of_mirrors(caster_ptr->current_floor_ptr) < 4 + plev / 10) {
			place_mirror(caster_ptr);
		}
		else {
			msg_format(_("これ以上鏡は制御できない！", "There are too many mirrors to control!"));
		}
		break;
	case 2:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;
		if (plev > 9 && is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x])) {
			fire_beam(caster_ptr, GF_LITE, dir, damroll(3 + ((plev - 1) / 5), 4));
		}
		else {
			fire_bolt(caster_ptr, GF_LITE, dir, damroll(3 + ((plev - 1) / 5), 4));
		}
		break;
		/* warped mirror */
	case 3:
		teleport_player(caster_ptr, 10, TELEPORT_SPONTANEOUS);
		break;
		/* mirror of light */
	case 4:
		(void)lite_area(caster_ptr, damroll(2, (plev / 2)), (plev / 10) + 1);
		break;
		/* mirror of wandering */
	case 5:
		teleport_player(caster_ptr, plev * 5, TELEPORT_SPONTANEOUS);
		break;
		/* robe of dust */
	case 6:
		set_dustrobe(caster_ptr, 20 + randint1(20), FALSE);
		break;
		/* banishing mirror */
	case 7:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;
		(void)fire_beam(caster_ptr, GF_AWAY_ALL, dir, plev);
		break;
		/* mirror clashing */
	case 8:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;
		fire_ball(caster_ptr, GF_SHARDS, dir, damroll(8 + ((plev - 5) / 4), 8),
			(plev > 20 ? (plev - 20) / 8 + 1 : 0));
		break;
		/* mirror sleeping */
	case 9:
		for (x = 0; x < caster_ptr->current_floor_ptr->width; x++) {
			for (y = 0; y < caster_ptr->current_floor_ptr->height; y++) {
				if (is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[y][x])) {
					project(caster_ptr, 0, 2, y, x, (HIT_POINT)plev, GF_OLD_SLEEP, (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI), -1);
				}
			}
		}
		break;
		/* seeker ray */
	case 10:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;
		fire_beam(caster_ptr, GF_SEEKER, dir, damroll(11 + (plev - 5) / 4, 8));
		break;
		/* seal of mirror */
	case 11:
		seal_of_mirror(caster_ptr, plev * 4 + 100);
		break;
		/* shield of water */
	case 12:
		t = 20 + randint1(20);
		set_shield(caster_ptr, t, FALSE);
		if (plev > 31)set_tim_reflect(caster_ptr, t, FALSE);
		if (plev > 39)set_resist_magic(caster_ptr, t, FALSE);
		break;
		/* super ray */
	case 13:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;
		fire_beam(caster_ptr, GF_SUPER_RAY, dir, 150 + randint1(2 * plev));
		break;
		/* illusion light */
	case 14:
		tmp = is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x]) ? 4 : 3;
		slow_monsters(caster_ptr, plev);
		stun_monsters(caster_ptr, plev*tmp);
		confuse_monsters(caster_ptr, plev*tmp);
		turn_monsters(caster_ptr, plev*tmp);
		stun_monsters(caster_ptr, plev*tmp);
		stasis_monsters(caster_ptr, plev*tmp);
		break;
		/* mirror shift */
	case 15:
		if (!is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x])) {
			msg_print(_("鏡の国の場所がわからない！", "You cannot find out where the mirror is!"));
			break;
		}
		reserve_alter_reality(caster_ptr);
		break;
		/* mirror tunnel */
	case 16:
		msg_print(_("鏡の世界を通り抜け…  ", "You try to enter the mirror..."));
		return mirror_tunnel(caster_ptr);

		/* mirror of recall */
	case 17:
		return recall_player(caster_ptr, randint0(21) + 15);
		/* multi-shadow */
	case 18:
		set_multishadow(caster_ptr, 6 + randint1(6), FALSE);
		break;
		/* binding field */
	case 19:
		if (!binding_field(caster_ptr, plev * 11 + 5))msg_print(_("適当な鏡を選べなかった！", "You were not able to choose suitable mirrors!"));
		break;
		/* mirror of Ruffnor */
	case 20:
		(void)set_invuln(caster_ptr, randint1(4) + 4, FALSE);
		break;
	default:
		msg_print(_("なに？", "Zap?"));

	}
	caster_ptr->magic_num1[0] = 0;

	return TRUE;
}

/*!
 * @brief 怒りの発動 /
 * do_cmd_cast calls this function if the player's class is 'berserker'.
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
static bool cast_berserk_spell(player_type *caster_ptr, int spell)
{
	POSITION y, x;
	DIRECTION dir;

	/* spell code */
	switch (spell)
	{
	case 0:
		detect_monsters_mind(caster_ptr, DETECT_RAD_DEFAULT);
		break;
	case 1:
	{
		if (caster_ptr->riding)
		{
			msg_print(_("乗馬中には無理だ。", "You cannot do it when riding."));
			return FALSE;
		}

		if (!get_direction(caster_ptr, &dir, FALSE, FALSE)) return FALSE;

		if (dir == 5) return FALSE;
		y = caster_ptr->y + ddy[dir];
		x = caster_ptr->x + ddx[dir];

		if (!caster_ptr->current_floor_ptr->grid_array[y][x].m_idx)
		{
			msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
			return FALSE;
		}

		do_cmd_attack(caster_ptr, y, x, 0);

		if (!player_can_enter(caster_ptr, caster_ptr->current_floor_ptr->grid_array[y][x].feat, 0) || is_trap(caster_ptr, caster_ptr->current_floor_ptr->grid_array[y][x].feat))
			break;

		y += ddy[dir];
		x += ddx[dir];

		if (player_can_enter(caster_ptr, caster_ptr->current_floor_ptr->grid_array[y][x].feat, 0) && !is_trap(caster_ptr, caster_ptr->current_floor_ptr->grid_array[y][x].feat) && !caster_ptr->current_floor_ptr->grid_array[y][x].m_idx)
		{
			msg_print(NULL);
			(void)move_player_effect(caster_ptr, y, x, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP);
		}
		break;
	}
	case 2:
	{
		if (!get_direction(caster_ptr, &dir, FALSE, FALSE)) return FALSE;
		y = caster_ptr->y + ddy[dir];
		x = caster_ptr->x + ddx[dir];
		exe_movement(caster_ptr, dir, easy_disarm, TRUE);
		break;
	}
	case 3:
		earthquake(caster_ptr, caster_ptr->y, caster_ptr->x, 8+randint0(5), 0);
		break;
	case 4:
		massacre(caster_ptr);
		break;
	default:
		msg_print(_("なに？", "Zap?"));

	}
	return TRUE;
}

/*!
 * @brief 忍術の発動 /
 * do_cmd_cast calls this function if the player's class is 'ninja'.
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
static bool cast_ninja_spell(player_type *caster_ptr, int spell)
{
	POSITION x = 0, y = 0;
	DIRECTION dir;
	PLAYER_LEVEL plev = caster_ptr->lev;

	switch (spell)
	{
	case 0:
		(void)unlite_area(caster_ptr, 0, 3);
		break;
	case 1:
		if (plev > 44)
		{
			wiz_lite(caster_ptr, TRUE);
		}
		detect_monsters_normal(caster_ptr, DETECT_RAD_DEFAULT);
		if (plev > 4)
		{
			detect_traps(caster_ptr, DETECT_RAD_DEFAULT, TRUE);
			detect_doors(caster_ptr, DETECT_RAD_DEFAULT);
			detect_stairs(caster_ptr, DETECT_RAD_DEFAULT);
		}
		if (plev > 14)
		{
			detect_objects_normal(caster_ptr, DETECT_RAD_DEFAULT);
		}
		break;
	case 2:
	{
		teleport_player(caster_ptr, 10, TELEPORT_SPONTANEOUS);
		break;
	}
	case 3:
	{
		if (!(caster_ptr->special_defense & NINJA_KAWARIMI))
		{
			msg_print(_("敵の攻撃に対して敏感になった。", "You are now prepared to evade any attacks."));
			caster_ptr->special_defense |= NINJA_KAWARIMI;
			caster_ptr->redraw |= (PR_STATUS);
		}
		break;
	}
	case 4:
	{
		teleport_player(caster_ptr, caster_ptr->lev * 5, TELEPORT_SPONTANEOUS);
		break;
	}
	case 5:
	{
		if(!hit_and_away(caster_ptr)) return FALSE;
		break;
	}
	case 6:
	{
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;
		(void)stasis_monster(caster_ptr, dir);
		break;
	}
	case 7:
		return ident_spell(caster_ptr, FALSE, 0);
	case 8:
		set_tim_levitation(caster_ptr, randint1(20) + 20, FALSE);
		break;
	case 9:
		fire_ball(caster_ptr, GF_FIRE, 0, 50+plev, plev/10+2);
		teleport_player(caster_ptr, 30, TELEPORT_SPONTANEOUS);
		set_oppose_fire(caster_ptr, (TIME_EFFECT)plev, FALSE);
		break;
	case 10:
		return rush_attack(caster_ptr, NULL);
	case 11:
	{
		int i;
		for (i = 0; i < 8; i++)
		{
			OBJECT_IDX slot;

			for (slot = 0; slot < INVEN_PACK; slot++)
			{
				if (caster_ptr->inventory_list[slot].tval == TV_SPIKE) break;
			}
			if (slot == INVEN_PACK)
			{
				if (!i) msg_print(_("くさびを持っていない。", "You have no Iron Spikes."));
				else msg_print(_("くさびがなくなった。", "You have no more Iron Spikes."));
				return FALSE;
			}

			/* Gives a multiplier of 2 at first, up to 3 at 40th */
			do_cmd_throw(caster_ptr, 1, FALSE, slot);

			take_turn(caster_ptr, 100);
		}
		break;
	}
	case 12:
		(void)fetch_monster(caster_ptr);
		break;
	case 13:
		if (!get_aim_dir(caster_ptr, &dir)) return FALSE;
		fire_ball(caster_ptr, GF_OLD_CONF, dir, plev*3, 3);
		break;
	case 14:
		project_length = -1;
		if (!get_aim_dir(caster_ptr, &dir))
		{
			project_length = 0;
			return FALSE;
		}
		project_length = 0;

		(void)teleport_swap(caster_ptr, dir);
		break;
	case 15:
		explosive_rune(caster_ptr, caster_ptr->y, caster_ptr->x);
		break;
	case 16:
		(void)set_pass_wall(caster_ptr, randint1(plev/2) + plev/2, FALSE);
		set_oppose_acid(caster_ptr, (TIME_EFFECT)plev, FALSE);
		break;
	case 17:
		fire_ball(caster_ptr, GF_POIS, 0, 75+plev*2/3, plev/5+2);
		fire_ball(caster_ptr, GF_HYPODYNAMIA, 0, 75+plev*2/3, plev/5+2);
		fire_ball(caster_ptr, GF_CONFUSION, 0, 75+plev*2/3, plev/5+2);
		teleport_player(caster_ptr, 30, TELEPORT_SPONTANEOUS);
		break;
	case 18:
	{
		int k;
		int num = damroll(3, 9);

		for (k = 0; k < num; k++)
		{
			EFFECT_ID typ = one_in_(2) ? GF_FIRE : one_in_(3) ? GF_NETHER : GF_PLASMA;
			int attempts = 1000;

			while (attempts--)
			{
				scatter(caster_ptr, &y, &x, caster_ptr->y, caster_ptr->x, 4, 0);

				if (!player_bold(caster_ptr, y, x)) break;
			}
			project(caster_ptr, 0, 0, y, x, damroll(6 + plev / 8, 10), typ,
				(PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_KILL), -1);
		}
		break;
	}
	case 19:
		set_multishadow(caster_ptr, 6+randint1(6), FALSE);
		break;
	default:
		msg_print(_("なに？", "Zap?"));

	}
	return TRUE;
}

/*!
 * @brief 特殊技能コマンドのメインルーチン /
 * @return なし
 */
void do_cmd_mind(player_type *caster_ptr)
{
	SPELL_IDX n = 0;
	int b = 0;
	PERCENTAGE chance;
	PERCENTAGE minfail = 0;
	PLAYER_LEVEL plev = caster_ptr->lev;
	int             old_csp = caster_ptr->csp;
	mind_type       spell;
	bool            cast;
	int             use_mind, mana_cost;
	concptr            p;
	bool		on_mirror = FALSE;

	if (cmd_limit_confused(caster_ptr)) return;
	if (!get_mind_power(caster_ptr, &n, FALSE)) return;

#ifdef JP
	switch(caster_ptr->pclass)
	{
		case CLASS_MINDCRAFTER: use_mind = MIND_MINDCRAFTER; p = "精神";break;
		case CLASS_FORCETRAINER:          use_mind = MIND_KI; p = "気";break;
		case CLASS_BERSERKER:   use_mind = MIND_BERSERKER; p = "怒り";break;
		case CLASS_MIRROR_MASTER:   use_mind = MIND_MIRROR_MASTER; p = "鏡魔法";break;
		case CLASS_NINJA:       use_mind = MIND_NINJUTSU; p = "精神";break;
		default:                use_mind = 0 ;p = "超能力"; break;
	}
#else
	switch(caster_ptr->pclass)
	{
		case CLASS_MINDCRAFTER: use_mind = MIND_MINDCRAFTER; break;
		case CLASS_FORCETRAINER: use_mind = MIND_KI; break;
		case CLASS_BERSERKER:   use_mind = MIND_BERSERKER; break;
		case CLASS_MIRROR_MASTER:   use_mind = MIND_MIRROR_MASTER; break;
		case CLASS_NINJA:       use_mind = MIND_NINJUTSU; break;
		default:                use_mind = 0; break;
	}
	p = "skill";
#endif
	spell = mind_powers[use_mind].info[n];

	/* Spell failure chance */
	chance = spell.fail;

	mana_cost = spell.mana_cost;
	if (use_mind == MIND_KI)
	{
		if (heavy_armor(caster_ptr)) chance += 20;
		if (caster_ptr->icky_wield[0]) chance += 20;
		else if (has_melee_weapon(caster_ptr, INVEN_RARM)) chance += 10;
		if (caster_ptr->icky_wield[1]) chance += 20;
		else if (has_melee_weapon(caster_ptr, INVEN_LARM)) chance += 10;
		if (n == 5)
		{
			int j;
			for (j = 0; j < get_current_ki(caster_ptr) / 50; j++)
				mana_cost += (j+1) * 3 / 2;
		}
	}

	/* Verify "dangerous" spells */
	if ((use_mind == MIND_BERSERKER) || (use_mind == MIND_NINJUTSU))
	{
		if (mana_cost > caster_ptr->chp)
		{
			msg_print(_("ＨＰが足りません。", "You do not have enough hp to use this power."));
			return;
		}
	}
	else if (mana_cost > caster_ptr->csp)
	{
		/* Warning */
		msg_print(_("ＭＰが足りません。", "You do not have enough mana to use this power."));

		if (!over_exert) return;

		/* Verify */
		if (!get_check(_("それでも挑戦しますか? ", "Attempt it anyway? "))) return;

	}

	if (chance)
	{
		/* Reduce failure rate by "effective" level adjustment */
		chance -= 3 * (plev - spell.min_lev);

		chance += caster_ptr->to_m_chance;

		/* Reduce failure rate by INT/WIS adjustment */
		chance -= 3 * (adj_mag_stat[caster_ptr->stat_ind[mp_ptr->spell_stat]] - 1);

		/* Not enough mana to cast */
		if ((mana_cost > caster_ptr->csp) && (use_mind != MIND_BERSERKER) && (use_mind != MIND_NINJUTSU))
		{
			chance += 5 * (mana_cost - caster_ptr->csp);
		}

		/* Extract the minimum failure rate */
		minfail = adj_mag_fail[caster_ptr->stat_ind[mp_ptr->spell_stat]];

		/* Minimum failure rate */
		if (chance < minfail) chance = minfail;

		/* Stunning makes spells harder */
		if (caster_ptr->stun > 50) chance += 25;
		else if (caster_ptr->stun) chance += 15;

		if (use_mind == MIND_KI)
		{
			if (heavy_armor(caster_ptr)) chance += 5;
			if (caster_ptr->icky_wield[0]) chance += 5;
			if (caster_ptr->icky_wield[1]) chance += 5;
		}
	}

	/* Always a 5 percent chance of working */
	if (chance > 95) chance = 95;

	/* Failed spell */
	if (randint0(100) < chance)
	{
		if (flush_failure) flush();
		msg_format(_("%sの集中に失敗した！", "You failed to concentrate hard enough for %s!"), p);

		sound(SOUND_FAIL);

		if ((use_mind != MIND_BERSERKER) && (use_mind != MIND_NINJUTSU))
		{
			if ((use_mind == MIND_KI) && (n != 5) && get_current_ki(caster_ptr))
			{
				msg_print(_("気が散ってしまった．．．", "Your improved Force has gone away..."));
				set_current_ki(caster_ptr, TRUE, 0);
			}

			if (randint1(100) < (chance / 2))
			{
				/* Backfire */
			  b = randint1(100);

			  if( use_mind == MIND_MINDCRAFTER ){
				if (b < 5)
				{
					msg_print(_("なんてこった！頭の中が真っ白になった！", "Oh, no! Your mind has gone blank!"));
					lose_all_info(caster_ptr);
				}
				else if (b < 15)
				{
					msg_print(_("奇妙な光景が目の前で踊っている...", "Weird visions seem to dance before your eyes..."));
					set_image(caster_ptr, caster_ptr->image + 5 + randint1(10));
				}
				else if (b < 45)
				{
					msg_print(_("あなたの頭は混乱した！", "Your brain is addled!"));
					set_confused(caster_ptr, caster_ptr->confused + randint1(8));
				}
				else if (b < 90)
				{
					set_stun(caster_ptr, caster_ptr->stun + randint1(8));
				}
				else
				{
					/* Mana storm */
					msg_format(_("%sの力が制御できない氾流となって解放された！", "Your mind unleashes its power in an uncontrollable storm!"), p);

					project(caster_ptr, PROJECT_WHO_UNCTRL_POWER, 2 + plev / 10, caster_ptr->y, caster_ptr->x, plev * 2,
						GF_MANA, PROJECT_JUMP | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM, -1);
					caster_ptr->csp = MAX(0, caster_ptr->csp - plev * MAX(1, plev / 10));
				}
			  }
			  if( use_mind == MIND_MIRROR_MASTER ){
				if (b < 51)
				{
				  /* Nothing has happen */
				}
				else if (b < 81)
				{
					msg_print(_("鏡の世界の干渉を受けた！", "Weird visions seem to dance before your eyes..."));
					teleport_player(caster_ptr, 10, TELEPORT_PASSIVE);
				}
				else if (b < 96)
				{
					msg_print(_("まわりのものがキラキラ輝いている！", "Your brain is addled!"));
					set_image(caster_ptr, caster_ptr->image + 5 + randint1(10));
				}
				else
				{
					/* Mana storm */
					msg_format(_("%sの力が制御できない氾流となって解放された！", "Your mind unleashes its power in an uncontrollable storm!"), p);

					project(caster_ptr, PROJECT_WHO_UNCTRL_POWER, 2 + plev / 10, caster_ptr->y, caster_ptr->x, plev * 2,
						GF_MANA, PROJECT_JUMP | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM, -1);
					caster_ptr->csp = MAX(0, caster_ptr->csp - plev * MAX(1, plev / 10));
				}
			  }
			}
		}
	}
	else
	{
		sound(SOUND_ZAP);

		switch(use_mind)
		{
		case MIND_MINDCRAFTER:
			
			cast = cast_mindcrafter_spell(caster_ptr, n);
			break;
		case MIND_KI:
			
			cast = cast_force_spell(caster_ptr, n);
			break;
		case MIND_BERSERKER:
			
			cast = cast_berserk_spell(caster_ptr, n);
			break;
		case MIND_MIRROR_MASTER:
			
			if(is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x]) )on_mirror = TRUE;
			cast = cast_mirror_spell(caster_ptr, n);
			break;
		case MIND_NINJUTSU:
			
			cast = cast_ninja_spell(caster_ptr, n);
			break;
		default:
			msg_format(_("謎の能力:%d, %d", "Mystery power:%d, %d"),use_mind, n);
			return;
		}

		if (!cast) return;
	}


	/* teleport from mirror costs small energy */
	if(on_mirror && caster_ptr->pclass == CLASS_MIRROR_MASTER)
	{
	  if( n==3 || n==5 || n==7 || n==16 ) take_turn(caster_ptr, 50);
	}
	else
	{
		take_turn(caster_ptr, 100);
	}

	if ((use_mind == MIND_BERSERKER) || (use_mind == MIND_NINJUTSU))
	{
		take_hit(caster_ptr, DAMAGE_USELIFE, mana_cost, _("過度の集中", "concentrating too hard"), -1);
		/* Redraw hp */
		caster_ptr->redraw |= (PR_HP);
	}

	/* Sufficient mana */
	else if (mana_cost <= old_csp)
	{
		/* Use some mana */
		caster_ptr->csp -= mana_cost;

		/* Limit */
		if (caster_ptr->csp < 0) caster_ptr->csp = 0;

		if ((use_mind == MIND_MINDCRAFTER) && (n == 13))
		{
			/* No mana left */
			caster_ptr->csp = 0;
			caster_ptr->csp_frac = 0;
		}
	}

	/* Over-exert the player */
	else
	{
		int oops = mana_cost - old_csp;

		/* No mana left */
		if ((caster_ptr->csp - mana_cost) < 0) caster_ptr->csp_frac = 0;
		caster_ptr->csp = MAX(0, caster_ptr->csp - mana_cost);

		msg_format(_("%sを集中しすぎて気を失ってしまった！", "You faint from the effort!"),p);

		/* Hack -- Bypass free action */
		(void)set_paralyzed(caster_ptr, caster_ptr->paralyzed + randint1(5 * oops + 1));

		/* Damage WIS (possibly permanently) */
		if (randint0(100) < 50)
		{
			bool perm = (randint0(100) < 25);

			msg_print(_("自分の精神を攻撃してしまった！", "You have damaged your mind!"));

			/* Reduce constitution */
			(void)dec_stat(caster_ptr, A_WIS, 15 + randint1(10), perm);
		}
	}
	caster_ptr->redraw |= (PR_MANA);
	caster_ptr->window |= (PW_PLAYER);
	caster_ptr->window |= (PW_SPELL);
}


/*!
 * @brief 現在プレイヤーが使用可能な特殊技能の一覧表示 /
 * @return なし
 */
void do_cmd_mind_browse(player_type *caster_ptr)
{
	SPELL_IDX n = 0;
	int j, line;
	char temp[62*5];
	int use_mind = 0;

	if (caster_ptr->pclass == CLASS_MINDCRAFTER) use_mind = MIND_MINDCRAFTER;
	else if (caster_ptr->pclass == CLASS_FORCETRAINER) use_mind = MIND_KI;
	else if (caster_ptr->pclass == CLASS_BERSERKER) use_mind = MIND_BERSERKER;
	else if (caster_ptr->pclass == CLASS_NINJA) use_mind = MIND_NINJUTSU;
	else if (caster_ptr->pclass == CLASS_MIRROR_MASTER)
	  use_mind = MIND_MIRROR_MASTER;

	screen_save();

	while (TRUE)
	{
		if (!get_mind_power(caster_ptr, &n, TRUE))
		{
			screen_load();
			return;
		}

		/* Clear lines, position cursor  (really should use strlen here) */
		term_erase(12, 21, 255);
		term_erase(12, 20, 255);
		term_erase(12, 19, 255);
		term_erase(12, 18, 255);
		term_erase(12, 17, 255);
		term_erase(12, 16, 255);

		shape_buffer(mind_tips[use_mind][n], 62, temp, sizeof(temp));
		for(j=0, line = 17;temp[j];j+=(1+strlen(&temp[j])))
		{
			prt(&temp[j], line, 15);
			line++;
		}
		switch (use_mind)
		{
		case MIND_MIRROR_MASTER:
		case MIND_NINJUTSU:
		  prt(_("何かキーを押して下さい。", "Hit any key."),0,0);
		  (void)inkey();
		}
	}
}
