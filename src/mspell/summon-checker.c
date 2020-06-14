﻿#include "mspell/summon-checker.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags4.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-flags8.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-util.h"
#include "spell/spells-summon.h"

/*!
 * @brief 指定されたモンスター種族がsummon_specific_typeで指定された召喚条件に合うかどうかを返す
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 召喚条件が一致するならtrue
 * @details
 */
bool check_summon_specific(player_type *player_ptr, MONRACE_IDX summoner_idx, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    bool is_match = FALSE;

    switch (summon_specific_type) {
    case SUMMON_ANT: {
        is_match = (r_ptr->d_char == 'a');
        break;
    }
    case SUMMON_SPIDER: {
        is_match = (r_ptr->d_char == 'S');
        break;
    }
    case SUMMON_HOUND: {
        is_match = ((r_ptr->d_char == 'C') || (r_ptr->d_char == 'Z'));
        break;
    }
    case SUMMON_HYDRA: {
        is_match = (r_ptr->d_char == 'M');
        break;
    }
    case SUMMON_ANGEL: {
        is_match = (r_ptr->d_char == 'A' && ((r_ptr->flags3 & RF3_EVIL) || (r_ptr->flags3 & RF3_GOOD)));
        break;
    }
    case SUMMON_DEMON: {
        is_match = (r_ptr->flags3 & RF3_DEMON);
        break;
    }
    case SUMMON_UNDEAD: {
        is_match = (r_ptr->flags3 & RF3_UNDEAD);
        break;
    }
    case SUMMON_DRAGON: {
        is_match = (r_ptr->flags3 & RF3_DRAGON);
        break;
    }
    case SUMMON_HI_UNDEAD: {
        is_match = ((r_ptr->d_char == 'L') || (r_ptr->d_char == 'V') || (r_ptr->d_char == 'W'));
        break;
    }
    case SUMMON_HI_DRAGON: {
        is_match = (r_ptr->d_char == 'D');
        break;
    }
    case SUMMON_HI_DEMON: {
        is_match = (((r_ptr->d_char == 'U') || (r_ptr->d_char == 'H') || (r_ptr->d_char == 'B')) && (r_ptr->flags3 & RF3_DEMON)) ? TRUE : FALSE;
        break;
    }
    case SUMMON_AMBERITES: {
        is_match = (r_ptr->flags3 & (RF3_AMBERITE)) ? TRUE : FALSE;
        break;
    }
    case SUMMON_UNIQUE: {
        is_match = (r_ptr->flags1 & (RF1_UNIQUE)) ? TRUE : FALSE;
        break;
    }
    case SUMMON_MOLD: {
        is_match = (r_ptr->d_char == 'm');
        break;
    }
    case SUMMON_BAT: {
        is_match = (r_ptr->d_char == 'b');
        break;
    }
    case SUMMON_QUYLTHULG: {
        is_match = (r_ptr->d_char == 'Q');
        break;
    }
    case SUMMON_COIN_MIMIC: {
        is_match = (r_ptr->d_char == '$');
        break;
    }
    case SUMMON_MIMIC: {
        is_match = ((r_ptr->d_char == '!') || (r_ptr->d_char == '?') || (r_ptr->d_char == '=') || (r_ptr->d_char == '$') || (r_ptr->d_char == '|'));
        break;
    }
    case SUMMON_GOLEM: {
        is_match = (r_ptr->d_char == 'g');
        break;
    }
    case SUMMON_CYBER: {
        is_match = ((r_ptr->d_char == 'U') && (r_ptr->flags4 & RF4_ROCKET));
        break;
    }
    case SUMMON_KIN: {
        SYMBOL_CODE summon_kin_type;
        if (summoner_idx) {
            summon_kin_type = r_info[summoner_idx].d_char;
        } else {
            summon_kin_type = get_summon_symbol_from_player(player_ptr);
        }

        is_match = ((r_ptr->d_char == summon_kin_type) && (r_idx != MON_HAGURE));
        break;
    }
    case SUMMON_DAWN: {
        is_match = (r_idx == MON_DAWN);
        break;
    }
    case SUMMON_ANIMAL: {
        is_match = (r_ptr->flags3 & (RF3_ANIMAL));
        break;
    }
    case SUMMON_ANIMAL_RANGER: {
        is_match = ((r_ptr->flags3 & (RF3_ANIMAL)) && (angband_strchr("abcflqrwBCHIJKMRS", r_ptr->d_char)) && !(r_ptr->flags3 & (RF3_DRAGON))
            && !(r_ptr->flags3 & (RF3_EVIL)) && !(r_ptr->flags3 & (RF3_UNDEAD)) && !(r_ptr->flags3 & (RF3_DEMON)) && !(r_ptr->flags2 & (RF2_MULTIPLY))
            && !(r_ptr->flags4 || r_ptr->a_ability_flags1 || r_ptr->a_ability_flags2));
        break;
    }
    case SUMMON_HI_DRAGON_LIVING: {
        is_match = ((r_ptr->d_char == 'D') && monster_living(r_idx));
        break;
    }
    case SUMMON_LIVING: {
        is_match = monster_living(r_idx);
        break;
    }
    case SUMMON_PHANTOM: {
        is_match = (r_idx == MON_PHANTOM_B || r_idx == MON_PHANTOM_W);
        break;
    }
    case SUMMON_BLUE_HORROR: {
        is_match = (r_idx == MON_BLUE_HORROR);
        break;
    }
    case SUMMON_ELEMENTAL: {
        is_match = (r_ptr->d_char == 'E');
        break;
    }
    case SUMMON_VORTEX: {
        is_match = (r_ptr->d_char == 'v');
        break;
    }
    case SUMMON_HYBRID: {
        is_match = (r_ptr->d_char == 'H');
        break;
    }
    case SUMMON_BIRD: {
        is_match = (r_ptr->d_char == 'B');
        break;
    }
    case SUMMON_KAMIKAZE: {
        int i;
        for (i = 0; i < 4; i++)
            if (r_ptr->blow[i].method == RBM_EXPLODE)
                is_match = TRUE;
        break;
    }
    case SUMMON_KAMIKAZE_LIVING: {
        int i;

        for (i = 0; i < 4; i++)
            if (r_ptr->blow[i].method == RBM_EXPLODE)
                is_match = TRUE;
        is_match = (is_match && monster_living(r_idx));
        break;
    }
    case SUMMON_MANES: {
        is_match = (r_idx == MON_MANES);
        break;
    }
    case SUMMON_LOUSE: {
        is_match = (r_idx == MON_LOUSE);
        break;
    }
    case SUMMON_GUARDIANS: {
        is_match = (r_ptr->flags7 & RF7_GUARDIAN);
        break;
    }
    case SUMMON_KNIGHTS: {
        is_match = ((r_idx == MON_NOV_PALADIN) || (r_idx == MON_NOV_PALADIN_G) || (r_idx == MON_PALADIN) || (r_idx == MON_W_KNIGHT)
            || (r_idx == MON_ULTRA_PALADIN) || (r_idx == MON_KNI_TEMPLAR));
        break;
    }
    case SUMMON_EAGLES: {
        is_match = (r_ptr->d_char == 'B' && (r_ptr->flags8 & RF8_WILD_MOUNTAIN) && (r_ptr->flags8 & RF8_WILD_ONLY));
        break;
    }
    case SUMMON_PIRANHAS: {
        is_match = (r_idx == MON_PIRANHA);
        break;
    }
    case SUMMON_ARMAGE_GOOD: {
        is_match = (r_ptr->d_char == 'A' && (r_ptr->flags3 & RF3_GOOD));
        break;
    }
    case SUMMON_ARMAGE_EVIL: {
        is_match = ((r_ptr->flags3 & RF3_DEMON) || (r_ptr->d_char == 'A' && (r_ptr->flags3 & RF3_EVIL)));
        break;
    }
    }

    return is_match;
}
