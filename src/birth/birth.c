/*!
 * @file birth.c
 * @brief プレイヤーの作成を行う / Create a player character
 * @date 2013/12/28
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 *\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2013 Deskull Doxygen向けのコメント整理\n
 */

#include "system/angband.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "io/read-pref-file.h"
#include "main/music-definitions-table.h"
#include "main/sound-definitions-table.h"
#include "market/building.h"
#include "util/util.h"

#include "object/artifact.h"
#include "autopick/autopick.h"
#include "player/avatar.h"
#include "birth/birth.h"
#include "birth/birth-explanations-table.h"
#include "cmd-pet.h"
#include "cmd/cmd-dump.h"
#include "cmd/cmd-help.h"
#include "dungeon/dungeon-file.h"
#include "dungeon/dungeon.h"
#include "floor/floor-town.h"
#include "floor/floor.h"
#include "birth/history.h"
#include "io/write-diary.h"
#include "locale/japanese.h"
#include "market/store.h"
#include "monster/monster.h"
#include "monster/monsterrace-hook.h"
#include "monster/monster-race.h"
#include "object/object-ego.h"
#include "object/object-kind.h"
#include "player/patron.h"
#include "player/player-class.h"
#include "player/player-sex.h"
#include "player/player-status.h"
#include "player/process-name.h"
#include "player/race-info-table.h"
#include "dungeon/quest.h"
#include "realm/realm.h"
#include "io/save.h"
#include "spell/spells-util.h"
#include "view/display-main-window.h" // 暫定。後で消す.
#include "view/display-player.h" // 暫定。後で消す.
#include "floor/wild.h"
#include "world/world.h"
#include "birth/birth-util.h"
#include "birth/birth-select-realm.h"
#include "birth/quick-start.h"
#include "birth/birth-stat.h"
#include "birth/history-generator.h"
#include "birth/birth-body-spec.h"
#include "birth/initial-equipments-table.h"
#include "view/display-birth.h" // 暫定。後で消す予定。

/*!
 * オートローラーの内容を描画する間隔 /
 * How often the autoroller will update the display and pause
 * to check for user interuptions.
 * Bigger values will make the autoroller faster, but slower
 * system may have problems because the user can't stop the
 * autoroller for this number of rolls.
 */
#define AUTOROLLER_STEP 5431L

/*!
  * ランダムクエストのモンスターを確定するために試行する回数 / Maximum number of tries for selection of a proper quest monster
  */
#define MAX_TRIES 100

/* 選択可能な職業の最大数 */
#define MAX_CLASS_CHOICE MAX_CLASS

/*! オートローラの能力値的要求水準 / Autoroll limit */
static s16b stat_limit[6];

/*! オートローラの年齢、身長、体重、社会的地位の要求水準 */
static struct {
    s16b agemin, agemax;
    s16b htmin, htmax;
    s16b wtmin, wtmax;
    s16b scmin, scmax;
} chara_limit;

/*! オートローラ中、各能力値が水準を超えた回数 / Autoroll matches */
s32b stat_match[6];

/*! オートローラの試行回数 / Autoroll round */
s32b auto_round;

/*!
 * @brief ベースアイテム構造体の鑑定済みフラグをリセットする。
 * @return なし
 */
static void k_info_reset(void)
{
    for (int i = 1; i < max_k_idx; i++) {
        object_kind* k_ptr = &k_info[i];
        k_ptr->tried = FALSE;
        k_ptr->aware = FALSE;
    }
}

/*!
 * @brief プレイヤー構造体の内容を初期値で消去する(名前を除く) / Clear all the global "character" data (without name)
 * @return なし
 */
static void player_wipe_without_name(player_type* creature_ptr)
{
    player_type tmp;

    COPY(&tmp, creature_ptr, player_type);
    if (creature_ptr->last_message)
        string_free(creature_ptr->last_message);

    if (creature_ptr->inventory_list != NULL)
        C_WIPE(creature_ptr->inventory_list, INVEN_TOTAL, object_type);

    (void)WIPE(creature_ptr, player_type);

    //TODO: キャラ作成からゲーム開始までに  current_floor_ptr を参照しなければならない処理は今後整理して外す。
    creature_ptr->current_floor_ptr = &floor_info;
    C_MAKE(creature_ptr->inventory_list, INVEN_TOTAL, object_type);
    for (int i = 0; i < 4; i++)
        strcpy(creature_ptr->history[i], "");

    for (int i = 0; i < max_q_idx; i++) {
        quest_type* const q_ptr = &quest[i];
        q_ptr->status = QUEST_STATUS_UNTAKEN;
        q_ptr->cur_num = 0;
        q_ptr->max_num = 0;
        q_ptr->type = 0;
        q_ptr->level = 0;
        q_ptr->r_idx = 0;
        q_ptr->complev = 0;
        q_ptr->comptime = 0;
    }

    creature_ptr->total_weight = 0;
    creature_ptr->inven_cnt = 0;
    creature_ptr->equip_cnt = 0;
    for (int i = 0; i < INVEN_TOTAL; i++)
        object_wipe(&creature_ptr->inventory_list[i]);

    for (int i = 0; i < max_a_idx; i++) {
        artifact_type* a_ptr = &a_info[i];
        a_ptr->cur_num = 0;
    }

    k_info_reset();
    for (int i = 1; i < max_r_idx; i++) {
        monster_race* r_ptr = &r_info[i];
        r_ptr->cur_num = 0;
        r_ptr->max_num = 100;
        if (r_ptr->flags1 & RF1_UNIQUE)
            r_ptr->max_num = 1;
        else if (r_ptr->flags7 & RF7_NAZGUL)
            r_ptr->max_num = MAX_NAZGUL_NUM;

        r_ptr->r_pkills = 0;
        r_ptr->r_akills = 0;
    }

    creature_ptr->food = PY_FOOD_FULL - 1;
    if (creature_ptr->pclass == CLASS_SORCERER) {
        creature_ptr->spell_learned1 = creature_ptr->spell_learned2 = 0xffffffffL;
        creature_ptr->spell_worked1 = creature_ptr->spell_worked2 = 0xffffffffL;
    } else {
        creature_ptr->spell_learned1 = creature_ptr->spell_learned2 = 0L;
        creature_ptr->spell_worked1 = creature_ptr->spell_worked2 = 0L;
    }

    creature_ptr->spell_forgotten1 = creature_ptr->spell_forgotten2 = 0L;
    for (int i = 0; i < 64; i++)
        creature_ptr->spell_order[i] = 99;

    creature_ptr->learned_spells = 0;
    creature_ptr->add_spells = 0;
    creature_ptr->knowledge = 0;
    creature_ptr->mutant_regenerate_mod = 100;

    cheat_peek = FALSE;
    cheat_hear = FALSE;
    cheat_room = FALSE;
    cheat_xtra = FALSE;
    cheat_know = FALSE;
    cheat_live = FALSE;
    cheat_save = FALSE;
    cheat_diary_output = FALSE;
    cheat_turn = FALSE;

    current_world_ptr->total_winner = FALSE;
    creature_ptr->timewalk = FALSE;
    creature_ptr->panic_save = 0;

    current_world_ptr->noscore = 0;
    current_world_ptr->wizard = FALSE;
    creature_ptr->wait_report_score = FALSE;
    creature_ptr->pet_follow_distance = PET_FOLLOW_DIST;
    creature_ptr->pet_extra_flags = (PF_TELEPORT | PF_ATTACK_SPELL | PF_SUMMON_SPELL);

    for (int i = 0; i < current_world_ptr->max_d_idx; i++)
        max_dlv[i] = 0;

    creature_ptr->visit = 1;
    creature_ptr->wild_mode = FALSE;

    for (int i = 0; i < 108; i++) {
        creature_ptr->magic_num1[i] = 0;
        creature_ptr->magic_num2[i] = 0;
    }

    creature_ptr->max_plv = creature_ptr->lev = 1;
    creature_ptr->arena_number = 0;
    creature_ptr->current_floor_ptr->inside_arena = FALSE;
    creature_ptr->current_floor_ptr->inside_quest = 0;
    for (int i = 0; i < MAX_MANE; i++) {
        creature_ptr->mane_spell[i] = -1;
        creature_ptr->mane_dam[i] = 0;
    }

    creature_ptr->mane_num = 0;
    creature_ptr->exit_bldg = TRUE;
    creature_ptr->today_mon = 0;
    update_gambling_monsters(creature_ptr);
    creature_ptr->muta1 = 0;
    creature_ptr->muta2 = 0;
    creature_ptr->muta3 = 0;

    for (int i = 0; i < 8; i++)
        creature_ptr->virtues[i] = 0;

    creature_ptr->dungeon_idx = 0;
    if (vanilla_town || ironman_downward) {
        creature_ptr->recall_dungeon = DUNGEON_ANGBAND;
    } else {
        creature_ptr->recall_dungeon = DUNGEON_GALGALS;
    }

    memcpy(creature_ptr->name, tmp.name, sizeof(tmp.name));
}

