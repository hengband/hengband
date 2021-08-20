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
#include "core/player-update-types.h"
#include "game-option/text-display-options.h"
#include "player/player-class.h"
#include "player/player-race-types.h"
#include "realm/realm-names-table.h"
#include "system/player-type-definition.h"

/*!
 * 徳の名称 / The names of the virtues
 */
concptr virtue[MAX_VIRTUE] = {
    _("情", "Compassion"),
    _("誉", "Honour"),
    _("正", "Justice"),
    _("犠", "Sacrifice"),
    _("識", "Knowledge"),
    _("誠", "Faith"),
    _("啓", "Enlightenment"),
    _("秘", "Mysticism"),
    _("運", "Chance"),
    _("然", "Nature"),
    _("調", "Harmony"),
    _("活", "Vitality"),
    _("死", "Unlife"),
    _("忍", "Patience"),
    _("節", "Temperance"),
    _("勤", "Diligence"),
    _("勇", "Valour"),
    _("個", "Individualism"),
};

/*!
 * @brief 該当の徳がプレイヤーに指定されているか否かに応じつつ、大小を比較する。
 * @details 徳がない場合は値0として比較する。
 * @param type 比較したい徳のID
 * @param num 比較基準値
 * @param tekitou VIRTUE_LARGE = 基準値より大きいか / VIRTUE_SMALL = 基準値より小さいか
 * @return 比較の真偽値を返す
 * @todo 引数名を直しておく
 */
bool compare_virtue(player_type *creature_ptr, int type, int num, int tekitou)
{
    int vir = virtue_number(creature_ptr, type) ? creature_ptr->virtues[virtue_number(creature_ptr, type) - 1] : 0;
    switch (tekitou) {
    case VIRTUE_LARGE:
        if (vir > num)
            return true;
        else
            return false;
    case VIRTUE_SMALL:
        if (vir < num)
            return true;
        else
            return false;
    default:
        return false;
    }
}

/*!
 * @brief プレイヤーの指定の徳が何番目のスロットに登録されているかを返す。 / Aux function
 * @param type 確認したい徳のID
 * @return スロットがあるならばスロットのID(0～7)+1、ない場合は0を返す。
 */
int virtue_number(player_type *creature_ptr, int type)
{
    for (int i = 0; i < 8; i++)
        if (creature_ptr->vir_types[i] == type)
            return i + 1;

    return 0;
}

/*!
 * @brief プレイヤーの職業や種族に依存しないランダムな徳を取得する / Aux function
 * @param which 確認したい徳のID
 */
static void get_random_virtue(player_type *creature_ptr, int which)
{
    int type = 0;
    while (!(type) || virtue_number(creature_ptr, type)) {
        switch (randint1(29)) {
        case 1:
        case 2:
        case 3:
            type = V_SACRIFICE;
            break;
        case 4:
        case 5:
        case 6:
            type = V_COMPASSION;
            break;
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
            type = V_VALOUR;
            break;
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
            type = V_HONOUR;
            break;
        case 18:
        case 19:
        case 20:
        case 21:
            type = V_JUSTICE;
            break;
        case 22:
        case 23:
            type = V_TEMPERANCE;
            break;
        case 24:
        case 25:
            type = V_HARMONY;
            break;
        case 26:
        case 27:
        case 28:
            type = V_PATIENCE;
            break;
        default:
            type = V_DILIGENCE;
            break;
        }
    }

    creature_ptr->vir_types[which] = (int16_t)type;
}

/*!
 * @brief プレイヤーの選んだ魔法領域に応じて対応する徳を返す。
 * @param realm 魔法領域のID
 * @return 対応する徳のID
 */
