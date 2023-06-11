/*!
 * @brief ウルティマ4を参考にした徳のシステムの実装 / Enable an Ultima IV style "avatar" game where you try to achieve perfection in various virtues.
 * @date 2013/12/23
 * @author
 * Topi Ylinen 1998
 * f1toyl@uta.fi
 * topi.ylinen@noodi.fi
 *
 * Copyright (c) 1989 James E. Wilson, Christopher J. Stuart
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "avatar/avatar.h"
#include "game-option/text-display-options.h"
#include "player-info/class-info.h"
#include "player-info/race-types.h"
#include "realm/realm-names-table.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/enum-converter.h"
#include "util/probability-table.h"

/*!
 * 徳の名称 / The names of the virtues
 */
const std::map<Virtue, std::string> virtue_names = {
    { Virtue::NONE, "" },
    { Virtue::COMPASSION, _("情", "Compassion") },
    { Virtue::HONOUR, _("誉", "Honour") },
    { Virtue::JUSTICE, _("正", "Justice") },
    { Virtue::SACRIFICE, _("犠", "Sacrifice") },
    { Virtue::KNOWLEDGE, _("識", "Knowledge") },
    { Virtue::FAITH, _("誠", "Faith") },
    { Virtue::ENLIGHTEN, _("啓", "Enlightenment") },
    { Virtue::ENCHANT, _("秘", "Mysticism") },
    { Virtue::CHANCE, _("運", "Chance") },
    { Virtue::NATURE, _("然", "Nature") },
    { Virtue::HARMONY, _("調", "Harmony") },
    { Virtue::VITALITY, _("活", "Vitality") },
    { Virtue::UNLIFE, _("死", "Unlife") },
    { Virtue::PATIENCE, _("忍", "Patience") },
    { Virtue::TEMPERANCE, _("節", "Temperance") },
    { Virtue::DILIGENCE, _("勤", "Diligence") },
    { Virtue::VALOUR, _("勇", "Valour") },
    { Virtue::INDIVIDUALISM, _("個", "Individualism") },
};

/*!
 * @brief 該当の徳がプレイヤーに指定されているか否かに応じつつ、大小を比較する。
 * @details 徳がない場合は値0として比較する。
 * @param virtue_names 比較したい徳のID
 * @param threshold 比較基準値
 * @return 比較の真偽値を返す
 */
bool compare_virtue(PlayerType *player_ptr, Virtue virtue, int threshold)
{
    const auto num = virtue_number(player_ptr, virtue);
    const auto virtue_value = num ? player_ptr->virtues[num - 1] : 0;
    return virtue_value > threshold;
}

/*!
 * @brief プレイヤーの指定の徳が何番目のスロットに登録されているかを返す。 / Aux function
 * @param virtue_names 確認したい徳のID
 * @return スロットがあるならばスロットのID(0～7)+1、ない場合は0を返す。
 */
int virtue_number(PlayerType *player_ptr, Virtue virtue)
{
    for (int i = 0; i < 8; i++) {
        if (player_ptr->vir_types[i] == virtue) {
            return i + 1;
        }
    }

    return 0;
}

/*!
 * @brief プレイヤーの職業や種族に依存しないランダムな徳を取得する / Aux function
 * @param which 確認したい徳のID
 */
static void get_random_virtue(PlayerType *player_ptr, int which)
{
    ProbabilityTable<Virtue> pt;
    pt.entry_item(Virtue::SACRIFICE, 3);
    pt.entry_item(Virtue::COMPASSION, 3);
    pt.entry_item(Virtue::VALOUR, 6);
    pt.entry_item(Virtue::HONOUR, 5);
    pt.entry_item(Virtue::JUSTICE, 4);
    pt.entry_item(Virtue::TEMPERANCE, 2);
    pt.entry_item(Virtue::HARMONY, 2);
    pt.entry_item(Virtue::PATIENCE, 3);
    pt.entry_item(Virtue::DILIGENCE, 1);

    while (true) {
        const auto type = pt.pick_one_at_random();
        if (virtue_number(player_ptr, type) == 0) {
            player_ptr->vir_types[which] = type;
            return;
        }
    }
}

/*!
 * @brief プレイヤーの選んだ魔法領域に応じて対応する徳を返す。
 * @param realm 魔法領域のID
 * @return 対応する徳のID
 */
