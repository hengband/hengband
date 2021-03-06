/*!
 * @brief 技能の経験を表示する
 * @date 2020/04/23
 * @author Hourier
 */

#include "knowledge/knowledge-experiences.h"
#include "core/show-file.h"
#include "flavor/object-flavor.h"
#include "game-option/cheat-options.h"
#include "io-dump/dump-util.h"
#include "object/object-kind.h"
#include "player/player-class.h"
#include "player/player-skill.h"
#include "realm/realm-names-table.h"
#include "spell/spells-execution.h"
#include "spell/technic-info-table.h"
#include "sv-definition/sv-bow-types.h"
#include "util/angband-files.h"

/*
 * Display weapon-exp
 */
void do_cmd_knowledge_weapon_exp(player_type *creature_ptr)
{
    FILE *fff = NULL;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name))
        return;

    for (int i = 0; i < 5; i++) {
        for (int num = 0; num < 64; num++) {
            SUB_EXP weapon_exp;
            char tmp[30];
            for (KIND_OBJECT_IDX j = 0; j < max_k_idx; j++) {
                object_kind *k_ptr = &k_info[j];

                if ((k_ptr->tval != TV_SWORD - i) || (k_ptr->sval != num))
                    continue;
                if ((k_ptr->tval == TV_BOW) && (k_ptr->sval == SV_CRIMSON || k_ptr->sval == SV_HARP))
                    continue;

                weapon_exp = creature_ptr->weapon_exp[4 - i][num];
                strip_name(tmp, j);
                fprintf(fff, "%-25s ", tmp);
                if (weapon_exp >= s_info[creature_ptr->pclass].w_max[4 - i][num])
                    fprintf(fff, "!");
                else
                    fprintf(fff, " ");
                fprintf(fff, "%s", exp_level_str[weapon_exp_level(weapon_exp)]);
                if (cheat_xtra)
                    fprintf(fff, " %d", weapon_exp);
                fprintf(fff, "\n");
                break;
            }
        }
    }

    angband_fclose(fff);
    (void)show_file(creature_ptr, TRUE, file_name, _("武器の経験値", "Weapon Proficiency"), 0, 0);
    fd_kill(file_name);
}

/*!
 * @brief 魔法の経験値を表示するコマンドのメインルーチン
 * Display spell-exp
 * @return なし
 */
void do_cmd_knowledge_spell_exp(player_type *creature_ptr)
{
    FILE *fff = NULL;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name))
        return;

    if (creature_ptr->realm1 != REALM_NONE) {
        fprintf(fff, _("%sの魔法書\n", "%s Spellbook\n"), realm_names[creature_ptr->realm1]);
        for (SPELL_IDX i = 0; i < 32; i++) {
            const magic_type *s_ptr;
            if (!is_magic(creature_ptr->realm1)) {
                s_ptr = &technic_info[creature_ptr->realm1 - MIN_TECHNIC][i];
            } else {
                s_ptr = &mp_ptr->info[creature_ptr->realm1 - 1][i];
            }

            if (s_ptr->slevel >= 99)
                continue;
            SUB_EXP spell_exp = creature_ptr->spell_exp[i];
            int exp_level = spell_exp_level(spell_exp);
            fprintf(fff, "%-25s ", exe_spell(creature_ptr, creature_ptr->realm1, i, SPELL_NAME));
            if (creature_ptr->realm1 == REALM_HISSATSU)
                fprintf(fff, "[--]");
            else {
                if (exp_level >= EXP_LEVEL_MASTER)
                    fprintf(fff, "!");
                else
                    fprintf(fff, " ");
                fprintf(fff, "%s", exp_level_str[exp_level]);
            }

            if (cheat_xtra)
                fprintf(fff, " %d", spell_exp);
            fprintf(fff, "\n");
        }
    }

    if (creature_ptr->realm2 != REALM_NONE) {
        fprintf(fff, _("%sの魔法書\n", "\n%s Spellbook\n"), realm_names[creature_ptr->realm2]);
        for (SPELL_IDX i = 0; i < 32; i++) {
            const magic_type *s_ptr;
            if (!is_magic(creature_ptr->realm1)) {
                s_ptr = &technic_info[creature_ptr->realm2 - MIN_TECHNIC][i];
            } else {
                s_ptr = &mp_ptr->info[creature_ptr->realm2 - 1][i];
            }

            if (s_ptr->slevel >= 99)
                continue;

            SUB_EXP spell_exp = creature_ptr->spell_exp[i + 32];
            int exp_level = spell_exp_level(spell_exp);
            fprintf(fff, "%-25s ", exe_spell(creature_ptr, creature_ptr->realm2, i, SPELL_NAME));
            if (exp_level >= EXP_LEVEL_EXPERT)
                fprintf(fff, "!");
            else
                fprintf(fff, " ");
            fprintf(fff, "%s", exp_level_str[exp_level]);
            if (cheat_xtra)
                fprintf(fff, " %d", spell_exp);
            fprintf(fff, "\n");
        }
    }

    angband_fclose(fff);
    (void)show_file(creature_ptr, TRUE, file_name, _("魔法の経験値", "Spell Proficiency"), 0, 0);
    fd_kill(file_name);
}

/*!
 * @brief スキル情報を表示するコマンドのメインルーチン /
 * Display skill-exp
 * @return なし
 */
void do_cmd_knowledge_skill_exp(player_type *creature_ptr)
{
    char skill_name[GINOU_TEMPMAX][20] = { _("マーシャルアーツ", "Martial Arts    "), _("二刀流          ", "Dual Wielding   "),
        _("乗馬            ", "Riding          "), _("盾              ", "Shield          ") };

    FILE *fff = NULL;
    char file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name))
        return;

    for (int i = 0; i < GINOU_TEMPMAX; i++) {
        int skill_exp = creature_ptr->skill_exp[i];
        fprintf(fff, "%-20s ", skill_name[i]);
        if (skill_exp >= s_info[creature_ptr->pclass].s_max[i])
            fprintf(fff, "!");
        else
            fprintf(fff, " ");
        fprintf(fff, "%s", exp_level_str[(i == GINOU_RIDING) ? riding_exp_level(skill_exp) : weapon_exp_level(skill_exp)]);
        if (cheat_xtra)
            fprintf(fff, " %d", skill_exp);
        fprintf(fff, "\n");
    }

    angband_fclose(fff);
    (void)show_file(creature_ptr, TRUE, file_name, _("技能の経験値", "Miscellaneous Proficiency"), 0, 0);
    fd_kill(file_name);
}
