/*!
 * @brief 自己分析処理/ Self knowledge
 * @date 2018/09/07
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * </pre>
 * 2018 Deskull
 * @details
 * spell2s.cから分離
 */

#include "player/selfinfo.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object/object-flags.h"
#include "player/attack-defense-types.h"
#include "player/avatar.h"
#include "player/player-class.h"
#include "player/player-race-types.h"
#include "player/player-race.h"
#include "player/player-status-flags.h"
#include "realm/realm-names-table.h"
#include "realm/realm-song-numbers.h"
#include "status/element-resistance.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 自己分析処理(Nethackからのアイデア) / self-knowledge... idea from nethack.
 * @return なし
 * @details
 * <pre>
 * Useful for determining powers and
 * resistences of items.  It saves the screen, clears it, then starts listing
 * attributes, a screenful at a time.  (There are a LOT of attributes to
 * list.  It will probably take 2 or 3 screens for a powerful character whose
 * using several artifacts...) -CFT
 *
 * It is now a lot more efficient. -BEN-
 *
 * See also "identify_fully()".
 *
 * Use the "show_file()" method, perhaps.
 * </pre>
 */
void self_knowledge(player_type *creature_ptr)
{
    int line = 0;
    char v_string[8][128];
    char s_string[6][128];
    BIT_FLAGS flags[TR_FLAG_SIZE];
    object_type *o_ptr;
    char plev_buf[80];
    char buf[2][80];
    concptr info[220];
    for (int j = 0; j < TR_FLAG_SIZE; j++)
        flags[j] = 0L;

    creature_ptr->knowledge |= (KNOW_STAT | KNOW_HPRATE);
    strcpy(plev_buf, "");
    int percent
        = (int)(((long)creature_ptr->player_hp[PY_MAX_LEVEL - 1] * 200L) / (2 * creature_ptr->hitdie + ((PY_MAX_LEVEL - 1 + 3) * (creature_ptr->hitdie + 1))));

    sprintf(plev_buf, _("現在の体力ランク : %d/100", "Your current Life Rating is %d/100."), percent);
    strcpy(buf[0], plev_buf);
    info[line++] = buf[0];
    info[line++] = "";
    chg_virtue(creature_ptr, V_KNOWLEDGE, 1);
    chg_virtue(creature_ptr, V_ENLIGHTEN, 1);
    for (int k = INVEN_RARM; k < INVEN_TOTAL; k++) {
        u32b tflgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[k];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, tflgs);
        for (int j = 0; j < TR_FLAG_SIZE; j++)
            flags[j] |= tflgs[j];
    }

    info[line++] = _("能力の最大値", "Limits of maximum stats");
    for (int v_nr = 0; v_nr < A_MAX; v_nr++) {
        char stat_desc[80];
        sprintf(stat_desc, "%s 18/%d", stat_names[v_nr], creature_ptr->stat_max_max[v_nr] - 18);
        strcpy(s_string[v_nr], stat_desc);
        info[line++] = s_string[v_nr];
    }

    info[line++] = "";
    sprintf(plev_buf, _("現在の属性 : %s(%ld)", "Your alignment : %s(%ld)"), your_alignment(creature_ptr), (long int)creature_ptr->align);
    strcpy(buf[1], plev_buf);
    info[line++] = buf[1];
    for (int v_nr = 0; v_nr < 8; v_nr++) {
        GAME_TEXT vir_name[20];
        char vir_desc[80];
        int tester = creature_ptr->virtues[v_nr];
        strcpy(vir_name, virtue[(creature_ptr->vir_types[v_nr]) - 1]);
        sprintf(vir_desc, _("おっと。%sの情報なし。", "Oops. No info about %s."), vir_name);
        if (tester < -100)
            sprintf(vir_desc, _("[%s]の対極 (%d)", "You are the polar opposite of %s (%d)."), vir_name, tester);
        else if (tester < -80)
            sprintf(vir_desc, _("[%s]の大敵 (%d)", "You are an arch-enemy of %s (%d)."), vir_name, tester);
        else if (tester < -60)
            sprintf(vir_desc, _("[%s]の強敵 (%d)", "You are a bitter enemy of %s (%d)."), vir_name, tester);
        else if (tester < -40)
            sprintf(vir_desc, _("[%s]の敵 (%d)", "You are an enemy of %s (%d)."), vir_name, tester);
        else if (tester < -20)
            sprintf(vir_desc, _("[%s]の罪者 (%d)", "You have sinned against %s (%d)."), vir_name, tester);
        else if (tester < 0)
            sprintf(vir_desc, _("[%s]の迷道者 (%d)", "You have strayed from the path of %s (%d)."), vir_name, tester);
        else if (tester == 0)
            sprintf(vir_desc, _("[%s]の中立者 (%d)", "You are neutral to %s (%d)."), vir_name, tester);
        else if (tester < 20)
            sprintf(vir_desc, _("[%s]の小徳者 (%d)", "You are somewhat virtuous in %s (%d)."), vir_name, tester);
        else if (tester < 40)
            sprintf(vir_desc, _("[%s]の中徳者 (%d)", "You are virtuous in %s (%d)."), vir_name, tester);
        else if (tester < 60)
            sprintf(vir_desc, _("[%s]の高徳者 (%d)", "You are very virtuous in %s (%d)."), vir_name, tester);
        else if (tester < 80)
            sprintf(vir_desc, _("[%s]の覇者 (%d)", "You are a champion of %s (%d)."), vir_name, tester);
        else if (tester < 100)
            sprintf(vir_desc, _("[%s]の偉大な覇者 (%d)", "You are a great champion of %s (%d)."), vir_name, tester);
        else
            sprintf(vir_desc, _("[%s]の具現者 (%d)", "You are the living embodiment of %s (%d)."), vir_name, tester);

        strcpy(v_string[v_nr], vir_desc);
        info[line++] = v_string[v_nr];
    }

    info[line++] = "";
    if (creature_ptr->mimic_form) {
        switch (creature_ptr->mimic_form) {
        case MIMIC_DEMON:
        case MIMIC_DEMON_LORD:
            sprintf(plev_buf, _("あなたは %d ダメージの地獄か火炎のブレスを吐くことができる。(%d MP)", "You can nether breathe, dam. %d (cost %d)."), 3 * creature_ptr->lev,
                10 + creature_ptr->lev / 3);

            info[line++] = plev_buf;
            break;
        case MIMIC_VAMPIRE:
            if (creature_ptr->lev > 1) {
                sprintf(plev_buf, _("あなたは敵から %d-%d HP の生命力を吸収できる。(%d MP)", "You can steal life from a foe, dam. %d-%d (cost %d)."),
                    creature_ptr->lev + MAX(1, creature_ptr->lev / 10), creature_ptr->lev + creature_ptr->lev * MAX(1, creature_ptr->lev / 10), 1 + (creature_ptr->lev / 3));
                info[line++] = plev_buf;
            }
            break;
        }
    } else {
        switch (creature_ptr->prace) {
        case RACE_NIBELUNG:
        case RACE_DWARF:
            if (creature_ptr->lev > 4)
                info[line++] = _("あなたは罠とドアと階段を感知できる。(5 MP)", "You can find traps, doors and stairs (cost 5).");
            break;
        case RACE_HOBBIT:
            if (creature_ptr->lev > 14) {
                info[line++] = _("あなたは食料を生成できる。(10 MP)", "You can produce food (cost 10).");
            }
            break;
        case RACE_GNOME:
            if (creature_ptr->lev > 4) {
                sprintf(plev_buf, _("あなたは範囲 %d 以内にテレポートできる。(%d MP)", "You can teleport, range %d (cost %d)."), (1 + creature_ptr->lev), (5 + (creature_ptr->lev / 5)));
                info[line++] = plev_buf;
            }
            break;
        case RACE_HALF_ORC:
            if (creature_ptr->lev > 2)
                info[line++] = _("あなたは恐怖を除去できる。(5 MP)", "You can remove fear (cost 5).");
            break;
        case RACE_HALF_TROLL:
            if (creature_ptr->lev > 9)
                info[line++] = _("あなたは狂暴化することができる。(12 MP) ", "You can enter a berserk fury (cost 12).");
            break;
        case RACE_AMBERITE:
            if (creature_ptr->lev > 29)
                info[line++] = _("あなたはシャドウシフトすることができる。(50 MP)", "You can Shift Shadows (cost 50).");

            if (creature_ptr->lev > 39)
                info[line++] = _("あなたは「パターン」を心に描いて歩くことができる。(75 MP)", "You can mentally Walk the Pattern (cost 75).");

            break;
        case RACE_BARBARIAN:
            if (creature_ptr->lev > 7)
                info[line++] = _("あなたは狂暴化することができる。(10 MP) ", "You can enter a berserk fury (cost 10).");

            break;
        case RACE_HALF_OGRE:
            if (creature_ptr->lev > 24)
                info[line++] = _("あなたは爆発のルーンを仕掛けることができる。(35 MP)", "You can set an Explosive Rune (cost 35).");

            break;
        case RACE_HALF_GIANT:
            if (creature_ptr->lev > 19)
                info[line++] = _("あなたは石の壁を壊すことができる。(10 MP)", "You can break stone walls (cost 10).");
            break;
        case RACE_HALF_TITAN:
            if (creature_ptr->lev > 34)
                info[line++] = _("あなたはモンスターをスキャンすることができる。(20 MP)", "You can probe monsters (cost 20).");
            break;
        case RACE_CYCLOPS:
            if (creature_ptr->lev > 19) {
                sprintf(plev_buf, _("あなたは %d ダメージの岩石を投げることができる。(15 MP)", "You can throw a boulder, dam. %d (cost 15)."), 3 * creature_ptr->lev);
                info[line++] = plev_buf;
            }
            break;
        case RACE_YEEK:
            if (creature_ptr->lev > 14)
                info[line++] = _("あなたは恐怖を呼び起こす叫び声を発することができる。(15 MP)", "You can make a terrifying scream (cost 15).");
            break;
        case RACE_KLACKON:
            if (creature_ptr->lev > 8) {
                sprintf(plev_buf, _("あなたは %d ダメージの酸を吹きかけることができる。(9 MP)", "You can spit acid, dam. %d (cost 9)."), creature_ptr->lev);
                info[line++] = plev_buf;
            }
            break;
        case RACE_KOBOLD:
            if (creature_ptr->lev > 11) {
                sprintf(plev_buf, _("あなたは %d ダメージの毒矢を投げることができる。(8 MP)", "You can throw a dart of poison, dam. %d (cost 8)."), creature_ptr->lev);
                info[line++] = plev_buf;
            }
            break;
        case RACE_DARK_ELF:
            if (creature_ptr->lev > 1) {
                sprintf(plev_buf, _("あなたは %d ダメージのマジック・ミサイルの呪文を使える。(2 MP)", "You can cast a Magic Missile, dam %d (cost 2)."),
                    (3 + ((creature_ptr->lev - 1) / 5)));
                info[line++] = plev_buf;
            }
            break;
        case RACE_DRACONIAN:
            sprintf(plev_buf, _("あなたは %d ダメージのブレスを吐くことができる。(%d MP)", "You can breathe, dam. %d (cost %d)."), 2 * creature_ptr->lev, creature_ptr->lev);
            info[line++] = plev_buf;
            break;
        case RACE_MIND_FLAYER:
            if (creature_ptr->lev > 14)
                sprintf(plev_buf, _("あなたは %d ダメージの精神攻撃をすることができる。(12 MP)", "You can mind blast your enemies, dam %d (cost 12)."), creature_ptr->lev);
            info[line++] = plev_buf;
            break;
        case RACE_IMP:
            if (creature_ptr->lev > 29) {
                sprintf(plev_buf, _("あなたは %d ダメージのファイア・ボールの呪文を使える。(15 MP)", "You can cast a Fire Ball, dam. %d (cost 15)."), creature_ptr->lev);
                info[line++] = plev_buf;
            } else if (creature_ptr->lev > 8) {
                sprintf(plev_buf, _("あなたは %d ダメージのファイア・ボルトの呪文を使える。(15 MP)", "You can cast a Fire Bolt, dam. %d (cost 15)."), creature_ptr->lev);
                info[line++] = plev_buf;
            }
            break;
        case RACE_GOLEM:
            if (creature_ptr->lev > 19)
                info[line++] = _("あなたは d20+30 ターンの間肌を石に変化させられる。(15 MP)", "You can turn your skin to stone, dur d20+30 (cost 15).");
            break;
        case RACE_ZOMBIE:
        case RACE_SKELETON:
            if (creature_ptr->lev > 29)
                info[line++] = _("あなたは失った経験値を回復することができる。(30 MP)", "You can restore lost experience (cost 30).");
            break;
        case RACE_VAMPIRE:
            if (creature_ptr->lev > 1) {
                sprintf(plev_buf, _("あなたは敵から %d-%d HP の生命力を吸収できる。(%d MP)", "You can steal life from a foe, dam. %d-%d (cost %d)."),
                    creature_ptr->lev + MAX(1, creature_ptr->lev / 10), creature_ptr->lev + creature_ptr->lev * MAX(1, creature_ptr->lev / 10), 1 + (creature_ptr->lev / 3));
                info[line++] = plev_buf;
            }
            break;
        case RACE_SPECTRE:
            if (creature_ptr->lev > 3) {
                info[line++] = _("あなたは泣き叫んで敵を恐怖させることができる。(3 MP)", "You can wail to terrify your enemies (cost 3).");
            }
            break;
        case RACE_SPRITE:
            if (creature_ptr->lev > 11) {
                info[line++] = _("あなたは敵を眠らせる魔法の粉を投げることができる。(12 MP)", "You can throw magical dust which induces sleep (cost 12).");
            }
            break;
        case RACE_BALROG:
            sprintf(plev_buf, _("あなたは %d ダメージの地獄か火炎のブレスを吐くことができる。(%d MP)", "You can breathe nether, dam. %d (cost %d)."), 3 * creature_ptr->lev,
                10 + creature_ptr->lev / 3);

            info[line++] = plev_buf;
            break;
        case RACE_KUTAR:
            if (creature_ptr->lev > 19)
                info[line++] = _("あなたは d20+30 ターンの間横に伸びることができる。(15 MP)", "You can expand horizontally, dur d20+30 (cost 15).");
            break;
        case RACE_ANDROID:
            if (creature_ptr->lev < 10)
                sprintf(
                    plev_buf, _("あなたは %d ダメージのレイガンを撃つことができる。(7 MP)", "You can fire a ray gun with damage %d (cost 7)."), (creature_ptr->lev + 1) / 2);
            else if (creature_ptr->lev < 25)
                sprintf(plev_buf, _("あなたは %d ダメージのブラスターを撃つことができる。(13 MP)", "You can fire a blaster with damage %d (cost 13)."), creature_ptr->lev);
            else if (creature_ptr->lev < 35)
                sprintf(plev_buf, _("あなたは %d ダメージのバズーカを撃つことができる。(26 MP)", "You can fire a bazooka with damage %d (cost 26)."), creature_ptr->lev * 2);
            else if (creature_ptr->lev < 45)
                sprintf(plev_buf, _("あなたは %d ダメージのビームキャノンを撃つことができる。(40 MP)", "You can fire a beam cannon with damage %d (cost 40)."),
                    creature_ptr->lev * 2);
            else
                sprintf(plev_buf, _("あなたは %d ダメージのロケットを撃つことができる。(60 MP)", "You can fire a rocket with damage %d (cost 60)."), creature_ptr->lev * 5);

            info[line++] = plev_buf;
            break;
        default:
            break;
        }
    }

    switch (creature_ptr->pclass) {
    case CLASS_WARRIOR:
        if (creature_ptr->lev > 39) {
            info[line++] = _("あなたはランダムな方向に対して数回攻撃することができる。(75 MP)", "You can attack some random directions simultaneously (cost 75).");
        }
        break;
    case CLASS_HIGH_MAGE:
        if (creature_ptr->realm1 == REALM_HEX)
            break;
        /* Fall through */
    case CLASS_MAGE:
    case CLASS_SORCERER:
        if (creature_ptr->lev > 24) {
            info[line++] = _("あなたはアイテムの魔力を吸収することができる。(1 MP)", "You can absorb charges from an item (cost 1).");
        }
        break;
    case CLASS_PRIEST:
        if (is_good_realm(creature_ptr->realm1)) {
            if (creature_ptr->lev > 34) {
                info[line++] = _("あなたは武器を祝福することができる。(70 MP)", "You can bless a weapon (cost 70).");
            }
        } else {
            if (creature_ptr->lev > 41) {
                info[line++] = _("あなたは周りのすべてのモンスターを攻撃することができる。(40 MP)", "You can damage all monsters in sight (cost 40).");
            }
        }
        break;
    case CLASS_ROGUE:
        if (creature_ptr->lev > 7) {
            info[line++] = _("あなたは攻撃して即座に逃げることができる。(12 MP)", "You can hit a monster and teleport away simultaneously (cost 12).");
        }
        break;
    case CLASS_RANGER:
        if (creature_ptr->lev > 14) {
            info[line++] = _("あなたは怪物を調査することができる。(20 MP)", "You can probe monsters (cost 20).");
        }
        break;
    case CLASS_PALADIN:
        if (is_good_realm(creature_ptr->realm1)) {
            if (creature_ptr->lev > 29) {
                info[line++] = _("あなたは聖なる槍を放つことができる。(30 MP)", "You can fire a holy spear (cost 30).");
            }
        } else {
            if (creature_ptr->lev > 29) {
                info[line++] = _("あなたは生命力を減少させる槍を放つことができる。(30 MP)", "You can fire a spear which drains vitality (cost 30).");
            }
        }
        break;
    case CLASS_WARRIOR_MAGE:
        if (creature_ptr->lev > 24) {
            info[line++] = _("あなたはＨＰをＭＰに変換することができる。(0 MP)", "You can convert HP to SP (cost 0).");
            info[line++] = _("あなたはＭＰをＨＰに変換することができる。(0 MP)", "You can convert SP to HP (cost 0).");
        }
        break;
    case CLASS_CHAOS_WARRIOR:
        if (creature_ptr->lev > 39) {
            info[line++] = _("あなたは周囲に怪物を惑わす光を発生させることができる。(50 MP)", "You can radiate light which confuses nearby monsters (cost 50).");
        }
        break;
    case CLASS_MONK:
        if (creature_ptr->lev > 24) {
            info[line++] = _("あなたは構えることができる。(0 MP)", "You can assume a special stance (cost 0).");
        }
        if (creature_ptr->lev > 29) {
            info[line++] = _("あなたは通常の2倍の攻撃を行うことができる。(30 MP)", "You can perform two attacks at the same time (cost 30).");
        }
        break;
    case CLASS_MINDCRAFTER:
    case CLASS_FORCETRAINER:
        if (creature_ptr->lev > 14) {
            info[line++] = _("あなたは精神を集中してＭＰを回復させることができる。(0 MP)", "You can concentrate to regenerate your mana (cost 0).");
        }
        break;
    case CLASS_TOURIST:
        info[line++] = _("あなたは写真を撮影することができる。(0 MP)", "You can take a photograph (cost 0).");
        if (creature_ptr->lev > 24) {
            info[line++] = _("あなたはアイテムを完全に鑑定することができる。(20 MP)", "You can *identify* items (cost 20).");
        }
        break;
    case CLASS_IMITATOR:
        if (creature_ptr->lev > 29) {
            info[line++] = _("あなたは怪物の特殊攻撃をダメージ2倍でまねることができる。(100 MP)",
                "You can imitate monster's special attacks with double damage (cost 100).");
        }
        break;
    case CLASS_BEASTMASTER:
        info[line++] = _("あなたは1体の生命のあるモンスターを支配することができる。(レベル/4 MP)", "You can dominate a monster (cost level/4).");
        if (creature_ptr->lev > 29) {
            info[line++] = _("あなたは視界内の生命のあるモンスターを支配することができる。((レベル+20)/2 MP)",
                "You can dominate living monsters in sight (cost (level+20)/4).");
        }
        break;
    case CLASS_MAGIC_EATER:
        info[line++] = _("あなたは杖/魔法棒/ロッドの魔力を自分のものにすることができる。", "You can absorb a staff, wand or rod itself.");
        break;
    case CLASS_RED_MAGE:
        if (creature_ptr->lev > 47) {
            info[line++] = _("あなたは1ターンに2回魔法を唱えることができる。(20 MP)", "You can cast two spells simultaneously (cost 20).");
        }
        break;
    case CLASS_SAMURAI: {
        info[line++] = _("あなたは精神を集中して気合いを溜めることができる。", "You can concentrate to regenerate your mana.");
    }
        if (creature_ptr->lev > 24) {
            info[line++] = _("あなたは特殊な型で構えることができる。", "You can assume a special stance.");
        }
        break;
    case CLASS_BLUE_MAGE:
        info[line++] = _("あなたは相手に使われた魔法を学ぶことができる。", "You can study spells which your enemy casts on you.");
        break;
    case CLASS_CAVALRY:
        if (creature_ptr->lev > 9) {
            info[line++] = _("あなたはモンスターに乗って無理矢理ペットにすることができる。", "You can ride on a hostile monster to forcibly turn it into a pet.");
        }
        break;
    case CLASS_BERSERKER:
        if (creature_ptr->lev > 9) {
            info[line++] = _("あなたは街とダンジョンの間を行き来することができる。", "You can travel between town and the depths.");
        }
        break;
    case CLASS_MIRROR_MASTER:
        info[line++] = _("あなたは鏡を作り出すことができる。(2 MP)", "You can create a Mirror (cost 2).");
        info[line++] = _("あなたは鏡を割ることができる。(0 MP)", "You can break distant Mirrors (cost 0).");
        break;
    case CLASS_NINJA:
        if (creature_ptr->lev > 19) {
            info[line++] = _("あなたは素早く移動することができる。", "You can walk extremely fast.");
        }
        break;
    }

    if (creature_ptr->muta1) {
        if (creature_ptr->muta1 & MUT1_SPIT_ACID) {
            info[line++] = _("あなたは酸を吹きかけることができる。(ダメージ レベルX1)", "You can spit acid (dam lvl).");
        }
        if (creature_ptr->muta1 & MUT1_BR_FIRE) {
            info[line++] = _("あなたは炎のブレスを吐くことができる。(ダメージ レベルX2)", "You can breathe fire (dam lvl * 2).");
        }
        if (creature_ptr->muta1 & MUT1_HYPN_GAZE) {
            info[line++] = _("あなたの睨みは催眠効果をもつ。", "Your gaze is hypnotic.");
        }
        if (creature_ptr->muta1 & MUT1_TELEKINES) {
            info[line++] = _("あなたは念動力をもっている。", "You are telekinetic.");
        }
        if (creature_ptr->muta1 & MUT1_VTELEPORT) {
            info[line++] = _("あなたは自分の意思でテレポートできる。", "You can teleport at will.");
        }
        if (creature_ptr->muta1 & MUT1_MIND_BLST) {
            info[line++] = _("あなたは精神攻撃を行える。(ダメージ 3～12d3)", "You can Mind Blast your enemies (3 to 12d3 dam).");
        }
        if (creature_ptr->muta1 & MUT1_RADIATION) {
            info[line++] = _("あなたは自分の意思で強い放射線を発生することができる。(ダメージ レベルX2)", "You can emit hard radiation at will (dam lvl * 2).");
        }
        if (creature_ptr->muta1 & MUT1_VAMPIRISM) {
            info[line++] = _("あなたは吸血鬼のように敵から生命力を吸収することができる。(ダメージ レベルX2)",
                "Like a vampire, you can drain life from a foe (dam lvl * 2).");
        }
        if (creature_ptr->muta1 & MUT1_SMELL_MET) {
            info[line++] = _("あなたは近くにある貴金属をかぎ分けることができる。", "You can smell nearby precious metal.");
        }
        if (creature_ptr->muta1 & MUT1_SMELL_MON) {
            info[line++] = _("あなたは近くのモンスターの存在をかぎ分けることができる。", "You can smell nearby monsters.");
        }
        if (creature_ptr->muta1 & MUT1_BLINK) {
            info[line++] = _("あなたは短い距離をテレポートできる。", "You can teleport yourself short distances.");
        }
        if (creature_ptr->muta1 & MUT1_EAT_ROCK) {
            info[line++] = _("あなたは硬い岩を食べることができる。", "You can consume solid rock.");
        }
        if (creature_ptr->muta1 & MUT1_SWAP_POS) {
            info[line++] = _("あなたは他の者と場所を入れ替わることができる。", "You can switch locations with another being.");
        }
        if (creature_ptr->muta1 & MUT1_SHRIEK) {
            info[line++] = _("あなたは身の毛もよだつ叫び声を発することができる。(ダメージ レベルX2)", "You can emit a horrible shriek (dam 2 * lvl).");
        }
        if (creature_ptr->muta1 & MUT1_ILLUMINE) {
            info[line++] = _("あなたは明るい光を放つことができる。", "You can emit bright light.");
        }
        if (creature_ptr->muta1 & MUT1_DET_CURSE) {
            info[line++] = _("あなたは邪悪な魔法の危険を感じとることができる。", "You can feel the danger of evil magic.");
        }
        if (creature_ptr->muta1 & MUT1_BERSERK) {
            info[line++] = _("あなたは自分の意思で狂乱戦闘状態になることができる。", "You can drive yourself into a berserk frenzy.");
        }
        if (creature_ptr->muta1 & MUT1_POLYMORPH) {
            info[line++] = _("あなたは自分の意志で変化できる。", "You can polymorph yourself at will.");
        }
        if (creature_ptr->muta1 & MUT1_MIDAS_TCH) {
            info[line++] = _("あなたは通常アイテムを金に変えることができる。", "You can turn ordinary items to gold.");
        }
        if (creature_ptr->muta1 & MUT1_GROW_MOLD) {
            info[line++] = _("あなたは周囲にキノコを生やすことができる。", "You can cause mold to grow near you.");
        }
        if (creature_ptr->muta1 & MUT1_RESIST) {
            info[line++] = _("あなたは元素の攻撃に対して身を硬くすることができる。", "You can harden yourself to the ravages of the elements.");
        }
        if (creature_ptr->muta1 & MUT1_EARTHQUAKE) {
            info[line++] = _("あなたは周囲のダンジョンを崩壊させることができる。", "You can bring down the dungeon around your ears.");
        }
        if (creature_ptr->muta1 & MUT1_EAT_MAGIC) {
            info[line++] = _("あなたは魔法のエネルギーを自分の物として使用できる。", "You can consume magic energy for your own use.");
        }
        if (creature_ptr->muta1 & MUT1_WEIGH_MAG) {
            info[line++] = _("あなたは自分に影響を与える魔法の力を感じることができる。", "You can feel the strength of the magics affecting you.");
        }
        if (creature_ptr->muta1 & MUT1_STERILITY) {
            info[line++] = _("あなたは集団的生殖不能を起こすことができる。", "You can cause mass impotence.");
        }
        if (creature_ptr->muta1 & MUT1_HIT_AND_AWAY) {
            info[line++] = _("あなたは攻撃した後身を守るため逃げることができる。", "You can run for your life after hitting something.");
        }
        if (creature_ptr->muta1 & MUT1_DAZZLE) {
            info[line++] = _("あなたは混乱と盲目を引き起こす放射能を発生することができる。 ", "You can emit confusing, blinding radiation.");
        }
        if (creature_ptr->muta1 & MUT1_LASER_EYE) {
            info[line++] = _("あなたは目からレーザー光線を発することができる。(ダメージ レベルX2)", "Your eyes can fire laser beams (dam 2 * lvl).");
        }
        if (creature_ptr->muta1 & MUT1_RECALL) {
            info[line++] = _("あなたは街とダンジョンの間を行き来することができる。", "You can travel between town and the depths.");
        }
        if (creature_ptr->muta1 & MUT1_BANISH) {
            info[line++] = _("あなたは邪悪なモンスターを地獄に落とすことができる。", "You can send evil creatures directly to Hell.");
        }
        if (creature_ptr->muta1 & MUT1_COLD_TOUCH) {
            info[line++] = _("あなたは敵を触って凍らせることができる。(ダメージ レベルX3)", "You can freeze things with a touch (dam 3 * lvl).");
        }
        if (creature_ptr->muta1 & MUT1_LAUNCHER) {
            info[line++] = _("あなたはアイテムを力強く投げることができる。", "You can hurl objects with great force.");
        }
    }

    if (creature_ptr->muta2) {
        if (creature_ptr->muta2 & MUT2_BERS_RAGE) {
            info[line++] = _("あなたは狂戦士化の発作を起こす。", "You are subject to berserker fits.");
        }
        if (creature_ptr->muta2 & MUT2_COWARDICE) {
            info[line++] = _("あなたは時々臆病になる。", "You are subject to cowardice.");
        }
        if (creature_ptr->muta2 & MUT2_RTELEPORT) {
            info[line++] = _("あなたはランダムにテレポートする。", "You may randomly teleport.");
        }
        if (creature_ptr->muta2 & MUT2_ALCOHOL) {
            info[line++] = _("あなたの体はアルコールを分泌する。", "Your body produces alcohol.");
        }
        if (creature_ptr->muta2 & MUT2_HALLU) {
            info[line++] = _("あなたは幻覚を引き起こす精神錯乱に侵されている。", "You have a hallucinatory insanity.");
        }
        if (creature_ptr->muta2 & MUT2_FLATULENT) {
            info[line++] = _("あなたは制御できない強烈な屁をこく。", "You are subject to uncontrollable flatulence.");
        }
        if (creature_ptr->muta2 & MUT2_PROD_MANA) {
            info[line++] = _("あなたは制御不能な魔法のエネルギーを発している。", "You produce magical energy uncontrollably.");
        }
        if (creature_ptr->muta2 & MUT2_ATT_DEMON) {
            info[line++] = _("あなたはデーモンを引きつける。", "You attract demons.");
        }
        if (creature_ptr->muta2 & MUT2_SCOR_TAIL) {
            info[line++] = _("あなたはサソリの尻尾が生えている。(毒、ダメージ 3d7)", "You have a scorpion tail (poison, 3d7).");
        }
        if (creature_ptr->muta2 & MUT2_HORNS) {
            info[line++] = _("あなたは角が生えている。(ダメージ 2d6)", "You have horns (dam. 2d6).");
        }
        if (creature_ptr->muta2 & MUT2_BEAK) {
            info[line++] = _("あなたはクチバシが生えている。(ダメージ 2d4)", "You have a beak (dam. 2d4).");
        }
        if (creature_ptr->muta2 & MUT2_SPEED_FLUX) {
            info[line++] = _("あなたはランダムに早く動いたり遅く動いたりする。", "You move faster or slower randomly.");
        }
        if (creature_ptr->muta2 & MUT2_BANISH_ALL) {
            info[line++] = _("あなたは時々近くのモンスターを消滅させる。", "You sometimes cause nearby creatures to vanish.");
        }
        if (creature_ptr->muta2 & MUT2_EAT_LIGHT) {
            info[line++] = _("あなたは時々周囲の光を吸収して栄養にする。", "You sometimes feed off of the light around you.");
        }
        if (creature_ptr->muta2 & MUT2_TRUNK) {
            info[line++] = _("あなたは象のような鼻を持っている。(ダメージ 1d4)", "You have an elephantine trunk (dam 1d4).");
        }
        if (creature_ptr->muta2 & MUT2_ATT_ANIMAL) {
            info[line++] = _("あなたは動物を引きつける。", "You attract animals.");
        }
        if (creature_ptr->muta2 & MUT2_TENTACLES) {
            info[line++] = _("あなたは邪悪な触手を持っている。(ダメージ 2d5)", "You have evil looking tentacles (dam 2d5).");
        }
        if (creature_ptr->muta2 & MUT2_RAW_CHAOS) {
            info[line++] = _("あなたはしばしば純カオスに包まれる。", "You occasionally are surrounded with raw chaos.");
        }
        if (creature_ptr->muta2 & MUT2_NORMALITY) {
            info[line++] = _("あなたは変異していたが、回復してきている。", "You may be mutated, but you're recovering.");
        }
        if (creature_ptr->muta2 & MUT2_WRAITH) {
            info[line++] = _("あなたの肉体は幽体化したり実体化したりする。", "You fade in and out of physical reality.");
        }
        if (creature_ptr->muta2 & MUT2_POLY_WOUND) {
            info[line++] = _("あなたの健康はカオスの力に影響を受ける。", "Your health is subject to chaotic forces.");
        }
        if (creature_ptr->muta2 & MUT2_WASTING) {
            info[line++] = _("あなたは衰弱する恐ろしい病気にかかっている。", "You have a horrible wasting disease.");
        }
        if (creature_ptr->muta2 & MUT2_ATT_DRAGON) {
            info[line++] = _("あなたはドラゴンを引きつける。", "You attract dragons.");
        }
        if (creature_ptr->muta2 & MUT2_WEIRD_MIND) {
            info[line++] = _("あなたの精神はランダムに拡大したり縮小したりしている。", "Your mind randomly expands and contracts.");
        }
        if (creature_ptr->muta2 & MUT2_NAUSEA) {
            info[line++] = _("あなたの胃は非常に落ち着きがない。", "You have a seriously upset stomach.");
        }
        if (creature_ptr->muta2 & MUT2_CHAOS_GIFT) {
            info[line++] = _("あなたはカオスの守護悪魔から褒美をうけとる。", "Chaos deities give you gifts.");
        }
        if (creature_ptr->muta2 & MUT2_WALK_SHAD) {
            info[line++] = _("あなたはしばしば他の「影」に迷い込む。", "You occasionally stumble into other shadows.");
        }
        if (creature_ptr->muta2 & MUT2_WARNING) {
            info[line++] = _("あなたは敵に関する警告を感じる。", "You receive warnings about your foes.");
        }
        if (creature_ptr->muta2 & MUT2_INVULN) {
            info[line++] = _("あなたは時々負け知らずな気分になる。", "You occasionally feel invincible.");
        }
        if (creature_ptr->muta2 & MUT2_SP_TO_HP) {
            info[line++] = _("あなたは時々血が筋肉にどっと流れる。", "Your blood sometimes rushes to your muscles.");
        }
        if (creature_ptr->muta2 & MUT2_HP_TO_SP) {
            info[line++] = _("あなたは時々頭に血がどっと流れる。", "Your blood sometimes rushes to your head.");
        }
        if (creature_ptr->muta2 & MUT2_DISARM) {
            info[line++] = _("あなたはよくつまづいて物を落とす。", "You occasionally stumble and drop things.");
        }
    }

    if (creature_ptr->muta3) {
        if (creature_ptr->muta3 & MUT3_HYPER_STR) {
            info[line++] = _("あなたは超人的に強い。(腕力+4)", "You are superhumanly strong (+4 STR).");
        }
        if (creature_ptr->muta3 & MUT3_PUNY) {
            info[line++] = _("あなたは虚弱だ。(腕力-4)", "You are puny (-4 STR).");
        }
        if (creature_ptr->muta3 & MUT3_HYPER_INT) {
            info[line++] = _("あなたの脳は生体コンピュータだ。(知能＆賢さ+4)", "Your brain is a living computer (+4 INT/WIS).");
        }
        if (creature_ptr->muta3 & MUT3_MORONIC) {
            info[line++] = _("あなたは精神薄弱だ。(知能＆賢さ-4)", "You are moronic (-4 INT/WIS).");
        }
        if (creature_ptr->muta3 & MUT3_RESILIENT) {
            info[line++] = _("あなたは非常にタフだ。(耐久+4)", "You are very resilient (+4 CON).");
        }
        if (creature_ptr->muta3 & MUT3_XTRA_FAT) {
            info[line++] = _("あなたは極端に太っている。(耐久+2,スピード-2)", "You are extremely fat (+2 CON, -2 speed).");
        }
        if (creature_ptr->muta3 & MUT3_ALBINO) {
            info[line++] = _("あなたはアルビノだ。(耐久-4)", "You are an albino (-4 CON).");
        }
        if (creature_ptr->muta3 & MUT3_FLESH_ROT) {
            info[line++] = _("あなたの肉体は腐敗している。(耐久-2,魅力-1)", "Your flesh is rotting (-2 CON, -1 CHR).");
        }
        if (creature_ptr->muta3 & MUT3_SILLY_VOI) {
            info[line++] = _("あなたの声は間抜けなキーキー声だ。(魅力-4)", "Your voice is a silly squeak (-4 CHR).");
        }
        if (creature_ptr->muta3 & MUT3_BLANK_FAC) {
            info[line++] = _("あなたはのっぺらぼうだ。(魅力-1)", "Your face is featureless (-1 CHR).");
        }
        if (creature_ptr->muta3 & MUT3_ILL_NORM) {
            info[line++] = _("あなたは幻影に覆われている。", "Your appearance is masked with illusion.");
        }
        if (creature_ptr->muta3 & MUT3_XTRA_EYES) {
            info[line++] = _("あなたは余分に二つの目を持っている。(探索+15)", "You have an extra pair of eyes (+15 search).");
        }
        if (creature_ptr->muta3 & MUT3_MAGIC_RES) {
            info[line++] = _("あなたは魔法への耐性をもっている。", "You are resistant to magic.");
        }
        if (creature_ptr->muta3 & MUT3_XTRA_NOIS) {
            info[line++] = _("あなたは変な音を発している。(隠密-3)", "You make a lot of strange noise (-3 stealth).");
        }
        if (creature_ptr->muta3 & MUT3_INFRAVIS) {
            info[line++] = _("あなたは素晴らしい赤外線視力を持っている。(+3)", "You have remarkable infravision (+3).");
        }
        if (creature_ptr->muta3 & MUT3_XTRA_LEGS) {
            info[line++] = _("あなたは余分に二本の足が生えている。(加速+3)", "You have an extra pair of legs (+3 speed).");
        }
        if (creature_ptr->muta3 & MUT3_SHORT_LEG) {
            info[line++] = _("あなたの足は短い突起だ。(加速-3)", "Your legs are short stubs (-3 speed).");
        }
        if (creature_ptr->muta3 & MUT3_ELEC_TOUC) {
            info[line++] = _("あなたの血管には電流が流れている。", "Electricity is running through your veins.");
        }
        if (creature_ptr->muta3 & MUT3_FIRE_BODY) {
            info[line++] = _("あなたの体は炎につつまれている。", "Your body is enveloped in flames.");
        }
        if (creature_ptr->muta3 & MUT3_WART_SKIN) {
            info[line++] = _("あなたの肌はイボに被われている。(魅力-2, AC+5)", "Your skin is covered with warts (-2 CHR, +5 AC).");
        }
        if (creature_ptr->muta3 & MUT3_SCALES) {
            info[line++] = _("あなたの肌は鱗になっている。(魅力-1, AC+10)", "Your skin has turned into scales (-1 CHR, +10 AC).");
        }
        if (creature_ptr->muta3 & MUT3_IRON_SKIN) {
            info[line++] = _("あなたの肌は鉄でできている。(器用-1, AC+25)", "Your skin is made of steel (-1 DEX, +25 AC).");
        }
        if (creature_ptr->muta3 & MUT3_WINGS) {
            info[line++] = _("あなたは羽を持っている。", "You have wings.");
        }
        if (creature_ptr->muta3 & MUT3_FEARLESS) {
            /* Unnecessary */
        }
        if (creature_ptr->muta3 & MUT3_REGEN) {
            /* Unnecessary */
        }
        if (creature_ptr->muta3 & MUT3_ESP) {
            /* Unnecessary */
        }
        if (creature_ptr->muta3 & MUT3_LIMBER) {
            info[line++] = _("あなたの体は非常にしなやかだ。(器用+3)", "Your body is very limber (+3 DEX).");
        }
        if (creature_ptr->muta3 & MUT3_ARTHRITIS) {
            info[line++] = _("あなたはいつも関節に痛みを感じている。(器用-3)", "Your joints ache constantly (-3 DEX).");
        }
        if (creature_ptr->muta3 & MUT3_VULN_ELEM) {
            info[line++] = _("あなたは元素の攻撃に弱い。", "You are susceptible to damage from the elements.");
        }
        if (creature_ptr->muta3 & MUT3_MOTION) {
            info[line++] = _("あなたの動作は正確で力強い。(隠密+1)", "Your movements are precise and forceful (+1 STL).");
        }
        if (have_good_luck(creature_ptr)) {
            info[line++] = _("あなたは白いオーラにつつまれている。", "There is a white aura surrounding you.");
        }
        if (creature_ptr->muta3 & MUT3_BAD_LUCK) {
            info[line++] = _("あなたは黒いオーラにつつまれている。", "There is a black aura surrounding you.");
        }
    }

    if (creature_ptr->blind) {
        info[line++] = _("あなたは目が見えない。", "You cannot see.");
    }
    if (creature_ptr->confused) {
        info[line++] = _("あなたは混乱している。", "You are confused.");
    }
    if (creature_ptr->afraid) {
        info[line++] = _("あなたは恐怖に侵されている。", "You are terrified.");
    }
    if (creature_ptr->cut) {
        info[line++] = _("あなたは出血している。", "You are bleeding.");
    }
    if (creature_ptr->stun) {
        info[line++] = _("あなたはもうろうとしている。", "You are stunned.");
    }
    if (creature_ptr->poisoned) {
        info[line++] = _("あなたは毒に侵されている。", "You are poisoned.");
    }
    if (creature_ptr->image) {
        info[line++] = _("あなたは幻覚を見ている。", "You are hallucinating.");
    }
    if (creature_ptr->cursed & TRC_TY_CURSE) {
        info[line++] = _("あなたは邪悪な怨念に包まれている。", "You carry an ancient foul curse.");
    }
    if (creature_ptr->cursed & TRC_AGGRAVATE) {
        info[line++] = _("あなたはモンスターを怒らせている。", "You aggravate monsters.");
    }
    if (creature_ptr->cursed & TRC_DRAIN_EXP) {
        info[line++] = _("あなたは経験値を吸われている。", "You occasionally lose experience for no reason.");
    }
    if (creature_ptr->cursed & TRC_SLOW_REGEN) {
        info[line++] = _("あなたの回復力は非常に遅い。", "You regenerate slowly.");
    }
    if (creature_ptr->cursed & TRC_ADD_L_CURSE) {
        info[line++] = _("あなたの弱い呪いは増える。", "Your weak curses multiply."); /* 暫定的 -- henkma */
    }
    if (creature_ptr->cursed & TRC_ADD_H_CURSE) {
        info[line++] = _("あなたの強い呪いは増える。", "Your heavy curses multiply."); /* 暫定的 -- henkma */
    }
    if (creature_ptr->cursed & TRC_CALL_ANIMAL) {
        info[line++] = _("あなたは動物に狙われている。", "You attract animals.");
    }
    if (creature_ptr->cursed & TRC_CALL_DEMON) {
        info[line++] = _("あなたは悪魔に狙われている。", "You attract demons.");
    }
    if (creature_ptr->cursed & TRC_CALL_DRAGON) {
        info[line++] = _("あなたはドラゴンに狙われている。", "You attract dragons.");
    }
    if (creature_ptr->cursed & TRC_COWARDICE) {
        info[line++] = _("あなたは時々臆病になる。", "You are subject to cowardice.");
    }
    if (creature_ptr->cursed & TRC_TELEPORT) {
        info[line++] = _("あなたの位置はひじょうに不安定だ。", "Your position is very uncertain.");
    }
    if (creature_ptr->cursed & TRC_LOW_MELEE) {
        info[line++] = _("あなたの武器は攻撃を外しやすい。", "Your weapon causes you to miss blows.");
    }
    if (creature_ptr->cursed & TRC_LOW_AC) {
        info[line++] = _("あなたは攻撃を受けやすい。", "You are subject to be hit.");
    }
    if (creature_ptr->cursed & TRC_LOW_MAGIC) {
        info[line++] = _("あなたは魔法を失敗しやすい。", "Your spells fail more frequently.");
    }
    if (creature_ptr->cursed & TRC_FAST_DIGEST) {
        info[line++] = _("あなたはすぐお腹がへる。", "You have a good appetite.");
    }
    if (creature_ptr->cursed & TRC_DRAIN_HP) {
        info[line++] = _("あなたは体力を吸われている。", "You occasionally lose hit points for no reason.");
    }
    if (creature_ptr->cursed & TRC_DRAIN_MANA) {
        info[line++] = _("あなたは魔力を吸われている。", "You occasionally lose spell points for no reason.");
    }
    if (is_blessed(creature_ptr)) {
        info[line++] = _("あなたは高潔さを感じている。", "You feel rightous.");
    }
    if (is_hero(creature_ptr)) {
        info[line++] = _("あなたはヒーロー気分だ。", "You feel heroic.");
    }
    if (creature_ptr->shero) {
        info[line++] = _("あなたは戦闘狂だ。", "You are in a battle rage.");
    }
    if (creature_ptr->protevil) {
        info[line++] = _("あなたは邪悪なる存在から守られている。", "You are protected from evil.");
    }
    if (creature_ptr->shield) {
        info[line++] = _("あなたは神秘のシールドで守られている。", "You are protected by a mystic shield.");
    }
    if (is_invuln(creature_ptr)) {
        info[line++] = _("あなたは現在傷つかない。", "You are temporarily invulnerable.");
    }
    if (creature_ptr->wraith_form) {
        info[line++] = _("あなたは一時的に幽体化している。", "You are temporarily incorporeal.");
    }
    if (creature_ptr->special_attack & ATTACK_CONFUSE) {
        info[line++] = _("あなたの手は赤く輝いている。", "Your hands are glowing dull red.");
    }
    if (creature_ptr->special_attack & ATTACK_FIRE) {
        info[line++] = _("あなたの手は火炎に覆われている。", "You can strike the enemy with flame.");
    }
    if (creature_ptr->special_attack & ATTACK_COLD) {
        info[line++] = _("あなたの手は冷気に覆われている。", "You can strike the enemy with cold.");
    }
    if (creature_ptr->special_attack & ATTACK_ACID) {
        info[line++] = _("あなたの手は酸に覆われている。", "You can strike the enemy with acid.");
    }
    if (creature_ptr->special_attack & ATTACK_ELEC) {
        info[line++] = _("あなたの手は電撃に覆われている。", "You can strike the enemy with electoric shock.");
    }
    if (creature_ptr->special_attack & ATTACK_POIS) {
        info[line++] = _("あなたの手は毒に覆われている。", "You can strike the enemy with poison.");
    }
    switch (creature_ptr->action) {
    case ACTION_SEARCH:
        info[line++] = _("あなたはひじょうに注意深く周囲を見渡している。", "You are looking around very carefully.");
        break;
    }
    if (creature_ptr->new_spells) {
        info[line++] = _("あなたは呪文や祈りを学ぶことができる。", "You can learn some spells/prayers.");
    }
    if (creature_ptr->word_recall) {
        info[line++] = _("あなたはすぐに帰還するだろう。", "You will soon be recalled.");
    }
    if (creature_ptr->alter_reality) {
        info[line++] = _("あなたはすぐにこの世界を離れるだろう。", "You will soon be altered.");
    }
    if (creature_ptr->see_infra) {
        info[line++] = _("あなたの瞳は赤外線に敏感である。", "Your eyes are sensitive to infrared light.");
    }
    if (creature_ptr->see_inv) {
        info[line++] = _("あなたは透明なモンスターを見ることができる。", "You can see invisible creatures.");
    }
    if (creature_ptr->levitation) {
        info[line++] = _("あなたは飛ぶことができる。", "You can fly.");
    }
    if (creature_ptr->free_act) {
        info[line++] = _("あなたは麻痺知らずの効果を持っている。", "You have free action.");
    }
    if (creature_ptr->regenerate) {
        info[line++] = _("あなたは素早く体力を回復する。", "You regenerate quickly.");
    }
    if (creature_ptr->slow_digest) {
        info[line++] = _("あなたは食欲が少ない。", "Your appetite is small.");
    }
    if (creature_ptr->telepathy) {
        info[line++] = _("あなたはテレパシー能力を持っている。", "You have ESP.");
    }
    if (creature_ptr->esp_animal) {
        info[line++] = _("あなたは自然界の生物の存在を感じる能力を持っている。", "You sense natural creatures.");
    }
    if (creature_ptr->esp_undead) {
        info[line++] = _("あなたはアンデッドの存在を感じる能力を持っている。", "You sense undead.");
    }
    if (creature_ptr->esp_demon) {
        info[line++] = _("あなたは悪魔の存在を感じる能力を持っている。", "You sense demons.");
    }
    if (creature_ptr->esp_orc) {
        info[line++] = _("あなたはオークの存在を感じる能力を持っている。", "You sense orcs.");
    }
    if (creature_ptr->esp_troll) {
        info[line++] = _("あなたはトロルの存在を感じる能力を持っている。", "You sense trolls.");
    }
    if (creature_ptr->esp_giant) {
        info[line++] = _("あなたは巨人の存在を感じる能力を持っている。", "You sense giants.");
    }
    if (creature_ptr->esp_dragon) {
        info[line++] = _("あなたはドラゴンの存在を感じる能力を持っている。", "You sense dragons.");
    }
    if (creature_ptr->esp_human) {
        info[line++] = _("あなたは人間の存在を感じる能力を持っている。", "You sense humans.");
    }
    if (creature_ptr->esp_evil) {
        info[line++] = _("あなたは邪悪な生き物の存在を感じる能力を持っている。", "You sense evil creatures.");
    }
    if (creature_ptr->esp_good) {
        info[line++] = _("あなたは善良な生き物の存在を感じる能力を持っている。", "You sense good creatures.");
    }
    if (creature_ptr->esp_nonliving) {
        info[line++] = _("あなたは活動する無生物体の存在を感じる能力を持っている。", "You sense non-living creatures.");
    }
    if (creature_ptr->esp_unique) {
        info[line++] = _("あなたは特別な強敵の存在を感じる能力を持っている。", "You sense unique monsters.");
    }
    if (creature_ptr->hold_exp) {
        info[line++] = _("あなたは自己の経験値をしっかりと維持する。", "You have a firm hold on your experience.");
    }
    if (creature_ptr->reflect) {
        info[line++] = _("あなたは矢の呪文を反射する。", "You reflect bolt spells.");
    }
    if (creature_ptr->sh_fire) {
        info[line++] = _("あなたは炎のオーラに包まれている。", "You are surrounded with a fiery aura.");
    }
    if (creature_ptr->sh_elec) {
        info[line++] = _("あなたは電気に包まれている。", "You are surrounded with electricity.");
    }
    if (creature_ptr->sh_cold) {
        info[line++] = _("あなたは冷気のオーラに包まれている。", "You are surrounded with an aura of coldness.");
    }
    if (creature_ptr->tim_sh_holy) {
        info[line++] = _("あなたは聖なるオーラに包まれている。", "You are surrounded with a holy aura.");
    }
    if (creature_ptr->tim_sh_touki) {
        info[line++] = _("あなたは闘気のオーラに包まれている。", "You are surrounded with an energy aura.");
    }
    if (creature_ptr->anti_magic) {
        info[line++] = _("あなたは反魔法シールドに包まれている。", "You are surrounded by an anti-magic shell.");
    }
    if (creature_ptr->anti_tele) {
        info[line++] = _("あなたはテレポートできない。", "You cannot teleport.");
    }
    if (creature_ptr->lite) {
        info[line++] = _("あなたの身体は光っている。", "You are carrying a permanent light.");
    }
    if (creature_ptr->warning) {
        info[line++] = _("あなたは行動の前に危険を察知することができる。", "You will be warned before dangerous actions.");
    }
    if (creature_ptr->dec_mana) {
        info[line++] = _("あなたは少ない消費魔力で魔法を唱えることができる。", "You can cast spells with fewer mana points.");
    }
    if (creature_ptr->easy_spell) {
        info[line++] = _("あなたは低い失敗率で魔法を唱えることができる。", "Fail rate of your magic is decreased.");
    }
    if (creature_ptr->heavy_spell) {
        info[line++] = _("あなたは高い失敗率で魔法を唱えなければいけない。", "Fail rate of your magic is increased.");
    }
    if (creature_ptr->mighty_throw) {
        info[line++] = _("あなたは強く物を投げる。", "You can throw objects powerfully.");
    }

    if (creature_ptr->immune_acid) {
        info[line++] = _("あなたは酸に対する完全なる免疫を持っている。", "You are completely immune to acid.");
    } else if (creature_ptr->resist_acid && is_oppose_acid(creature_ptr)) {
        info[line++] = _("あなたは酸への強力な耐性を持っている。", "You resist acid exceptionally well.");
    } else if (creature_ptr->resist_acid || is_oppose_acid(creature_ptr)) {
        info[line++] = _("あなたは酸への耐性を持っている。", "You are resistant to acid.");
    }

    if (creature_ptr->immune_elec) {
        info[line++] = _("あなたは電撃に対する完全なる免疫を持っている。", "You are completely immune to lightning.");
    } else if (creature_ptr->resist_elec && is_oppose_elec(creature_ptr)) {
        info[line++] = _("あなたは電撃への強力な耐性を持っている。", "You resist lightning exceptionally well.");
    } else if (creature_ptr->resist_elec || is_oppose_elec(creature_ptr)) {
        info[line++] = _("あなたは電撃への耐性を持っている。", "You are resistant to lightning.");
    }

    if (is_specific_player_race(creature_ptr, RACE_ANDROID) && !creature_ptr->immune_elec) {
        info[line++] = _("あなたは電撃に弱い。", "You are susceptible to damage from lightning.");
    }

    if (creature_ptr->immune_fire) {
        info[line++] = _("あなたは火に対する完全なる免疫を持っている。", "You are completely immune to fire.");
    } else if (creature_ptr->resist_fire && is_oppose_fire(creature_ptr)) {
        info[line++] = _("あなたは火への強力な耐性を持っている。", "You resist fire exceptionally well.");
    } else if (creature_ptr->resist_fire || is_oppose_fire(creature_ptr)) {
        info[line++] = _("あなたは火への耐性を持っている。", "You are resistant to fire.");
    }

    if (is_specific_player_race(creature_ptr, RACE_ENT) && !creature_ptr->immune_fire) {
        info[line++] = _("あなたは火に弱い。", "You are susceptible to damage from fire.");
    }

    if (creature_ptr->immune_cold) {
        info[line++] = _("あなたは冷気に対する完全なる免疫を持っている。", "You are completely immune to cold.");
    } else if (creature_ptr->resist_cold && is_oppose_cold(creature_ptr)) {
        info[line++] = _("あなたは冷気への強力な耐性を持っている。", "You resist cold exceptionally well.");
    } else if (creature_ptr->resist_cold || is_oppose_cold(creature_ptr)) {
        info[line++] = _("あなたは冷気への耐性を持っている。", "You are resistant to cold.");
    }

    if (creature_ptr->resist_pois && is_oppose_pois(creature_ptr)) {
        info[line++] = _("あなたは毒への強力な耐性を持っている。", "You resist poison exceptionally well.");
    } else if (creature_ptr->resist_pois || is_oppose_pois(creature_ptr)) {
        info[line++] = _("あなたは毒への耐性を持っている。", "You are resistant to poison.");
    }

    if (creature_ptr->resist_lite) {
        info[line++] = _("あなたは閃光への耐性を持っている。", "You are resistant to bright light.");
    }

    if (is_specific_player_race(creature_ptr, RACE_VAMPIRE) || is_specific_player_race(creature_ptr, RACE_S_FAIRY)
        || (creature_ptr->mimic_form == MIMIC_VAMPIRE)) {
        info[line++] = _("あなたは閃光に弱い。", "You are susceptible to damage from bright light.");
    }

    if (is_specific_player_race(creature_ptr, RACE_VAMPIRE) || (creature_ptr->mimic_form == MIMIC_VAMPIRE) || creature_ptr->wraith_form) {
        info[line++] = _("あなたは暗黒に対する完全なる免疫を持っている。", "You are completely immune to darkness.");
    }

    else if (creature_ptr->resist_dark) {
        info[line++] = _("あなたは暗黒への耐性を持っている。", "You are resistant to darkness.");
    }
    if (creature_ptr->resist_conf) {
        info[line++] = _("あなたは混乱への耐性を持っている。", "You are resistant to confusion.");
    }
    if (creature_ptr->resist_sound) {
        info[line++] = _("あなたは音波の衝撃への耐性を持っている。", "You are resistant to sonic attacks.");
    }
    if (creature_ptr->resist_disen) {
        info[line++] = _("あなたは劣化への耐性を持っている。", "You are resistant to disenchantment.");
    }
    if (creature_ptr->resist_chaos) {
        info[line++] = _("あなたはカオスの力への耐性を持っている。", "You are resistant to chaos.");
    }
    if (creature_ptr->resist_shard) {
        info[line++] = _("あなたは破片の攻撃への耐性を持っている。", "You are resistant to blasts of shards.");
    }
    if (creature_ptr->resist_nexus) {
        info[line++] = _("あなたは因果混乱の攻撃への耐性を持っている。", "You are resistant to nexus attacks.");
    }

    if (is_specific_player_race(creature_ptr, RACE_SPECTRE)) {
        info[line++] = _("あなたは地獄の力を吸収できる。", "You can drain nether forces.");
    } else if (creature_ptr->resist_neth) {
        info[line++] = _("あなたは地獄の力への耐性を持っている。", "You are resistant to nether forces.");
    }
    if (creature_ptr->resist_fear) {
        info[line++] = _("あなたは全く恐怖を感じない。", "You are completely fearless.");
    }
    if (creature_ptr->resist_blind) {
        info[line++] = _("あなたの目は盲目への耐性を持っている。", "Your eyes are resistant to blindness.");
    }
    if (creature_ptr->resist_time) {
        info[line++] = _("あなたは時間逆転への耐性を持っている。", "You are resistant to time.");
    }

    if (creature_ptr->sustain_str) {
        info[line++] = _("あなたの腕力は維持されている。", "Your strength is sustained.");
    }
    if (creature_ptr->sustain_int) {
        info[line++] = _("あなたの知能は維持されている。", "Your intelligence is sustained.");
    }
    if (creature_ptr->sustain_wis) {
        info[line++] = _("あなたの賢さは維持されている。", "Your wisdom is sustained.");
    }
    if (creature_ptr->sustain_con) {
        info[line++] = _("あなたの耐久力は維持されている。", "Your constitution is sustained.");
    }
    if (creature_ptr->sustain_dex) {
        info[line++] = _("あなたの器用さは維持されている。", "Your dexterity is sustained.");
    }
    if (creature_ptr->sustain_chr) {
        info[line++] = _("あなたの魅力は維持されている。", "Your charisma is sustained.");
    }

    if (have_flag(flags, TR_STR)) {
        info[line++] = _("あなたの腕力は装備によって影響を受けている。", "Your strength is affected by your equipment.");
    }
    if (have_flag(flags, TR_INT)) {
        info[line++] = _("あなたの知能は装備によって影響を受けている。", "Your intelligence is affected by your equipment.");
    }
    if (have_flag(flags, TR_WIS)) {
        info[line++] = _("あなたの賢さは装備によって影響を受けている。", "Your wisdom is affected by your equipment.");
    }
    if (have_flag(flags, TR_DEX)) {
        info[line++] = _("あなたの器用さは装備によって影響を受けている。", "Your dexterity is affected by your equipment.");
    }
    if (have_flag(flags, TR_CON)) {
        info[line++] = _("あなたの耐久力は装備によって影響を受けている。", "Your constitution is affected by your equipment.");
    }
    if (have_flag(flags, TR_CHR)) {
        info[line++] = _("あなたの魅力は装備によって影響を受けている。", "Your charisma is affected by your equipment.");
    }

    if (have_flag(flags, TR_STEALTH)) {
        info[line++] = _("あなたの隠密行動能力は装備によって影響を受けている。", "Your stealth is affected by your equipment.");
    }
    if (have_flag(flags, TR_SEARCH)) {
        info[line++] = _("あなたの探索能力は装備によって影響を受けている。", "Your searching ability is affected by your equipment.");
    }
    if (have_flag(flags, TR_INFRA)) {
        info[line++] = _("あなたの赤外線視力は装備によって影響を受けている。", "Your infravision is affected by your equipment.");
    }
    if (have_flag(flags, TR_TUNNEL)) {
        info[line++] = _("あなたの採掘能力は装備によって影響を受けている。", "Your digging ability is affected by your equipment.");
    }
    if (have_flag(flags, TR_SPEED)) {
        info[line++] = _("あなたのスピードは装備によって影響を受けている。", "Your speed is affected by your equipment.");
    }
    if (have_flag(flags, TR_BLOWS)) {
        info[line++] = _("あなたの攻撃速度は装備によって影響を受けている。", "Your attack speed is affected by your equipment.");
    }

    /* Access the current weapon */
    o_ptr = &creature_ptr->inventory_list[INVEN_RARM];

    /* Analyze the weapon */
    if (o_ptr->k_idx) {
        /* Indicate Blessing */
        if (have_flag(flags, TR_BLESSED)) {
            info[line++] = _("あなたの武器は神の祝福を受けている。", "Your weapon has been blessed by the gods.");
        }

        if (have_flag(flags, TR_CHAOTIC)) {
            info[line++] = _("あなたの武器はログルスの徴の属性をもつ。", "Your weapon is branded with the Sign of Logrus.");
        }

        if (have_flag(flags, TR_IMPACT)) {
            info[line++] = _("あなたの武器は打撃で地震を発生することができる。", "The impact of your weapon can cause earthquakes.");
        }

        if (have_flag(flags, TR_VORPAL)) {
            info[line++] = _("あなたの武器は非常に鋭い。", "Your weapon is very sharp.");
        }

        if (have_flag(flags, TR_VAMPIRIC)) {
            info[line++] = _("あなたの武器は敵から生命力を吸収する。", "Your weapon drains life from your foes.");
        }

        /* Special "Attack Bonuses" */
        if (have_flag(flags, TR_BRAND_ACID)) {
            info[line++] = _("あなたの武器は敵を溶かす。", "Your weapon melts your foes.");
        }
        if (have_flag(flags, TR_BRAND_ELEC)) {
            info[line++] = _("あなたの武器は敵を感電させる。", "Your weapon shocks your foes.");
        }
        if (have_flag(flags, TR_BRAND_FIRE)) {
            info[line++] = _("あなたの武器は敵を燃やす。", "Your weapon burns your foes.");
        }
        if (have_flag(flags, TR_BRAND_COLD)) {
            info[line++] = _("あなたの武器は敵を凍らせる。", "Your weapon freezes your foes.");
        }
        if (have_flag(flags, TR_BRAND_POIS)) {
            info[line++] = _("あなたの武器は敵を毒で侵す。", "Your weapon poisons your foes.");
        }

        /* Special "slay" flags */
        if (have_flag(flags, TR_KILL_ANIMAL)) {
            info[line++] = _("あなたの武器は動物の天敵である。", "Your weapon is a great bane of animals.");
        } else if (have_flag(flags, TR_SLAY_ANIMAL)) {
            info[line++] = _("あなたの武器は動物に対して強い力を発揮する。", "Your weapon strikes at animals with extra force.");
        }
        if (have_flag(flags, TR_KILL_EVIL)) {
            info[line++] = _("あなたの武器は邪悪なる存在の天敵である。", "Your weapon is a great bane of evil.");
        } else if (have_flag(flags, TR_SLAY_EVIL)) {
            info[line++] = _("あなたの武器は邪悪なる存在に対して強い力を発揮する。", "Your weapon strikes at evil with extra force.");
        }
        if (have_flag(flags, TR_KILL_HUMAN)) {
            info[line++] = _("あなたの武器は人間の天敵である。", "Your weapon is a great bane of humans.");
        } else if (have_flag(flags, TR_SLAY_HUMAN)) {
            info[line++] = _("あなたの武器は人間に対して特に強い力を発揮する。", "Your weapon is especially deadly against humans.");
        }
        if (have_flag(flags, TR_KILL_UNDEAD)) {
            info[line++] = _("あなたの武器はアンデッドの天敵である。", "Your weapon is a great bane of undead.");
        } else if (have_flag(flags, TR_SLAY_UNDEAD)) {
            info[line++] = _("あなたの武器はアンデッドに対して神聖なる力を発揮する。", "Your weapon strikes at undead with holy wrath.");
        }
        if (have_flag(flags, TR_KILL_DEMON)) {
            info[line++] = _("あなたの武器はデーモンの天敵である。", "Your weapon is a great bane of demons.");
        } else if (have_flag(flags, TR_SLAY_DEMON)) {
            info[line++] = _("あなたの武器はデーモンに対して神聖なる力を発揮する。", "Your weapon strikes at demons with holy wrath.");
        }
        if (have_flag(flags, TR_KILL_ORC)) {
            info[line++] = _("あなたの武器はオークの天敵である。", "Your weapon is a great bane of orcs.");
        } else if (have_flag(flags, TR_SLAY_ORC)) {
            info[line++] = _("あなたの武器はオークに対して特に強い力を発揮する。", "Your weapon is especially deadly against orcs.");
        }
        if (have_flag(flags, TR_KILL_TROLL)) {
            info[line++] = _("あなたの武器はトロルの天敵である。", "Your weapon is a great bane of trolls.");
        } else if (have_flag(flags, TR_SLAY_TROLL)) {
            info[line++] = _("あなたの武器はトロルに対して特に強い力を発揮する。", "Your weapon is especially deadly against trolls.");
        }
        if (have_flag(flags, TR_KILL_GIANT)) {
            info[line++] = _("あなたの武器は巨人の天敵である。", "Your weapon is a great bane of giants.");
        } else if (have_flag(flags, TR_SLAY_GIANT)) {
            info[line++] = _("あなたの武器は巨人に対して特に強い力を発揮する。", "Your weapon is especially deadly against giants.");
        }
        /* Special "kill" flags */
        if (have_flag(flags, TR_KILL_DRAGON)) {
            info[line++] = _("あなたの武器はドラゴンの天敵である。", "Your weapon is a great bane of dragons.");
        } else if (have_flag(flags, TR_SLAY_DRAGON)) {
            info[line++] = _("あなたの武器はドラゴンに対して特に強い力を発揮する。", "Your weapon is especially deadly against dragons.");
        }

        if (have_flag(flags, TR_FORCE_WEAPON))
            info[line++] = _("あなたの武器はMPを使って攻撃する。", "Your weapon causes greate damages using your MP.");

        if (have_flag(flags, TR_THROW))
            info[line++] = _("あなたの武器は投げやすい。", "Your weapon can be thrown well.");
    }

    screen_save();
    for (int k = 1; k < 24; k++)
        prt("", k, 13);

    prt(_("        あなたの状態:", "     Your Attributes:"), 1, 15);
    int k = 2;
    for (int j = 0; j < line; j++) {
        prt(info[j], k++, 15);

        /* Every 20 entries (lines 2 to 21), start over */
        if ((k != 22) || (j + 1 >= line))
            continue;

        prt(_("-- 続く --", "-- more --"), k, 15);
        inkey();
        for (; k > 2; k--)
            prt("", k, 15);
    }

    prt(_("[何かキーを押すとゲームに戻ります]", "[Press any key to continue]"), k, 13);
    inkey();
    screen_load();
}

