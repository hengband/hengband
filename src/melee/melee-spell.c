#include "melee/melee-spell.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "floor/cave.h"
#include "floor/floor.h"
#include "monster-floor/monster-move.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags4.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "mspell/assign-monster-spell.h"
#include "mspell/mspell-mask-definitions.h"
#include "mspell/mspell-util.h"
#include "mspell/mspells1.h"
#include "mspell/mspells2.h"
#include "pet/pet-util.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"

#define RF4_SPELL_SIZE 32
#define RF5_SPELL_SIZE 32
#define RF6_SPELL_SIZE 32

/*!
 * todo モンスターからモンスターへの呪文なのにplayer_typeが引数になり得るのは間違っている……
 * @brief モンスターが敵モンスターに特殊能力を使う処理のメインルーチン /
 * Monster tries to 'cast a spell' (or breath, etc) at another monster.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 術者のモンスターID
 * @return 実際に特殊能力を使った場合TRUEを返す
 * @details
 * The player is only disturbed if able to be affected by the spell.
 */
bool monst_spell_monst(player_type *target_ptr, MONSTER_IDX m_idx)
{
    POSITION y = 0, x = 0;
    int k;
    MONSTER_IDX target_idx = 0;
    int thrown_spell;
    HIT_POINT dam = 0;
    int start;
    int plus = 1;

    byte spell[96], num = 0;

    GAME_TEXT m_name[160];
    GAME_TEXT t_name[160];

#ifdef JP
#else
    char m_poss[160];
#endif

    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    monster_type *t_ptr = NULL;
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    bool see_m = is_seen(target_ptr, m_ptr);
    bool maneable = player_has_los_bold(target_ptr, m_ptr->fy, m_ptr->fx);
    bool pet = is_pet(m_ptr);
    bool in_no_magic_dungeon = (d_info[target_ptr->dungeon_idx].flags1 & DF1_NO_MAGIC) && floor_ptr->dun_level
        && (!floor_ptr->inside_quest || is_fixed_quest_idx(floor_ptr->inside_quest));
    bool can_use_lite_area = FALSE;
    bool can_remember;

    if (monster_confused_remaining(m_ptr))
        return FALSE;

    BIT_FLAGS f4 = r_ptr->flags4;
    BIT_FLAGS f5 = r_ptr->a_ability_flags1;
    BIT_FLAGS f6 = r_ptr->a_ability_flags2;
    if (target_ptr->pet_t_m_idx && pet) {
        target_idx = target_ptr->pet_t_m_idx;
        t_ptr = &floor_ptr->m_list[target_idx];
        if ((m_idx == target_idx) || !projectable(target_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx)) {
            target_idx = 0;
        }
    }

    if (!target_idx && m_ptr->target_y) {
        target_idx = floor_ptr->grid_array[m_ptr->target_y][m_ptr->target_x].m_idx;
        if (target_idx) {
            t_ptr = &floor_ptr->m_list[target_idx];
            if ((m_idx == target_idx) || ((target_idx != target_ptr->pet_t_m_idx) && !are_enemies(target_ptr, m_ptr, t_ptr))) {
                target_idx = 0;
            } else if (!projectable(target_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx)) {
                f4 &= RF4_INDIRECT_MASK;
                f5 &= RF5_INDIRECT_MASK;
                f6 &= RF6_INDIRECT_MASK;
            }
        }
    }

    if (!target_idx) {
        bool success = FALSE;
        if (target_ptr->phase_out) {
            start = randint1(floor_ptr->m_max - 1) + floor_ptr->m_max;
            if (randint0(2))
                plus = -1;
        } else
            start = floor_ptr->m_max + 1;

        for (int i = start; ((i < start + floor_ptr->m_max) && (i > start - floor_ptr->m_max)); i += plus) {
            MONSTER_IDX dummy = (i % floor_ptr->m_max);
            if (!dummy)
                continue;

            target_idx = dummy;
            t_ptr = &floor_ptr->m_list[target_idx];
            if (!monster_is_valid(t_ptr))
                continue;

            if ((m_idx == target_idx) || !are_enemies(target_ptr, m_ptr, t_ptr))
                continue;

            if (!projectable(target_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx))
                continue;

            success = TRUE;
            break;
        }

        if (!success)
            return FALSE;
    }

    y = t_ptr->fy;
    x = t_ptr->fx;
    reset_target(m_ptr);
    f6 &= ~(RF6_WORLD | RF6_TRAPS | RF6_FORGET);
    if (f4 & RF4_BR_LITE) {
        if (!los(target_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx))
            f4 &= ~(RF4_BR_LITE);
    }

    if (f6 & RF6_SPECIAL) {
        if ((m_ptr->r_idx != MON_ROLENTO) && (r_ptr->d_char != 'B'))
            f6 &= ~(RF6_SPECIAL);
    }

    if (f6 & RF6_DARKNESS) {
        bool vs_ninja = (target_ptr->pclass == CLASS_NINJA) && !is_hostile(t_ptr);
        if (vs_ninja && !(r_ptr->flags3 & (RF3_UNDEAD | RF3_HURT_LITE)) && !(r_ptr->flags7 & RF7_DARK_MASK))
            can_use_lite_area = TRUE;

        if (!(r_ptr->flags2 & RF2_STUPID)) {
            if (d_info[target_ptr->dungeon_idx].flags1 & DF1_DARKNESS)
                f6 &= ~(RF6_DARKNESS);
            else if (vs_ninja && !can_use_lite_area)
                f6 &= ~(RF6_DARKNESS);
        }
    }

    if (in_no_magic_dungeon && !(r_ptr->flags2 & RF2_STUPID)) {
        f4 &= (RF4_NOMAGIC_MASK);
        f5 &= (RF5_NOMAGIC_MASK);
        f6 &= (RF6_NOMAGIC_MASK);
    }

    if (floor_ptr->inside_arena || target_ptr->phase_out) {
        f4 &= ~(RF4_SUMMON_MASK);
        f5 &= ~(RF5_SUMMON_MASK);
        f6 &= ~(RF6_SUMMON_MASK | RF6_TELE_LEVEL);

        if (m_ptr->r_idx == MON_ROLENTO)
            f6 &= ~(RF6_SPECIAL);
    }

    if (target_ptr->phase_out && !one_in_(3)) {
        f6 &= ~(RF6_HEAL);
    }

    if (m_idx == target_ptr->riding) {
        f4 &= ~(RF4_RIDING_MASK);
        f5 &= ~(RF5_RIDING_MASK);
        f6 &= ~(RF6_RIDING_MASK);
    }

    if (pet) {
        f4 &= ~(RF4_SHRIEK);
        f6 &= ~(RF6_DARKNESS | RF6_TRAPS);

        if (!(target_ptr->pet_extra_flags & PF_TELEPORT)) {
            f6 &= ~(RF6_BLINK | RF6_TPORT | RF6_TELE_TO | RF6_TELE_AWAY | RF6_TELE_LEVEL);
        }

        if (!(target_ptr->pet_extra_flags & PF_ATTACK_SPELL)) {
            f4 &= ~(RF4_ATTACK_MASK);
            f5 &= ~(RF5_ATTACK_MASK);
            f6 &= ~(RF6_ATTACK_MASK);
        }

        if (!(target_ptr->pet_extra_flags & PF_SUMMON_SPELL)) {
            f4 &= ~(RF4_SUMMON_MASK);
            f5 &= ~(RF5_SUMMON_MASK);
            f6 &= ~(RF6_SUMMON_MASK);
        }

        if (!(target_ptr->pet_extra_flags & PF_BALL_SPELL) && (m_idx != target_ptr->riding)) {
            if ((f4 & (RF4_BALL_MASK & ~(RF4_ROCKET))) || (f5 & RF5_BALL_MASK) || (f6 & RF6_BALL_MASK)) {
                POSITION real_y = y;
                POSITION real_x = x;

                get_project_point(target_ptr, m_ptr->fy, m_ptr->fx, &real_y, &real_x, 0L);

                if (projectable(target_ptr, real_y, real_x, target_ptr->y, target_ptr->x)) {
                    int dist = distance(real_y, real_x, target_ptr->y, target_ptr->x);

                    if (dist <= 2) {
                        f4 &= ~(RF4_BALL_MASK & ~(RF4_ROCKET));
                        f5 &= ~(RF5_BALL_MASK);
                        f6 &= ~(RF6_BALL_MASK);
                    } else if (dist <= 4) {
                        f4 &= ~(RF4_BIG_BALL_MASK);
                        f5 &= ~(RF5_BIG_BALL_MASK);
                        f6 &= ~(RF6_BIG_BALL_MASK);
                    }
                } else if (f5 & RF5_BA_LITE) {
                    if ((distance(real_y, real_x, target_ptr->y, target_ptr->x) <= 4) && los(target_ptr, real_y, real_x, target_ptr->y, target_ptr->x))
                        f5 &= ~(RF5_BA_LITE);
                }
            }

            if (f4 & RF4_ROCKET) {
                POSITION real_y = y;
                POSITION real_x = x;
                get_project_point(target_ptr, m_ptr->fy, m_ptr->fx, &real_y, &real_x, PROJECT_STOP);
                if (projectable(target_ptr, real_y, real_x, target_ptr->y, target_ptr->x) && (distance(real_y, real_x, target_ptr->y, target_ptr->x) <= 2))
                    f4 &= ~(RF4_ROCKET);
            }

            if (((f4 & RF4_BEAM_MASK) || (f5 & RF5_BEAM_MASK) || (f6 & RF6_BEAM_MASK))
                && !direct_beam(target_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx, m_ptr)) {
                f4 &= ~(RF4_BEAM_MASK);
                f5 &= ~(RF5_BEAM_MASK);
                f6 &= ~(RF6_BEAM_MASK);
            }

            if ((f4 & RF4_BREATH_MASK) || (f5 & RF5_BREATH_MASK) || (f6 & RF6_BREATH_MASK)) {
                POSITION rad = (r_ptr->flags2 & RF2_POWERFUL) ? 3 : 2;
                if (!breath_direct(target_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx, rad, 0, TRUE)) {
                    f4 &= ~(RF4_BREATH_MASK);
                    f5 &= ~(RF5_BREATH_MASK);
                    f6 &= ~(RF6_BREATH_MASK);
                } else if ((f4 & RF4_BR_LITE) && !breath_direct(target_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx, rad, GF_LITE, TRUE)) {
                    f4 &= ~(RF4_BR_LITE);
                } else if ((f4 & RF4_BR_DISI) && !breath_direct(target_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx, rad, GF_DISINTEGRATE, TRUE)) {
                    f4 &= ~(RF4_BR_DISI);
                }
            }
        }

        if (f6 & RF6_SPECIAL) {
            if (m_ptr->r_idx == MON_ROLENTO) {
                if ((target_ptr->pet_extra_flags & (PF_ATTACK_SPELL | PF_SUMMON_SPELL)) != (PF_ATTACK_SPELL | PF_SUMMON_SPELL))
                    f6 &= ~(RF6_SPECIAL);
            } else if (r_ptr->d_char == 'B') {
                if ((target_ptr->pet_extra_flags & (PF_ATTACK_SPELL | PF_TELEPORT)) != (PF_ATTACK_SPELL | PF_TELEPORT))
                    f6 &= ~(RF6_SPECIAL);
            } else
                f6 &= ~(RF6_SPECIAL);
        }
    }

    if (!(r_ptr->flags2 & RF2_STUPID)) {
        if (((f4 & RF4_BOLT_MASK) || (f5 & RF5_BOLT_MASK) || (f6 & RF6_BOLT_MASK))
            && !clean_shot(target_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx, pet)) {
            f4 &= ~(RF4_BOLT_MASK);
            f5 &= ~(RF5_BOLT_MASK);
            f6 &= ~(RF6_BOLT_MASK);
        }

        if (((f4 & RF4_SUMMON_MASK) || (f5 & RF5_SUMMON_MASK) || (f6 & RF6_SUMMON_MASK)) && !(summon_possible(target_ptr, t_ptr->fy, t_ptr->fx))) {
            f4 &= ~(RF4_SUMMON_MASK);
            f5 &= ~(RF5_SUMMON_MASK);
            f6 &= ~(RF6_SUMMON_MASK);
        }

        if ((f4 & RF4_DISPEL) && !dispel_check_monster(target_ptr, m_idx, target_idx)) {
            f4 &= ~(RF4_DISPEL);
        }

        if ((f6 & RF6_RAISE_DEAD) && !raise_possible(target_ptr, m_ptr)) {
            f6 &= ~(RF6_RAISE_DEAD);
        }

        if (f6 & RF6_SPECIAL) {
            if ((m_ptr->r_idx == MON_ROLENTO) && !summon_possible(target_ptr, t_ptr->fy, t_ptr->fx)) {
                f6 &= ~(RF6_SPECIAL);
            }
        }
    }

    if (r_ptr->flags2 & RF2_SMART) {
        if ((m_ptr->hp < m_ptr->maxhp / 10) && (randint0(100) < 50)) {
            f4 &= (RF4_INT_MASK);
            f5 &= (RF5_INT_MASK);
            f6 &= (RF6_INT_MASK);
        }

        if ((f6 & RF6_TELE_LEVEL) && is_teleport_level_ineffective(target_ptr, (target_idx == target_ptr->riding) ? 0 : target_idx)) {
            f6 &= ~(RF6_TELE_LEVEL);
        }
    }

    if (!f4 && !f5 && !f6)
        return FALSE;

    for (k = 0; k < 32; k++) {
        if (f4 & (1L << k))
            spell[num++] = k + RF4_SPELL_START;
    }

    for (k = 0; k < 32; k++) {
        if (f5 & (1L << k))
            spell[num++] = k + RF5_SPELL_START;
    }

    for (k = 0; k < 32; k++) {
        if (f6 & (1L << k))
            spell[num++] = k + RF6_SPELL_START;
    }

    if (!num)
        return FALSE;

    if (!target_ptr->playing || target_ptr->is_dead)
        return FALSE;

    if (target_ptr->leaving)
        return FALSE;

    /* Get the monster name (or "it") */
    monster_desc(target_ptr, m_name, m_ptr, 0x00);
#ifdef JP
#else
    /* Get the monster possessive ("his"/"her"/"its") */
    monster_desc(target_ptr, m_poss, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);
#endif

    /* Get the target's name (or "it") */
    monster_desc(target_ptr, t_name, t_ptr, 0x00);

    thrown_spell = spell[randint0(num)];

    if (target_ptr->riding && (m_idx == target_ptr->riding))
        disturb(target_ptr, TRUE, TRUE);

    if (!spell_is_inate(thrown_spell) && (in_no_magic_dungeon || (monster_stunned_remaining(m_ptr) && one_in_(2)))) {
        disturb(target_ptr, TRUE, TRUE);
        if (see_m)
            msg_format(_("%^sは呪文を唱えようとしたが失敗した。", "%^s tries to cast a spell, but fails."), m_name);

        return TRUE;
    }

    if (!spell_is_inate(thrown_spell) && magic_barrier(target_ptr, m_idx)) {
        if (see_m)
            msg_format(_("反魔法バリアが%^sの呪文をかき消した。", "Anti magic barrier cancels the spell which %^s casts."), m_name);
        return TRUE;
    }

    can_remember = is_original_ap_and_seen(target_ptr, m_ptr);
    dam = monspell_to_monster(target_ptr, thrown_spell, y, x, m_idx, target_idx, FALSE);
    if (dam < 0)
        return FALSE;

    bool is_special_magic = m_ptr->ml;
    is_special_magic &= maneable;
    is_special_magic &= current_world_ptr->timewalk_m_idx == 0;
    is_special_magic &= !target_ptr->blind;
    is_special_magic &= target_ptr->pclass == CLASS_IMITATOR;
    is_special_magic &= thrown_spell != 167; /* Not RF6_SPECIAL */
    if (is_special_magic) {
        if (target_ptr->mane_num == MAX_MANE) {
            target_ptr->mane_num--;
            for (int i = 0; i < target_ptr->mane_num - 1; i++) {
                target_ptr->mane_spell[i] = target_ptr->mane_spell[i + 1];
                target_ptr->mane_dam[i] = target_ptr->mane_dam[i + 1];
            }
        }

        target_ptr->mane_spell[target_ptr->mane_num] = thrown_spell - RF4_SPELL_START;
        target_ptr->mane_dam[target_ptr->mane_num] = dam;
        target_ptr->mane_num++;
        target_ptr->new_mane = TRUE;

        target_ptr->redraw |= PR_IMITATION;
    }

    if (can_remember) {
        if (thrown_spell < RF4_SPELL_START + RF4_SPELL_SIZE) {
            r_ptr->r_flags4 |= (1L << (thrown_spell - RF4_SPELL_START));
            if (r_ptr->r_cast_spell < MAX_UCHAR)
                r_ptr->r_cast_spell++;
        } else if (thrown_spell < RF5_SPELL_START + RF5_SPELL_SIZE) {
            r_ptr->r_flags5 |= (1L << (thrown_spell - RF5_SPELL_START));
            if (r_ptr->r_cast_spell < MAX_UCHAR)
                r_ptr->r_cast_spell++;
        } else if (thrown_spell < RF6_SPELL_START + RF6_SPELL_SIZE) {
            r_ptr->r_flags6 |= (1L << (thrown_spell - RF6_SPELL_START));
            if (r_ptr->r_cast_spell < MAX_UCHAR)
                r_ptr->r_cast_spell++;
        }
    }

    if (target_ptr->is_dead && (r_ptr->r_deaths < MAX_SHORT) && !floor_ptr->inside_arena) {
        r_ptr->r_deaths++;
    }

    return TRUE;
}