static enum virtue_idx get_realm_virtues(player_type *creature_ptr, int16_t realm)
{
    switch (realm) {
    case REALM_LIFE:
        if (virtue_number(creature_ptr, V_VITALITY))
            return V_TEMPERANCE;
        else
            return V_VITALITY;
    case REALM_SORCERY:
        if (virtue_number(creature_ptr, V_KNOWLEDGE))
            return V_ENCHANT;
        else
            return V_KNOWLEDGE;
    case REALM_NATURE:
        if (virtue_number(creature_ptr, V_NATURE))
            return V_HARMONY;
        else
            return V_NATURE;
    case REALM_CHAOS:
        if (virtue_number(creature_ptr, V_CHANCE))
            return V_INDIVIDUALISM;
        else
            return V_CHANCE;
    case REALM_DEATH:
        return V_UNLIFE;
    case REALM_TRUMP:
        return V_KNOWLEDGE;
    case REALM_ARCANE:
        return V_NONE;
    case REALM_CRAFT:
        if (virtue_number(creature_ptr, V_ENCHANT))
            return V_INDIVIDUALISM;
        else
            return V_ENCHANT;
    case REALM_DAEMON:
        if (virtue_number(creature_ptr, V_JUSTICE))
            return V_FAITH;
        else
            return V_JUSTICE;
    case REALM_CRUSADE:
        if (virtue_number(creature_ptr, V_JUSTICE))
            return V_HONOUR;
        else
            return V_JUSTICE;
    case REALM_HEX:
        if (virtue_number(creature_ptr, V_COMPASSION))
            return V_JUSTICE;
        else
            return V_COMPASSION;
    default:
        return V_NONE;
    };
}

/*!
 * @brief 作成中のプレイヤーキャラクターに徳8種類を与える。 / Select virtues & reset values for a new character
 * @details 職業に応じて1～4種が固定、種族に応じて1種類が与えられ、後は重複なくランダムに選択される。
 */