/*!
 * @brief ダンジョン内部のクエストを初期化する / Initialize random quests and final quests
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void init_dungeon_quests(player_type* creature_ptr)
{
    int number_of_quests = MAX_RANDOM_QUEST - MIN_RANDOM_QUEST + 1;
    init_flags = INIT_ASSIGN;
    floor_type* floor_ptr = creature_ptr->current_floor_ptr;
    floor_ptr->inside_quest = MIN_RANDOM_QUEST;
    process_dungeon_file(creature_ptr, "q_info.txt", 0, 0, 0, 0);
    floor_ptr->inside_quest = 0;
    for (int i = MIN_RANDOM_QUEST + number_of_quests - 1; i >= MIN_RANDOM_QUEST; i--) {
        quest_type* q_ptr = &quest[i];
        monster_race* quest_r_ptr;
        q_ptr->status = QUEST_STATUS_TAKEN;
        determine_random_questor(creature_ptr, q_ptr);
        quest_r_ptr = &r_info[q_ptr->r_idx];
        quest_r_ptr->flags1 |= RF1_QUESTOR;
        q_ptr->max_num = 1;
    }

    init_flags = INIT_ASSIGN;
    floor_ptr->inside_quest = QUEST_OBERON;
    process_dungeon_file(creature_ptr, "q_info.txt", 0, 0, 0, 0);
    quest[QUEST_OBERON].status = QUEST_STATUS_TAKEN;

    floor_ptr->inside_quest = QUEST_SERPENT;
    process_dungeon_file(creature_ptr, "q_info.txt", 0, 0, 0, 0);
    quest[QUEST_SERPENT].status = QUEST_STATUS_TAKEN;
    floor_ptr->inside_quest = 0;
}

/*!
 * @brief ゲームターンを初期化する / Reset turn
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details アンデッド系種族は開始時刻を夜からにする / Undead start just sunset
 * @details        
 */
static void init_turn(player_type* creature_ptr)
{
    if ((creature_ptr->prace == RACE_VAMPIRE) || (creature_ptr->prace == RACE_SKELETON) || (creature_ptr->prace == RACE_ZOMBIE) || (creature_ptr->prace == RACE_SPECTRE)) {
        current_world_ptr->game_turn = (TURNS_PER_TICK * 3 * TOWN_DAWN) / 4 + 1;
        current_world_ptr->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * MAX_DAYS + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
    } else {
        current_world_ptr->game_turn = 1;
        current_world_ptr->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
    }

    current_world_ptr->dungeon_turn = 1;
    current_world_ptr->dungeon_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
}

/*!
 * @brief 所持状態にあるアイテムの中から一部枠の装備可能なものを装備させる。
 * @return なし
 */
static void wield_all(player_type* creature_ptr)
{
    object_type object_type_body;
    for (INVENTORY_IDX item = INVEN_PACK - 1; item >= 0; item--) {
        object_type *o_ptr;
        o_ptr = &creature_ptr->inventory_list[item];
        if (!o_ptr->k_idx)
            continue;

        int slot = wield_slot(creature_ptr, o_ptr);
        if (slot < INVEN_RARM)
            continue;
        if (slot == INVEN_LITE)
            continue;
        if (creature_ptr->inventory_list[slot].k_idx)
            continue;

        object_type *i_ptr;
        i_ptr = &object_type_body;
        object_copy(i_ptr, o_ptr);
        i_ptr->number = 1;

        if (item >= 0) {
            inven_item_increase(creature_ptr, item, -1);
            inven_item_optimize(creature_ptr, item);
        }
        else {
            floor_item_increase(creature_ptr->current_floor_ptr, 0 - item, -1);
            floor_item_optimize(creature_ptr, 0 - item);
        }

        o_ptr = &creature_ptr->inventory_list[slot];
        object_copy(o_ptr, i_ptr);
        creature_ptr->total_weight += i_ptr->weight;
        creature_ptr->equip_cnt++;
    }
}

/*!
 * @brief 初期所持アイテムの処理 / Add an outfit object
 * @details アイテムを既知のものとした上でwield_all()関数により装備させる。
 * @param o_ptr 処理したいオブジェクト構造体の参照ポインタ
 * @return なし
 */
static void add_outfit(player_type* creature_ptr, object_type* o_ptr)
{
    object_aware(creature_ptr, o_ptr);
    object_known(o_ptr);
    s16b slot = inven_carry(creature_ptr, o_ptr);
    autopick_alter_item(creature_ptr, slot, FALSE);
    wield_all(creature_ptr);
}

/*!
 * @brief 種族/職業/性格などに基づき初期所持アイテムを設定するメインセット関数。 / Init players with some belongings
 * @details Having an item makes the player "aware" of its purpose.
 * @return なし
 */
void player_outfit(player_type* creature_ptr)
{
    OBJECT_TYPE_VALUE tv;
    OBJECT_SUBTYPE_VALUE sv;
    object_type forge;
    object_type* q_ptr;
    q_ptr = &forge;

    switch (creature_ptr->prace) {
    case RACE_VAMPIRE:
        /* Nothing! */
        /* Vampires can drain blood of creatures */
        break;

    case RACE_DEMON:
        /* Demon can drain vitality from humanoid corpse */
        get_mon_num_prep(creature_ptr, monster_hook_human, NULL);

        for (int i = rand_range(3, 4); i > 0; i--) {
            object_prep(q_ptr, lookup_kind(TV_CORPSE, SV_CORPSE));
            q_ptr->pval = get_mon_num(creature_ptr, 2, 0);
            if (q_ptr->pval) {
                q_ptr->number = 1;
                add_outfit(creature_ptr, q_ptr);
            }
        }

        break;

    case RACE_SKELETON:
    case RACE_GOLEM:
    case RACE_ZOMBIE:
    case RACE_SPECTRE:
        /* Staff (of Nothing) */
        object_prep(q_ptr, lookup_kind(TV_STAFF, SV_STAFF_NOTHING));
        q_ptr->number = 1;

        add_outfit(creature_ptr, q_ptr);
        break;

    case RACE_ENT:
        /* Potions of Water */
        object_prep(q_ptr, lookup_kind(TV_POTION, SV_POTION_WATER));
        q_ptr->number = (ITEM_NUMBER)rand_range(15, 23);
        add_outfit(creature_ptr, q_ptr);

        break;

    case RACE_ANDROID:
        /* Flasks of oil */
        object_prep(q_ptr, lookup_kind(TV_FLASK, SV_ANY));

        /* Fuel with oil (move pval to xtra4) */
        apply_magic(creature_ptr, q_ptr, 1, AM_NO_FIXED_ART);

        q_ptr->number = (ITEM_NUMBER)rand_range(7, 12);
        add_outfit(creature_ptr, q_ptr);

        break;

    default:
        /* Food rations */
        object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_RATION));
        q_ptr->number = (ITEM_NUMBER)rand_range(3, 7);

        add_outfit(creature_ptr, q_ptr);
    }

    q_ptr = &forge;

    if ((creature_ptr->prace == RACE_VAMPIRE) && (creature_ptr->pclass != CLASS_NINJA)) {
        object_prep(q_ptr, lookup_kind(TV_SCROLL, SV_SCROLL_DARKNESS));
        q_ptr->number = (ITEM_NUMBER)rand_range(2, 5);
        add_outfit(creature_ptr, q_ptr);
    } else if (creature_ptr->pclass != CLASS_NINJA) {
        object_prep(q_ptr, lookup_kind(TV_LITE, SV_LITE_TORCH));
        q_ptr->number = (ITEM_NUMBER)rand_range(3, 7);
        q_ptr->xtra4 = rand_range(3, 7) * 500;

        add_outfit(creature_ptr, q_ptr);
    }

    q_ptr = &forge;
    if (creature_ptr->prace == RACE_MERFOLK) {
        object_prep(q_ptr, lookup_kind(TV_RING, SV_RING_LEVITATION_FALL));
        q_ptr->number = 1;
        add_outfit(creature_ptr, q_ptr);
    }

    if ((creature_ptr->pclass == CLASS_RANGER) || (creature_ptr->pclass == CLASS_CAVALRY)) {
        object_prep(q_ptr, lookup_kind(TV_ARROW, SV_AMMO_NORMAL));
        q_ptr->number = (byte)rand_range(15, 20);
        add_outfit(creature_ptr, q_ptr);
    }

    if (creature_ptr->pclass == CLASS_RANGER) {
        object_prep(q_ptr, lookup_kind(TV_BOW, SV_SHORT_BOW));
        add_outfit(creature_ptr, q_ptr);
    } else if (creature_ptr->pclass == CLASS_ARCHER) {
        object_prep(q_ptr, lookup_kind(TV_ARROW, SV_AMMO_NORMAL));
        q_ptr->number = (ITEM_NUMBER)rand_range(15, 20);
        add_outfit(creature_ptr, q_ptr);
    } else if (creature_ptr->pclass == CLASS_HIGH_MAGE) {
        object_prep(q_ptr, lookup_kind(TV_WAND, SV_WAND_MAGIC_MISSILE));
        q_ptr->number = 1;
        q_ptr->pval = (PARAMETER_VALUE)rand_range(25, 30);
        add_outfit(creature_ptr, q_ptr);
    } else if (creature_ptr->pclass == CLASS_SORCERER) {
        OBJECT_TYPE_VALUE book_tval;
        for (book_tval = TV_LIFE_BOOK; book_tval <= TV_LIFE_BOOK + MAX_MAGIC - 1; book_tval++) {
            object_prep(q_ptr, lookup_kind(book_tval, 0));
            q_ptr->number = 1;
            add_outfit(creature_ptr, q_ptr);
        }
    } else if (creature_ptr->pclass == CLASS_TOURIST) {
        if (creature_ptr->pseikaku != SEIKAKU_SEXY) {
            object_prep(q_ptr, lookup_kind(TV_SHOT, SV_AMMO_LIGHT));
            q_ptr->number = rand_range(15, 20);
            add_outfit(creature_ptr, q_ptr);
        }

        object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_BISCUIT));
        q_ptr->number = rand_range(2, 4);

        add_outfit(creature_ptr, q_ptr);

        object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_WAYBREAD));
        q_ptr->number = rand_range(2, 4);

        add_outfit(creature_ptr, q_ptr);

        object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_JERKY));
        q_ptr->number = rand_range(1, 3);

        add_outfit(creature_ptr, q_ptr);

        object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_PINT_OF_ALE));
        q_ptr->number = rand_range(2, 4);

        add_outfit(creature_ptr, q_ptr);

        object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_PINT_OF_WINE));
        q_ptr->number = rand_range(2, 4);

        add_outfit(creature_ptr, q_ptr);
    } else if (creature_ptr->pclass == CLASS_NINJA) {
        object_prep(q_ptr, lookup_kind(TV_SPIKE, 0));
        q_ptr->number = rand_range(15, 20);
        add_outfit(creature_ptr, q_ptr);
    } else if (creature_ptr->pclass == CLASS_SNIPER) {
        object_prep(q_ptr, lookup_kind(TV_BOLT, SV_AMMO_NORMAL));
        q_ptr->number = rand_range(15, 20);
        add_outfit(creature_ptr, q_ptr);
    }

    if (creature_ptr->pseikaku == SEIKAKU_SEXY) {
        player_init[creature_ptr->pclass][2][0] = TV_HAFTED;
        player_init[creature_ptr->pclass][2][1] = SV_WHIP;
    }

    for (int i = 0; i < 3; i++) {
        tv = player_init[creature_ptr->pclass][i][0];
        sv = player_init[creature_ptr->pclass][i][1];
        if ((creature_ptr->prace == RACE_ANDROID) && ((tv == TV_SOFT_ARMOR) || (tv == TV_HARD_ARMOR)))
            continue;

        if (tv == TV_SORCERY_BOOK)
            tv = TV_LIFE_BOOK + creature_ptr->realm1 - 1;
        else if (tv == TV_DEATH_BOOK)
            tv = TV_LIFE_BOOK + creature_ptr->realm2 - 1;
        else if (tv == TV_RING && sv == SV_RING_RES_FEAR && creature_ptr->prace == RACE_BARBARIAN)
            sv = SV_RING_SUSTAIN_STR;
        else if (tv == TV_RING && sv == SV_RING_SUSTAIN_INT && creature_ptr->prace == RACE_MIND_FLAYER) {
            tv = TV_POTION;
            sv = SV_POTION_RESTORE_MANA;
        }

        q_ptr = &forge;
        object_prep(q_ptr, lookup_kind(tv, sv));
        if ((tv == TV_SWORD || tv == TV_HAFTED) && (creature_ptr->pclass == CLASS_ROGUE && creature_ptr->realm1 == REALM_DEATH)) /* Only assassins get a poisoned weapon */
            q_ptr->name2 = EGO_BRAND_POIS;

        add_outfit(creature_ptr, q_ptr);
    }

    k_info[lookup_kind(TV_POTION, SV_POTION_WATER)].aware = TRUE;
}