static enum Virtue get_realm_virtues(PlayerType *player_ptr, int16_t realm)
{
    switch (realm) {
    case REALM_LIFE:
        if (virtue_number(player_ptr, Virtue::VITALITY)) {
            return Virtue::TEMPERANCE;
        } else {
            return Virtue::VITALITY;
        }
    case REALM_SORCERY:
        if (virtue_number(player_ptr, Virtue::KNOWLEDGE)) {
            return Virtue::ENCHANT;
        } else {
            return Virtue::KNOWLEDGE;
        }
    case REALM_NATURE:
        if (virtue_number(player_ptr, Virtue::NATURE)) {
            return Virtue::HARMONY;
        } else {
            return Virtue::NATURE;
        }
    case REALM_CHAOS:
        if (virtue_number(player_ptr, Virtue::CHANCE)) {
            return Virtue::INDIVIDUALISM;
        } else {
            return Virtue::CHANCE;
        }
    case REALM_DEATH:
        return Virtue::UNLIFE;
    case REALM_TRUMP:
        return Virtue::KNOWLEDGE;
    case REALM_ARCANE:
        return Virtue::NONE;
    case REALM_CRAFT:
        if (virtue_number(player_ptr, Virtue::ENCHANT)) {
            return Virtue::INDIVIDUALISM;
        } else {
            return Virtue::ENCHANT;
        }
    case REALM_DAEMON:
        if (virtue_number(player_ptr, Virtue::JUSTICE)) {
            return Virtue::FAITH;
        } else {
            return Virtue::JUSTICE;
        }
    case REALM_CRUSADE:
        if (virtue_number(player_ptr, Virtue::JUSTICE)) {
            return Virtue::HONOUR;
        } else {
            return Virtue::JUSTICE;
        }
    case REALM_HEX:
        if (virtue_number(player_ptr, Virtue::COMPASSION)) {
            return Virtue::JUSTICE;
        } else {
            return Virtue::COMPASSION;
        }
    default:
        return Virtue::NONE;
    };
}

/*!
 * @brief 作成中のプレイヤーキャラクターに徳8種類を与える。 / Select virtues & reset values for a new character
 * @details 職業に応じて1～4種が固定、種族に応じて1種類が与えられ、後は重複なくランダムに選択される。
 */