void initialize_virtues(player_type *creature_ptr)
{
    int i = 0, j = 0;
    int16_t tmp_vir;

    /* Reset */
    for (i = 0; i < 8; i++) {
        creature_ptr->virtues[i] = 0;
        creature_ptr->vir_types[i] = 0;
    }

    i = 0;

    /* Get pre-defined types */
    /* 1 or more virtues based on class */
    switch (creature_ptr->pclass) {
    case CLASS_WARRIOR:
    case CLASS_SAMURAI:
        creature_ptr->vir_types[i++] = V_VALOUR;
        creature_ptr->vir_types[i++] = V_HONOUR;
        break;
    case CLASS_MAGE:
        creature_ptr->vir_types[i++] = V_KNOWLEDGE;
        creature_ptr->vir_types[i++] = V_ENCHANT;
        break;
    case CLASS_PRIEST:
        creature_ptr->vir_types[i++] = V_FAITH;
        creature_ptr->vir_types[i++] = V_TEMPERANCE;
        break;
    case CLASS_ROGUE:
    case CLASS_SNIPER:
        creature_ptr->vir_types[i++] = V_HONOUR;
        break;
    case CLASS_RANGER:
    case CLASS_ARCHER:
        creature_ptr->vir_types[i++] = V_NATURE;
        creature_ptr->vir_types[i++] = V_TEMPERANCE;
        break;
    case CLASS_PALADIN:
        creature_ptr->vir_types[i++] = V_JUSTICE;
        creature_ptr->vir_types[i++] = V_VALOUR;
        creature_ptr->vir_types[i++] = V_HONOUR;
        creature_ptr->vir_types[i++] = V_FAITH;
        break;
    case CLASS_WARRIOR_MAGE:
    case CLASS_RED_MAGE:
        creature_ptr->vir_types[i++] = V_ENCHANT;
        creature_ptr->vir_types[i++] = V_VALOUR;
        break;
    case CLASS_CHAOS_WARRIOR:
        creature_ptr->vir_types[i++] = V_CHANCE;
        creature_ptr->vir_types[i++] = V_INDIVIDUALISM;
        break;
    case CLASS_MONK:
    case CLASS_FORCETRAINER:
        creature_ptr->vir_types[i++] = V_FAITH;
        creature_ptr->vir_types[i++] = V_HARMONY;
        creature_ptr->vir_types[i++] = V_TEMPERANCE;
        creature_ptr->vir_types[i++] = V_PATIENCE;
        break;
    case CLASS_MINDCRAFTER:
    case CLASS_MIRROR_MASTER:
        creature_ptr->vir_types[i++] = V_HARMONY;
        creature_ptr->vir_types[i++] = V_ENLIGHTEN;
        creature_ptr->vir_types[i++] = V_PATIENCE;
        break;
    case CLASS_HIGH_MAGE:
    case CLASS_SORCERER:
        creature_ptr->vir_types[i++] = V_ENLIGHTEN;
        creature_ptr->vir_types[i++] = V_ENCHANT;
        creature_ptr->vir_types[i++] = V_KNOWLEDGE;
        break;
    case CLASS_TOURIST:
        creature_ptr->vir_types[i++] = V_ENLIGHTEN;
        creature_ptr->vir_types[i++] = V_CHANCE;
        break;
    case CLASS_IMITATOR:
        creature_ptr->vir_types[i++] = V_CHANCE;
        break;
    case CLASS_BLUE_MAGE:
        creature_ptr->vir_types[i++] = V_CHANCE;
        creature_ptr->vir_types[i++] = V_KNOWLEDGE;
        break;
    case CLASS_BEASTMASTER:
        creature_ptr->vir_types[i++] = V_NATURE;
        creature_ptr->vir_types[i++] = V_CHANCE;
        creature_ptr->vir_types[i++] = V_VITALITY;
        break;
    case CLASS_MAGIC_EATER:
        creature_ptr->vir_types[i++] = V_ENCHANT;
        creature_ptr->vir_types[i++] = V_KNOWLEDGE;
        break;
    case CLASS_BARD:
        creature_ptr->vir_types[i++] = V_HARMONY;
        creature_ptr->vir_types[i++] = V_COMPASSION;
        break;
    case CLASS_CAVALRY:
        creature_ptr->vir_types[i++] = V_VALOUR;
        creature_ptr->vir_types[i++] = V_HARMONY;
        break;
    case CLASS_BERSERKER:
        creature_ptr->vir_types[i++] = V_VALOUR;
        creature_ptr->vir_types[i++] = V_INDIVIDUALISM;
        break;
    case CLASS_SMITH:
        creature_ptr->vir_types[i++] = V_HONOUR;
        creature_ptr->vir_types[i++] = V_KNOWLEDGE;
        break;
    case CLASS_NINJA:
        creature_ptr->vir_types[i++] = V_PATIENCE;
        creature_ptr->vir_types[i++] = V_KNOWLEDGE;
        creature_ptr->vir_types[i++] = V_FAITH;
        creature_ptr->vir_types[i++] = V_UNLIFE;
        break;
    case CLASS_ELEMENTALIST:
        creature_ptr->vir_types[i++] = V_NATURE;
        break;
    case MAX_CLASS:
        break;
    };

    /* Get one virtue based on race */
    switch (creature_ptr->prace) {
    case player_race_type::HUMAN:
    case player_race_type::HALF_ELF:
    case player_race_type::DUNADAN:
        creature_ptr->vir_types[i++] = V_INDIVIDUALISM;
        break;
    case player_race_type::ELF:
    case player_race_type::SPRITE:
    case player_race_type::ENT:
    case player_race_type::MERFOLK:
        creature_ptr->vir_types[i++] = V_NATURE;
        break;
    case player_race_type::HOBBIT:
    case player_race_type::HALF_OGRE:
        creature_ptr->vir_types[i++] = V_TEMPERANCE;
        break;
    case player_race_type::DWARF:
    case player_race_type::KLACKON:
    case player_race_type::ANDROID:
        creature_ptr->vir_types[i++] = V_DILIGENCE;
        break;
    case player_race_type::GNOME:
    case player_race_type::CYCLOPS:
        creature_ptr->vir_types[i++] = V_KNOWLEDGE;
        break;
    case player_race_type::HALF_ORC:
    case player_race_type::AMBERITE:
    case player_race_type::KOBOLD:
        creature_ptr->vir_types[i++] = V_HONOUR;
        break;
    case player_race_type::HALF_TROLL:
    case player_race_type::BARBARIAN:
        creature_ptr->vir_types[i++] = V_VALOUR;
        break;
    case player_race_type::HIGH_ELF:
    case player_race_type::KUTAR:
        creature_ptr->vir_types[i++] = V_VITALITY;
        break;
    case player_race_type::HALF_GIANT:
    case player_race_type::GOLEM:
    case player_race_type::ARCHON:
    case player_race_type::BALROG:
        creature_ptr->vir_types[i++] = V_JUSTICE;
        break;
    case player_race_type::HALF_TITAN:
        creature_ptr->vir_types[i++] = V_HARMONY;
        break;
    case player_race_type::YEEK:
        creature_ptr->vir_types[i++] = V_SACRIFICE;
        break;
    case player_race_type::MIND_FLAYER:
        creature_ptr->vir_types[i++] = V_ENLIGHTEN;
        break;
    case player_race_type::DARK_ELF:
    case player_race_type::DRACONIAN:
    case player_race_type::S_FAIRY:
        creature_ptr->vir_types[i++] = V_ENCHANT;
        break;
    case player_race_type::NIBELUNG:
        creature_ptr->vir_types[i++] = V_PATIENCE;
        break;
    case player_race_type::IMP:
        creature_ptr->vir_types[i++] = V_FAITH;
        break;
    case player_race_type::ZOMBIE:
    case player_race_type::SKELETON:
    case player_race_type::VAMPIRE:
    case player_race_type::SPECTRE:
        creature_ptr->vir_types[i++] = V_UNLIFE;
        break;
    case player_race_type::BEASTMAN:
        creature_ptr->vir_types[i++] = V_CHANCE;
        break;
    case player_race_type::MAX:
        break;
    }

    /* Get a virtue for realms */
    if (creature_ptr->realm1) {
        tmp_vir = get_realm_virtues(creature_ptr, creature_ptr->realm1);
        if (tmp_vir)
            creature_ptr->vir_types[i++] = tmp_vir;
    }

    if (creature_ptr->realm2) {
        tmp_vir = get_realm_virtues(creature_ptr, creature_ptr->realm2);
        if (tmp_vir)
            creature_ptr->vir_types[i++] = tmp_vir;
    }

    /* Eliminate doubles */
    for (i = 0; i < 8; i++)
        for (j = i + 1; j < 8; j++)
            if ((creature_ptr->vir_types[j] != 0) && (creature_ptr->vir_types[j] == creature_ptr->vir_types[i]))
                creature_ptr->vir_types[j] = 0;

    /* Fill in the blanks */
    for (i = 0; i < 8; i++)
        if (creature_ptr->vir_types[i] == 0)
            get_random_virtue(creature_ptr, i);
}

