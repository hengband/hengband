#include "monster-race/monster-race-hook.h"
#include "dungeon/dungeon.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags4.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-flags8.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "mspell/mspell-mask-definitions.h"
#include "system/floor-type-definition.h"
#include "util/string-processor.h"

/*! 通常pit生成時のモンスターの構成条件ID / Race index for "monster pit (clone)" */
int vault_aux_race;

/*! 単一シンボルpit生成時の指定シンボル / Race index for "monster pit (symbol clone)" */
char vault_aux_char;

/*! ブレス属性に基づくドラゴンpit生成時条件マスク / Breath mask for "monster pit (dragon)" */
BIT_FLAGS vault_aux_dragon_mask4;

/*!
 * @brief pit/nestの基準となる単種モンスターを決める /
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void vault_prep_clone(player_type *player_ptr)
{
    get_mon_num_prep(player_ptr, vault_aux_simple, NULL);
    vault_aux_race = get_mon_num(player_ptr, player_ptr->current_floor_ptr->dun_level + 10, 0);
    get_mon_num_prep(player_ptr, NULL, NULL);
}

/*!
 * @brief pit/nestの基準となるモンスターシンボルを決める /
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void vault_prep_symbol(player_type *player_ptr)
{
    get_mon_num_prep(player_ptr, vault_aux_simple, NULL);
    MONRACE_IDX r_idx = get_mon_num(player_ptr, player_ptr->current_floor_ptr->dun_level + 10, 0);
    get_mon_num_prep(player_ptr, NULL, NULL);
    vault_aux_char = r_info[r_idx].d_char;
}

/*!
 * @brief pit/nestの基準となるドラゴンの種類を決める /
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void vault_prep_dragon(player_type *player_ptr)
{
    /* Unused */
    (void)player_ptr;

    switch (randint0(6)) {
    case 0: /* Black */
        vault_aux_dragon_mask4 = RF4_BR_ACID;
        break;
    case 1: /* Blue */
        vault_aux_dragon_mask4 = RF4_BR_ELEC;
        break;
    case 2: /* Red */
        vault_aux_dragon_mask4 = RF4_BR_FIRE;
        break;
    case 3: /* White */
        vault_aux_dragon_mask4 = RF4_BR_COLD;
        break;
    case 4: /* Green */
        vault_aux_dragon_mask4 = RF4_BR_POIS;
        break;
    default: /* Multi-hued */
        vault_aux_dragon_mask4 = (RF4_BR_ACID | RF4_BR_ELEC | RF4_BR_FIRE | RF4_BR_COLD | RF4_BR_POIS);
        break;
    }
}

/*!
 * @brief モンスターがクエストの討伐対象に成り得るかを返す / Hook function for quest monsters
 * @param r_idx モンスターＩＤ
 * @return 討伐対象にできるならTRUEを返す。
 */
bool mon_hook_quest(player_type *player_ptr, MONRACE_IDX r_idx)
{
    /* Unused */
    (void)player_ptr;

    monster_race *r_ptr = &r_info[r_idx];
    if (r_ptr->flags8 & RF8_WILD_ONLY)
        return FALSE;

    if (r_ptr->flags7 & RF7_AQUATIC)
        return FALSE;

    if (r_ptr->flags2 & RF2_MULTIPLY)
        return FALSE;

    if (r_ptr->flags7 & RF7_FRIENDLY)
        return FALSE;

    return TRUE;
}

/*!
 * @brief モンスターがダンジョンに出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return ダンジョンに出現するならばTRUEを返す
 */
bool mon_hook_dungeon(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if ((r_ptr->flags8 & RF8_WILD_ONLY) == 0)
        return TRUE;

    dungeon_type *d_ptr = &d_info[player_ptr->dungeon_idx];
    return (((d_ptr->mflags8 & RF8_WILD_MOUNTAIN) != 0) && ((r_ptr->flags8 & RF8_WILD_MOUNTAIN) != 0));
}

/*!
 * @brief モンスターが海洋に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 海洋に出現するならばTRUEを返す
 */