void initialize_virtues(PlayerType *player_ptr)
{
    int i = 0, j = 0;
    Virtue tmp_vir;

    /* Reset */
    for (i = 0; i < 8; i++) {
        player_ptr->virtues[i] = 0;
        player_ptr->vir_types[i] = Virtue::NONE;
    }

    i = 0;

    /* Get pre-defined types */
    /* 1 or more virtues based on class */
    switch (player_ptr->pclass) {
    case PlayerClassType::WARRIOR:
    case PlayerClassType::SAMURAI:
        player_ptr->vir_types[i++] = Virtue::VALOUR;
        player_ptr->vir_types[i++] = Virtue::HONOUR;
        break;
    case PlayerClassType::MAGE:
        player_ptr->vir_types[i++] = Virtue::KNOWLEDGE;
        player_ptr->vir_types[i++] = Virtue::ENCHANT;
        break;
    case PlayerClassType::PRIEST:
        player_ptr->vir_types[i++] = Virtue::FAITH;
        player_ptr->vir_types[i++] = Virtue::TEMPERANCE;
        break;
    case PlayerClassType::ROGUE:
    case PlayerClassType::SNIPER:
        player_ptr->vir_types[i++] = Virtue::HONOUR;
        break;
    case PlayerClassType::RANGER:
    case PlayerClassType::ARCHER:
        player_ptr->vir_types[i++] = Virtue::NATURE;
        player_ptr->vir_types[i++] = Virtue::TEMPERANCE;
        break;
    case PlayerClassType::PALADIN:
        player_ptr->vir_types[i++] = Virtue::JUSTICE;
        player_ptr->vir_types[i++] = Virtue::VALOUR;
        player_ptr->vir_types[i++] = Virtue::HONOUR;
        player_ptr->vir_types[i++] = Virtue::FAITH;
        break;
    case PlayerClassType::WARRIOR_MAGE:
    case PlayerClassType::RED_MAGE:
        player_ptr->vir_types[i++] = Virtue::ENCHANT;
        player_ptr->vir_types[i++] = Virtue::VALOUR;
        break;
    case PlayerClassType::CHAOS_WARRIOR:
        player_ptr->vir_types[i++] = Virtue::CHANCE;
        player_ptr->vir_types[i++] = Virtue::INDIVIDUALISM;
        break;
    case PlayerClassType::MONK:
    case PlayerClassType::FORCETRAINER:
        player_ptr->vir_types[i++] = Virtue::FAITH;
        player_ptr->vir_types[i++] = Virtue::HARMONY;
        player_ptr->vir_types[i++] = Virtue::TEMPERANCE;
        player_ptr->vir_types[i++] = Virtue::PATIENCE;
        break;
    case PlayerClassType::MINDCRAFTER:
    case PlayerClassType::MIRROR_MASTER:
        player_ptr->vir_types[i++] = Virtue::HARMONY;
        player_ptr->vir_types[i++] = Virtue::ENLIGHTEN;
        player_ptr->vir_types[i++] = Virtue::PATIENCE;
        break;
    case PlayerClassType::HIGH_MAGE:
    case PlayerClassType::SORCERER:
        player_ptr->vir_types[i++] = Virtue::ENLIGHTEN;
        player_ptr->vir_types[i++] = Virtue::ENCHANT;
        player_ptr->vir_types[i++] = Virtue::KNOWLEDGE;
        break;
    case PlayerClassType::TOURIST:
        player_ptr->vir_types[i++] = Virtue::ENLIGHTEN;
        player_ptr->vir_types[i++] = Virtue::CHANCE;
        break;
    case PlayerClassType::IMITATOR:
        player_ptr->vir_types[i++] = Virtue::CHANCE;
        break;
    case PlayerClassType::BLUE_MAGE:
        player_ptr->vir_types[i++] = Virtue::CHANCE;
        player_ptr->vir_types[i++] = Virtue::KNOWLEDGE;
        break;
    case PlayerClassType::BEASTMASTER:
        player_ptr->vir_types[i++] = Virtue::NATURE;
        player_ptr->vir_types[i++] = Virtue::CHANCE;
        player_ptr->vir_types[i++] = Virtue::VITALITY;
        break;
    case PlayerClassType::MAGIC_EATER:
        player_ptr->vir_types[i++] = Virtue::ENCHANT;
        player_ptr->vir_types[i++] = Virtue::KNOWLEDGE;
        break;
    case PlayerClassType::BARD:
        player_ptr->vir_types[i++] = Virtue::HARMONY;
        player_ptr->vir_types[i++] = Virtue::COMPASSION;
        break;
    case PlayerClassType::CAVALRY:
        player_ptr->vir_types[i++] = Virtue::VALOUR;
        player_ptr->vir_types[i++] = Virtue::HARMONY;
        break;
    case PlayerClassType::BERSERKER:
        player_ptr->vir_types[i++] = Virtue::VALOUR;
        player_ptr->vir_types[i++] = Virtue::INDIVIDUALISM;
        break;
    case PlayerClassType::SMITH:
        player_ptr->vir_types[i++] = Virtue::HONOUR;
        player_ptr->vir_types[i++] = Virtue::KNOWLEDGE;
        break;
    case PlayerClassType::NINJA:
        player_ptr->vir_types[i++] = Virtue::PATIENCE;
        player_ptr->vir_types[i++] = Virtue::KNOWLEDGE;
        player_ptr->vir_types[i++] = Virtue::FAITH;
        player_ptr->vir_types[i++] = Virtue::UNLIFE;
        break;
    case PlayerClassType::ELEMENTALIST:
        player_ptr->vir_types[i++] = Virtue::NATURE;
        break;
    case PlayerClassType::MAX:
        break;
    };

    /* Get one virtue_names based on race */
    switch (player_ptr->prace) {
    case PlayerRaceType::HUMAN:
    case PlayerRaceType::HALF_ELF:
    case PlayerRaceType::DUNADAN:
        player_ptr->vir_types[i++] = Virtue::INDIVIDUALISM;
        break;
    case PlayerRaceType::ELF:
    case PlayerRaceType::SPRITE:
    case PlayerRaceType::ENT:
    case PlayerRaceType::MERFOLK:
        player_ptr->vir_types[i++] = Virtue::NATURE;
        break;
    case PlayerRaceType::HOBBIT:
    case PlayerRaceType::HALF_OGRE:
        player_ptr->vir_types[i++] = Virtue::TEMPERANCE;
        break;
    case PlayerRaceType::DWARF:
    case PlayerRaceType::KLACKON:
    case PlayerRaceType::ANDROID:
        player_ptr->vir_types[i++] = Virtue::DILIGENCE;
        break;
    case PlayerRaceType::GNOME:
    case PlayerRaceType::CYCLOPS:
        player_ptr->vir_types[i++] = Virtue::KNOWLEDGE;
        break;
    case PlayerRaceType::HALF_ORC:
    case PlayerRaceType::AMBERITE:
    case PlayerRaceType::KOBOLD:
        player_ptr->vir_types[i++] = Virtue::HONOUR;
        break;
    case PlayerRaceType::HALF_TROLL:
    case PlayerRaceType::BARBARIAN:
        player_ptr->vir_types[i++] = Virtue::VALOUR;
        break;
    case PlayerRaceType::HIGH_ELF:
    case PlayerRaceType::KUTAR:
        player_ptr->vir_types[i++] = Virtue::VITALITY;
        break;
    case PlayerRaceType::HALF_GIANT:
    case PlayerRaceType::GOLEM:
    case PlayerRaceType::ARCHON:
    case PlayerRaceType::BALROG:
        player_ptr->vir_types[i++] = Virtue::JUSTICE;
        break;
    case PlayerRaceType::HALF_TITAN:
        player_ptr->vir_types[i++] = Virtue::HARMONY;
        break;
    case PlayerRaceType::YEEK:
        player_ptr->vir_types[i++] = Virtue::SACRIFICE;
        break;
    case PlayerRaceType::MIND_FLAYER:
        player_ptr->vir_types[i++] = Virtue::ENLIGHTEN;
        break;
    case PlayerRaceType::DARK_ELF:
    case PlayerRaceType::DRACONIAN:
    case PlayerRaceType::S_FAIRY:
        player_ptr->vir_types[i++] = Virtue::ENCHANT;
        break;
    case PlayerRaceType::NIBELUNG:
        player_ptr->vir_types[i++] = Virtue::PATIENCE;
        break;
    case PlayerRaceType::IMP:
        player_ptr->vir_types[i++] = Virtue::FAITH;
        break;
    case PlayerRaceType::ZOMBIE:
    case PlayerRaceType::SKELETON:
    case PlayerRaceType::VAMPIRE:
    case PlayerRaceType::SPECTRE:
        player_ptr->vir_types[i++] = Virtue::UNLIFE;
        break;
    case PlayerRaceType::BEASTMAN:
        player_ptr->vir_types[i++] = Virtue::CHANCE;
        break;
    case PlayerRaceType::MAX:
        break;
    }

    /* Get a virtue_names for realms */
    if (player_ptr->realm1) {
        tmp_vir = get_realm_virtues(player_ptr, player_ptr->realm1);
        if (tmp_vir != Virtue::NONE) {
            player_ptr->vir_types[i++] = tmp_vir;
        }
    }

    if (player_ptr->realm2) {
        tmp_vir = get_realm_virtues(player_ptr, player_ptr->realm2);
        if (tmp_vir != Virtue::NONE) {
            player_ptr->vir_types[i++] = tmp_vir;
        }
    }

    /* Eliminate doubles */
    for (i = 0; i < 8; i++) {
        for (j = i + 1; j < 8; j++) {
            if ((player_ptr->vir_types[j] != Virtue::NONE) && (player_ptr->vir_types[j] == player_ptr->vir_types[i])) {
                player_ptr->vir_types[j] = Virtue::NONE;
            }
        }
    }

    /* Fill in the blanks */
    for (i = 0; i < 8; i++) {
        if (player_ptr->vir_types[i] == Virtue::NONE) {
            get_random_virtue(player_ptr, i);
        }
    }
}

