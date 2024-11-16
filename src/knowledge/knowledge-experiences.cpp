/*!
 * @brief 技能の経験を表示する
 * @date 2020/04/23
 * @author Hourier
 */

#include "knowledge/knowledge-experiences.h"
#include "core/show-file.h"
#include "game-option/cheat-options.h"
#include "game-option/text-display-options.h"
#include "io-dump/dump-util.h"
#include "player-info/class-info.h"
#include "player/player-realm.h"
#include "player/player-skill.h"
#include "player/player-status.h"
#include "spell/spells-execution.h"
#include "spell/technic-info-table.h"
#include "sv-definition/sv-bow-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"

/*
 * Display weapon-exp
 */
void do_cmd_knowledge_weapon_exp(PlayerType *player_ptr)
{
    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name)) {
        return;
    }

    for (auto tval : { ItemKindType::SWORD, ItemKindType::POLEARM, ItemKindType::HAFTED, ItemKindType::DIGGING, ItemKindType::BOW }) {
        for (int num = 0; num < 64; num++) {
            BaseitemKey bi_key(tval, num);
            for (const auto &baseitem : BaseitemList::get_instance()) {
                if (baseitem.bi_key != bi_key) {
                    continue;
                }

                const auto sval = baseitem.bi_key.sval();
                if ((baseitem.bi_key.tval() == ItemKindType::BOW) && (sval == SV_CRIMSON || sval == SV_HARP)) {
                    continue;
                }

                SUB_EXP weapon_exp = player_ptr->weapon_exp[tval][num];
                SUB_EXP weapon_max = player_ptr->weapon_exp_max[tval][num];
                fprintf(fff, "%-25s ", baseitem.stripped_name().data());
                if (show_actual_value) {
                    fprintf(fff, "%4d/%4d ", weapon_exp, weapon_max);
                }
                if (weapon_exp >= weapon_max) {
                    fprintf(fff, "!");
                } else {
                    fprintf(fff, " ");
                }
                auto skill_rank = PlayerSkill::weapon_skill_rank(weapon_exp);
                fprintf(fff, "%s", PlayerSkill::skill_rank_str(skill_rank));
                if (cheat_xtra) {
                    fprintf(fff, " %d", weapon_exp);
                }
                fprintf(fff, "\n");
                break;
            }
        }
    }

    angband_fclose(fff);
    FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, _("武器の経験値", "Weapon Proficiency"));
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
    if (!open_temporary_file(&fff, file_name)) {
        return;
    }

    PlayerRealm pr(player_ptr);

    if (pr.realm1().is_available()) {
        fprintf(fff, _("%sの魔法書\n", "%s Spellbook\n"), pr.realm1().get_name().data());
        for (SPELL_IDX i = 0; i < 32; i++) {
            const auto &spell = pr.realm1().get_spell_info(i);

            if (spell.slevel >= 99) {
                continue;
            }
            SUB_EXP spell_exp = player_ptr->spell_exp[i];
            auto skill_rank = PlayerSkill::spell_skill_rank(spell_exp);
            const auto &spell_name = pr.realm1().get_spell_name(i);
            fprintf(fff, "%-25s ", spell_name.data());
            if (pr.realm1().equals(RealmType::HISSATSU)) {
                if (show_actual_value) {
                    fprintf(fff, "----/---- ");
                }
                fprintf(fff, "[--]");
            } else {
                if (show_actual_value) {
                    fprintf(fff, "%4d/%4d ", spell_exp, PlayerSkill::spell_exp_at(PlayerSkillRank::MASTER));
                }
                if (skill_rank >= PlayerSkillRank::MASTER) {
                    fprintf(fff, "!");
                } else {
                    fprintf(fff, " ");
                }
                fprintf(fff, "%s", PlayerSkill::skill_rank_str(skill_rank));
            }

            if (cheat_xtra) {
                fprintf(fff, " %d", spell_exp);
            }
            fprintf(fff, "\n");
        }
    }

    if (pr.realm2().is_available()) {
        fprintf(fff, _("%sの魔法書\n", "\n%s Spellbook\n"), pr.realm2().get_name().data());
        for (SPELL_IDX i = 0; i < 32; i++) {
            const auto &spell = pr.realm2().get_spell_info(i);

            if (spell.slevel >= 99) {
                continue;
            }

            SUB_EXP spell_exp = player_ptr->spell_exp[i + 32];
            auto skill_rank = PlayerSkill::spell_skill_rank(spell_exp);
            const auto spell_name = pr.realm2().get_spell_name(i);
            fprintf(fff, "%-25s ", spell_name.data());
            if (show_actual_value) {
                fprintf(fff, "%4d/%4d ", spell_exp, PlayerSkill::spell_exp_at(PlayerSkillRank::MASTER));
            }
            if (skill_rank >= PlayerSkillRank::EXPERT) {
                fprintf(fff, "!");
            } else {
                fprintf(fff, " ");
            }
            fprintf(fff, "%s", PlayerSkill::skill_rank_str(skill_rank));
            if (cheat_xtra) {
                fprintf(fff, " %d", spell_exp);
            }
            fprintf(fff, "\n");
        }
    }

    angband_fclose(fff);
    FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, _("魔法の経験値", "Spell Proficiency"));
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
    if (!open_temporary_file(&fff, file_name)) {
        return;
    }

    for (auto i : PLAYER_SKILL_KIND_TYPE_RANGE) {
        SUB_EXP skill_exp = player_ptr->skill_exp[i];
        SUB_EXP skill_max = class_skills_info[enum2i(player_ptr->pclass)].s_max[i];
        fprintf(fff, "%-20s ", PlayerSkill::skill_name(i));
        if (show_actual_value) {
            fprintf(fff, "%4d/%4d ", std::min(skill_exp, skill_max), skill_max);
        }
        if (skill_exp >= skill_max) {
            fprintf(fff, "!");
        } else {
            fprintf(fff, " ");
        }
        auto skill_rank = (i == PlayerSkillKindType::RIDING) ? PlayerSkill::riding_skill_rank(skill_exp) : PlayerSkill::weapon_skill_rank(skill_exp);
        fprintf(fff, "%s", PlayerSkill::skill_rank_str(skill_rank));
        if (cheat_xtra) {
            fprintf(fff, " %d", skill_exp);
        }
        fprintf(fff, "\n");
    }

    angband_fclose(fff);
    FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, _("技能の経験値", "Miscellaneous Proficiency"));
    fd_kill(file_name);
}
