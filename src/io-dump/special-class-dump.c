/*!
 * @brief 一部職業でのみダンプする能力の出力処理
 * @date 2020/03/07
 * @author Hourier
 */

#include "io-dump/special-class-dump.h"
#include "blue-magic/blue-magic-checker.h"
#include "cmd-item/cmd-magiceat.h"
#include "cmd-item/cmd-smith.h"
#include "mind/mind-blue-mage.h"
#include "mspell/monster-power-table.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"

typedef struct {
    BIT_FLAGS f4;
    BIT_FLAGS f5;
    BIT_FLAGS f6;
} learnt_spell_table;

/*!
 * @brief 魔力喰いを持つクラスの情報をダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_magic_eater(player_type *creature_ptr, FILE *fff)
{
    char s[EATER_EXT][MAX_NLEN];
    fprintf(fff, _("\n\n  [取り込んだ魔法道具]\n", "\n\n  [Magic devices eaten]\n"));

    for (int ext = 0; ext < 3; ext++) {
        tval_type tval = 0;
        switch (ext) {
        case 0:
            tval = TV_STAFF;
            fprintf(fff, _("\n[杖]\n", "\n[Staffs]\n"));
            break;
        case 1:
            tval = TV_WAND;
            fprintf(fff, _("\n[魔法棒]\n", "\n[Wands]\n"));
            break;
        case 2:
            tval = TV_ROD;
            fprintf(fff, _("\n[ロッド]\n", "\n[Rods]\n"));
            break;
        }

        int eat_num = 0;
        for (OBJECT_SUBTYPE_VALUE i = 0; i < EATER_EXT; i++) {
            int idx = EATER_EXT * ext + i;
            int magic_num = creature_ptr->magic_num2[idx];
            if (!magic_num)
                continue;

            KIND_OBJECT_IDX k_idx = lookup_kind(tval, i);
            if (!k_idx)
                continue;
            sprintf(s[eat_num], "%23s (%2d)", (k_name + k_info[k_idx].name), magic_num);
            eat_num++;
        }

        if (eat_num <= 0) {
            fputs(_("  (なし)\n", "  (none)\n"), fff);
            continue;
        }

        OBJECT_SUBTYPE_VALUE i;
        for (i = 0; i < eat_num; i++) {
            fputs(s[i], fff);
            if (i % 3 < 2)
                fputs("    ", fff);
            else
                fputs("\n", fff);
        }

        if (i % 3 > 0)
            fputs("\n", fff);
    }
}

/*!
 * @brief 鍛冶師のエッセンス情報をダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_smith(player_type *creature_ptr, FILE *fff)
{
    int i, id[250], n = 0, row;
    fprintf(fff, _("\n\n  [手に入れたエッセンス]\n\n", "\n\n  [Get Essence]\n\n"));
    fprintf(fff, _("エッセンス   個数     エッセンス   個数     エッセンス   個数", "Essence      Num      Essence      Num      Essence      Num "));
    for (i = 0; essence_name[i]; i++) {
        if (!essence_name[i][0])
            continue;
        id[n] = i;
        n++;
    }

    row = n / 3 + 1;
    for (i = 0; i < row; i++) {
        fprintf(fff, "\n");
        fprintf(fff, "%-11s %5d     ", essence_name[id[i]], (int)creature_ptr->magic_num1[id[i]]);
        if (i + row < n)
            fprintf(fff, "%-11s %5d     ", essence_name[id[i + row]], (int)creature_ptr->magic_num1[id[i + row]]);
        if (i + row * 2 < n)
            fprintf(fff, "%-11s %5d", essence_name[id[i + row * 2]], (int)creature_ptr->magic_num1[id[i + row * 2]]);
    }

    fputs("\n", fff);
}

/*!
 * @brief ダンプする情報に学習済魔法の種類を追加する
 * @param p ダンプ用のバッファ
 * @param col 行数
 * @param spell_type 魔法の種類
 * @param learnt_spell_ptr 学習済魔法のテーブル
 * @return なし
 */