/*!
 * @brief 対応する徳をプレイヤーがスロットに登録している場合に加減を行う。
 * @details 範囲は-125～125、基本的に絶対値が大きいほど絶対値が上がり辛くなる。
 * @param virtue_names 徳のID
 * @param amount 加減量
 */
void chg_virtue(PlayerType *player_ptr, Virtue virtue_id, int amount)
{
    for (int i = 0; i < 8; i++) {
        if (player_ptr->vir_types[i] != virtue_id) {
            continue;
        }

        if (amount > 0) {
            if ((amount + player_ptr->virtues[i] > 50) && one_in_(2)) {
                player_ptr->virtues[i] = std::max<short>(player_ptr->virtues[i], 50);
                return;
            }

            if ((amount + player_ptr->virtues[i] > 80) && one_in_(2)) {
                player_ptr->virtues[i] = std::max<short>(player_ptr->virtues[i], 80);
                return;
            }

            if ((amount + player_ptr->virtues[i] > 100) && one_in_(2)) {
                player_ptr->virtues[i] = std::max<short>(player_ptr->virtues[i], 100);
                return;
            }

            if (amount + player_ptr->virtues[i] > 125) {
                player_ptr->virtues[i] = 125;
            } else {
                player_ptr->virtues[i] = player_ptr->virtues[i] + amount;
            }
        } else {
            if ((amount + player_ptr->virtues[i] < -50) && one_in_(2)) {
                player_ptr->virtues[i] = std::min<short>(player_ptr->virtues[i], -50);
                return;
            }

            if ((amount + player_ptr->virtues[i] < -80) && one_in_(2)) {
                player_ptr->virtues[i] = std::min<short>(player_ptr->virtues[i], -80);
                return;
            }

            if ((amount + player_ptr->virtues[i] < -100) && one_in_(2)) {
                player_ptr->virtues[i] = std::min<short>(player_ptr->virtues[i], -100);
                return;
            }

            if (amount + player_ptr->virtues[i] < -125) {
                player_ptr->virtues[i] = -125;
            } else {
                player_ptr->virtues[i] = player_ptr->virtues[i] + amount;
            }
        }

        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
        return;
    }
}