/*!
 * @brief プレイヤーの種族選択を行う / Player race
 * @return なし
 */
static bool get_player_race(player_type* creature_ptr)
{
    char p2 = ')';
    char buf[80];
    char cur[80];

    clear_from(10);
    put_str(_("注意：《種族》によってキャラクターの先天的な資質やボーナスが変化します。",
                "Note: Your 'race' determines various intrinsic factors and bonuses."), 23, 5);

    char sym[MAX_RACES];
    for (int n = 0; n < MAX_RACES; n++) {
        rp_ptr = &race_info[n];
        concptr str = rp_ptr->title;
        if (n < 26)
            sym[n] = I2A(n);
        else
            sym[n] = ('A' + n - 26);

        sprintf(buf, "%c%c%s", sym[n], p2, str);
        put_str(buf, 12 + (n / 5), 1 + 16 * (n % 5));
    }

    sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));
    int k = -1;
    int cs = creature_ptr->prace;
    int os = MAX_RACES;
    while (TRUE) {
        if (cs != os) {
            c_put_str(TERM_WHITE, cur, 12 + (os / 5), 1 + 16 * (os % 5));
            put_str("                                   ", 3, 40);
            if (cs == MAX_RACES) {
                sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));
                put_str("                                   ", 4, 40);
                put_str("                                   ", 5, 40);
            } else {
                rp_ptr = &race_info[cs];
                concptr str = rp_ptr->title;
                sprintf(cur, "%c%c%s", sym[cs], p2, str);
                c_put_str(TERM_L_BLUE, rp_ptr->title, 3, 40);
                put_str(_("腕力 知能 賢さ 器用 耐久 魅力 経験 ", "Str  Int  Wis  Dex  Con  Chr   EXP "), 4, 40);
                put_str(_("の種族修正", ": Race modification"), 3, 40 + strlen(rp_ptr->title));

                sprintf(buf, "%+3d  %+3d  %+3d  %+3d  %+3d  %+3d %+4d%% ",
                    rp_ptr->r_adj[0], rp_ptr->r_adj[1], rp_ptr->r_adj[2], rp_ptr->r_adj[3],
                    rp_ptr->r_adj[4], rp_ptr->r_adj[5], (rp_ptr->r_exp - 100));
                c_put_str(TERM_L_BLUE, buf, 5, 40);
            }
            c_put_str(TERM_YELLOW, cur, 12 + (cs / 5), 1 + 16 * (cs % 5));
            os = cs;
        }

        if (k >= 0)
            break;

        sprintf(buf, _("種族を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a race (%c-%c) ('=' for options): "), sym[0], sym[MAX_RACES - 1]);

        put_str(buf, 10, 10);
        char c = inkey();
        if (c == 'Q')
            birth_quit();
        if (c == 'S')
            return FALSE;
        if (c == ' ' || c == '\r' || c == '\n') {
            if (cs == MAX_RACES) {
                k = randint0(MAX_RACES);
                cs = k;
                continue;
            } else {
                k = cs;
                break;
            }
        }

        if (c == '*') {
            k = randint0(MAX_RACES);
            cs = k;
            continue;
        }

        if (c == '8') {
            if (cs >= 5)
                cs -= 5;
        }

        if (c == '4') {
            if (cs > 0)
                cs--;
        }

        if (c == '6') {
            if (cs < MAX_RACES)
                cs++;
        }

        if (c == '2') {
            if ((cs + 5) <= MAX_RACES)
                cs += 5;
        }

        k = (islower(c) ? A2I(c) : -1);
        if ((k >= 0) && (k < MAX_RACES)) {
            cs = k;
            continue;
        }

        k = (isupper(c) ? (26 + c - 'A') : -1);
        if ((k >= 26) && (k < MAX_RACES)) {
            cs = k;
            continue;
        } else
            k = -1;

        if (c == '?') {
            show_help(creature_ptr, _("jraceclas.txt#TheRaces", "raceclas.txt#TheRaces"));
        } else if (c == '=') {
            screen_save();
            do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
            screen_load();
        } else if (c != '2' && c != '4' && c != '6' && c != '8')
            bell();
    }

    creature_ptr->prace = (byte)k;
    rp_ptr = &race_info[creature_ptr->prace];
    c_put_str(TERM_L_BLUE, rp_ptr->title, 4, 15);
    return TRUE;
}

/*!
 * @brief プレイヤーの職業選択を行う / Player class
 * @return なし
 */
static bool get_player_class(player_type* creature_ptr)
{
    char buf[80];
    char cur[80];
    clear_from(10);
    put_str(_("注意：《職業》によってキャラクターの先天的な能力やボーナスが変化します。",
                "Note: Your 'class' determines various intrinsic abilities and bonuses."), 23, 5);
    put_str(_("()で囲まれた選択肢はこの種族には似合わない職業です。",
                "Any entries in parentheses should only be used by advanced players."), 11, 5);

    char sym[MAX_CLASS_CHOICE];
    char p2 = ')';
    for (int n = 0; n < MAX_CLASS_CHOICE; n++) {
        cp_ptr = &class_info[n];
        mp_ptr = &m_info[n];
        concptr str = cp_ptr->title;
        if (n < 26)
            sym[n] = I2A(n);
        else
            sym[n] = ('A' + n - 26);

        if (!(rp_ptr->choice & (1L << n)))
            sprintf(buf, "%c%c(%s)", sym[n], p2, str);
        else
            sprintf(buf, "%c%c%s", sym[n], p2, str);

        put_str(buf, 13 + (n / 4), 2 + 19 * (n % 4));
    }

    sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));
    int k = -1;
    int cs = creature_ptr->pclass;
    int os = MAX_CLASS_CHOICE;
    while (TRUE) {
        if (cs != os) {
            c_put_str(TERM_WHITE, cur, 13 + (os / 4), 2 + 19 * (os % 4));
            put_str("                                   ", 3, 40);
            if (cs == MAX_CLASS_CHOICE) {
                sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));
                put_str("                                   ", 4, 40);
                put_str("                                   ", 5, 40);
            } else {
                cp_ptr = &class_info[cs];
                mp_ptr = &m_info[cs];
                concptr str = cp_ptr->title;
                if (!(rp_ptr->choice & (1L << cs)))
                    sprintf(cur, "%c%c(%s)", sym[cs], p2, str);
                else
                    sprintf(cur, "%c%c%s", sym[cs], p2, str);

                c_put_str(TERM_L_BLUE, cp_ptr->title, 3, 40);
                put_str(_("の職業修正", ": Class modification"), 3, 40 + strlen(cp_ptr->title));
                put_str(_("腕力 知能 賢さ 器用 耐久 魅力 経験 ", "Str  Int  Wis  Dex  Con  Chr   EXP "), 4, 40);
                sprintf(buf, "%+3d  %+3d  %+3d  %+3d  %+3d  %+3d %+4d%% ",
                    cp_ptr->c_adj[0], cp_ptr->c_adj[1], cp_ptr->c_adj[2], cp_ptr->c_adj[3],
                    cp_ptr->c_adj[4], cp_ptr->c_adj[5], cp_ptr->c_exp);
                c_put_str(TERM_L_BLUE, buf, 5, 40);
            }

            c_put_str(TERM_YELLOW, cur, 13 + (cs / 4), 2 + 19 * (cs % 4));
            os = cs;
        }

        if (k >= 0)
            break;

        sprintf(buf, _("職業を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a class (%c-%c) ('=' for options): "), sym[0], sym[MAX_CLASS_CHOICE - 1]);

        put_str(buf, 10, 10);
        char c = inkey();
        if (c == 'Q')
            birth_quit();
        if (c == 'S')
            return FALSE;
        if (c == ' ' || c == '\r' || c == '\n') {
            if (cs == MAX_CLASS_CHOICE) {
                k = randint0(MAX_CLASS_CHOICE);
                cs = k;
                continue;
            } else {
                k = cs;
                break;
            }
        }

        if (c == '*') {
            k = randint0(MAX_CLASS_CHOICE);
            cs = k;
            continue;
        }

        if (c == '8') {
            if (cs >= 4)
                cs -= 4;
        }

        if (c == '4') {
            if (cs > 0)
                cs--;
        }

        if (c == '6') {
            if (cs < MAX_CLASS_CHOICE)
                cs++;
        }

        if (c == '2') {
            if ((cs + 4) <= MAX_CLASS_CHOICE)
                cs += 4;
        }

        k = (islower(c) ? A2I(c) : -1);
        if ((k >= 0) && (k < MAX_CLASS_CHOICE)) {
            cs = k;
            continue;
        }

        k = (isupper(c) ? (26 + c - 'A') : -1);
        if ((k >= 26) && (k < MAX_CLASS_CHOICE)) {
            cs = k;
            continue;
        } else
            k = -1;

        if (c == '?') {
#ifdef JP
            show_help(creature_ptr, "jraceclas.txt#TheClasses");
#else
            show_help(creature_ptr, "raceclas.txt#TheClasses");
#endif
        } else if (c == '=') {
            screen_save();
            do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
            screen_load();
        } else if (c != '2' && c != '4' && c != '6' && c != '8')
            bell();
    }

    creature_ptr->pclass = (byte)k;
    cp_ptr = &class_info[creature_ptr->pclass];
    mp_ptr = &m_info[creature_ptr->pclass];
    c_put_str(TERM_L_BLUE, cp_ptr->title, 5, 15);
    return TRUE;
}