/*!
 * @brief 対応する徳をプレイヤーがスロットに登録している場合に加減を行う。
 * @details 範囲は-125～125、基本的に絶対値が大きいほど絶対値が上がり辛くなる。
 * @param virtue 徳のID
 * @param amount 加減量
 */
void chg_virtue(player_type *creature_ptr, int virtue_id, int amount)
{
    for (int i = 0; i < 8; i++) {
        if (creature_ptr->vir_types[i] != virtue_id)
            continue;

        if (amount > 0) {
            if ((amount + creature_ptr->virtues[i] > 50) && one_in_(2)) {
                creature_ptr->virtues[i] = MAX(creature_ptr->virtues[i], 50);
                return;
            }

            if ((amount + creature_ptr->virtues[i] > 80) && one_in_(2)) {
                creature_ptr->virtues[i] = MAX(creature_ptr->virtues[i], 80);
                return;
            }

            if ((amount + creature_ptr->virtues[i] > 100) && one_in_(2)) {
                creature_ptr->virtues[i] = MAX(creature_ptr->virtues[i], 100);
                return;
            }

            if (amount + creature_ptr->virtues[i] > 125)
                creature_ptr->virtues[i] = 125;
            else
                creature_ptr->virtues[i] = creature_ptr->virtues[i] + amount;
        } else {
            if ((amount + creature_ptr->virtues[i] < -50) && one_in_(2)) {
                creature_ptr->virtues[i] = MIN(creature_ptr->virtues[i], -50);
                return;
            }

            if ((amount + creature_ptr->virtues[i] < -80) && one_in_(2)) {
                creature_ptr->virtues[i] = MIN(creature_ptr->virtues[i], -80);
                return;
            }

            if ((amount + creature_ptr->virtues[i] < -100) && one_in_(2)) {
                creature_ptr->virtues[i] = MIN(creature_ptr->virtues[i], -100);
                return;
            }

            if (amount + creature_ptr->virtues[i] < -125)
                creature_ptr->virtues[i] = -125;
            else
                creature_ptr->virtues[i] = creature_ptr->virtues[i] + amount;
        }

        creature_ptr->update |= PU_BONUS;
        return;
    }
}

