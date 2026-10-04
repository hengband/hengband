/*!
 * @file mutation-roll-table.cpp
 * @brief 突然変異の抽選表
 */

#include "mutation/mutation-roll-table.h"
#include "locale/language-switcher.h"
#include "mutation/mutation-flag-types.h"
#include <array>
#include <range/v3/algorithm.hpp>

namespace {
/*!
 * @brief 突然変異の抽選表
 * @details 抽選値の上限の昇順に並べる。突然変異の獲得と喪失で同じ表を使う。
 * 抽選値はトランプ魔法やデバッグコマンドから直接指定されることがあるため、区間を変えるときは呼び出し元も確認すること。
 */
constexpr std::array<MutationRollEntry, 96> MUTATION_ROLL_TABLE = { {
    { 4, PlayerMutationType::SPIT_ACID, _("酸を吐く能力を得た。", "You gain the ability to spit acid."), _("酸を吹きかける能力を失った。", "You lose the ability to spit acid.") },
    { 7, PlayerMutationType::BR_FIRE, _("火を吐く能力を得た。", "You gain the ability to breathe fire."), _("炎のブレスを吐く能力を失った。", "You lose the ability to breathe fire.") },
    { 9, PlayerMutationType::HYPN_GAZE, _("催眠眼の能力を得た。", "Your eyes look mesmerizing..."), _("あなたの目はつまらない目になった。", "Your eyes look uninteresting.") },
    { 11, PlayerMutationType::TELEKINES, _("物体を念動力で動かす能力を得た。", "You gain the ability to move objects telekinetically."), _("念動力で物を動かす能力を失った。", "You lose the ability to move objects telekinetically.") },
    { 14, PlayerMutationType::VTELEPORT, _("自分の意思でテレポートする能力を得た。", "You gain the power of teleportation at will."), _("自分の意思でテレポートする能力を失った。", "You lose the power of teleportation at will.") },
    { 16, PlayerMutationType::MIND_BLST, _("精神攻撃の能力を得た。", "You gain the power of Mind Blast."), _("精神攻撃の能力を失った。", "You lose the power of Mind Blast.") },
    { 18, PlayerMutationType::RADIATION, _("あなたは強い放射線を発生し始めた。", "You start emitting hard radiation."), _("あなたは放射能を発生しなくなった。", "You stop emitting hard radiation.") },
    { 20, PlayerMutationType::VAMPIRISM, _("生命力を吸収できるようになった。", "You become vampiric."), _("吸血の能力を失った。", "You are no longer vampiric.") },
    { 23, PlayerMutationType::SMELL_MET, _("金属の匂いを嗅ぎ分けられるようになった。", "You smell a metallic odor."), _("金属の臭いを嗅げなくなった。", "You no longer smell a metallic odor.") },
    { 27, PlayerMutationType::SMELL_MON, _("モンスターの臭いを嗅ぎ分けられるようになった。", "You smell filthy monsters."), _("不潔なモンスターの臭いを嗅げなくなった。", "You no longer smell filthy monsters.") },
    { 30, PlayerMutationType::BLINK, _("近距離テレポートの能力を得た。", "You gain the power of minor teleportation."), _("近距離テレポートの能力を失った。", "You lose the power of minor teleportation.") },
    { 32, PlayerMutationType::EAT_ROCK, _("壁が美味しそうに見える。", "The walls look delicious."), _("壁は美味しそうに見えなくなった。", "The walls look unappetizing.") },
    { 34, PlayerMutationType::SWAP_POS, _("他人の靴で一マイル歩くような気分がする。", "You feel like walking a mile in someone else's shoes."), _("あなたは自分の靴に留まる感じがする。", "You feel like staying in your own shoes.") },
    { 37, PlayerMutationType::SHRIEK, _("あなたの声は相当強くなった。", "Your vocal cords get much tougher."), _("あなたの声質は弱くなった。", "Your vocal cords get much weaker.") },
    { 40, PlayerMutationType::ILLUMINE, _("あなたは光り輝いて部屋を明るくするようになった。", "You can light up rooms with your presence."), _("部屋を明るく照らすことが出来なくなった。", "You can no longer light up rooms with your presence.") },
    { 42, PlayerMutationType::DET_CURSE, _("邪悪な魔法を感知できるようになった。", "You can feel evil magics."), _("邪悪な魔法を感じられなくなった。", "You can no longer feel evil magics.") },
    { 45, PlayerMutationType::BERSERK, _("制御できる激情を感じる。", "You feel a controlled rage."), _("制御できる激情を感じなくなった。", "You no longer feel a controlled rage.") },
    { 46, PlayerMutationType::POLYMORPH, _("体が変異しやすくなった。", "Your body seems mutable."), _("あなたの体は安定したように見える。", "Your body seems stable.") },
    { 48, PlayerMutationType::MIDAS_TCH, _("「ミダス王の手」の能力を得た。", "You gain the Midas touch."), _("ミダスの手の能力を失った。", "You lose the Midas touch.") },
    { 49, PlayerMutationType::GROW_MOLD, _("突然カビに親しみを覚えた。", "You feel a sudden affinity for mold."), _("突然カビが嫌いになった。", "You feel a sudden dislike for mold.") },
    { 52, PlayerMutationType::RESIST, _("あなたは自分自身を守れる気がする。", "You feel like you can protect yourself."), _("傷つき易くなった気がする。", "You feel like you might be vulnerable.") },
    { 55, PlayerMutationType::EARTHQUAKE, _("ダンジョンを破壊する能力を得た。", "You gain the ability to wreck the dungeon."), _("ダンジョンを壊す能力を失った。", "You lose the ability to wreck the dungeon.") },
    { 56, PlayerMutationType::EAT_MAGIC, _("魔法のアイテムが美味そうに見える。", "Your magic items look delicious."), _("魔法のアイテムはもう美味しそうに見えなくなった。", "Your magic items no longer look delicious.") },
    { 58, PlayerMutationType::WEIGH_MAG, _("あなたは周囲にある魔法をより良く理解できる気がする。", "You feel you can better understand the magic around you."), _("魔力を感じられなくなった。", "You no longer sense magic.") },
    { 59, PlayerMutationType::STERILITY, _("周りの全ての者に頭痛を起こすことができる。", "You can give everything around you a headache."), _("たくさんの安堵の吐息が聞こえた。", "You hear a massed sigh of relief.") },
    { 61, PlayerMutationType::HIT_AND_AWAY, _("突然、泥棒の気分が分かるようになった。", "You suddenly understand how thieves feel."), _("あちこちへ跳べる気分がなくなった。", "You no longer feel jumpy.") },
    { 64, PlayerMutationType::DAZZLE, _("眩い閃光を発する能力を得た。", "You gain the ability to emit dazzling lights."), _("まばゆい閃光を発する能力を失った。", "You lose the ability to emit dazzling lights.") },
    { 67, PlayerMutationType::LASER_EYE, _("あなたの目は一瞬焼け付いた。", "Your eyes burn for a moment."), _("眼が少しの間焼き付いて、痛みが和らいだ。", "Your eyes burn for a moment, then feel soothed.") },
    { 69, PlayerMutationType::RECALL, _("少しだけホームシックになったが、すぐ直った。", "You feel briefly homesick, but it passes."), _("少しの間ホームシックになった。", "You feel briefly homesick.") },
    { 70, PlayerMutationType::BANISH, _("神聖な怒りの力に満たされた。", "You feel a holy wrath fill you."), _("神聖な怒りの力を感じなくなった。", "You no longer feel a holy wrath.") },
    { 72, PlayerMutationType::COLD_TOUCH, _("あなたの両手はとても冷たくなった。", "Your hands get very cold."), _("手が暖かくなった。", "Your hands warm up.") },
    { 74, PlayerMutationType::LAUNCHER, _("あなたの物を投げる手はかなり強くなった気がする。", "Your throwing arm feels much stronger."), _("物を投げる手が弱くなった気がする。", "Your throwing arm feels much weaker.") },
    { 75, PlayerMutationType::BERS_RAGE, _("あなたは狂暴化の発作を起こすようになった！", "You become subject to fits of berserk rage!"), _("凶暴化の発作にさらされなくなった！", "You are no longer subject to fits of berserk rage!") },
    { 76, PlayerMutationType::COWARDICE, _("信じられないくらい臆病になった！", "You become an incredible coward!"), _("もう信じがたいほど臆病ではなくなった！", "You are no longer an incredible coward!") },
    { 77, PlayerMutationType::RTELEPORT, _("あなたの位置は非常に不確定になった。", "Your position seems very uncertain..."), _("あなたの位置はより確定的になった。", "Your position seems more certain.") },
    { 78, PlayerMutationType::ALCOHOL, _("あなたはアルコールを分泌するようになった。", "Your body starts producing alcohol!"), _("あなたはアルコールを分泌しなくなった！", "Your body stops producing alcohol!") },
    { 79, PlayerMutationType::HALLU, _("あなたは幻覚を引き起こす精神錯乱に侵された。", "You are afflicted by a hallucinatory insanity!"), _("幻覚をひき起こす精神障害を起こさなくなった！", "You are no longer afflicted by a hallucinatory insanity!") },
    { 80, PlayerMutationType::FLATULENT, _("あなたは制御不能な強烈な屁をこくようになった。", "You become subject to uncontrollable flatulence."), _("もう強烈な屁はこかなくなった。", "You are no longer subject to uncontrollable flatulence.") },
    { 82, PlayerMutationType::SCOR_TAIL, _("サソリの尻尾が生えてきた！", "You grow a scorpion tail!"), _("サソリの尻尾がなくなった！", "You lose your scorpion tail!") },
    { 84, PlayerMutationType::HORNS, _("額に角が生えた！", "Horns pop forth into your forehead!"), _("額から角が消えた！", "Your horns vanish from your forehead!") },
    { 86, PlayerMutationType::BEAK, _("口が鋭く強いクチバシに変化した！", "Your mouth turns into a sharp, powerful beak!"), _("口が普通に戻った！", "Your mouth reverts to normal!") },
    { 88, PlayerMutationType::ATT_DEMON, _("悪魔を引き付けるようになった。", "You start attracting demons."), _("デーモンを引き寄せなくなった。", "You stop attracting demons.") },
    { 89, PlayerMutationType::PROD_MANA, _("あなたは制御不能な魔法のエネルギーを発生するようになった。", "You start producing magical energy uncontrollably."), _("制御不能な魔法のエネルギーを発生しなくなった。", "You stop producing magical energy uncontrollably.") },
    { 91, PlayerMutationType::SPEED_FLUX, _("あなたは躁鬱質になった。", "You become manic-depressive."), _("躁鬱質でなくなった。", "You are no longer manic-depressive.") },
    { 93, PlayerMutationType::BANISH_ALL, _("恐ろしい力があなたの背後に潜んでいる気がする。", "You feel a terrifying power lurking behind you."), _("背後に恐ろしい力を感じなくなった。", "You no longer feel a terrifying power lurking behind you.") },
    { 94, PlayerMutationType::EAT_LIGHT, _("あなたはウンゴリアントに奇妙な親しみを覚えるようになった。", "You feel a strange kinship with Ungoliant."), _("世界が明るいと感じる。", "You feel the world's a brighter place.") },
    { 96, PlayerMutationType::TRUNK, _("あなたの鼻は伸びて象の鼻のようになった。", "Your nose grows into an elephant-like trunk."), _("鼻が普通の長さに戻った。", "Your nose returns to a normal length.") },
    { 97, PlayerMutationType::ATT_ANIMAL, _("動物を引き付けるようになった。", "You start attracting animals."), _("動物を引き寄せなくなった。", "You stop attracting animals.") },
    { 98, PlayerMutationType::TENTACLES, _("邪悪な触手が体の両側に生えてきた。", "Evil-looking tentacles sprout from your sides."), _("触手が消えた。", "Your tentacles vanish from your sides.") },
    { 99, PlayerMutationType::RAW_CHAOS, _("周囲の空間が不安定になった気がする。", "You feel the universe is less stable around you."), _("周囲の空間が安定した気がする。", "You feel the universe is more stable around you.") },
    { 102, PlayerMutationType::NORMALITY, _("あなたは奇妙なほど普通になった気がする。", "You feel strangely normal."), _("普通に奇妙な感じがする。", "You feel normally strange.") },
    { 103, PlayerMutationType::WRAITH, _("あなたは幽体化したり実体化したりするようになった。", "You start to fade in and out of the physical world."), _("あなたは物質世界にしっかり存在している。", "You are firmly in the physical world.") },
    { 104, PlayerMutationType::POLY_WOUND, _("あなたはカオスの力が古い傷に入り込んでくるのを感じた。", "You feel forces of chaos entering your old scars."), _("古い傷からカオスの力が去っていった。", "You feel forces of chaos departing your old scars.") },
    { 105, PlayerMutationType::WASTING, _("あなたは突然おぞましい衰弱病にかかった。", "You suddenly contract a horrible wasting disease."), _("おぞましい衰弱病が治った！", "You are cured of the horrible wasting disease!") },
    { 106, PlayerMutationType::ATT_DRAGON, _("あなたはドラゴンを引きつけるようになった。", "You start attracting dragons."), _("ドラゴンを引き寄せなくなった。", "You stop attracting dragons.") },
    { 108, PlayerMutationType::WEIRD_MIND, _("あなたの思考は突然おかしな方向に向き始めた。", "Your thoughts suddenly take off in strange directions."), _("思考が退屈な方向に戻った。", "Your thoughts return to boring paths.") },
    { 109, PlayerMutationType::NAUSEA, _("胃袋がピクピクしはじめた。", "Your stomach starts to roil nauseously."), _("胃が痙攣しなくなった。", "Your stomach stops roiling.") },
    { 111, PlayerMutationType::CHAOS_GIFT, _("あなたはカオスの守護悪魔の注意を惹くようになった。", "You attract the notice of a chaos deity!"), _("混沌の神々の興味を惹かなくなった。", "You lose the attention of the chaos deities.") },
    { 112, PlayerMutationType::WALK_SHAD, _("あなたは現実が紙のように薄いと感じるようになった。", "You feel like reality is as thin as paper."), _("物質世界に捕らわれている気がする。", "You feel like you're trapped in reality.") },
    { 114, PlayerMutationType::WARNING, _("あなたは突然パラノイアになった気がする。", "You suddenly feel paranoid."), _("パラノイアでなくなった。", "You no longer feel paranoid.") },
    { 115, PlayerMutationType::INVULN, _("あなたは祝福され、無敵状態になる発作を起こすようになった。", "You are blessed with fits of invulnerability."), _("無敵状態の発作を起こさなくなった。", "You are no longer blessed with fits of invulnerability.") },
    { 117, PlayerMutationType::SP_TO_HP, _("魔法の治癒の発作を起こすようになった。", "You are subject to fits of magical healing."), _("魔法の治癒の発作に襲われなくなった。", "You are no longer subject to fits of magical healing.") },
    { 118, PlayerMutationType::HP_TO_SP, _("痛みを伴う精神明瞭化の発作を起こすようになった。", "You are subject to fits of painful clarity."), _("痛みを伴う精神明瞭化の発作に襲われなくなった。", "You are no longer subject to fits of painful clarity.") },
    { 119, PlayerMutationType::DISARM, _("あなたの脚は長さが四倍になった。", "Your feet grow to four times their former size."), _("脚が元の大きさに戻った。", "Your feet shrink to their former size.") },
    { 122, PlayerMutationType::HYPER_STR, _("超人的に強くなった！", "You turn into a superhuman he-man!"), _("筋肉が普通に戻った。", "Your muscles revert to normal.") },
    { 125, PlayerMutationType::PUNY, _("筋肉が弱ってしまった...", "Your muscles wither away..."), _("筋肉が普通に戻った。", "Your muscles revert to normal.") },
    { 128, PlayerMutationType::HYPER_INT, _("あなたの脳は生体コンピュータに進化した！", "Your brain evolves into a living computer!"), _("脳が普通に戻った。", "Your brain reverts to normal.") },
    { 131, PlayerMutationType::MORONIC, _("脳が萎縮してしまった...", "Your brain withers away..."), _("脳が普通に戻った。", "Your brain reverts to normal.") },
    { 133, PlayerMutationType::RESILIENT, _("並外れてタフになった。", "You become extraordinarily resilient."), _("普通の丈夫さに戻った。", "You become ordinarily resilient again.") },
    { 135, PlayerMutationType::XTRA_FAT, _("あなたは気持ち悪いくらい太った！", "You become sickeningly fat!"), _("奇跡的なダイエットに成功した！", "You benefit from a miracle diet!") },
    { 137, PlayerMutationType::ALBINO, _("アルビノになった！弱くなった気がする...", "You turn into an albino! You feel frail..."), _("アルビノでなくなった！", "You are no longer an albino!") },
    { 140, PlayerMutationType::FLESH_ROT, _("あなたの肉体は腐敗する病気に侵された！", "Your flesh is afflicted by a rotting disease!"), _("肉体を腐敗させる病気が治った！", "Your flesh is no longer afflicted by a rotting disease!") },
    { 142, PlayerMutationType::SILLY_VOI, _("声が間抜けなキーキー声になった！", "Your voice turns into a ridiculous squeak!"), _("声質が普通に戻った。", "Your voice returns to normal.") },
    { 144, PlayerMutationType::BLANK_FAC, _("のっぺらぼうになった！", "Your face becomes completely featureless!"), _("顔に目鼻が戻った。", "Your facial features return.") },
    { 145, PlayerMutationType::ILL_NORM, _("心の安らぐ幻影を映し出すようになった。", "You start projecting a reassuring image."), _("心が安らぐ幻影を映し出さなくなった。", "You stop projecting a reassuring image.") },
    { 148, PlayerMutationType::XTRA_EYES, _("新たに二つの目が出来た！", "You grow an extra pair of eyes!"), _("余分な目が消えてしまった！", "Your extra eyes vanish!") },
    { 150, PlayerMutationType::MAGIC_RES, _("魔法への耐性がついた。", "You become resistant to magic."), _("魔法に弱くなった。", "You become susceptible to magic again.") },
    { 153, PlayerMutationType::XTRA_NOIS, _("あなたは奇妙な音を立て始めた！", "You start making strange noise!"), _("奇妙な音を立てなくなった！", "You stop making strange noise!") },
    { 156, PlayerMutationType::INFRAVIS, _("赤外線視力が増した。", "Your infravision is improved."), _("赤外線視力が落ちた。", "Your infravision is degraded.") },
    { 158, PlayerMutationType::XTRA_LEGS, _("新たに二本の足が生えてきた！", "You grow an extra pair of legs!"), _("余分な脚が消えてしまった！", "Your extra legs disappear!") },
    { 160, PlayerMutationType::SHORT_LEG, _("足が短い突起になってしまった！", "Your legs turn into short stubs!"), _("脚の長さが普通に戻った。", "Your legs lengthen to normal.") },
    { 162, PlayerMutationType::ELEC_TOUC, _("血管を電流が流れ始めた！", "Electricity starts running through you!"), _("体を電流が流れなくなった。", "Electricity stops running through you.") },
    { 164, PlayerMutationType::FIRE_BODY, _("あなたの体は炎につつまれている。", "Your body is enveloped in flames!"), _("体が炎に包まれなくなった。", "Your body is no longer enveloped in flames.") },
    { 167, PlayerMutationType::WART_SKIN, _("気持ち悪いイボイボが体中にできた！", "Disgusting warts appear everywhere on you!"), _("イボイボが消えた！", "Your warts disappear!") },
    { 170, PlayerMutationType::SCALES, _("肌が黒い鱗に変わった！", "Your skin turns into black scales!"), _("鱗が消えた！", "Your scales vanish!") },
    { 172, PlayerMutationType::IRON_SKIN, _("あなたの肌は鉄になった！", "Your skin turns to steel!"), _("肌が肉にもどった！", "Your skin reverts to flesh!") },
    { 174, PlayerMutationType::WINGS, _("背中に羽が生えた。", "You grow a pair of wings."), _("背中の羽根が取れ落ちた。", "Your wings fall off.") },
    { 177, PlayerMutationType::FEARLESS, _("完全に怖れ知らずになった。", "You become completely fearless."), _("再び恐怖を感じるようになった。", "You begin to feel fear again.") },
    { 179, PlayerMutationType::REGEN, _("急速に回復し始めた。", "You start regenerating."), _("急速回復しなくなった。", "You stop regenerating.") },
    { 181, PlayerMutationType::ESP, _("テレパシーの能力を得た！", "You develop a telepathic ability!"), _("テレパシーの能力を失った！", "You lose your telepathic ability!") },
    { 184, PlayerMutationType::LIMBER, _("筋肉がしなやかになった。", "Your muscles become limber."), _("筋肉が硬くなった。", "Your muscles stiffen.") },
    { 187, PlayerMutationType::ARTHRITIS, _("関節が突然痛み出した。", "Your joints suddenly hurt."), _("関節が痛くなくなった。", "Your joints stop hurting.") },
    { 188, PlayerMutationType::BAD_LUCK, _("悪意に満ちた黒いオーラがあなたをとりまいた...", "There is a malignant black aura surrounding you..."), _("黒いオーラは渦巻いて消えた。", "Your black aura swirls and fades.") },
    { 189, PlayerMutationType::VULN_ELEM, _("妙に無防備になった気がする。", "You feel strangely exposed."), _("無防備な感じはなくなった。", "You feel less exposed.") },
    { 192, PlayerMutationType::MOTION, _("体の動作がより正確になった。", "You move with new assurance."), _("動作の正確さがなくなった。", "You move with less assurance.") },
    { 193, PlayerMutationType::GOOD_LUCK, _("慈悲深い白いオーラがあなたをとりまいた...", "There is a benevolent white aura surrounding you..."), _("白いオーラは輝いて消えた。", "Your white aura shimmers and fades.") },
} };

static_assert(ranges::is_sorted(MUTATION_ROLL_TABLE, ranges::less{}, &MutationRollEntry::max_roll));
static_assert(MUTATION_ROLL_TABLE.back().max_roll == MUTATION_ROLL_MAX);
}

/*!
 * @brief 抽選値に対応する突然変異を抽選表から引く
 * @param roll 抽選値 (1～MUTATION_ROLL_MAX)
 * @return 抽選値に対応する抽選表の要素。抽選値が範囲外ならnullopt
 */
tl::optional<const MutationRollEntry &> find_mutation_by_roll(int roll)
{
    if ((roll < 1) || (roll > MUTATION_ROLL_MAX)) {
        return tl::nullopt;
    }

    const auto it = ranges::lower_bound(MUTATION_ROLL_TABLE, roll, ranges::less{}, &MutationRollEntry::max_roll);
    return *it;
}