/*!
 * @brief プレイヤーの性格選択を行う / Player Player seikaku
 * @return なし
 */
static bool get_player_seikaku(player_type* creature_ptr)
{
    concptr str;
    clear_from(10);
    put_str(_("注意：《性格》によってキャラクターの能力やボーナスが変化します。", "Note: Your personality determines various intrinsic abilities and bonuses."), 23, 5);
    char sym[MAX_SEIKAKU];
    char buf[80];
    char p2 = ')';
    int n;
    for (n = 0; n < MAX_SEIKAKU; n++) {
        if (seikaku_info[n].sex && (seikaku_info[n].sex != (creature_ptr->psex + 1)))
            continue;

        ap_ptr = &seikaku_info[n];
        str = ap_ptr->title;
        if (n < 26)
            sym[n] = I2A(n);
        else
            sym[n] = ('A' + n - 26);

        sprintf(buf, "%c%c%s", I2A(n), p2, str);
        put_str(buf, 12 + (n / 4), 2 + 18 * (n % 4));
    }

    char cur[80];
    sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));
    int k = -1;
    int cs = creature_ptr->pseikaku;
    int os = MAX_SEIKAKU;
    while (TRUE) {
        if (cs != os) {
            c_put_str(TERM_WHITE, cur, 12 + (os / 4), 2 + 18 * (os % 4));
            put_str("                                   ", 3, 40);
            if (cs == MAX_SEIKAKU) {
                sprintf(cur, "%c%c%s", '*', p2, _("ランダム", "Random"));
                put_str("                                   ", 4, 40);
                put_str("                                   ", 5, 40);
            } else {
                ap_ptr = &seikaku_info[cs];
                str = ap_ptr->title;
                sprintf(cur, "%c%c%s", sym[cs], p2, str);
                c_put_str(TERM_L_BLUE, ap_ptr->title, 3, 40);
                put_str(_("の性格修正", ": Personality modification"), 3, 40 + strlen(ap_ptr->title));
                put_str(_("腕力 知能 賢さ 器用 耐久 魅力      ", "Str  Int  Wis  Dex  Con  Chr       "), 4, 40);
                sprintf(buf, "%+3d  %+3d  %+3d  %+3d  %+3d  %+3d       ",
                    ap_ptr->a_adj[0], ap_ptr->a_adj[1], ap_ptr->a_adj[2], ap_ptr->a_adj[3],
                    ap_ptr->a_adj[4], ap_ptr->a_adj[5]);
                c_put_str(TERM_L_BLUE, buf, 5, 40);
            }
            c_put_str(TERM_YELLOW, cur, 12 + (cs / 4), 2 + 18 * (cs % 4));
            os = cs;
        }

        if (k >= 0)
            break;

        sprintf(buf, _("性格を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a personality (%c-%c) ('=' for options): "), sym[0], sym[MAX_SEIKAKU - 1]);
        put_str(buf, 10, 10);
        char c = inkey();
        if (c == 'Q')
            birth_quit();
        if (c == 'S')
            return FALSE;
        if (c == ' ' || c == '\r' || c == '\n') {
            if (cs == MAX_SEIKAKU) {
                do {
                    k = randint0(MAX_SEIKAKU);
                } while (seikaku_info[k].sex && (seikaku_info[k].sex != (creature_ptr->psex + 1)));
                cs = k;
                continue;
            } else {
                k = cs;
                break;
            }
        }

        if (c == '*') {
            do {
                k = randint0(n);
            } while (seikaku_info[k].sex && (seikaku_info[k].sex != (creature_ptr->psex + 1)));
            cs = k;
            continue;
        }

        if (c == '8') {
            if (cs >= 4)
                cs -= 4;
            if (cs != MAX_SEIKAKU && seikaku_info[cs].sex && (seikaku_info[cs].sex != (creature_ptr->psex + 1))) {
                if ((cs - 4) > 0)
                    cs -= 4;
                else
                    cs += 4;
            }
        }

        if (c == '4') {
            if (cs > 0)
                cs--;
            if (cs != MAX_SEIKAKU && seikaku_info[cs].sex && (seikaku_info[cs].sex != (creature_ptr->psex + 1))) {
                if ((cs - 1) > 0)
                    cs--;
                else
                    cs++;
            }
        }

        if (c == '6') {
            if (cs < MAX_SEIKAKU)
                cs++;
            if (cs != MAX_SEIKAKU && seikaku_info[cs].sex && (seikaku_info[cs].sex != (creature_ptr->psex + 1))) {
                if ((cs + 1) <= MAX_SEIKAKU)
                    cs++;
                else
                    cs--;
            }
        }

        if (c == '2') {
            if ((cs + 4) <= MAX_SEIKAKU)
                cs += 4;
            if (cs != MAX_SEIKAKU && seikaku_info[cs].sex && (seikaku_info[cs].sex != (creature_ptr->psex + 1))) {
                if ((cs + 4) <= MAX_SEIKAKU)
                    cs += 4;
                else
                    cs -= 4;
            }
        }

        k = (islower(c) ? A2I(c) : -1);
        if ((k >= 0) && (k < MAX_SEIKAKU)) {
            if ((seikaku_info[k].sex == 0) || (seikaku_info[k].sex == (creature_ptr->psex + 1))) {
                cs = k;
                continue;
            }
        }

        k = (isupper(c) ? (26 + c - 'A') : -1);
        if ((k >= 26) && (k < MAX_SEIKAKU)) {
            if ((seikaku_info[k].sex == 0) || (seikaku_info[k].sex == (creature_ptr->psex + 1))) {
                cs = k;
                continue;
            }
        } else
            k = -1;

        if (c == '?') {
            show_help(creature_ptr, _("jraceclas.txt#ThePersonalities", "raceclas.txt#ThePersonalities"));
        } else if (c == '=') {
            screen_save();
            do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
            screen_load();
        } else if (c != '2' && c != '4' && c != '6' && c != '8')
            bell();
    }

    creature_ptr->pseikaku = (CHARACTER_IDX)k;
    ap_ptr = &seikaku_info[creature_ptr->pseikaku];
    char tmp[64];
#ifdef JP
    strcpy(tmp, ap_ptr->title);
    if (ap_ptr->no == 1)
        strcat(tmp, "の");
#else
    strcpy(tmp, ap_ptr->title);
    strcat(tmp, " ");
#endif
    strcat(tmp, creature_ptr->name);
    c_put_str(TERM_L_BLUE, tmp, 1, 34);
    return TRUE;
}