bool mon_hook_ocean(player_type *player_ptr, MONRACE_IDX r_idx)
{
    /* Unused */
    (void)player_ptr;

    monster_race *r_ptr = &r_info[r_idx];
    return (r_ptr->flags8 & RF8_WILD_OCEAN) != 0;
}

/*!
 * @brief モンスターが海岸に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 海岸に出現するならばTRUEを返す
 */
bool mon_hook_shore(player_type *player_ptr, MONRACE_IDX r_idx)
{
    /* Unused */
    (void)player_ptr;

    monster_race *r_ptr = &r_info[r_idx];
    return (r_ptr->flags8 & RF8_WILD_SHORE) != 0;
}

/*!
 * @brief モンスターが荒地に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 荒地に出現するならばTRUEを返す
 */
bool mon_hook_waste(player_type *player_ptr, MONRACE_IDX r_idx)
{
    /* Unused */
    (void)player_ptr;

    monster_race *r_ptr = &r_info[r_idx];
    return (r_ptr->flags8 & (RF8_WILD_WASTE | RF8_WILD_ALL)) != 0;
}

/*!
 * @brief モンスターが町に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 荒地に出現するならばTRUEを返す
 */
bool mon_hook_town(player_type *player_ptr, MONRACE_IDX r_idx)
{
    /* Unused */
    (void)player_ptr;

    monster_race *r_ptr = &r_info[r_idx];
    return (r_ptr->flags8 & (RF8_WILD_TOWN | RF8_WILD_ALL)) != 0;
}

/*!
 * @brief モンスターが森林に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 森林に出現するならばTRUEを返す
 */
bool mon_hook_wood(player_type *player_ptr, MONRACE_IDX r_idx)
{
    /* Unused */
    (void)player_ptr;

    monster_race *r_ptr = &r_info[r_idx];
    return (r_ptr->flags8 & (RF8_WILD_WOOD | RF8_WILD_ALL)) != 0;
}

/*!
 * @brief モンスターが火山に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 火山に出現するならばTRUEを返す
 */
bool mon_hook_volcano(player_type *player_ptr, MONRACE_IDX r_idx)
{
    /* Unused */
    (void)player_ptr;

    monster_race *r_ptr = &r_info[r_idx];
    return (r_ptr->flags8 & RF8_WILD_VOLCANO) != 0;
}

/*!
 * @brief モンスターが山地に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 山地に出現するならばTRUEを返す
 */
bool mon_hook_mountain(player_type *player_ptr, MONRACE_IDX r_idx)
{
    /* Unused */
    (void)player_ptr;

    monster_race *r_ptr = &r_info[r_idx];
    return (r_ptr->flags8 & RF8_WILD_MOUNTAIN) != 0;
}

/*!
 * @brief モンスターが草原に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 森林に出現するならばTRUEを返す
 */
bool mon_hook_grass(player_type *player_ptr, MONRACE_IDX r_idx)
{
    /* Unused */
    (void)player_ptr;

    monster_race *r_ptr = &r_info[r_idx];
    return (r_ptr->flags8 & (RF8_WILD_GRASS | RF8_WILD_ALL)) != 0;
}

/*!
 * @brief モンスターが深い水地形に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 深い水地形に出現するならばTRUEを返す
 */
bool mon_hook_deep_water(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!mon_hook_dungeon(player_ptr, r_idx))
        return FALSE;

    return (r_ptr->flags7 & RF7_AQUATIC) != 0;
}

/*!
 * @brief モンスターが浅い水地形に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 浅い水地形に出現するならばTRUEを返す
 */
bool mon_hook_shallow_water(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!mon_hook_dungeon(player_ptr, r_idx))
        return FALSE;

    return (r_ptr->flags2 & RF2_AURA_FIRE) != 0;
}

/*!
 * @brief モンスターが溶岩地形に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 溶岩地形に出現するならばTRUEを返す
 */
bool mon_hook_lava(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!mon_hook_dungeon(player_ptr, r_idx))
        return FALSE;

    return ((r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK) || (r_ptr->flags7 & RF7_CAN_FLY)) && !(r_ptr->flags3 & RF3_AURA_COLD);
}

