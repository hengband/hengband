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
#include "core/player-update-types.h"
#include "core/show-file.h"
#include "core/stuff-handler.h"
#include "effect/spells-effect-util.h"
#include "game-option/play-record-options.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
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
#include "mutation/gain-mutation-switcher.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-investor.h" // todo 相互依存している、このファイルからの依存はOK.
#include "mutation/mutation-techniques.h"
#include "mutation/mutation-util.h"
#include "object-enchant/item-feeling.h"
#include "object-hook/hook-checker.h"
#include "player/avatar.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-personalities-types.h"
#include "player/player-race-types.h"
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
#include "spell/spell-types.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "status/element-resistance.h"
#include "status/shape-changer.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤーから突然変異を取り除く
 * @param choose_mut 取り除きたい突然変異のID、0ならばランダムに消去
 * @return なし
 */
bool lose_mutation(player_type *creature_ptr, MUTATION_IDX choose_mut)
{
    glm_type tmp_glm;
    glm_type *glm_ptr = initialize_gm_type(&tmp_glm, choose_mut);
    int attempts_left = 20;
    if (glm_ptr->choose_mut)
        attempts_left = 1;

    while (attempts_left--) {
        switch ((glm_ptr->choose_mut != 0) ? glm_ptr->choose_mut : randint1(193)) {
        case 1:
        case 2:
        case 3:
        case 4:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_SPIT_ACID;
            glm_ptr->muta_desc = _("酸を吹きかける能力を失った。", "You lose the ability to spit acid.");
            break;
        case 5:
        case 6:
        case 7:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_BR_FIRE;
            glm_ptr->muta_desc = _("炎のブレスを吐く能力を失った。", "You lose the ability to breathe fire.");
            break;
        case 8:
        case 9:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_HYPN_GAZE;
            glm_ptr->muta_desc = _("あなたの目はつまらない目になった。", "Your eyes look uninteresting.");
            break;
        case 10:
        case 11:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_TELEKINES;
            glm_ptr->muta_desc = _("念動力で物を動かす能力を失った。", "You lose the ability to move objects telekinetically.");
            break;
        case 12:
        case 13:
        case 14:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_VTELEPORT;
            glm_ptr->muta_desc = _("自分の意思でテレポートする能力を失った。", "You lose the power of teleportation at will.");
            break;
        case 15:
        case 16:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_MIND_BLST;
            glm_ptr->muta_desc = _("精神攻撃の能力を失った。", "You lose the power of Mind Blast.");
            break;
        case 17:
        case 18:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_RADIATION;
            glm_ptr->muta_desc = _("あなたは放射能を発生しなくなった。", "You stop emitting hard radiation.");
            break;
        case 19:
        case 20:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_VAMPIRISM;
            glm_ptr->muta_desc = _("吸血の能力を失った。", "You are no longer vampiric.");
            break;
        case 21:
        case 22:
        case 23:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_SMELL_MET;
            glm_ptr->muta_desc = _("金属の臭いを嗅げなくなった。", "You no longer smell a metallic odor.");
            break;
        case 24:
        case 25:
        case 26:
        case 27:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_SMELL_MON;
            glm_ptr->muta_desc = _("不潔なモンスターの臭いを嗅げなくなった。", "You no longer smell filthy monsters.");
            break;
        case 28:
        case 29:
        case 30:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_BLINK;
            glm_ptr->muta_desc = _("近距離テレポートの能力を失った。", "You lose the power of minor teleportation.");
            break;
        case 31:
        case 32:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_EAT_ROCK;
            glm_ptr->muta_desc = _("壁は美味しそうに見えなくなった。", "The walls look unappetizing.");
            break;
        case 33:
        case 34:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_SWAP_POS;
            glm_ptr->muta_desc = _("あなたは自分の靴に留まる感じがする。", "You feel like staying in your own shoes.");
            break;
        case 35:
        case 36:
        case 37:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_SHRIEK;
            glm_ptr->muta_desc = _("あなたの声質は弱くなった。", "Your vocal cords get much weaker.");
            break;
        case 38:
        case 39:
        case 40:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_ILLUMINE;
            glm_ptr->muta_desc = _("部屋を明るく照らすことが出来なくなった。", "You can no longer light up rooms with your presence.");
            break;
        case 41:
        case 42:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_DET_CURSE;
            glm_ptr->muta_desc = _("邪悪な魔法を感じられなくなった。", "You can no longer feel evil magics.");
            break;
        case 43:
        case 44:
        case 45:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_BERSERK;
            glm_ptr->muta_desc = _("制御できる激情を感じなくなった。", "You no longer feel a controlled rage.");
            break;
        case 46:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_POLYMORPH;
            glm_ptr->muta_desc = _("あなたの体は安定したように見える。", "Your body seems stable.");
            break;
        case 47:
        case 48:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_MIDAS_TCH;
            glm_ptr->muta_desc = _("ミダスの手の能力を失った。", "You lose the Midas touch.");
            break;
        case 49:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_GROW_MOLD;
            glm_ptr->muta_desc = _("突然カビが嫌いになった。", "You feel a sudden dislike for mold.");
            break;
        case 50:
        case 51:
        case 52:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_RESIST;
            glm_ptr->muta_desc = _("傷つき易くなった気がする。", "You feel like you might be vulnerable.");
            break;
        case 53:
        case 54:
        case 55:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_EARTHQUAKE;
            glm_ptr->muta_desc = _("ダンジョンを壊す能力を失った。", "You lose the ability to wreck the dungeon.");
            break;
        case 56:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_EAT_MAGIC;
            glm_ptr->muta_desc = _("魔法のアイテムはもう美味しそうに見えなくなった。", "Your magic items no longer look delicious.");
            break;
        case 57:
        case 58:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_WEIGH_MAG;
            glm_ptr->muta_desc = _("魔力を感じられなくなった。", "You no longer sense magic.");
            break;
        case 59:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_STERILITY;
            glm_ptr->muta_desc = _("たくさんの安堵の吐息が聞こえた。", "You hear a massed sigh of relief.");
            break;
        case 60:
        case 61:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_HIT_AND_AWAY;
            glm_ptr->muta_desc = _("あちこちへ跳べる気分がなくなった。", "You no longer feel jumpy.");
            break;
        case 62:
        case 63:
        case 64:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_DAZZLE;
            glm_ptr->muta_desc = _("まばゆい閃光を発する能力を失った。", "You lose the ability to emit dazzling lights.");
            break;
        case 65:
        case 66:
        case 67:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_LASER_EYE;
            glm_ptr->muta_desc = _("眼が少しの間焼き付いて、痛みが和らいだ。", "Your eyes burn for a moment, then feel soothed.");
            break;
        case 68:
        case 69:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_RECALL;
            glm_ptr->muta_desc = _("少しの間ホームシックになった。", "You feel briefly homesick.");
            break;
        case 70:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_BANISH;
            glm_ptr->muta_desc = _("神聖な怒りの力を感じなくなった。", "You no longer feel a holy wrath.");
            break;
        case 71:
        case 72:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_COLD_TOUCH;
            glm_ptr->muta_desc = _("手が暖かくなった。", "Your hands warm up.");
            break;
        case 73:
        case 74:
            glm_ptr->muta_class = &(creature_ptr->muta1);
            glm_ptr->muta_which = MUT1_LAUNCHER;
            glm_ptr->muta_desc = _("物を投げる手が弱くなった気がする。", "Your throwing arm feels much weaker.");
            break;
        case 75:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_BERS_RAGE;
            glm_ptr->muta_desc = _("凶暴化の発作にさらされなくなった！", "You are no longer subject to fits of berserk rage!");
            break;
        case 76:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_COWARDICE;
            glm_ptr->muta_desc = _("もう信じがたいほど臆病ではなくなった！", "You are no longer an incredible coward!");
            break;
        case 77:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_RTELEPORT;
            glm_ptr->muta_desc = _("あなたの位置はより確定的になった。", "Your position seems more certain.");
            break;
        case 78:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_ALCOHOL;
            glm_ptr->muta_desc = _("あなたはアルコールを分泌しなくなった！", "Your body stops producing alcohol!");
            break;
        case 79:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_HALLU;
            glm_ptr->muta_desc = _("幻覚をひき起こす精神障害を起こさなくなった！", "You are no longer afflicted by a hallucinatory insanity!");
            break;
        case 80:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_FLATULENT;
            glm_ptr->muta_desc = _("もう強烈な屁はこかなくなった。", "You are no longer subject to uncontrollable flatulence.");
            break;
        case 81:
        case 82:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_SCOR_TAIL;
            glm_ptr->muta_desc = _("サソリの尻尾がなくなった！", "You lose your scorpion tail!");
            break;
        case 83:
        case 84:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_HORNS;
            glm_ptr->muta_desc = _("額から角が消えた！", "Your horns vanish from your forehead!");
            break;
        case 85:
        case 86:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_BEAK;
            glm_ptr->muta_desc = _("口が普通に戻った！", "Your mouth reverts to normal!");
            break;
        case 87:
        case 88:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_ATT_DEMON;
            glm_ptr->muta_desc = _("デーモンを引き寄せなくなった。", "You stop attracting demons.");
            break;
        case 89:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_PROD_MANA;
            glm_ptr->muta_desc = _("制御不能な魔法のエネルギーを発生しなくなった。", "You stop producing magical energy uncontrollably.");
            break;
        case 90:
        case 91:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_SPEED_FLUX;
            glm_ptr->muta_desc = _("躁鬱質でなくなった。", "You are no longer manic-depressive.");
            break;
        case 92:
        case 93:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_BANISH_ALL;
            glm_ptr->muta_desc = _("背後に恐ろしい力を感じなくなった。", "You no longer feel a terrifying power lurking behind you.");
            break;
        case 94:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_EAT_LIGHT;
            glm_ptr->muta_desc = _("世界が明るいと感じる。", "You feel the world's a brighter place.");
            break;
        case 95:
        case 96:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_TRUNK;
            glm_ptr->muta_desc = _("鼻が普通の長さに戻った。", "Your nose returns to a normal length.");
            break;
        case 97:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_ATT_ANIMAL;
            glm_ptr->muta_desc = _("動物を引き寄せなくなった。", "You stop attracting animals.");
            break;
        case 98:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_TENTACLES;
            glm_ptr->muta_desc = _("触手が消えた。", "Your tentacles vanish from your sides.");
            break;
        case 99:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_RAW_CHAOS;
            glm_ptr->muta_desc = _("周囲の空間が安定した気がする。", "You feel the universe is more stable around you.");
            break;
        case 100:
        case 101:
        case 102:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_NORMALITY;
            glm_ptr->muta_desc = _("普通に奇妙な感じがする。", "You feel normally strange.");
            break;
        case 103:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_WRAITH;
            glm_ptr->muta_desc = _("あなたは物質世界にしっかり存在している。", "You are firmly in the physical world.");
            break;
        case 104:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_POLY_WOUND;
            glm_ptr->muta_desc = _("古い傷からカオスの力が去っていった。", "You feel forces of chaos departing your old scars.");
            break;
        case 105:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_WASTING;
            glm_ptr->muta_desc = _("おぞましい衰弱病が治った！", "You are cured of the horrible wasting disease!");
            break;
        case 106:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_ATT_DRAGON;
            glm_ptr->muta_desc = _("ドラゴンを引き寄せなくなった。", "You stop attracting dragons.");
            break;
        case 107:
        case 108:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_WEIRD_MIND;
            glm_ptr->muta_desc = _("思考が退屈な方向に戻った。", "Your thoughts return to boring paths.");
            break;
        case 109:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_NAUSEA;
            glm_ptr->muta_desc = _("胃が痙攣しなくなった。", "Your stomach stops roiling.");
            break;
        case 110:
        case 111:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_CHAOS_GIFT;
            glm_ptr->muta_desc = _("混沌の神々の興味を惹かなくなった。", "You lose the attention of the chaos deities.");
            break;
        case 112:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_WALK_SHAD;
            glm_ptr->muta_desc = _("物質世界に捕らわれている気がする。", "You feel like you're trapped in reality.");
            break;
        case 113:
        case 114:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_WARNING;
            glm_ptr->muta_desc = _("パラノイアでなくなった。", "You no longer feel paranoid.");
            break;
        case 115:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_INVULN;
            glm_ptr->muta_desc = _("無敵状態の発作を起こさなくなった。", "You are no longer blessed with fits of invulnerability.");
            break;
        case 116:
        case 117:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_SP_TO_HP;
            glm_ptr->muta_desc = _("魔法の治癒の発作に襲われなくなった。", "You are no longer subject to fits of magical healing.");
            break;
        case 118:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_HP_TO_SP;
            glm_ptr->muta_desc = _("痛みを伴う精神明瞭化の発作に襲われなくなった。", "You are no longer subject to fits of painful clarity.");
            break;
        case 119:
            glm_ptr->muta_class = &(creature_ptr->muta2);
            glm_ptr->muta_which = MUT2_DISARM;
            glm_ptr->muta_desc = _("脚が元の大きさに戻った。", "Your feet shrink to their former size.");
            break;
        case 120:
        case 121:
        case 122:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_HYPER_STR;
            glm_ptr->muta_desc = _("筋肉が普通に戻った。", "Your muscles revert to normal.");
            break;
        case 123:
        case 124:
        case 125:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_PUNY;
            glm_ptr->muta_desc = _("筋肉が普通に戻った。", "Your muscles revert to normal.");
            break;
        case 126:
        case 127:
        case 128:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_HYPER_INT;
            glm_ptr->muta_desc = _("脳が普通に戻った。", "Your brain reverts to normal.");
            break;
        case 129:
        case 130:
        case 131:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_MORONIC;
            glm_ptr->muta_desc = _("脳が普通に戻った。", "Your brain reverts to normal.");
            break;
        case 132:
        case 133:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_RESILIENT;
            glm_ptr->muta_desc = _("普通の丈夫さに戻った。", "You become ordinarily resilient again.");
            break;
        case 134:
        case 135:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_XTRA_FAT;
            glm_ptr->muta_desc = _("奇跡的なダイエットに成功した！", "You benefit from a miracle diet!");
            break;
        case 136:
        case 137:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_ALBINO;
            glm_ptr->muta_desc = _("アルビノでなくなった！", "You are no longer an albino!");
            break;
        case 138:
        case 139:
        case 140:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_FLESH_ROT;
            glm_ptr->muta_desc = _("肉体を腐敗させる病気が治った！", "Your flesh is no longer afflicted by a rotting disease!");
            break;
        case 141:
        case 142:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_SILLY_VOI;
            glm_ptr->muta_desc = _("声質が普通に戻った。", "Your voice returns to normal.");
            break;
        case 143:
        case 144:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_BLANK_FAC;
            glm_ptr->muta_desc = _("顔に目鼻が戻った。", "Your facial features return.");
            break;
        case 145:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_ILL_NORM;
            glm_ptr->muta_desc = _("心が安らぐ幻影を映し出さなくなった。", "You stop projecting a reassuring image.");
            break;
        case 146:
        case 147:
        case 148:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_XTRA_EYES;
            glm_ptr->muta_desc = _("余分な目が消えてしまった！", "Your extra eyes vanish!");
            break;
        case 149:
        case 150:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_MAGIC_RES;
            glm_ptr->muta_desc = _("魔法に弱くなった。", "You become susceptible to magic again.");
            break;
        case 151:
        case 152:
        case 153:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_XTRA_NOIS;
            glm_ptr->muta_desc = _("奇妙な音を立てなくなった！", "You stop making strange noise!");
            break;
        case 154:
        case 155:
        case 156:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_INFRAVIS;
            glm_ptr->muta_desc = _("赤外線視力が落ちた。", "Your infravision is degraded.");
            break;
        case 157:
        case 158:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_XTRA_LEGS;
            glm_ptr->muta_desc = _("余分な脚が消えてしまった！", "Your extra legs disappear!");
            break;
        case 159:
        case 160:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_SHORT_LEG;
            glm_ptr->muta_desc = _("脚の長さが普通に戻った。", "Your legs lengthen to normal.");
            break;
        case 161:
        case 162:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_ELEC_TOUC;
            glm_ptr->muta_desc = _("体を電流が流れなくなった。", "Electricity stops running through you.");
            break;
        case 163:
        case 164:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_FIRE_BODY;
            glm_ptr->muta_desc = _("体が炎に包まれなくなった。", "Your body is no longer enveloped in flames.");
            break;
        case 165:
        case 166:
        case 167:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_WART_SKIN;
            glm_ptr->muta_desc = _("イボイボが消えた！", "Your warts disappear!");
            break;
        case 168:
        case 169:
        case 170:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_SCALES;
            glm_ptr->muta_desc = _("鱗が消えた！", "Your scales vanish!");
            break;
        case 171:
        case 172:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_IRON_SKIN;
            glm_ptr->muta_desc = _("肌が肉にもどった！", "Your skin reverts to flesh!");
            break;
        case 173:
        case 174:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_WINGS;
            glm_ptr->muta_desc = _("背中の羽根が取れ落ちた。", "Your wings fall off.");
            break;
        case 175:
        case 176:
        case 177:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_FEARLESS;
            glm_ptr->muta_desc = _("再び恐怖を感じるようになった。", "You begin to feel fear again.");
            break;
        case 178:
        case 179:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_REGEN;
            glm_ptr->muta_desc = _("急速回復しなくなった。", "You stop regenerating.");
            break;
        case 180:
        case 181:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_ESP;
            glm_ptr->muta_desc = _("テレパシーの能力を失った！", "You lose your telepathic ability!");
            break;
        case 182:
        case 183:
        case 184:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_LIMBER;
            glm_ptr->muta_desc = _("筋肉が硬くなった。", "Your muscles stiffen.");
            break;
        case 185:
        case 186:
        case 187:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_ARTHRITIS;
            glm_ptr->muta_desc = _("関節が痛くなくなった。", "Your joints stop hurting.");
            break;
        case 188:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_BAD_LUCK;
            glm_ptr->muta_desc = _("黒いオーラは渦巻いて消えた。", "Your black aura swirls and fades.");
            break;
        case 189:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_VULN_ELEM;
            glm_ptr->muta_desc = _("無防備な感じはなくなった。", "You feel less exposed.");
            break;
        case 190:
        case 191:
        case 192:
            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_MOTION;
            glm_ptr->muta_desc = _("動作の正確さがなくなった。", "You move with less assurance.");
            break;
        case 193:
            if (creature_ptr->pseikaku == PERSONALITY_LUCKY)
                break;

            glm_ptr->muta_class = &(creature_ptr->muta3);
            glm_ptr->muta_which = MUT3_GOOD_LUCK;
            glm_ptr->muta_desc = _("白いオーラは輝いて消えた。", "Your white aura shimmers and fades.");
            break;
        default:
            glm_ptr->muta_class = NULL;
            glm_ptr->muta_which = 0;
            break;
        }

        if (glm_ptr->muta_class && glm_ptr->muta_which) {
            if (*(glm_ptr->muta_class)&glm_ptr->muta_which) {
                glm_ptr->muta_chosen = TRUE;
            }
        }

        if (glm_ptr->muta_chosen)
            break;
    }

    if (!glm_ptr->muta_chosen)
        return FALSE;

    msg_print(glm_ptr->muta_desc);
    *glm_ptr->muta_class &= ~(glm_ptr->muta_which);
    creature_ptr->update |= PU_BONUS;
    handle_stuff(creature_ptr);
    creature_ptr->mutant_regenerate_mod = calc_mutant_regenerate_mod(creature_ptr);
    return TRUE;
}