/*!
 * @brief オートローラで得たい能力値の基準を決める。
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static bool get_stat_limits(player_type* creature_ptr)
{
    clear_from(10);
    put_str(_("最低限得たい能力値を設定して下さい。", "Set minimum stats."), 10, 10);
    put_str(_("2/8で項目選択、4/6で値の増減、Enterで次へ", "2/8 for Select, 4/6 for Change value, Enter for Goto next"), 11, 10);
    put_str(_("         基本値  種族 職業 性格     合計値  最大値", "           Base   Rac  Cla  Per      Total  Maximum"), 13, 10);

    int cval[6];
    char buf[80];
    char cur[80];
    char inp[80];
    for (int i = 0; i < A_MAX; i++) {
        stat_match[i] = 0;
        cval[i] = 3;
        int j = rp_ptr->r_adj[i] + cp_ptr->c_adj[i] + ap_ptr->a_adj[i];
        int m = adjust_stat(17, j);
        if (m > 18)
            sprintf(cur, "18/%02d", (m - 18));
        else
            sprintf(cur, "%2d", m);

        m = adjust_stat(cval[i], j);
        if (m > 18)
            sprintf(inp, "18/%02d", (m - 18));
        else
            sprintf(inp, "%2d", m);

        sprintf(buf, "%6s       %2d   %+3d  %+3d  %+3d  =  %6s  %6s",
            stat_names[i], cval[i], rp_ptr->r_adj[i], cp_ptr->c_adj[i],
            ap_ptr->a_adj[i], inp, cur);
        put_str(buf, 14 + i, 10);
    }

    int cs = 0;
    int os = 6;
    while (TRUE) {
        if (cs != os) {
            if (os == 6) {
                c_put_str(TERM_WHITE, _("決定する", "Accept"), 21, 35);
            } else if (os < A_MAX) {
                c_put_str(TERM_WHITE, cur, 14 + os, 10);
            }
            if (cs == 6) {
                c_put_str(TERM_YELLOW, _("決定する", "Accept"), 21, 35);
            } else {
                int j = rp_ptr->r_adj[cs] + cp_ptr->c_adj[cs] + ap_ptr->a_adj[cs];
                int m = adjust_stat(cval[cs], j);
                if (m > 18)
                    sprintf(inp, "18/%02d", (m - 18));
                else
                    sprintf(inp, "%2d", m);

                sprintf(cur, "%6s       %2d   %+3d  %+3d  %+3d  =  %6s",
                    stat_names[cs], cval[cs], rp_ptr->r_adj[cs],
                    cp_ptr->c_adj[cs], ap_ptr->a_adj[cs], inp);
                c_put_str(TERM_YELLOW, cur, 14 + cs, 10);
            }

            os = cs;
        }

        char c = inkey();
        switch (c) {
        case 'Q':
            birth_quit();
            break;
        case 'S':
            return FALSE;
        case ESCAPE:
            break;
        case ' ':
        case '\r':
        case '\n':
            if (cs == 6)
                break;
            cs++;
            c = '2';
            break;
        case '8':
        case 'k':
            if (cs > 0)
                cs--;
            break;
        case '2':
        case 'j':
            if (cs < A_MAX)
                cs++;
            break;
        case '4':
        case 'h':
            if (cs != 6) {
                if (cval[cs] == 3) {
                    cval[cs] = 17;
                    os = 7;
                } else if (cval[cs] > 3) {
                    cval[cs]--;
                    os = 7;
                } else
                    return FALSE;
            }

            break;
        case '6':
        case 'l':
            if (cs != 6) {
                if (cval[cs] == 17) {
                    cval[cs] = 3;
                    os = 7;
                } else if (cval[cs] < 17) {
                    cval[cs]++;
                    os = 7;
                } else
                    return FALSE;
            }

            break;
        case 'm':
            if (cs != 6) {
                cval[cs] = 17;
                os = 7;
            }

            break;
        case 'n':
            if (cs != 6) {
                cval[cs] = 3;
                os = 7;
            }

            break;
        case '?':
#ifdef JP
            show_help(creature_ptr, "jbirth.txt#AutoRoller");
#else
            show_help(creature_ptr, "birth.txt#AutoRoller");
#endif
            break;
        case '=':
            screen_save();
#ifdef JP
            do_cmd_options_aux(OPT_PAGE_BIRTH, "初期オプション((*)はスコアに影響)");
#else
            do_cmd_options_aux(OPT_PAGE_BIRTH, "Birth Option((*)s effect score)");
#endif

            screen_load();
            break;
        default:
            bell();
            break;
        }

        if (c == ESCAPE || ((c == ' ' || c == '\r' || c == '\n') && cs == 6))
            break;
    }

    for (int i = 0; i < A_MAX; i++)
        stat_limit[i] = (s16b)cval[i];

    return TRUE;
}

/*!
 * @brief オートローラで得たい年齢、身長、体重、社会的地位の基準を決める。
 * @return なし
 */
static bool get_chara_limits(player_type* creature_ptr)
{
#define MAXITEMS 8

    char buf[80], cur[80];
    concptr itemname[] = {
        _("年齢", "age"),
        _("身長(インチ)", "height"),
        _("体重(ポンド)", "weight"),
        _("社会的地位", "social class")
    };

    clear_from(10);
    put_str(_("2/4/6/8で項目選択、+/-で値の増減、Enterで次へ",
        "2/4/6/8 for Select, +/- for Change value, Enter for Goto next"), 11, 10);
    put_str(_("注意：身長と体重の最大値/最小値ぎりぎりの値は非常に出現確率が低くなります。",
        "Caution: Values near minimum or maximum are extremely rare."), 23, 2);

    int max_percent, min_percent;
    if (creature_ptr->psex == SEX_MALE) {
        max_percent = (int)(rp_ptr->m_b_ht + rp_ptr->m_m_ht * 4 - 1) * 100 / (int)(rp_ptr->m_b_ht);
        min_percent = (int)(rp_ptr->m_b_ht - rp_ptr->m_m_ht * 4 + 1) * 100 / (int)(rp_ptr->m_b_ht);
    } else {
        max_percent = (int)(rp_ptr->f_b_ht + rp_ptr->f_m_ht * 4 - 1) * 100 / (int)(rp_ptr->f_b_ht);
        min_percent = (int)(rp_ptr->f_b_ht - rp_ptr->f_m_ht * 4 + 1) * 100 / (int)(rp_ptr->f_b_ht);
    }

    put_str(_("体格/地位の最小値/最大値を設定して下さい。", "Set minimum/maximum attribute."), 10, 10);
    put_str(_("  項    目                 最小値  最大値", " Parameter                    Min     Max"), 13, 20);
    int mval[MAXITEMS];
    int cval[MAXITEMS];
    for (int i = 0; i < MAXITEMS; i++) {
        int m;
        switch (i) {
        case 0: /* Minimum age */
            m = rp_ptr->b_age + 1;
            break;
        case 1: /* Maximum age */
            m = rp_ptr->b_age + rp_ptr->m_age;
            break;

        case 2: /* Minimum height */
            if (creature_ptr->psex == SEX_MALE)
                m = rp_ptr->m_b_ht - rp_ptr->m_m_ht * 4 + 1;
            else
                m = rp_ptr->f_b_ht - rp_ptr->f_m_ht * 4 + 1;
            break;
        case 3: /* Maximum height */
            if (creature_ptr->psex == SEX_MALE)
                m = rp_ptr->m_b_ht + rp_ptr->m_m_ht * 4 - 1;
            else
                m = rp_ptr->f_b_ht + rp_ptr->f_m_ht * 4 - 1;
            break;
        case 4: /* Minimum weight */
            if (creature_ptr->psex == SEX_MALE)
                m = (rp_ptr->m_b_wt * min_percent / 100) - (rp_ptr->m_m_wt * min_percent / 75) + 1;
            else
                m = (rp_ptr->f_b_wt * min_percent / 100) - (rp_ptr->f_m_wt * min_percent / 75) + 1;
            break;
        case 5: /* Maximum weight */
            if (creature_ptr->psex == SEX_MALE)
                m = (rp_ptr->m_b_wt * max_percent / 100) + (rp_ptr->m_m_wt * max_percent / 75) - 1;
            else
                m = (rp_ptr->f_b_wt * max_percent / 100) + (rp_ptr->f_m_wt * max_percent / 75) - 1;
            break;
        case 6: /* Minimum social class */
            m = 1;
            break;
        case 7: /* Maximum social class */
            m = 100;
            break;
        default:
            m = 1;
            break;
        }

        mval[i] = m;
        cval[i] = m;
    }

    for (int i = 0; i < 4; i++) {
        sprintf(buf, "%-12s (%3d - %3d)", itemname[i], mval[i * 2], mval[i * 2 + 1]);
        put_str(buf, 14 + i, 20);
        for (int j = 0; j < 2; j++) {
            sprintf(buf, "     %3d", cval[i * 2 + j]);
            put_str(buf, 14 + i, 45 + 8 * j);
        }
    }

    int cs = 0;
    int os = MAXITEMS;
    while (TRUE) {
        if (cs != os) {
            const char accept[] = _("決定する", "Accept");
            if (os == MAXITEMS)
                c_put_str(TERM_WHITE, accept, 19, 35);
            else
                c_put_str(TERM_WHITE, cur, 14 + os / 2, 45 + 8 * (os % 2));

            if (cs == MAXITEMS) {
                c_put_str(TERM_YELLOW, accept, 19, 35);
            } else {
                sprintf(cur, "     %3d", cval[cs]);
                c_put_str(TERM_YELLOW, cur, 14 + cs / 2, 45 + 8 * (cs % 2));
            }

            os = cs;
        }

        char c = inkey();
        switch (c) {
        case 'Q':
            birth_quit();
            break;
        case 'S':
            return FALSE;
        case ESCAPE:
            break; /*後でもう一回breakせんと*/
        case ' ':
        case '\r':
        case '\n':
            if (cs == MAXITEMS)
                break;
            cs++;
            c = '6';
            break;
        case '8':
        case 'k':
            if (cs - 2 >= 0)
                cs -= 2;
            break;
        case '2':
        case 'j':
            if (cs < MAXITEMS)
                cs += 2;
            if (cs > MAXITEMS)
                cs = MAXITEMS;
            break;
        case '4':
        case 'h':
            if (cs > 0)
                cs--;
            break;
        case '6':
        case 'l':
            if (cs < MAXITEMS)
                cs++;
            break;
        case '-':
        case '<':
            if (cs != MAXITEMS) {
                if (cs % 2) {
                    if (cval[cs] > cval[cs - 1]) {
                        cval[cs]--;
                        os = 127;
                    }
                } else {
                    if (cval[cs] > mval[cs]) {
                        cval[cs]--;
                        os = 127;
                    }
                }
            }

            break;
        case '+':
        case '>':
            if (cs != MAXITEMS) {
                if (cs % 2) {
                    if (cval[cs] < mval[cs]) {
                        cval[cs]++;
                        os = 127;
                    }
                } else {
                    if (cval[cs] < cval[cs + 1]) {
                        cval[cs]++;
                        os = 127;
                    }
                }
            }

            break;
        case 'm':
            if (cs != MAXITEMS) {
                if (cs % 2) {
                    if (cval[cs] < mval[cs]) {
                        cval[cs] = mval[cs];
                        os = 127;
                    }
                } else {
                    if (cval[cs] < cval[cs + 1]) {
                        cval[cs] = cval[cs + 1];
                        os = 127;
                    }
                }
            }

            break;
        case 'n':
            if (cs != MAXITEMS) {
                if (cs % 2) {
                    if (cval[cs] > cval[cs - 1]) {
                        cval[cs] = cval[cs - 1];
                        os = 255;
                    }
                } else {
                    if (cval[cs] > mval[cs]) {
                        cval[cs] = mval[cs];
                        os = 255;
                    }
                }
            }

            break;
        case '?':
#ifdef JP
            show_help(creature_ptr, "jbirth.txt#AutoRoller");
#else
            show_help(creature_ptr, "birth.txt#AutoRoller");
#endif
            break;
        case '=':
            screen_save();
            do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
            screen_load();
            break;
        default:
            bell();
            break;
        }

        if (c == ESCAPE || ((c == ' ' || c == '\r' || c == '\n') && cs == MAXITEMS))
            break;
    }

    chara_limit.agemin = (s16b)cval[0];
    chara_limit.agemax = (s16b)cval[1];
    chara_limit.htmin = (s16b)cval[2];
    chara_limit.htmax = (s16b)cval[3];
    chara_limit.wtmin = (s16b)cval[4];
    chara_limit.wtmax = (s16b)cval[5];
    chara_limit.scmin = (s16b)cval[6];
    chara_limit.scmax = (s16b)cval[7];
    return TRUE;
}