/*!
 * @brief モンスターが通常の床地形に出現するかどうかを返す
 * @param r_idx 判定するモンスターの種族ID
 * @return 通常の床地形に出現するならばTRUEを返す
 */
bool mon_hook_floor(player_type *player_ptr, MONRACE_IDX r_idx)
{
    /* Unused */
    (void)player_ptr;

    monster_race *r_ptr = &r_info[r_idx];
    if (!(r_ptr->flags7 & RF7_AQUATIC) || (r_ptr->flags7 & RF7_CAN_FLY))
        return TRUE;
    else
        return FALSE;
}

/*
 * Helper function for "glass room"
 */
bool vault_aux_lite(player_type *player_ptr, MONRACE_IDX r_idx)
{
    /* Unused */
    (void)player_ptr;

    monster_race *r_ptr = &r_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx))
        return FALSE;

    if (!(r_ptr->flags4 & RF4_BR_LITE) && !(r_ptr->a_ability_flags1 & RF5_BA_LITE))
        return FALSE;

    if (r_ptr->flags2 & (RF2_PASS_WALL | RF2_KILL_WALL))
        return FALSE;

    if (r_ptr->flags4 & RF4_BR_DISI)
        return FALSE;

    return TRUE;
}

/*
 * Helper function for "glass room"
 */
bool vault_aux_shards(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx))
        return FALSE;

    if (!(r_ptr->flags4 & RF4_BR_SHAR))
        return FALSE;

    return TRUE;
}

/*!
 * @brief モンスターがVault生成の最低必要条件を満たしているかを返す /
 * Helper monster selection function
 * @param r_idx 確認したいモンスター種族ID
 * @return Vault生成の最低必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_simple(player_type *player_ptr, MONRACE_IDX r_idx)
{
    /* Unused */
    (void)player_ptr;

    return (vault_monster_okay(player_ptr, r_idx));
}