/*!
 * @brief 対応する徳をプレイヤーがスロットに登録している場合に固定値をセットする
 * @param virtue 徳のID
 * @param amount セットしたい値
 */
void set_virtue(player_type *creature_ptr, int virtue_id, int amount)
{
    for (int i = 0; i < 8; i++)
        if (creature_ptr->vir_types[i] == virtue_id) {
            creature_ptr->virtues[i] = (int16_t)amount;
            return;
        }
}

/*!
 * @brief 徳のダンプ表示を行う
 * @param out_file ファイルポインタ
 */
void dump_virtues(player_type *creature_ptr, FILE *out_file)
{
    if (!out_file)
        return;

    for (int v_nr = 0; v_nr < 8; v_nr++) {
        GAME_TEXT vir_name[20];
        int tester = creature_ptr->virtues[v_nr];
        strcpy(vir_name, virtue[(creature_ptr->vir_types[v_nr]) - 1]);
        concptr vir_val = show_actual_value ? format(" (%d)", tester) : "";
        if (creature_ptr->vir_types[v_nr] == 0 || creature_ptr->vir_types[v_nr] > MAX_VIRTUE)
            fprintf(out_file, _("おっと。%sの情報なし。", "Oops. No info about %s."), vir_name);

        else if (tester < -100)
            fprintf(out_file, _("[%s]の対極%s", "You are the polar opposite of %s.%s"), vir_name, vir_val);
        else if (tester < -80)
            fprintf(out_file, _("[%s]の大敵%s", "You are an arch-enemy of %s.%s"), vir_name, vir_val);
        else if (tester < -60)
            fprintf(out_file, _("[%s]の強敵%s", "You are a bitter enemy of %s.%s"), vir_name, vir_val);
        else if (tester < -40)
            fprintf(out_file, _("[%s]の敵%s", "You are an enemy of %s.%s"), vir_name, vir_val);
        else if (tester < -20)
            fprintf(out_file, _("[%s]の罪者%s", "You have sinned against %s.%s"), vir_name, vir_val);
        else if (tester < 0)
            fprintf(out_file, _("[%s]の迷道者%s", "You have strayed from the path of %s.%s"), vir_name, vir_val);
        else if (tester == 0)
            fprintf(out_file, _("[%s]の中立者%s", "You are neutral to %s.%s"), vir_name, vir_val);
        else if (tester < 20)
            fprintf(out_file, _("[%s]の小徳者%s", "You are somewhat virtuous in %s.%s"), vir_name, vir_val);
        else if (tester < 40)
            fprintf(out_file, _("[%s]の中徳者%s", "You are virtuous in %s.%s"), vir_name, vir_val);
        else if (tester < 60)
            fprintf(out_file, _("[%s]の高徳者%s", "You are very virtuous in %s.%s"), vir_name, vir_val);
        else if (tester < 80)
            fprintf(out_file, _("[%s]の覇者%s", "You are a champion of %s.%s"), vir_name, vir_val);
        else if (tester < 100)
            fprintf(out_file, _("[%s]の偉大な覇者%s", "You are a great champion of %s.%s"), vir_name, vir_val);
        else
            fprintf(out_file, _("[%s]の具現者%s", "You are the living embodiment of %s.%s"), vir_name, vir_val);

        fprintf(out_file, "\n");
    }
}