#define HISTPREF_LIMIT 1024
static char* histpref_buf = NULL;

/*!
 * @brief 生い立ちメッセージの内容をバッファに加える。 / Hook function for reading the histpref.prf file.
 * @return なし
 */
void add_history_from_pref_line(concptr t)
{
    if (!histpref_buf)
        return;

    my_strcat(histpref_buf, t, HISTPREF_LIMIT);
}

/*!
 * @brief 生い立ちメッセージをファイルからロードする。
 * @return なし
 */
static bool do_cmd_histpref(player_type* creature_ptr, void (*process_autopick_file_command)(char*))
{
    char buf[80];
    errr err;
    int i, j, n;
    char *s, *t;
    char temp[64 * 4];
    char histbuf[HISTPREF_LIMIT];

    if (!get_check(_("生い立ち設定ファイルをロードしますか? ", "Load background history preference file? ")))
        return FALSE;

    histbuf[0] = '\0';
    histpref_buf = histbuf;

    sprintf(buf, _("histedit-%s.prf", "histpref-%s.prf"), creature_ptr->base_name);
    err = process_histpref_file(creature_ptr, buf, process_autopick_file_command);

    if (0 > err) {
        strcpy(buf, _("histedit.prf", "histpref.prf"));
        err = process_histpref_file(creature_ptr, buf, process_autopick_file_command);
    }

    if (err) {
        msg_print(_("生い立ち設定ファイルの読み込みに失敗しました。", "Failed to load background history preference."));
        msg_print(NULL);
        histpref_buf = NULL;
        return FALSE;
    } else if (!histpref_buf[0]) {
        msg_print(_("有効な生い立ち設定はこのファイルにありません。", "There does not exist valid background history preference."));
        msg_print(NULL);
        histpref_buf = NULL;
        return FALSE;
    }

    for (i = 0; i < 4; i++)
        creature_ptr->history[i][0] = '\0';

    /* loop */
    for (s = histpref_buf; *s == ' '; s++)
        ;

    n = strlen(s);
    while ((n > 0) && (s[n - 1] == ' '))
        s[--n] = '\0';

    roff_to_buf(s, 60, temp, sizeof(temp));
    t = temp;
    for (i = 0; i < 4; i++) {
        if (t[0] == 0)
            break;
        else {
            strcpy(creature_ptr->history[i], t);
            t += strlen(t) + 1;
        }
    }

    for (i = 0; i < 4; i++) {
        /* loop */
        for (j = 0; creature_ptr->history[i][j]; j++)
            ;

        for (; j < 59; j++)
            creature_ptr->history[i][j] = ' ';
        creature_ptr->history[i][59] = '\0';
    }

    histpref_buf = NULL;
    return TRUE;
}

/*!
 * @brief 生い立ちメッセージを編集する。/Character background edit-mode
 * @return なし
 */
static void edit_history(player_type* creature_ptr, void (*process_autopick_file_command)(char*))
{
    char old_history[4][60];
    for (int i = 0; i < 4; i++) {
        sprintf(old_history[i], "%s", creature_ptr->history[i]);
    }

    for (int i = 0; i < 4; i++) {
        /* loop */
        int j; 
        for (j = 0; creature_ptr->history[i][j]; j++)
            ;

        for (; j < 59; j++)
            creature_ptr->history[i][j] = ' ';
        creature_ptr->history[i][59] = '\0';
    }

    display_player(creature_ptr, 1, map_name);
    c_put_str(TERM_L_GREEN, _("(キャラクターの生い立ち - 編集モード)", "(Character Background - Edit Mode)"), 11, 20);
    put_str(_("[ カーソルキーで移動、Enterで終了、Ctrl-Aでファイル読み込み ]", "[ Cursor key for Move, Enter for End, Ctrl-A for Read pref ]"), 17, 10);
    TERM_LEN y = 0;
    TERM_LEN x = 0;
    while (TRUE) {
        int skey;
        char c;

        for (int i = 0; i < 4; i++) {
            put_str(creature_ptr->history[i], i + 12, 10);
        }
#ifdef JP
        if (iskanji2(creature_ptr->history[y], x))
            c_put_str(TERM_L_BLUE, format("%c%c", creature_ptr->history[y][x], creature_ptr->history[y][x + 1]), y + 12, x + 10);
        else
#endif
            c_put_str(TERM_L_BLUE, format("%c", creature_ptr->history[y][x]), y + 12, x + 10);

        Term_gotoxy(x + 10, y + 12);
        skey = inkey_special(TRUE);
        if (!(skey & SKEY_MASK))
            c = (char)skey;
        else
            c = 0;

        if (skey == SKEY_UP || c == KTRL('p')) {
            y--;
            if (y < 0)
                y = 3;
#ifdef JP
            if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1)))
                x--;
#endif
        } else if (skey == SKEY_DOWN || c == KTRL('n')) {
            y++;
            if (y > 3)
                y = 0;
#ifdef JP
            if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1)))
                x--;
#endif
        } else if (skey == SKEY_RIGHT || c == KTRL('f')) {
#ifdef JP
            if (iskanji2(creature_ptr->history[y], x))
                x++;
#endif
            x++;
            if (x > 58) {
                x = 0;
                if (y < 3)
                    y++;
            }
        } else if (skey == SKEY_LEFT || c == KTRL('b')) {
            x--;
            if (x < 0) {
                if (y) {
                    y--;
                    x = 58;
                } else
                    x = 0;
            }

#ifdef JP
            if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1)))
                x--;
#endif
        } else if (c == '\r' || c == '\n') {
            Term_erase(0, 11, 255);
            Term_erase(0, 17, 255);
            put_str(_("(キャラクターの生い立ち - 編集済み)", "(Character Background - Edited)"), 11, 20);
            break;
        } else if (c == ESCAPE) {
            clear_from(11);
            put_str(_("(キャラクターの生い立ち)", "(Character Background)"), 11, 25);
            for (int i = 0; i < 4; i++) {
                sprintf(creature_ptr->history[i], "%s", old_history[i]);
                put_str(creature_ptr->history[i], i + 12, 10);
            }

            break;
        } else if (c == KTRL('A')) {
            if (do_cmd_histpref(creature_ptr, process_autopick_file_command)) {
#ifdef JP
                if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1)))
                    x--;
#endif
            }
        } else if (c == '\010') {
            x--;
            if (x < 0) {
                if (y) {
                    y--;
                    x = 58;
                } else
                    x = 0;
            }

            creature_ptr->history[y][x] = ' ';
#ifdef JP
            if ((x > 0) && (iskanji2(creature_ptr->history[y], x - 1))) {
                x--;
                creature_ptr->history[y][x] = ' ';
            }
#endif
        }
#ifdef JP
        else if (iskanji(c) || isprint(c))
#else
        else if (isprint(c)) /* BUGFIX */
#endif
        {
#ifdef JP
            if (iskanji2(creature_ptr->history[y], x)) {
                creature_ptr->history[y][x + 1] = ' ';
            }

            if (iskanji(c)) {
                if (x > 57) {
                    x = 0;
                    y++;
                    if (y > 3)
                        y = 0;
                }

                if (iskanji2(creature_ptr->history[y], x + 1)) {
                    creature_ptr->history[y][x + 2] = ' ';
                }

                creature_ptr->history[y][x++] = c;

                c = inkey();
            }
#endif
            creature_ptr->history[y][x++] = c;
            if (x > 58) {
                x = 0;
                y++;
                if (y > 3)
                    y = 0;
            }
        }
    }
}

