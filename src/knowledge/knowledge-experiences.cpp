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
void do_cmd_knowledge_weapon_exp(PlayerType *player_ptr)
{
    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name))
        return;

    for (auto tval : { ItemKindType::SWORD, ItemKindType::POLEARM, ItemKindType::HAFTED, ItemKindType::DIGGING, ItemKindType::BOW }) {
        for (int num = 0; num < 64; num++) {
            char tmp[30];
            for (const auto &k_ref : k_info) {
                if ((k_ref.tval != tval) || (k_ref.sval != num))
                    continue;
                if ((k_ref.tval == ItemKindType::BOW) && (k_ref.sval == SV_CRIMSON || k_ref.sval == SV_HARP))
                    continue;

                SUB_EXP weapon_exp = player_ptr->weapon_exp[tval][num];
                SUB_EXP weapon_max = s_info[enum2i(player_ptr->pclass)].w_max[tval][num];
                strip_name(tmp, k_ref.idx);
                fprintf(fff, "%-25s ", tmp);
                if (show_actual_value)
                    fprintf(fff, "%4d/%4d ", std::min(weapon_exp, weapon_max), weapon_max);
                if (weapon_exp >= weapon_max)
                    fprintf(fff, "!");
                else
                    fprintf(fff, " ");
                auto skill_rank = PlayerSkill::weapon_skill_rank(weapon_exp);
                fprintf(fff, "%s", PlayerSkill::skill_rank_str(skill_rank));
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
void do_cmd_knowledge_spell_exp(PlayerType *player_ptr)
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
            auto skill_rank = PlayerSkill::spell_skill_rank(spell_exp);
            fprintf(fff, "%-25s ", exe_spell(player_ptr, player_ptr->realm1, i, SpellProcessType::NAME));
            if (player_ptr->realm1 == REALM_HISSATSU) {
                if (show_actual_value)
                    fprintf(fff, "----/---- ");
                fprintf(fff, "[--]");
            } else {
                if (show_actual_value)
                    fprintf(fff, "%4d/%4d ", spell_exp, PlayerSkill::spell_exp_at(PlayerSkillRank::MASTER));
                if (skill_rank >= PlayerSkillRank::MASTER)
                    fprintf(fff, "!");
                else
                    fprintf(fff, " ");
                fprintf(fff, "%s", PlayerSkill::skill_rank_str(skill_rank));
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
            auto skill_rank = PlayerSkill::spell_skill_rank(spell_exp);
            fprintf(fff, "%-25s ", exe_spell(player_ptr, player_ptr->realm2, i, SpellProcessType::NAME));
            if (show_actual_value)
                fprintf(fff, "%4d/%4d ", spell_exp, PlayerSkill::spell_exp_at(PlayerSkillRank::MASTER));
            if (skill_rank >= PlayerSkillRank::EXPERT)
                fprintf(fff, "!");
            else
                fprintf(fff, " ");
            fprintf(fff, "%s", PlayerSkill::skill_rank_str(skill_rank));
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
void do_cmd_knowledge_skill_exp(PlayerType *player_ptr)
{
    FILE *fff = nullptr;
    char file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name))
        return;

    for (auto i : PLAYER_SKILL_KIND_TYPE_RANGE) {
        SUB_EXP skill_exp = player_ptr->skill_exp[i];
        SUB_EXP skill_max = s_info[enum2i(player_ptr->pclass)].s_max[i];
        fprintf(fff, "%-20s ", PlayerSkill::skill_name(i));
        if (show_actual_value)
            fprintf(fff, "%4d/%4d ", std::min(skill_exp, skill_max), skill_max);
        if (skill_exp >= skill_max)
            fprintf(fff, "!");
        else
            fprintf(fff, " ");
        auto skill_rank = (i == PlayerSkillKindType::RIDING) ? PlayerSkill::riding_skill_rank(skill_exp) : PlayerSkill::weapon_skill_rank(skill_exp);
        fprintf(fff, "%s", PlayerSkill::skill_rank_str(skill_rank));
        if (cheat_xtra)
            fprintf(fff, " %d", skill_exp);
        fprintf(fff, "\n");
    }

    angband_fclose(fff);
    (void)show_file(player_ptr, true, file_name, _("技能の経験値", "Miscellaneous Proficiency"), 0, 0);
    fd_kill(file_name);
}