/*!
 * @brief 対応する徳をプレイヤーがスロットに登録している場合に固定値をセットする
 * @param virtue_names 徳のID
 * @param amount セットしたい値
 */
void set_virtue(PlayerType *player_ptr, Virtue virtue_id, int amount)
{
    for (int i = 0; i < 8; i++) {
        if (player_ptr->vir_types[i] == virtue_id) {
            player_ptr->virtues[i] = (int16_t)amount;
            return;
        }
    }
}

/*!
 * @brief 徳のダンプ表示を行う
 * @param out_file ファイルポインタ
 */
void dump_virtues(PlayerType *player_ptr, FILE *out_file)
{
    if (!out_file) {
        return;
    }

    for (int v_nr = 0; v_nr < 8; v_nr++) {
        GAME_TEXT vir_name[20];
        int tester = player_ptr->virtues[v_nr];
        strcpy(vir_name, virtue_names.at(player_ptr->vir_types[v_nr]).data());
        const std::string vir_val_str = format(" (%d)", tester);
        const auto vir_val = show_actual_value ? vir_val_str.data() : "";
        if ((player_ptr->vir_types[v_nr] == Virtue::NONE) || (player_ptr->vir_types[v_nr] >= Virtue::MAX)) {
            fprintf(out_file, _("おっと。%sの情報なし。", "Oops. No info about %s."), vir_name);
        }

        else if (tester < -100) {
            fprintf(out_file, _("[%s]の対極%s", "You are the polar opposite of %s.%s"), vir_name, vir_val);
        } else if (tester < -80) {
            fprintf(out_file, _("[%s]の大敵%s", "You are an arch-enemy of %s.%s"), vir_name, vir_val);
        } else if (tester < -60) {
            fprintf(out_file, _("[%s]の強敵%s", "You are a bitter enemy of %s.%s"), vir_name, vir_val);
        } else if (tester < -40) {
            fprintf(out_file, _("[%s]の敵%s", "You are an enemy of %s.%s"), vir_name, vir_val);
        } else if (tester < -20) {
            fprintf(out_file, _("[%s]の罪者%s", "You have sinned against %s.%s"), vir_name, vir_val);
        } else if (tester < 0) {
            fprintf(out_file, _("[%s]の迷道者%s", "You have strayed from the path of %s.%s"), vir_name, vir_val);
        } else if (tester == 0) {
            fprintf(out_file, _("[%s]の中立者%s", "You are neutral to %s.%s"), vir_name, vir_val);
        } else if (tester < 20) {
            fprintf(out_file, _("[%s]の小徳者%s", "You are somewhat virtuous in %s.%s"), vir_name, vir_val);
        } else if (tester < 40) {
            fprintf(out_file, _("[%s]の中徳者%s", "You are virtuous in %s.%s"), vir_name, vir_val);
        } else if (tester < 60) {
            fprintf(out_file, _("[%s]の高徳者%s", "You are very virtuous in %s.%s"), vir_name, vir_val);
        } else if (tester < 80) {
            fprintf(out_file, _("[%s]の覇者%s", "You are a champion of %s.%s"), vir_name, vir_val);
        } else if (tester < 100) {
            fprintf(out_file, _("[%s]の偉大な覇者%s", "You are a great champion of %s.%s"), vir_name, vir_val);
        } else {
            fprintf(out_file, _("[%s]の具現者%s", "You are the living embodiment of %s.%s"), vir_name, vir_val);
        }

        fprintf(out_file, "\n");
    }
}