/*!
 * @brief player_birth()関数のサブセット/Helper function for 'player_birth()'
 * @details
 * The delay may be reduced, but is recommended to keep players
 * from continuously rolling up characters, which can be VERY
 * expensive CPU wise.  And it cuts down on player stupidity.
 * @return なし
 */
static bool player_birth_aux(player_type* creature_ptr, void (*process_autopick_file_command)(char*))
{
    BIT_FLAGS mode = 0;
    bool flag = FALSE;
    bool prev = FALSE;
    concptr str;
    char p2 = ')';
    char b1 = '[';
    char b2 = ']';
    char buf[80], cur[80];

    Term_clear();
    put_str(_("名前  :", "Name  :"), 1, 26);
    put_str(_("性別        :", "Sex         :"), 3, 1);
    put_str(_("種族        :", "Race        :"), 4, 1);
    put_str(_("職業        :", "Class       :"), 5, 1);
    c_put_str(TERM_L_BLUE, creature_ptr->name, 1, 34);
    put_str(_("キャラクターを作成します。('S'やり直す, 'Q'終了, '?'ヘルプ)",
        "Make your charactor. ('S' Restart, 'Q' Quit, '?' Help)"), 8, 10);
    put_str(_("注意：《性別》の違いはゲーム上ほとんど影響を及ぼしません。",
        "Note: Your 'sex' does not have any significant gameplay effects."), 23, 5);
    int n;
    for (n = 0; n < MAX_SEXES; n++) {
        sp_ptr = &sex_info[n];
        sprintf(buf, _("%c%c%s", "%c%c %s"), I2A(n), p2, sp_ptr->title);
        put_str(buf, 12 + (n / 5), 2 + 15 * (n % 5));
    }

    sprintf(cur, _("%c%c%s", "%c%c %s"), '*', p2, _("ランダム", "Random"));
    int k = -1;
    int cs = 0;
    int os = MAX_SEXES;
    while (TRUE) {
        if (cs != os) {
            put_str(cur, 12 + (os / 5), 2 + 15 * (os % 5));
            if (cs == MAX_SEXES)
                sprintf(cur, _("%c%c%s", "%c%c %s"), '*', p2, _("ランダム", "Random"));
            else {
                sp_ptr = &sex_info[cs];
                str = sp_ptr->title;
                sprintf(cur, _("%c%c%s", "%c%c %s"), I2A(cs), p2, str);
            }

            c_put_str(TERM_YELLOW, cur, 12 + (cs / 5), 2 + 15 * (cs % 5));
            os = cs;
        }

        if (k >= 0)
            break;

        sprintf(buf, _("性別を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a sex (%c-%c) ('=' for options): "), I2A(0), I2A(n - 1));
        put_str(buf, 10, 10);
        char c = inkey();
        if (c == 'Q')
            birth_quit();
        if (c == 'S')
            return FALSE;
        if (c == ' ' || c == '\r' || c == '\n') {
            k = cs == MAX_SEXES ? randint0(MAX_SEXES) : cs;
            break;
        }

        if (c == '*') {
            k = randint0(MAX_SEXES);
            break;
        }

        if (c == '4') {
            if (cs > 0)
                cs--;
        }

        if (c == '6') {
            if (cs < MAX_SEXES)
                cs++;
        }

        k = (islower(c) ? A2I(c) : -1);
        if ((k >= 0) && (k < MAX_SEXES)) {
            cs = k;
            continue;
        } else
            k = -1;

        if (c == '?')
            do_cmd_help(creature_ptr);
        else if (c == '=') {
            screen_save();
            do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
            screen_load();
        } else if (c != '4' && c != '6')
            bell();
    }

    creature_ptr->psex = (byte)k;
    sp_ptr = &sex_info[creature_ptr->psex];
    c_put_str(TERM_L_BLUE, sp_ptr->title, 3, 15);
    clear_from(10);
    creature_ptr->prace = 0;
    while (TRUE) {
        char temp[80 * 10];
        concptr t;
        if (!get_player_race(creature_ptr))
            return FALSE;

        clear_from(10);
        roff_to_buf(race_explanations[creature_ptr->prace], 74, temp, sizeof(temp));
        t = temp;
        for (int i = 0; i < 10; i++) {
            if (t[0] == 0)
                break;
            else {
                prt(t, 12 + i, 3);
                t += strlen(t) + 1;
            }
        }
        if (get_check_strict(_("よろしいですか？", "Are you sure? "), CHECK_DEFAULT_Y))
            break;

        clear_from(10);
        c_put_str(TERM_WHITE, "              ", 4, 15);
    }

    clear_from(10);
    creature_ptr->pclass = 0;
    while (TRUE) {
        char temp[80 * 9];
        concptr t;
        if (!get_player_class(creature_ptr))
            return FALSE;

        clear_from(10);
        roff_to_buf(class_explanations[creature_ptr->pclass], 74, temp, sizeof(temp));
        t = temp;
        for (int i = 0; i < 9; i++) {
            if (t[0] == 0)
                break;
            else {
                prt(t, 12 + i, 3);
                t += strlen(t) + 1;
            }
        }

        if (get_check_strict(_("よろしいですか？", "Are you sure? "), CHECK_DEFAULT_Y))
            break;

        c_put_str(TERM_WHITE, "              ", 5, 15);
    }

    if (!get_player_realms(creature_ptr))
        return FALSE;

    creature_ptr->pseikaku = 0;
    while (TRUE) {
        char temp[80 * 8];
        concptr t;
        if (!get_player_seikaku(creature_ptr))
            return FALSE;

        clear_from(10);
        roff_to_buf(personality_explanations[creature_ptr->pseikaku], 74, temp, sizeof(temp));
        t = temp;
        for (int i = 0; i < A_MAX; i++) {
            if (t[0] == 0)
                break;
            else {
                prt(t, 12 + i, 3);
                t += strlen(t) + 1;
            }
        }

        if (get_check_strict(_("よろしいですか？", "Are you sure? "), CHECK_DEFAULT_Y))
            break;

        c_put_str(TERM_L_BLUE, creature_ptr->name, 1, 34);
        prt("", 1, 34 + strlen(creature_ptr->name));
    }

    clear_from(10);
    put_str("                                   ", 3, 40);
    put_str("                                   ", 4, 40);
    put_str("                                   ", 5, 40);

    screen_save();
    do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
    screen_load();
    if (autoroller || autochara)
        auto_round = 0L;

    if (autoroller)
        if (!get_stat_limits(creature_ptr))
            return FALSE;

    if (autochara)
        if (!get_chara_limits(creature_ptr))
            return FALSE;

    clear_from(10);
    init_turn(creature_ptr);
    while (TRUE) {
        int col;
        col = 42;
        if (autoroller || autochara) {
            Term_clear();
            put_str(_("回数 :", "Round:"), 10, col + 13);
            put_str(_("(ESCで停止)", "(Hit ESC to stop)"), 12, col + 13);
        } else {
            get_stats(creature_ptr);
            get_ahw(creature_ptr);
            get_history(creature_ptr);
        }

        if (autoroller) {
            put_str(_("最小値", " Limit"), 2, col + 5);
            put_str(_("成功率", "  Freq"), 2, col + 13);
            put_str(_("現在値", "  Roll"), 2, col + 24);
            for (int i = 0; i < A_MAX; i++) {
                int j, m;
                put_str(stat_names[i], 3 + i, col);
                j = rp_ptr->r_adj[i] + cp_ptr->c_adj[i] + ap_ptr->a_adj[i];
                m = adjust_stat(stat_limit[i], j);
                cnv_stat(m, buf);
                c_put_str(TERM_L_BLUE, buf, 3 + i, col + 5);
            }
        }

        while (autoroller || autochara) {
            bool accept = TRUE;
            get_stats(creature_ptr);
            auto_round++;
            if (auto_round >= 1000000000L) {
                auto_round = 1;
                if (autoroller) {
                    for (int i = 0; i < A_MAX; i++) {
                        stat_match[i] = 0;
                    }
                }
            }

            if (autoroller) {
                for (int i = 0; i < A_MAX; i++) {
                    if (creature_ptr->stat_max[i] >= stat_limit[i])
                        stat_match[i]++;
                    else
                        accept = FALSE;
                }
            }

            if (accept) {
                get_ahw(creature_ptr);
                get_history(creature_ptr);

                if (autochara) {
                    if ((creature_ptr->age < chara_limit.agemin) || (creature_ptr->age > chara_limit.agemax))
                        accept = FALSE;
                    if ((creature_ptr->ht < chara_limit.htmin) || (creature_ptr->ht > chara_limit.htmax))
                        accept = FALSE;
                    if ((creature_ptr->wt < chara_limit.wtmin) || (creature_ptr->wt > chara_limit.wtmax))
                        accept = FALSE;
                    if ((creature_ptr->sc < chara_limit.scmin) || (creature_ptr->sc > chara_limit.scmax))
                        accept = FALSE;
                }

                if (accept)
                    break;
            }

            flag = (!(auto_round % AUTOROLLER_STEP));
            if (flag) {
                birth_put_stats(creature_ptr);
                put_str(format("%10ld", auto_round), 10, col + 20);
                Term_fresh();
                inkey_scan = TRUE;
                if (inkey()) {
                    get_ahw(creature_ptr);
                    get_history(creature_ptr);
                    break;
                }
            }
        }

        if (autoroller || autochara)
            sound(SOUND_LEVEL);

        flush();

        mode = 0;
        get_extra(creature_ptr, TRUE);
        get_money(creature_ptr);
        creature_ptr->chaos_patron = (s16b)randint0(MAX_PATRON);
        char c;
        while (TRUE) {
            creature_ptr->update |= (PU_BONUS | PU_HP);
            update_creature(creature_ptr);
            creature_ptr->chp = creature_ptr->mhp;
            creature_ptr->csp = creature_ptr->msp;
            display_player(creature_ptr, mode, map_name);
            Term_gotoxy(2, 23);
            Term_addch(TERM_WHITE, b1);
            Term_addstr(-1, TERM_WHITE, _("'r' 次の数値", "'r'eroll"));
            if (prev)
                Term_addstr(-1, TERM_WHITE, _(", 'p' 前の数値", "'p'previous"));

            if (mode)
                Term_addstr(-1, TERM_WHITE, _(", 'h' その他の情報", ", 'h' Misc."));
            else
                Term_addstr(-1, TERM_WHITE, _(", 'h' 生い立ちを表示", ", 'h'istory"));

            Term_addstr(-1, TERM_WHITE, _(", Enter この数値に決定", ", or Enter to accept"));
            Term_addch(TERM_WHITE, b2);
            c = inkey();
            if (c == 'Q')
                birth_quit();
            if (c == 'S')
                return FALSE;

            if (c == '\r' || c == '\n' || c == ESCAPE)
                break;
            if ((c == ' ') || (c == 'r'))
                break;

            if (prev && (c == 'p')) {
                load_prev_data(creature_ptr, TRUE);
                continue;
            }

            if ((c == 'H') || (c == 'h')) {
                mode = ((mode != 0) ? 0 : 1);
                continue;
            }

            if (c == '?') {
                show_help(creature_ptr, _("jbirth.txt#AutoRoller", "birth.txt#AutoRoller"));
                continue;
            } else if (c == '=') {
                screen_save();
                do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
                screen_load();
                continue;
            }

            bell();
        }

        if (c == '\r' || c == '\n' || c == ESCAPE)
            break;

        save_prev_data(creature_ptr, &previous_char);
        previous_char.quick_ok = FALSE;
        prev = TRUE;
    }

    clear_from(23);
    get_name(creature_ptr);
    process_player_name(creature_ptr, current_world_ptr->creating_savefile);
    edit_history(creature_ptr, process_autopick_file_command);
    get_max_stats(creature_ptr);
    get_virtues(creature_ptr);
    prt(_("[ 'Q' 中断, 'S' 初めから, Enter ゲーム開始 ]", "['Q'uit, 'S'tart over, or Enter to continue]"), 23, _(14, 10));

    char c = inkey();
    if (c == 'Q')
        birth_quit();

    if (c == 'S')
        return FALSE;

    init_dungeon_quests(creature_ptr);
    save_prev_data(creature_ptr, &previous_char);
    previous_char.quick_ok = TRUE;
    return TRUE;
}

