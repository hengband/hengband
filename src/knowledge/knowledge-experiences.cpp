/*!
 * @brief 技能の経験を表示する
 * @date 2020/04/23
 * @author Hourier
 */

#include "knowledge/knowledge-experiences.h"
#include "core/show-file.h"
#include "flavor/object-flavor.h"
#include "game-option/cheat-options.h"
#include "game-option/text-display-options.h"
#include "io-dump/dump-util.h"
#include "object/object-kind.h"
#include "player-info/class-info.h"
#include "player/player-skill.h"
#include "player/player-status.h"
#include "realm/realm-names-table.h"
#include "spell/spells-execution.h"
#include "spell/technic-info-table.h"
#include "sv-definition/sv-bow-types.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"

/*
 * Display weapon-exp
 */
void do_cmd_knowledge_weapon_exp(player_type *player_ptr)
{
    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name))
        return;

    for (int i = 0; i < 5; i++) {
        for (int num = 0; num < 64; num++) {
            char tmp[30];
            for (const auto &k_ref : k_info) {
                if ((k_ref.tval != TV_SWORD - i) || (k_ref.sval != num))
                    continue;
                if ((k_ref.tval == TV_BOW) && (k_ref.sval == SV_CRIMSON || k_ref.sval == SV_HARP))
                    continue;

                SUB_EXP weapon_exp = player_ptr->weapon_exp[4 - i][num];
                SUB_EXP weapon_max = s_info[player_ptr->pclass].w_max[4 - i][num];
                strip_name(tmp, k_ref.idx);
                fprintf(fff, "%-25s ", tmp);
                if (show_actual_value)
                    fprintf(fff, "%4d/%4d ", MIN(weapon_exp, weapon_max), weapon_max);
                if (weapon_exp >= weapon_max)
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
    (void)show_file(player_ptr, true, file_name, _("武器の経験値", "Weapon Proficiency"), 0, 0);
    fd_kill(file_name);
}

/*!
 * @brief 魔法の経験値を表示するコマンドのメインルーチン
 * Display spell-exp
 */
void do_cmd_knowledge_spell_exp(player_type *player_ptr)
{
    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name))
        return;

    if (player_ptr->realm1 != REALM_NONE) {
        fprintf(fff, _("%sの魔法書\n", "%s Spellbook\n"), realm_names[player_ptr->realm1]);
        for (SPELL_IDX i = 0; i < 32; i++) {
            const magic_type *s_ptr;
            if (!is_magic(player_ptr->realm1)) {
                s_ptr = &technic_info[player_ptr->realm1 - MIN_TECHNIC][i];
            } else {
                s_ptr = &mp_ptr->info[player_ptr->realm1 - 1][i];
            }

            if (s_ptr->slevel >= 99)
                continue;
            SUB_EXP spell_exp = player_ptr->spell_exp[i];
            int exp_level = spell_exp_level(spell_exp);
            fprintf(fff, "%-25s ", exe_spell(player_ptr, player_ptr->realm1, i, SPELL_NAME));
            if (player_ptr->realm1 == REALM_HISSATSU) {
                if (show_actual_value)
                    fprintf(fff, "----/---- ");
                fprintf(fff, "[--]");
            } else {
                if (show_actual_value)
                    fprintf(fff, "%4d/%4d ", MIN(spell_exp, SPELL_EXP_MASTER), SPELL_EXP_MASTER);
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

    if (player_ptr->realm2 != REALM_NONE) {
        fprintf(fff, _("%sの魔法書\n", "\n%s Spellbook\n"), realm_names[player_ptr->realm2]);
        for (SPELL_IDX i = 0; i < 32; i++) {
            const magic_type *s_ptr;
            if (!is_magic(player_ptr->realm1)) {
                s_ptr = &technic_info[player_ptr->realm2 - MIN_TECHNIC][i];
            } else {
                s_ptr = &mp_ptr->info[player_ptr->realm2 - 1][i];
            }

            if (s_ptr->slevel >= 99)
                continue;

            SUB_EXP spell_exp = player_ptr->spell_exp[i + 32];
            int exp_level = spell_exp_level(spell_exp);
            fprintf(fff, "%-25s ", exe_spell(player_ptr, player_ptr->realm2, i, SPELL_NAME));
            if (show_actual_value)
                fprintf(fff, "%4d/%4d ", MIN(spell_exp, SPELL_EXP_MASTER), SPELL_EXP_MASTER);
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
    (void)show_file(player_ptr, true, file_name, _("魔法の経験値", "Spell Proficiency"), 0, 0);
    fd_kill(file_name);
}

/*!
 * @brief スキル情報を表示するコマンドのメインルーチン /
 * Display skill-exp
 */
void do_cmd_knowledge_skill_exp(player_type *player_ptr)
{
    const char *skill_name[SKILL_MAX] = { _("マーシャルアーツ", "Martial Arts    "), _("二刀流          ", "Dual Wielding   "),
        _("乗馬            ", "Riding          "), _("盾              ", "Shield          ") };

    FILE *fff = nullptr;
    char file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name))
        return;

    for (int i = 0; i < SKILL_MAX; i++) {
        SUB_EXP skill_exp = player_ptr->skill_exp[i];
        SUB_EXP skill_max = s_info[player_ptr->pclass].s_max[i];
        fprintf(fff, "%-20s ", skill_name[i]);
        if (show_actual_value)
            fprintf(fff, "%4d/%4d ", MIN(skill_exp, skill_max), skill_max);
        if (skill_exp >= skill_max)
            fprintf(fff, "!");
        else
            fprintf(fff, " ");
        fprintf(fff, "%s", exp_level_str[(i == SKILL_RIDING) ? riding_exp_level(skill_exp) : weapon_exp_level(skill_exp)]);
        if (cheat_xtra)
            fprintf(fff, " %d", skill_exp);
        fprintf(fff, "\n");
    }

    angband_fclose(fff);
    (void)show_file(player_ptr, true, file_name, _("技能の経験値", "Miscellaneous Proficiency"), 0, 0);
    fd_kill(file_name);
}