/*!
 * @brief 魔法効果時間のターン数に基づいて表現IDを返す。
 * @param dur 効果ターン数
 * @return 効果時間の表現ID
 */
static int report_magics_aux(int dur)
{
    if (dur <= 5) {
        return 0;
    } else if (dur <= 10) {
        return 1;
    } else if (dur <= 20) {
        return 2;
    } else if (dur <= 50) {
        return 3;
    } else if (dur <= 100) {
        return 4;
    } else if (dur <= 200) {
        return 5;
    } else {
        return 6;
    }
}

static concptr report_magic_durations[] = {
#ifdef JP
    "ごく短い間", "少しの間", "しばらくの間", "多少長い間", "長い間", "非常に長い間", "信じ難いほど長い間", "モンスターを攻撃するまで"
#else
    "for a short time", "for a little while", "for a while", "for a long while", "for a long time", "for a very long time", "for an incredibly long time",
    "until you hit a monster"
#endif

};

/*!
 * @brief 現在の一時的効果一覧を返す / Report all currently active magical effects.
 * @return なし
 */
void report_magics(player_type *creature_ptr)
{
    int i = 0, j, k;
    char Dummy[80];
    concptr info[128];
    int info2[128];

    if (creature_ptr->blind) {
        info2[i] = report_magics_aux(creature_ptr->blind);
        info[i++] = _("あなたは目が見えない", "You cannot see");
    }
    if (creature_ptr->confused) {
        info2[i] = report_magics_aux(creature_ptr->confused);
        info[i++] = _("あなたは混乱している", "You are confused");
    }
    if (creature_ptr->afraid) {
        info2[i] = report_magics_aux(creature_ptr->afraid);
        info[i++] = _("あなたは恐怖に侵されている", "You are terrified");
    }
    if (creature_ptr->poisoned) {
        info2[i] = report_magics_aux(creature_ptr->poisoned);
        info[i++] = _("あなたは毒に侵されている", "You are poisoned");
    }
    if (creature_ptr->image) {
        info2[i] = report_magics_aux(creature_ptr->image);
        info[i++] = _("あなたは幻覚を見ている", "You are hallucinating");
    }
    if (creature_ptr->blessed) {
        info2[i] = report_magics_aux(creature_ptr->blessed);
        info[i++] = _("あなたは高潔さを感じている", "You feel rightous");
    }
    if (creature_ptr->hero) {
        info2[i] = report_magics_aux(creature_ptr->hero);
        info[i++] = _("あなたはヒーロー気分だ", "You feel heroic");
    }
    if (creature_ptr->shero) {
        info2[i] = report_magics_aux(creature_ptr->shero);
        info[i++] = _("あなたは戦闘狂だ", "You are in a battle rage");
    }
    if (creature_ptr->protevil) {
        info2[i] = report_magics_aux(creature_ptr->protevil);
        info[i++] = _("あなたは邪悪なる存在から守られている", "You are protected from evil");
    }
    if (creature_ptr->shield) {
        info2[i] = report_magics_aux(creature_ptr->shield);
        info[i++] = _("あなたは神秘のシールドで守られている", "You are protected by a mystic shield");
    }
    if (creature_ptr->invuln) {
        info2[i] = report_magics_aux(creature_ptr->invuln);
        info[i++] = _("あなたは無敵だ", "You are invulnerable");
    }
    if (creature_ptr->wraith_form) {
        info2[i] = report_magics_aux(creature_ptr->wraith_form);
        info[i++] = _("あなたは幽体化している", "You are incorporeal");
    }
    if (creature_ptr->special_attack & ATTACK_CONFUSE) {
        info2[i] = 7;
        info[i++] = _("あなたの手は赤く輝いている", "Your hands are glowing dull red.");
    }
    if (creature_ptr->word_recall) {
        info2[i] = report_magics_aux(creature_ptr->word_recall);
        info[i++] = _("この後帰還の詔が発動する", "You are waiting to be recalled");
    }
    if (creature_ptr->alter_reality) {
        info2[i] = report_magics_aux(creature_ptr->alter_reality);
        info[i++] = _("この後現実変容が発動する", "You waiting to be altered");
    }
    if (creature_ptr->oppose_acid) {
        info2[i] = report_magics_aux(creature_ptr->oppose_acid);
        info[i++] = _("あなたは酸への耐性を持っている", "You are resistant to acid");
    }
    if (creature_ptr->oppose_elec) {
        info2[i] = report_magics_aux(creature_ptr->oppose_elec);
        info[i++] = _("あなたは電撃への耐性を持っている", "You are resistant to lightning");
    }
    if (creature_ptr->oppose_fire) {
        info2[i] = report_magics_aux(creature_ptr->oppose_fire);
        info[i++] = _("あなたは火への耐性を持っている", "You are resistant to fire");
    }
    if (creature_ptr->oppose_cold) {
        info2[i] = report_magics_aux(creature_ptr->oppose_cold);
        info[i++] = _("あなたは冷気への耐性を持っている", "You are resistant to cold");
    }
    if (creature_ptr->oppose_pois) {
        info2[i] = report_magics_aux(creature_ptr->oppose_pois);
        info[i++] = _("あなたは毒への耐性を持っている", "You are resistant to poison");
    }
    screen_save();

    /* Erase the screen */
    for (k = 1; k < 24; k++)
        prt("", k, 13);

    /* Label the information */
    prt(_("    現在かかっている魔法     :", "     Your Current Magic:"), 1, 15);

    /* We will print on top of the map (column 13) */
    for (k = 2, j = 0; j < i; j++) {
        /* Show the info */
        sprintf(Dummy, _("%-28s : 期間 - %s ", "%s %s."), info[j], report_magic_durations[info2[j]]);
        prt(Dummy, k++, 15);

        /* Every 20 entries (lines 2 to 21), start over */
        if ((k == 22) && (j + 1 < i)) {
            prt(_("-- 続く --", "-- more --"), k, 15);
            inkey();
            for (; k > 2; k--)
                prt("", k, 15);
        }
    }

    /* Pause */
    prt(_("[何かキーを押すとゲームに戻ります]", "[Press any key to continue]"), k, 13);
    inkey();
    screen_load();
}