/*!
 * @brief クイックスタート処理の問い合わせと実行を行う。/Ask whether the player use Quick Start or not.
 * @return なし
 */
static bool ask_quick_start(player_type* creature_ptr)
{
    if (!previous_char.quick_ok)
        return FALSE;

    Term_clear();
    put_str(_("クイック・スタートを使うと以前と全く同じキャラクターで始められます。",
        "Do you want to use the quick start function(same character as your last one)."), 11, 2);
    while (TRUE) {
        char c;
        put_str(_("クイック・スタートを使いますか？[y/N]", "Use quick start? [y/N]"), 14, 10);
        c = inkey();
        if (c == 'Q')
            quit(NULL);
        else if (c == 'S')
            return FALSE;
        else if (c == '?')
            show_help(creature_ptr, _("jbirth.txt#QuickStart", "birth.txt#QuickStart"));
        else if ((c == 'y') || (c == 'Y'))
            break;
        else
            return FALSE;
    }

    load_prev_data(creature_ptr, FALSE);
    init_turn(creature_ptr);
    init_dungeon_quests(creature_ptr);

    sp_ptr = &sex_info[creature_ptr->psex];
    rp_ptr = &race_info[creature_ptr->prace];
    cp_ptr = &class_info[creature_ptr->pclass];
    mp_ptr = &m_info[creature_ptr->pclass];
    ap_ptr = &seikaku_info[creature_ptr->pseikaku];

    get_extra(creature_ptr, FALSE);
    creature_ptr->update |= (PU_BONUS | PU_HP);
    update_creature(creature_ptr);
    creature_ptr->chp = creature_ptr->mhp;
    creature_ptr->csp = creature_ptr->msp;
    process_player_name(creature_ptr, FALSE);
    return TRUE;
}

/*!
 * @brief プレイヤー作成処理のメインルーチン/ Create a new character.
 * @details
 * Note that we may be called with "junk" leftover in the various
 * fields, so we must be sure to clear them first.
 * @return なし
 */
void player_birth(player_type* creature_ptr, void (*process_autopick_file_command)(char*))
{
    current_world_ptr->play_time = 0;
    wipe_monsters_list(creature_ptr);
    player_wipe_without_name(creature_ptr);
    if (!ask_quick_start(creature_ptr)) {
        play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_DEFAULT);
        while (TRUE) {
            if (player_birth_aux(creature_ptr, process_autopick_file_command))
                break;

            player_wipe_without_name(creature_ptr);
        }
    }

    message_add(" ");
    message_add("  ");
    message_add("====================");
    message_add(" ");
    message_add("  ");

    exe_write_diary(creature_ptr, DIARY_GAMESTART, 1, _("-------- 新規ゲーム開始 --------", "------- Started New Game -------"));
    exe_write_diary(creature_ptr, DIARY_DIALY, 0, NULL);
    char buf[80];
    sprintf(buf, _("                            性別に%sを選択した。", "                            chose %s gender."),
        sex_info[creature_ptr->psex].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    sprintf(buf, _("                            種族に%sを選択した。", "                            chose %s race."),
        race_info[creature_ptr->prace].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    sprintf(buf, _("                            職業に%sを選択した。", "                            chose %s class."),
        class_info[creature_ptr->pclass].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    if (creature_ptr->realm1) {
        sprintf(buf, _("                            魔法の領域に%s%sを選択した。", "                            chose %s%s."),
            realm_names[creature_ptr->realm1], creature_ptr->realm2 ? format(_("と%s", " and %s realms"), realm_names[creature_ptr->realm2]) : _("", " realm"));
        exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    }

    sprintf(buf, _("                            性格に%sを選択した。", "                            chose %s personality."),
        seikaku_info[creature_ptr->pseikaku].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);

    for (int i = 1; i < max_towns; i++) {
        for (int j = 0; j < MAX_STORES; j++) {
            store_init(i, j);
        }
    }

    seed_wilderness();
    if (creature_ptr->prace == RACE_BEASTMAN)
        creature_ptr->hack_mutation = TRUE;
    else
        creature_ptr->hack_mutation = FALSE;

    if (!window_flag[1])
        window_flag[1] |= PW_MESSAGE;

    if (!window_flag[2])
        window_flag[2] |= PW_INVEN;
}

/*!
 * @brief プレイヤー作成処理中のステータス表示処理
 * @param fff ファイルポインタ
 * @return なし
 */
void dump_yourself(player_type* creature_ptr, FILE* fff)
{
    if (!fff)
        return;

    char temp[80 * 10];
    roff_to_buf(race_explanations[creature_ptr->prace], 78, temp, sizeof(temp));
    fprintf(fff, "\n\n");
    fprintf(fff, _("種族: %s\n", "Race: %s\n"), race_info[creature_ptr->prace].title);
    concptr t = temp;

    for (int i = 0; i < 10; i++) {
        if (t[0] == 0)
            break;
        fprintf(fff, "%s\n", t);
        t += strlen(t) + 1;
    }

    roff_to_buf(class_explanations[creature_ptr->pclass], 78, temp, sizeof(temp));
    fprintf(fff, "\n");
    fprintf(fff, _("職業: %s\n", "Class: %s\n"), class_info[creature_ptr->pclass].title);

    t = temp;
    for (int i = 0; i < 10; i++) {
        if (t[0] == 0)
            break;
        fprintf(fff, "%s\n", t);
        t += strlen(t) + 1;
    }

    roff_to_buf(personality_explanations[creature_ptr->pseikaku], 78, temp, sizeof(temp));
    fprintf(fff, "\n");
    fprintf(fff, _("性格: %s\n", "Pesonality: %s\n"), seikaku_info[creature_ptr->pseikaku].title);

    t = temp;
    for (int i = 0; i < A_MAX; i++) {
        if (t[0] == 0)
            break;
        fprintf(fff, "%s\n", t);
        t += strlen(t) + 1;
    }

    fprintf(fff, "\n");
    if (creature_ptr->realm1) {
        roff_to_buf(realm_explanations[technic2magic(creature_ptr->realm1) - 1], 78, temp, sizeof(temp));
        fprintf(fff, _("魔法: %s\n", "Realm: %s\n"), realm_names[creature_ptr->realm1]);

        t = temp;
        for (int i = 0; i < A_MAX; i++) {
            if (t[0] == 0)
                break;

            fprintf(fff, "%s\n", t);
            t += strlen(t) + 1;
        }
    }

    fprintf(fff, "\n");
    if (creature_ptr->realm2) {
        roff_to_buf(realm_explanations[technic2magic(creature_ptr->realm2) - 1], 78, temp, sizeof(temp));
        fprintf(fff, _("魔法: %s\n", "Realm: %s\n"), realm_names[creature_ptr->realm2]);

        t = temp;
        for (int i = 0; i < A_MAX; i++) {
            if (t[0] == 0)
                break;

            fprintf(fff, "%s\n", t);
            t += strlen(t) + 1;
        }
    }
}