static void add_monster_spell_type(char p[][80], int col, blue_magic_type spell_type, learnt_spell_table *learnt_spell_ptr)
{
    learnt_spell_ptr->f4 = 0;
    learnt_spell_ptr->f5 = 0;
    learnt_spell_ptr->f6 = 0;
    set_rf_masks(&learnt_spell_ptr->f4, &learnt_spell_ptr->f5, &learnt_spell_ptr->f6, spell_type);
    switch (spell_type) {
    case MONSPELL_TYPE_BOLT:
        strcat(p[col], _("\n     [ボルト型]\n", "\n     [Bolt  Type]\n"));
        break;

    case MONSPELL_TYPE_BALL:
        strcat(p[col], _("\n     [ボール型]\n", "\n     [Ball  Type]\n"));
        break;

    case MONSPELL_TYPE_BREATH:
        strcat(p[col], _("\n     [ブレス型]\n", "\n     [  Breath  ]\n"));
        break;

    case MONSPELL_TYPE_SUMMON:
        strcat(p[col], _("\n     [召喚魔法]\n", "\n     [Summonning]\n"));
        break;

    case MONSPELL_TYPE_OTHER:
        strcat(p[col], _("\n     [ その他 ]\n", "\n     [Other Type]\n"));
        break;
    }
}

/*!
 * @brief 青魔道士の学習済魔法をダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_blue_mage(player_type *creature_ptr, FILE *fff)
{
    char p[60][80];
    for (int i = 0; i < 60; i++) {
        p[i][0] = '\0';
    }

    int col = 0;
    strcat(p[col], _("\n\n  [学習済みの青魔法]\n", "\n\n  [Learned Blue Magic]\n"));

    int spellnum[MAX_MONSPELLS];
    for (int spell_type = 1; spell_type < 6; spell_type++) {
        col++;
        learnt_spell_table learnt_magic;
        add_monster_spell_type(p, col, spell_type, &learnt_magic);

        int num = 0;
        for (int i = 0; i < 32; i++) {
            if ((0x00000001 << i) & learnt_magic.f4)
                spellnum[num++] = i;
        }

        for (int i = 32; i < 64; i++) {
            if ((0x00000001 << (i - 32)) & learnt_magic.f5)
                spellnum[num++] = i;
        }

        for (int i = 64; i < 96; i++) {
            if ((0x00000001 << (i - 64)) & learnt_magic.f6)
                spellnum[num++] = i;
        }

        col++;
        bool pcol = FALSE;
        strcat(p[col], "       ");

        for (int i = 0; i < num; i++) {
            if (creature_ptr->magic_num2[spellnum[i]] == 0)
                continue;

            pcol = TRUE;
            int l1 = strlen(p[col]);
            int l2 = strlen(monster_powers_short[spellnum[i]]);
            if ((l1 + l2) >= 75) {
                strcat(p[col], "\n");
                col++;
                strcat(p[col], "       ");
            }

            strcat(p[col], monster_powers_short[spellnum[i]]);
            strcat(p[col], ", ");
        }

        if (!pcol) {
            strcat(p[col], _("なし", "None"));
            strcat(p[col], "\n");
            continue;
        }

        if (p[col][strlen(p[col]) - 2] == ',') {
            p[col][strlen(p[col]) - 2] = '\0';
        } else {
            p[col][strlen(p[col]) - 10] = '\0';
        }

        strcat(p[col], "\n");
    }

    for (int i = 0; i <= col; i++) {
        fputs(p[i], fff);
    }
}

/*!
 * @brief プレイヤーの職業能力情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
void dump_aux_class_special(player_type *creature_ptr, FILE *fff)
{
    switch (creature_ptr->pclass) {
    case CLASS_MAGIC_EATER: {
        dump_magic_eater(creature_ptr, fff);
        return;
    }
    case CLASS_SMITH: {
        dump_smith(creature_ptr, fff);
        return;
    }
    case CLASS_BLUE_MAGE: {
        dump_blue_mage(creature_ptr, fff);
        return;
    }
    default:
        return;
    }
}