void lose_all_mutations(player_type *creature_ptr)
{
    if (creature_ptr->muta1 || creature_ptr->muta2 || creature_ptr->muta3) {
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
    return count_bits(creature_ptr->muta1) + count_bits(creature_ptr->muta2) + count_bits(creature_ptr->muta3);
}

/*!
 * @brief 突然変異による自然回復ペナルティをパーセント値で返す /
 * Return the modifier to the regeneration rate (in percent)
 * @return ペナルティ修正(%)
 * @details
 * Beastman get 10 "free" mutations and only 5% decrease per additional mutation.
 * Max 90% decrease in regeneration speed.
 */
int calc_mutant_regenerate_mod(player_type *creature_ptr)
{
    int regen;
    int mod = 10;
    int count = count_mutations(creature_ptr);
    if (creature_ptr->pseikaku == PERSONALITY_LUCKY)
        count--;

    if (creature_ptr->prace == RACE_BEASTMAN) {
        count -= 10;
        mod = 5;
    }

    if (count <= 0)
        return 100;

    regen = 100 - count * mod;
    if (regen < 10)
        regen = 10;

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
    switch (power) {
    case MUT1_SPIT_ACID:
        if (!get_aim_dir(creature_ptr, &dir))
            return FALSE;

        stop_mouth(creature_ptr);
        msg_print(_("酸を吐きかけた...", "You spit acid..."));
        fire_ball(creature_ptr, GF_ACID, dir, lvl, 1 + (lvl / 30));
        return TRUE;
    case MUT1_BR_FIRE:
        if (!get_aim_dir(creature_ptr, &dir))
            return FALSE;

        stop_mouth(creature_ptr);
        msg_print(_("あなたは火炎のブレスを吐いた...", "You breathe fire..."));
        fire_breath(creature_ptr, GF_FIRE, dir, lvl * 2, 1 + (lvl / 20));
        return TRUE;
    case MUT1_HYPN_GAZE:
        if (!get_aim_dir(creature_ptr, &dir))
            return FALSE;

        msg_print(_("あなたの目は幻惑的になった...", "Your eyes look mesmerizing..."));
        (void)charm_monster(creature_ptr, dir, lvl);
        return TRUE;
    case MUT1_TELEKINES:
        if (!get_aim_dir(creature_ptr, &dir))
            return FALSE;

        msg_print(_("集中している...", "You concentrate..."));
        fetch_item(creature_ptr, dir, lvl * 10, TRUE);
        return TRUE;
    case MUT1_VTELEPORT:
        msg_print(_("集中している...", "You concentrate..."));
        teleport_player(creature_ptr, 10 + 4 * lvl, TELEPORT_SPONTANEOUS);
        return TRUE;
    case MUT1_MIND_BLST:
        if (!get_aim_dir(creature_ptr, &dir))
            return FALSE;

        msg_print(_("集中している...", "You concentrate..."));
        fire_bolt(creature_ptr, GF_PSI, dir, damroll(3 + ((lvl - 1) / 5), 3));
        return TRUE;
    case MUT1_RADIATION:
        msg_print(_("体から放射能が発生した！", "Radiation flows from your body!"));
        fire_ball(creature_ptr, GF_NUKE, 0, (lvl * 2), 3 + (lvl / 20));
        return TRUE;
    case MUT1_VAMPIRISM:
        vampirism(creature_ptr);
        return TRUE;
    case MUT1_SMELL_MET:
        stop_mouth(creature_ptr);
        (void)detect_treasure(creature_ptr, DETECT_RAD_DEFAULT);
        return TRUE;
    case MUT1_SMELL_MON:
        stop_mouth(creature_ptr);
        (void)detect_monsters_normal(creature_ptr, DETECT_RAD_DEFAULT);
        return TRUE;
    case MUT1_BLINK:
        teleport_player(creature_ptr, 10, TELEPORT_SPONTANEOUS);
        return TRUE;
    case MUT1_EAT_ROCK:
        return eat_rock(creature_ptr);
    case MUT1_SWAP_POS:
        project_length = -1;
        if (!get_aim_dir(creature_ptr, &dir)) {
            project_length = 0;
            return FALSE;
        }

        (void)teleport_swap(creature_ptr, dir);
        project_length = 0;
        return TRUE;
    case MUT1_SHRIEK:
        stop_mouth(creature_ptr);
        (void)fire_ball(creature_ptr, GF_SOUND, 0, 2 * lvl, 8);
        (void)aggravate_monsters(creature_ptr, 0);
        return TRUE;
    case MUT1_ILLUMINE:
        (void)lite_area(creature_ptr, damroll(2, (lvl / 2)), (lvl / 10) + 1);
        return TRUE;
    case MUT1_DET_CURSE:
        for (int i = 0; i < INVEN_TOTAL; i++) {
            object_type *o_ptr = &creature_ptr->inventory_list[i];
            if ((o_ptr->k_idx == 0) || !object_is_cursed(o_ptr))
                continue;

            o_ptr->feeling = FEEL_CURSED;
        }

        return TRUE;
    case MUT1_BERSERK:
        (void)berserk(creature_ptr, randint1(25) + 25);
        return TRUE;
    case MUT1_POLYMORPH:
        if (!get_check(_("変身します。よろしいですか？", "You will polymorph your self. Are you sure? ")))
            return FALSE;

        do_poly_self(creature_ptr);
        return TRUE;
    case MUT1_MIDAS_TCH:
        return alchemy(creature_ptr);
    case MUT1_GROW_MOLD:
        for (DIRECTION i = 0; i < 8; i++)
            summon_specific(creature_ptr, -1, creature_ptr->y, creature_ptr->x, lvl, SUMMON_MOLD, PM_FORCE_PET);

        return TRUE;
    case MUT1_RESIST: {
        int num = lvl / 10;
        TIME_EFFECT dur = randint1(20) + 20;
        if (randint0(5) < num) {
            (void)set_oppose_acid(creature_ptr, dur, FALSE);
            num--;
        }

        if (randint0(4) < num) {
            (void)set_oppose_elec(creature_ptr, dur, FALSE);
            num--;
        }

        if (randint0(3) < num) {
            (void)set_oppose_fire(creature_ptr, dur, FALSE);
            num--;
        }

        if (randint0(2) < num) {
            (void)set_oppose_cold(creature_ptr, dur, FALSE);
            num--;
        }

        if (num != 0) {
            (void)set_oppose_pois(creature_ptr, dur, FALSE);
            num--;
        }

        return TRUE;
    }
    case MUT1_EARTHQUAKE:
        (void)earthquake(creature_ptr, creature_ptr->y, creature_ptr->x, 10, 0);
        return TRUE;
    case MUT1_EAT_MAGIC:
        return eat_magic(creature_ptr, creature_ptr->lev * 2);
    case MUT1_WEIGH_MAG:
        report_magics(creature_ptr);
        return TRUE;
    case MUT1_STERILITY:
        msg_print(_("突然頭が痛くなった！", "You suddenly have a headache!"));
        take_hit(creature_ptr, DAMAGE_LOSELIFE, randint1(17) + 17, _("禁欲を強いた疲労", "the strain of forcing abstinence"), -1);
        creature_ptr->current_floor_ptr->num_repro += MAX_REPRO;
        return TRUE;
    case MUT1_HIT_AND_AWAY:
        return hit_and_away(creature_ptr);
    case MUT1_DAZZLE:
        stun_monsters(creature_ptr, lvl * 4);
        confuse_monsters(creature_ptr, lvl * 4);
        turn_monsters(creature_ptr, lvl * 4);
        return TRUE;
    case MUT1_LASER_EYE:
        if (!get_aim_dir(creature_ptr, &dir))
            return FALSE;

        fire_beam(creature_ptr, GF_LITE, dir, 2 * lvl);
        return TRUE;
    case MUT1_RECALL:
        return recall_player(creature_ptr, randint0(21) + 15);
    case MUT1_BANISH: {
        if (!get_direction(creature_ptr, &dir, FALSE, FALSE))
            return FALSE;

        POSITION y = creature_ptr->y + ddy[dir];
        POSITION x = creature_ptr->x + ddx[dir];
        grid_type *g_ptr;
        g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

        if (!g_ptr->m_idx) {
            msg_print(_("邪悪な存在を感じとれません！", "You sense no evil there!"));
            return TRUE;
        }

        monster_type *m_ptr;
        m_ptr = &creature_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
        monster_race *r_ptr;
        r_ptr = &r_info[m_ptr->r_idx];
        if ((r_ptr->flags3 & RF3_EVIL) && !(r_ptr->flags1 & RF1_QUESTOR) && !(r_ptr->flags1 & RF1_UNIQUE) && !creature_ptr->current_floor_ptr->inside_arena
            && !creature_ptr->current_floor_ptr->inside_quest && (r_ptr->level < randint1(creature_ptr->lev + 50)) && !(m_ptr->mflag2 & MFLAG2_NOGENO)) {
            if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname) {
                GAME_TEXT m_name[MAX_NLEN];
                monster_desc(creature_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
                exe_write_diary(creature_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_GENOCIDE, m_name);
            }

            delete_monster_idx(creature_ptr, g_ptr->m_idx);
            msg_print(_("その邪悪なモンスターは硫黄臭い煙とともに消え去った！", "The evil creature vanishes in a puff of sulfurous smoke!"));
            return TRUE;
        }

        msg_print(_("祈りは効果がなかった！", "Your invocation is ineffectual!"));
        if (one_in_(13))
            m_ptr->mflag2 |= MFLAG2_NOGENO;

        return TRUE;
    }
    case MUT1_COLD_TOUCH: {
        if (!get_direction(creature_ptr, &dir, FALSE, FALSE))
            return FALSE;

        POSITION y = creature_ptr->y + ddy[dir];
        POSITION x = creature_ptr->x + ddx[dir];
        grid_type *g_ptr;
        g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
        if (!g_ptr->m_idx) {
            msg_print(_("あなたは何もない場所で手を振った。", "You wave your hands in the air."));
            return TRUE;
        }

        fire_bolt(creature_ptr, GF_COLD, dir, 2 * lvl);
        return TRUE;
    }
    case 3:
        return do_cmd_throw(creature_ptr, 2 + lvl / 40, FALSE, -1);
    default:
        free_turn(creature_ptr);
        msg_format(_("能力 %s は実装されていません。", "Power %s not implemented. Oops."), power);
        return TRUE;
    }
}

void become_living_trump(player_type *creature_ptr)
{
    /* 1/7 Teleport control and 6/7 Random teleportation (uncontrolled) */
    MUTATION_IDX mutation = one_in_(7) ? 12 : 77;
    if (gain_mutation(creature_ptr, mutation))
        msg_print(_("あなたは生きているカードに変わった。", "You have turned into a Living Trump."));
}

void set_mutation_flags(player_type *creature_ptr)
{
    if (creature_ptr->muta3 == 0)
        return;

    if (creature_ptr->muta3 & MUT3_FLESH_ROT)
        creature_ptr->regenerate = FALSE;

    if (creature_ptr->muta3 & MUT3_ELEC_TOUC)
        creature_ptr->sh_elec = TRUE;

    if (creature_ptr->muta3 & MUT3_FIRE_BODY) {
        creature_ptr->sh_fire = TRUE;
        creature_ptr->lite = TRUE;
    }

    if (creature_ptr->muta3 & MUT3_WINGS)
        creature_ptr->levitation = TRUE;

    if (creature_ptr->muta3 & MUT3_FEARLESS)
        creature_ptr->resist_fear = TRUE;

    if (creature_ptr->muta3 & MUT3_REGEN)
        creature_ptr->regenerate = TRUE;

    if (creature_ptr->muta3 & MUT3_ESP)
        creature_ptr->telepathy = TRUE;

    if (creature_ptr->muta3 & MUT3_MOTION)
        creature_ptr->free_act = TRUE;
}
