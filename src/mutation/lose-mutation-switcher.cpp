#include "mutation/lose-mutation-switcher.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-util.h"
#include "system/player-type-definition.h"

void switch_lose_mutation(player_type *player_ptr, glm_type *glm_ptr)
{
    switch ((glm_ptr->choose_mut != 0) ? glm_ptr->choose_mut : randint1(193)) {
    case 1:
    case 2:
    case 3:
    case 4:
        glm_ptr->muta_which = MUTA::SPIT_ACID;
        glm_ptr->muta_desc = _("酸を吹きかける能力を失った。", "You lose the ability to spit acid.");
        break;
    case 5:
    case 6:
    case 7:
        glm_ptr->muta_which = MUTA::BR_FIRE;
        glm_ptr->muta_desc = _("炎のブレスを吐く能力を失った。", "You lose the ability to breathe fire.");
        break;
    case 8:
    case 9:
        glm_ptr->muta_which = MUTA::HYPN_GAZE;
        glm_ptr->muta_desc = _("あなたの目はつまらない目になった。", "Your eyes look uninteresting.");
        break;
    case 10:
    case 11:
        glm_ptr->muta_which = MUTA::TELEKINES;
        glm_ptr->muta_desc = _("念動力で物を動かす能力を失った。", "You lose the ability to move objects telekinetically.");
        break;
    case 12:
    case 13:
    case 14:
        glm_ptr->muta_which = MUTA::VTELEPORT;
        glm_ptr->muta_desc = _("自分の意思でテレポートする能力を失った。", "You lose the power of teleportation at will.");
        break;
    case 15:
    case 16:
        glm_ptr->muta_which = MUTA::MIND_BLST;
        glm_ptr->muta_desc = _("精神攻撃の能力を失った。", "You lose the power of Mind Blast.");
        break;
    case 17:
    case 18:
        glm_ptr->muta_which = MUTA::RADIATION;
        glm_ptr->muta_desc = _("あなたは放射能を発生しなくなった。", "You stop emitting hard radiation.");
        break;
    case 19:
    case 20:
        glm_ptr->muta_which = MUTA::VAMPIRISM;
        glm_ptr->muta_desc = _("吸血の能力を失った。", "You are no longer vampiric.");
        break;
    case 21:
    case 22:
    case 23:
        glm_ptr->muta_which = MUTA::SMELL_MET;
        glm_ptr->muta_desc = _("金属の臭いを嗅げなくなった。", "You no longer smell a metallic odor.");
        break;
    case 24:
    case 25:
    case 26:
    case 27:
        glm_ptr->muta_which = MUTA::SMELL_MON;
        glm_ptr->muta_desc = _("不潔なモンスターの臭いを嗅げなくなった。", "You no longer smell filthy monsters.");
        break;
    case 28:
    case 29:
    case 30:
        glm_ptr->muta_which = MUTA::BLINK;
        glm_ptr->muta_desc = _("近距離テレポートの能力を失った。", "You lose the power of minor teleportation.");
        break;
    case 31:
    case 32:
        glm_ptr->muta_which = MUTA::EAT_ROCK;
        glm_ptr->muta_desc = _("壁は美味しそうに見えなくなった。", "The walls look unappetizing.");
        break;
    case 33:
    case 34:
        glm_ptr->muta_which = MUTA::SWAP_POS;
        glm_ptr->muta_desc = _("あなたは自分の靴に留まる感じがする。", "You feel like staying in your own shoes.");
        break;
    case 35:
    case 36:
    case 37:
        glm_ptr->muta_which = MUTA::SHRIEK;
        glm_ptr->muta_desc = _("あなたの声質は弱くなった。", "Your vocal cords get much weaker.");
        break;
    case 38:
    case 39:
    case 40:
        glm_ptr->muta_which = MUTA::ILLUMINE;
        glm_ptr->muta_desc = _("部屋を明るく照らすことが出来なくなった。", "You can no longer light up rooms with your presence.");
        break;
    case 41:
    case 42:
        glm_ptr->muta_which = MUTA::DET_CURSE;
        glm_ptr->muta_desc = _("邪悪な魔法を感じられなくなった。", "You can no longer feel evil magics.");
        break;
    case 43:
    case 44:
    case 45:
        glm_ptr->muta_which = MUTA::BERSERK;
        glm_ptr->muta_desc = _("制御できる激情を感じなくなった。", "You no longer feel a controlled rage.");
        break;
    case 46:
        glm_ptr->muta_which = MUTA::POLYMORPH;
        glm_ptr->muta_desc = _("あなたの体は安定したように見える。", "Your body seems stable.");
        break;
    case 47:
    case 48:
        glm_ptr->muta_which = MUTA::MIDAS_TCH;
        glm_ptr->muta_desc = _("ミダスの手の能力を失った。", "You lose the Midas touch.");
        break;
    case 49:
        glm_ptr->muta_which = MUTA::GROW_MOLD;
        glm_ptr->muta_desc = _("突然カビが嫌いになった。", "You feel a sudden dislike for mold.");
        break;
    case 50:
    case 51:
    case 52:
        glm_ptr->muta_which = MUTA::RESIST;
        glm_ptr->muta_desc = _("傷つき易くなった気がする。", "You feel like you might be vulnerable.");
        break;
    case 53:
    case 54:
    case 55:
        glm_ptr->muta_which = MUTA::EARTHQUAKE;
        glm_ptr->muta_desc = _("ダンジョンを壊す能力を失った。", "You lose the ability to wreck the dungeon.");
        break;
    case 56:
        glm_ptr->muta_which = MUTA::EAT_MAGIC;
        glm_ptr->muta_desc = _("魔法のアイテムはもう美味しそうに見えなくなった。", "Your magic items no longer look delicious.");
        break;
    case 57:
    case 58:
        glm_ptr->muta_which = MUTA::WEIGH_MAG;
        glm_ptr->muta_desc = _("魔力を感じられなくなった。", "You no longer sense magic.");
        break;
    case 59:
        glm_ptr->muta_which = MUTA::STERILITY;
        glm_ptr->muta_desc = _("たくさんの安堵の吐息が聞こえた。", "You hear a massed sigh of relief.");
        break;
    case 60:
    case 61:
        glm_ptr->muta_which = MUTA::HIT_AND_AWAY;
        glm_ptr->muta_desc = _("あちこちへ跳べる気分がなくなった。", "You no longer feel jumpy.");
        break;
    case 62:
    case 63:
    case 64:
        glm_ptr->muta_which = MUTA::DAZZLE;
        glm_ptr->muta_desc = _("まばゆい閃光を発する能力を失った。", "You lose the ability to emit dazzling lights.");
        break;
    case 65:
    case 66:
    case 67:
        glm_ptr->muta_which = MUTA::LASER_EYE;
        glm_ptr->muta_desc = _("眼が少しの間焼き付いて、痛みが和らいだ。", "Your eyes burn for a moment, then feel soothed.");
        break;
    case 68:
    case 69:
        glm_ptr->muta_which = MUTA::RECALL;
        glm_ptr->muta_desc = _("少しの間ホームシックになった。", "You feel briefly homesick.");
        break;
    case 70:
        glm_ptr->muta_which = MUTA::BANISH;
        glm_ptr->muta_desc = _("神聖な怒りの力を感じなくなった。", "You no longer feel a holy wrath.");
        break;
    case 71:
    case 72:
        glm_ptr->muta_which = MUTA::COLD_TOUCH;
        glm_ptr->muta_desc = _("手が暖かくなった。", "Your hands warm up.");
        break;
    case 73:
    case 74:
        glm_ptr->muta_which = MUTA::LAUNCHER;
        glm_ptr->muta_desc = _("物を投げる手が弱くなった気がする。", "Your throwing arm feels much weaker.");
        break;
    case 75:
        glm_ptr->muta_which = MUTA::BERS_RAGE;
        glm_ptr->muta_desc = _("凶暴化の発作にさらされなくなった！", "You are no longer subject to fits of berserk rage!");
        break;
    case 76:
        glm_ptr->muta_which = MUTA::COWARDICE;
        glm_ptr->muta_desc = _("もう信じがたいほど臆病ではなくなった！", "You are no longer an incredible coward!");
        break;
    case 77:
        glm_ptr->muta_which = MUTA::RTELEPORT;
        glm_ptr->muta_desc = _("あなたの位置はより確定的になった。", "Your position seems more certain.");
        break;
    case 78:
        glm_ptr->muta_which = MUTA::ALCOHOL;
        glm_ptr->muta_desc = _("あなたはアルコールを分泌しなくなった！", "Your body stops producing alcohol!");
        break;
    case 79:
        glm_ptr->muta_which = MUTA::HALLU;
        glm_ptr->muta_desc = _("幻覚をひき起こす精神障害を起こさなくなった！", "You are no longer afflicted by a hallucinatory insanity!");
        break;
    case 80:
        glm_ptr->muta_which = MUTA::FLATULENT;
        glm_ptr->muta_desc = _("もう強烈な屁はこかなくなった。", "You are no longer subject to uncontrollable flatulence.");
        break;
    case 81:
    case 82:
        glm_ptr->muta_which = MUTA::SCOR_TAIL;
        glm_ptr->muta_desc = _("サソリの尻尾がなくなった！", "You lose your scorpion tail!");
        break;
    case 83:
    case 84:
        glm_ptr->muta_which = MUTA::HORNS;
        glm_ptr->muta_desc = _("額から角が消えた！", "Your horns vanish from your forehead!");
        break;
    case 85:
    case 86:
        glm_ptr->muta_which = MUTA::BEAK;
        glm_ptr->muta_desc = _("口が普通に戻った！", "Your mouth reverts to normal!");
        break;
    case 87:
    case 88:
        glm_ptr->muta_which = MUTA::ATT_DEMON;
        glm_ptr->muta_desc = _("デーモンを引き寄せなくなった。", "You stop attracting demons.");
        break;
    case 89:
        glm_ptr->muta_which = MUTA::PROD_MANA;
        glm_ptr->muta_desc = _("制御不能な魔法のエネルギーを発生しなくなった。", "You stop producing magical energy uncontrollably.");
        break;
    case 90:
    case 91:
        glm_ptr->muta_which = MUTA::SPEED_FLUX;
        glm_ptr->muta_desc = _("躁鬱質でなくなった。", "You are no longer manic-depressive.");
        break;
    case 92:
    case 93:
        glm_ptr->muta_which = MUTA::BANISH_ALL;
        glm_ptr->muta_desc = _("背後に恐ろしい力を感じなくなった。", "You no longer feel a terrifying power lurking behind you.");
        break;
    case 94:
        glm_ptr->muta_which = MUTA::EAT_LIGHT;
        glm_ptr->muta_desc = _("世界が明るいと感じる。", "You feel the world's a brighter place.");
        break;
    case 95:
    case 96:
        glm_ptr->muta_which = MUTA::TRUNK;
        glm_ptr->muta_desc = _("鼻が普通の長さに戻った。", "Your nose returns to a normal length.");
        break;
    case 97:
        glm_ptr->muta_which = MUTA::ATT_ANIMAL;
        glm_ptr->muta_desc = _("動物を引き寄せなくなった。", "You stop attracting animals.");
        break;
    case 98:
        glm_ptr->muta_which = MUTA::TENTACLES;
        glm_ptr->muta_desc = _("触手が消えた。", "Your tentacles vanish from your sides.");
        break;
    case 99:
        glm_ptr->muta_which = MUTA::RAW_CHAOS;
        glm_ptr->muta_desc = _("周囲の空間が安定した気がする。", "You feel the universe is more stable around you.");
        break;
    case 100:
    case 101:
    case 102:
        glm_ptr->muta_which = MUTA::NORMALITY;
        glm_ptr->muta_desc = _("普通に奇妙な感じがする。", "You feel normally strange.");
        break;
    case 103:
        glm_ptr->muta_which = MUTA::WRAITH;
        glm_ptr->muta_desc = _("あなたは物質世界にしっかり存在している。", "You are firmly in the physical world.");
        break;
    case 104:
        glm_ptr->muta_which = MUTA::POLY_WOUND;
        glm_ptr->muta_desc = _("古い傷からカオスの力が去っていった。", "You feel forces of chaos departing your old scars.");
        break;
    case 105:
        glm_ptr->muta_which = MUTA::WASTING;
        glm_ptr->muta_desc = _("おぞましい衰弱病が治った！", "You are cured of the horrible wasting disease!");
        break;
    case 106:
        glm_ptr->muta_which = MUTA::ATT_DRAGON;
        glm_ptr->muta_desc = _("ドラゴンを引き寄せなくなった。", "You stop attracting dragons.");
        break;
    case 107:
    case 108:
        glm_ptr->muta_which = MUTA::WEIRD_MIND;
        glm_ptr->muta_desc = _("思考が退屈な方向に戻った。", "Your thoughts return to boring paths.");
        break;
    case 109:
        glm_ptr->muta_which = MUTA::NAUSEA;
        glm_ptr->muta_desc = _("胃が痙攣しなくなった。", "Your stomach stops roiling.");
        break;
    case 110:
    case 111:
        glm_ptr->muta_which = MUTA::CHAOS_GIFT;
        glm_ptr->muta_desc = _("混沌の神々の興味を惹かなくなった。", "You lose the attention of the chaos deities.");
        break;
    case 112:
        glm_ptr->muta_which = MUTA::WALK_SHAD;
        glm_ptr->muta_desc = _("物質世界に捕らわれている気がする。", "You feel like you're trapped in reality.");
        break;
    case 113:
    case 114:
        glm_ptr->muta_which = MUTA::WARNING;
        glm_ptr->muta_desc = _("パラノイアでなくなった。", "You no longer feel paranoid.");
        break;
    case 115:
        glm_ptr->muta_which = MUTA::INVULN;
        glm_ptr->muta_desc = _("無敵状態の発作を起こさなくなった。", "You are no longer blessed with fits of invulnerability.");
        break;
    case 116:
    case 117:
        glm_ptr->muta_which = MUTA::SP_TO_HP;
        glm_ptr->muta_desc = _("魔法の治癒の発作に襲われなくなった。", "You are no longer subject to fits of magical healing.");
        break;
    case 118:
        glm_ptr->muta_which = MUTA::HP_TO_SP;
        glm_ptr->muta_desc = _("痛みを伴う精神明瞭化の発作に襲われなくなった。", "You are no longer subject to fits of painful clarity.");
        break;
    case 119:
        glm_ptr->muta_which = MUTA::DISARM;
        glm_ptr->muta_desc = _("脚が元の大きさに戻った。", "Your feet shrink to their former size.");
        break;
    case 120:
    case 121:
    case 122:
        glm_ptr->muta_which = MUTA::HYPER_STR;
        glm_ptr->muta_desc = _("筋肉が普通に戻った。", "Your muscles revert to normal.");
        break;
    case 123:
    case 124:
    case 125:
        glm_ptr->muta_which = MUTA::PUNY;
        glm_ptr->muta_desc = _("筋肉が普通に戻った。", "Your muscles revert to normal.");
        break;
    case 126:
    case 127:
    case 128:
        glm_ptr->muta_which = MUTA::HYPER_INT;
        glm_ptr->muta_desc = _("脳が普通に戻った。", "Your brain reverts to normal.");
        break;
    case 129:
    case 130:
    case 131:
        glm_ptr->muta_which = MUTA::MORONIC;
        glm_ptr->muta_desc = _("脳が普通に戻った。", "Your brain reverts to normal.");
        break;
    case 132:
    case 133:
        glm_ptr->muta_which = MUTA::RESILIENT;
        glm_ptr->muta_desc = _("普通の丈夫さに戻った。", "You become ordinarily resilient again.");
        break;
    case 134:
    case 135:
        glm_ptr->muta_which = MUTA::XTRA_FAT;
        glm_ptr->muta_desc = _("奇跡的なダイエットに成功した！", "You benefit from a miracle diet!");
        break;
    case 136:
    case 137:
        glm_ptr->muta_which = MUTA::ALBINO;
        glm_ptr->muta_desc = _("アルビノでなくなった！", "You are no longer an albino!");
        break;
    case 138:
    case 139:
    case 140:
        glm_ptr->muta_which = MUTA::FLESH_ROT;
        glm_ptr->muta_desc = _("肉体を腐敗させる病気が治った！", "Your flesh is no longer afflicted by a rotting disease!");
        break;
    case 141:
    case 142:
        glm_ptr->muta_which = MUTA::SILLY_VOI;
        glm_ptr->muta_desc = _("声質が普通に戻った。", "Your voice returns to normal.");
        break;
    case 143:
    case 144:
        glm_ptr->muta_which = MUTA::BLANK_FAC;
        glm_ptr->muta_desc = _("顔に目鼻が戻った。", "Your facial features return.");
        break;
    case 145:
        glm_ptr->muta_which = MUTA::ILL_NORM;
        glm_ptr->muta_desc = _("心が安らぐ幻影を映し出さなくなった。", "You stop projecting a reassuring image.");
        break;
    case 146:
    case 147:
    case 148:
        glm_ptr->muta_which = MUTA::XTRA_EYES;
        glm_ptr->muta_desc = _("余分な目が消えてしまった！", "Your extra eyes vanish!");
        break;
    case 149:
    case 150:
        glm_ptr->muta_which = MUTA::MAGIC_RES;
        glm_ptr->muta_desc = _("魔法に弱くなった。", "You become susceptible to magic again.");
        break;
    case 151:
    case 152:
    case 153:
        glm_ptr->muta_which = MUTA::XTRA_NOIS;
        glm_ptr->muta_desc = _("奇妙な音を立てなくなった！", "You stop making strange noise!");
        break;
    case 154:
    case 155:
    case 156:
        glm_ptr->muta_which = MUTA::INFRAVIS;
        glm_ptr->muta_desc = _("赤外線視力が落ちた。", "Your infravision is degraded.");
        break;
    case 157:
    case 158:
        glm_ptr->muta_which = MUTA::XTRA_LEGS;
        glm_ptr->muta_desc = _("余分な脚が消えてしまった！", "Your extra legs disappear!");
        break;
    case 159:
    case 160:
        glm_ptr->muta_which = MUTA::SHORT_LEG;
        glm_ptr->muta_desc = _("脚の長さが普通に戻った。", "Your legs lengthen to normal.");
        break;
    case 161:
    case 162:
        glm_ptr->muta_which = MUTA::ELEC_TOUC;
        glm_ptr->muta_desc = _("体を電流が流れなくなった。", "Electricity stops running through you.");
        break;
    case 163:
    case 164:
        glm_ptr->muta_which = MUTA::FIRE_BODY;
        glm_ptr->muta_desc = _("体が炎に包まれなくなった。", "Your body is no longer enveloped in flames.");
        break;
    case 165:
    case 166:
    case 167:
        glm_ptr->muta_which = MUTA::WART_SKIN;
        glm_ptr->muta_desc = _("イボイボが消えた！", "Your warts disappear!");
        break;
    case 168:
    case 169:
    case 170:
        glm_ptr->muta_which = MUTA::SCALES;
        glm_ptr->muta_desc = _("鱗が消えた！", "Your scales vanish!");
        break;
    case 171:
    case 172:
        glm_ptr->muta_which = MUTA::IRON_SKIN;
        glm_ptr->muta_desc = _("肌が肉にもどった！", "Your skin reverts to flesh!");
        break;
    case 173:
    case 174:
        glm_ptr->muta_which = MUTA::WINGS;
        glm_ptr->muta_desc = _("背中の羽根が取れ落ちた。", "Your wings fall off.");
        break;
    case 175:
    case 176:
    case 177:
        glm_ptr->muta_which = MUTA::FEARLESS;
        glm_ptr->muta_desc = _("再び恐怖を感じるようになった。", "You begin to feel fear again.");
        break;
    case 178:
    case 179:
        glm_ptr->muta_which = MUTA::REGEN;
        glm_ptr->muta_desc = _("急速回復しなくなった。", "You stop regenerating.");
        break;
    case 180:
    case 181:
        glm_ptr->muta_which = MUTA::ESP;
        glm_ptr->muta_desc = _("テレパシーの能力を失った！", "You lose your telepathic ability!");
        break;
    case 182:
    case 183:
    case 184:
        glm_ptr->muta_which = MUTA::LIMBER;
        glm_ptr->muta_desc = _("筋肉が硬くなった。", "Your muscles stiffen.");
        break;
    case 185:
    case 186:
    case 187:
        glm_ptr->muta_which = MUTA::ARTHRITIS;
        glm_ptr->muta_desc = _("関節が痛くなくなった。", "Your joints stop hurting.");
        break;
    case 188:
        glm_ptr->muta_which = MUTA::BAD_LUCK;
        glm_ptr->muta_desc = _("黒いオーラは渦巻いて消えた。", "Your black aura swirls and fades.");
        break;
    case 189:
        glm_ptr->muta_which = MUTA::VULN_ELEM;
        glm_ptr->muta_desc = _("無防備な感じはなくなった。", "You feel less exposed.");
        break;
    case 190:
    case 191:
    case 192:
        glm_ptr->muta_which = MUTA::MOTION;
        glm_ptr->muta_desc = _("動作の正確さがなくなった。", "You move with less assurance.");
        break;
    case 193:
        if (player_ptr->ppersonality == PERSONALITY_LUCKY)
            break;

        glm_ptr->muta_which = MUTA::GOOD_LUCK;
        glm_ptr->muta_desc = _("白いオーラは輝いて消えた。", "Your white aura shimmers and fades.");
        break;
    default:
        glm_ptr->muta_which = MUTA::MAX;
        break;
    }
}