/*!
 * @brief モンスターがゼリーnestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (jelly)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_jelly(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx))
        return FALSE;

    if ((r_ptr->flags2 & RF2_KILL_BODY) && !(r_ptr->flags1 & RF1_NEVER_BLOW))
        return FALSE;

    if (r_ptr->flags3 & (RF3_EVIL))
        return FALSE;

    if (!angband_strchr("ijm,", r_ptr->d_char))
        return FALSE;

    return TRUE;
}

/*!
 * @brief モンスターが動物nestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (animal)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_animal(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx))
        return FALSE;

    if (!(r_ptr->flags3 & (RF3_ANIMAL)))
        return FALSE;

    return TRUE;
}

/*!
 * @brief モンスターがアンデッドnestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (undead)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_undead(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx))
        return FALSE;

    if (!(r_ptr->flags3 & (RF3_UNDEAD)))
        return FALSE;

    return TRUE;
}

/*!
 * @brief モンスターが聖堂nestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (chapel)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_chapel_g(player_type *player_ptr, MONRACE_IDX r_idx)
{
    static int chapel_list[] = { MON_NOV_PRIEST, MON_NOV_PALADIN, MON_NOV_PRIEST_G, MON_NOV_PALADIN_G, MON_PRIEST, MON_JADE_MONK, MON_IVORY_MONK,
        MON_ULTRA_PALADIN, MON_EBONY_MONK, MON_W_KNIGHT, MON_KNI_TEMPLAR, MON_PALADIN, MON_TOPAZ_MONK, 0 };

    monster_race *r_ptr = &r_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx))
        return FALSE;

    if (r_ptr->flags3 & (RF3_EVIL))
        return FALSE;

    if ((r_idx == MON_A_GOLD) || (r_idx == MON_A_SILVER))
        return FALSE;

    if (r_ptr->d_char == 'A')
        return TRUE;

    for (int i = 0; chapel_list[i]; i++)
        if (r_idx == chapel_list[i])
            return TRUE;

    return FALSE;
}

/*!
 * @brief モンスターが犬小屋nestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (kennel)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_kennel(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx))
        return FALSE;

    if (!angband_strchr("CZ", r_ptr->d_char))
        return FALSE;

    return TRUE;
}

/*!
 * @brief モンスターがミミックnestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (mimic)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_mimic(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx))
        return FALSE;

    if (!angband_strchr("!$&(/=?[\\|", r_ptr->d_char))
        return FALSE;

    return TRUE;
}

/*!
 * @brief モンスターが単一クローンnestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (clone)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_clone(player_type *player_ptr, MONRACE_IDX r_idx)
{
    if (!vault_monster_okay(player_ptr, r_idx))
        return FALSE;

    return (r_idx == vault_aux_race);
}

/*!
 * @brief モンスターが邪悪属性シンボルクローンnestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (symbol clone)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_symbol_e(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx))
        return FALSE;

    if ((r_ptr->flags2 & RF2_KILL_BODY) && !(r_ptr->flags1 & RF1_NEVER_BLOW))
        return FALSE;

    if (r_ptr->flags3 & (RF3_GOOD))
        return FALSE;

    if (r_ptr->d_char != vault_aux_char)
        return FALSE;

    return TRUE;
}

/*!
 * @brief モンスターが善良属性シンボルクローンnestの生成必要条件を満たしているかを返す /
 * Helper function for "monster nest (symbol clone)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_symbol_g(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx))
        return FALSE;

    if ((r_ptr->flags2 & RF2_KILL_BODY) && !(r_ptr->flags1 & RF1_NEVER_BLOW))
        return FALSE;

    if (r_ptr->flags3 & (RF3_EVIL))
        return FALSE;

    if (r_ptr->d_char != vault_aux_char)
        return FALSE;

    return TRUE;
}

/*!
 * @brief モンスターがオークpitの生成必要条件を満たしているかを返す /
 * Helper function for "monster pit (orc)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_orc(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx))
        return FALSE;

    if (!(r_ptr->flags3 & RF3_ORC))
        return FALSE;

    if (r_ptr->flags3 & RF3_UNDEAD)
        return FALSE;

    return TRUE;
}

/*!
 * @brief モンスターがトロルpitの生成必要条件を満たしているかを返す /
 * Helper function for "monster pit (troll)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_troll(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx))
        return FALSE;

    if (!(r_ptr->flags3 & RF3_TROLL))
        return FALSE;

    if (r_ptr->flags3 & RF3_UNDEAD)
        return FALSE;

    return TRUE;
}

/*!
 * @brief モンスターが巨人pitの生成必要条件を満たしているかを返す /
 * Helper function for "monster pit (giant)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_giant(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx))
        return FALSE;

    if (!(r_ptr->flags3 & RF3_GIANT))
        return FALSE;

    if (r_ptr->flags3 & RF3_GOOD)
        return FALSE;

    if (r_ptr->flags3 & RF3_UNDEAD)
        return FALSE;

    return TRUE;
}

/*!
 * @brief モンスターがドラゴンpitの生成必要条件を満たしているかを返す /
 * Helper function for "monster pit (dragon)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_dragon(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx))
        return FALSE;

    if (!(r_ptr->flags3 & RF3_DRAGON))
        return FALSE;

    if (r_ptr->flags4 != vault_aux_dragon_mask4)
        return FALSE;

    if (r_ptr->flags3 & RF3_UNDEAD)
        return FALSE;

    return TRUE;
}

/*!
 * @brief モンスターが悪魔pitの生成必要条件を満たしているかを返す /
 * Helper function for "monster pit (demon)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_demon(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx))
        return FALSE;

    if ((r_ptr->flags2 & RF2_KILL_BODY) && !(r_ptr->flags1 & RF1_NEVER_BLOW))
        return FALSE;

    if (!(r_ptr->flags3 & RF3_DEMON))
        return FALSE;

    return TRUE;
}

/*!
 * @brief モンスターが狂気pitの生成必要条件を満たしているかを返す /
 * Helper function for "monster pit (lovecraftian)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_cthulhu(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!vault_monster_okay(player_ptr, r_idx))
        return FALSE;

    if ((r_ptr->flags2 & RF2_KILL_BODY) && !(r_ptr->flags1 & RF1_NEVER_BLOW))
        return FALSE;

    if (!(r_ptr->flags2 & (RF2_ELDRITCH_HORROR)))
        return FALSE;

    return TRUE;
}

/*!
 * @brief モンスターがダークエルフpitの生成必要条件を満たしているかを返す /
 * Helper function for "monster pit (dark elf)"
 * @param r_idx 確認したいモンスター種族ID
 * @return 生成必要条件を満たしているならTRUEを返す。
 */
bool vault_aux_dark_elf(player_type *player_ptr, MONRACE_IDX r_idx)
{
    static int dark_elf_list[] = {
        MON_D_ELF,
        MON_D_ELF_MAGE,
        MON_D_ELF_WARRIOR,
        MON_D_ELF_PRIEST,
        MON_D_ELF_LORD,
        MON_D_ELF_WARLOCK,
        MON_D_ELF_DRUID,
        MON_NIGHTBLADE,
        MON_D_ELF_SORC,
        MON_D_ELF_SHADE,
        0,
    };

    if (!vault_monster_okay(player_ptr, r_idx))
        return FALSE;

    for (int i = 0; dark_elf_list[i]; i++)
        if (r_idx == dark_elf_list[i])
            return TRUE;

    return FALSE;
}

/*!
 * @brief モンスターが生命体かどうかを返す
 * Is the monster "alive"?
 * @param r_ptr 判定するモンスターの種族情報構造体参照ポインタ
 * @return 生命体ならばTRUEを返す
 * @details
 * Used to determine the message to print for a killed monster.
 * ("dies", "destroyed")
 */
bool monster_living(MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    return (r_ptr->flags3 & (RF3_DEMON | RF3_UNDEAD | RF3_NONLIVING)) == 0;
}

/*!
 * @brief モンスターが特殊能力上、賞金首から排除する必要があるかどうかを返す。
 * Is the monster "alive"? / Is this monster declined to be questor or bounty?
 * @param r_idx モンスターの種族ID
 * @return 賞金首に加えられないならばTRUEを返す
 * @details
 * 実質バーノール＝ルパート用。
 */
bool no_questor_or_bounty_uniques(MONRACE_IDX r_idx)
{
    switch (r_idx) {
        /*
         * Decline them to be questor or bounty because they use
         * special motion "split and combine"
         */
    case MON_BANORLUPART:
    case MON_BANOR:
    case MON_LUPART:
        return TRUE;
    default:
        return FALSE;
    }
}

/*!
 * @brief バルログが死体を食べられるモンスターかの判定 / Hook function for human corpses
 * @param r_idx モンスターＩＤ
 * @return 死体を食べられるならTRUEを返す。
 */
bool monster_hook_human(player_type *player_ptr, MONRACE_IDX r_idx)
{
    /* Unused */
    (void)player_ptr;

    monster_race *r_ptr = &r_info[r_idx];
    if (r_ptr->flags1 & (RF1_UNIQUE))
        return FALSE;

    if (angband_strchr("pht", r_ptr->d_char))
        return TRUE;

    return FALSE;
}

/*!
 * @brief 悪夢の元凶となるモンスターかどうかを返す。
 * @param r_idx 判定対象となるモンスターのＩＤ
 * @return 悪夢の元凶となり得るか否か。
 */
bool get_nightmare(player_type *player_ptr, MONRACE_IDX r_idx)
{
    monster_race *r_ptr = &r_info[r_idx];
    if (!(r_ptr->flags2 & (RF2_ELDRITCH_HORROR)))
        return FALSE;

    if (r_ptr->level <= player_ptr->lev)
        return FALSE;

    return TRUE;
}

/*!
 * @brief モンスター種族が釣れる種族かどうかを判定する。
 * @param r_idx 判定したいモンスター種族のID
 * @return 釣れる対象ならばTRUEを返す
 */
bool monster_is_fishing_target(player_type *player_ptr, MONRACE_IDX r_idx)
{
    /* Unused */
    (void)player_ptr;

    monster_race *r_ptr = &r_info[r_idx];
    if ((r_ptr->flags7 & RF7_AQUATIC) && !(r_ptr->flags1 & RF1_UNIQUE) && angband_strchr("Jjlw", r_ptr->d_char))
        return TRUE;
    else
        return FALSE;
}

/*!
 * @brief モンスター闘技場に参加できるモンスターの判定
 * @param r_idx モンスターＩＤ
 * @details 基準はNEVER_MOVE MULTIPLY QUANTUM RF7_AQUATIC RF7_CHAMELEONのいずれも持たず、
 * 自爆以外のなんらかのHP攻撃手段を持っていること。
 * @return 参加できるか否か
 */
bool monster_can_entry_arena(player_type *player_ptr, MONRACE_IDX r_idx)
{
    /* Unused */
    (void)player_ptr;

    HIT_POINT dam = 0;
    monster_race *r_ptr = &r_info[r_idx];
    bool unselectable = (r_ptr->flags1 & RF1_NEVER_MOVE) != 0;
    unselectable |= (r_ptr->flags2 & RF2_MULTIPLY) != 0;
    unselectable |= ((r_ptr->flags2 & RF2_QUANTUM) != 0) && ((r_ptr->flags1 & RF1_UNIQUE) == 0);
    unselectable |= (r_ptr->flags7 & RF7_AQUATIC) != 0;
    unselectable |= (r_ptr->flags7 & RF7_CHAMELEON) != 0;
    if (unselectable)
        return FALSE;

    for (int i = 0; i < 4; i++) {
        if (r_ptr->blow[i].method == RBM_EXPLODE)
            return FALSE;

        if (r_ptr->blow[i].effect != RBE_DR_MANA)
            dam += r_ptr->blow[i].d_dice;
    }

    if (!dam && !(r_ptr->flags4 & (RF4_BOLT_MASK | RF4_BEAM_MASK | RF4_BALL_MASK | RF4_BREATH_MASK))
        && !(r_ptr->a_ability_flags1 & (RF5_BOLT_MASK | RF5_BEAM_MASK | RF5_BALL_MASK | RF5_BREATH_MASK))
        && !(r_ptr->a_ability_flags2 & (RF6_BOLT_MASK | RF6_BEAM_MASK | RF6_BALL_MASK | RF6_BREATH_MASK)))
        return FALSE;

    return TRUE;
}

/*!
 * @brief モンスターが人形のベースにできるかを返す
 * @param r_idx チェックしたいモンスター種族のID
 * @return 人形にできるならTRUEを返す
 */
bool item_monster_okay(player_type *player_ptr, MONRACE_IDX r_idx)
{
    /* Unused */
    (void)player_ptr;

    monster_race *r_ptr = &r_info[r_idx];
    if (r_ptr->flags1 & RF1_UNIQUE)
        return FALSE;

    if (r_ptr->flags7 & RF7_KAGE)
        return FALSE;

    if (r_ptr->flagsr & RFR_RES_ALL)
        return FALSE;

    if (r_ptr->flags7 & RF7_NAZGUL)
        return FALSE;

    if (r_ptr->flags1 & RF1_FORCE_DEPTH)
        return FALSE;

    if (r_ptr->flags7 & RF7_UNIQUE2)
        return FALSE;

    return TRUE;
}

/*!
 * vaultに配置可能なモンスターの条件を指定する / Monster validation
 * @param r_idx モンスター種別ID
 * @param Vaultに配置可能であればTRUE
 * @details
 * Line 1 -- forbid town monsters
 * Line 2 -- forbid uniques
 * Line 3 -- forbid aquatic monsters
 */
bool vault_monster_okay(player_type *player_ptr, MONRACE_IDX r_idx)
{
    return (mon_hook_dungeon(player_ptr, r_idx) && !(r_info[r_idx].flags1 & RF1_UNIQUE) && !(r_info[r_idx].flags7 & RF7_UNIQUE2)
        && !(r_info[r_idx].flagsr & RFR_RES_ALL)
        && !(r_info[r_idx].flags7 & RF7_AQUATIC));
}
