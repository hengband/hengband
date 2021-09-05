#include "dungeon/quest.h"
#include "cmd-io/cmd-dump.h"
#include "core/asking-player.h"
#include "core/player-update-types.h"
#include "dungeon/dungeon.h"
#include "floor/cave.h"
#include "floor/floor-events.h"
#include "floor/floor-mode-changer.h"
#include "floor/floor-object.h"
#include "game-option/play-record-options.h"
#include "grid/feature.h"
#include "io/write-diary.h"
#include "locale/english.h"
#include "main/music-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-flags8.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "monster/smart-learn-types.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/trg-types.h"
#include "player-status/player-energy.h"
#include "player/player-personality-types.h"
#include "player/player-status.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

quest_type *quest; /*!< Quest info */
QUEST_IDX max_q_idx; /*!< Maximum number of quests */
char quest_text[10][80]; /*!< Quest text */
int quest_text_line; /*!< Current line of the quest text */
int leaving_quest = 0;

/*!
 * @brief クエスト突入時のメッセージテーブル / Array of places to find an inscription
 */
static concptr find_quest[] = {
    _("床にメッセージが刻まれている:", "You find the following inscription in the floor"),
    _("壁にメッセージが刻まれている:", "You see a message inscribed in the wall"),
    _("メッセージを見つけた:", "There is a sign saying"),
    _("何かが階段の上に書いてある:", "Something is written on the staircase"),
    _("巻物を見つけた。メッセージが書いてある:", "You find a scroll with the following message"),
};

/*!
 * @brief ランダムクエストの討伐ユニークを決める / Determine the random quest uniques
 * @param q_ptr クエスト構造体の参照ポインタ
 */
void determine_random_questor(player_type *player_ptr, quest_type *q_ptr)
{
    get_mon_num_prep(player_ptr, mon_hook_quest, nullptr);

    MONRACE_IDX r_idx;
    while (true) {
        /*
         * Random monster 5 - 10 levels out of depth
         * (depending on level)
         */
        r_idx = get_mon_num(player_ptr, 0, q_ptr->level + 5 + randint1(q_ptr->level / 10), GMN_ARENA);
        monster_race *r_ptr;
        r_ptr = &r_info[r_idx];

        if (!(r_ptr->flags1 & RF1_UNIQUE))
            continue;
        if (r_ptr->flags8 & RF8_NO_QUEST)
            continue;
        if (r_ptr->flags1 & RF1_QUESTOR)
            continue;
        if (r_ptr->rarity > 100)
            continue;
        if (r_ptr->flags7 & RF7_FRIENDLY)
            continue;
        if (r_ptr->flags7 & RF7_AQUATIC)
            continue;
        if (r_ptr->flags8 & RF8_WILD_ONLY)
            continue;
        if (no_questor_or_bounty_uniques(r_idx))
            continue;

        /*
         * Accept monsters that are 2 - 6 levels
         * out of depth depending on the quest level
         */
        if (r_ptr->level > (q_ptr->level + (q_ptr->level / 20)))
            break;
    }

    q_ptr->r_idx = r_idx;
}

/*!
 * @brief クエストの最終状態を記録する(成功or失敗、時間)
 * @param player_type プレイヤー情報への参照ポインタ
 * @param q_ptr クエスト情報への参照ポインタ
 * @param stat ステータス(成功or失敗)
 */
void record_quest_final_status(quest_type *q_ptr, PLAYER_LEVEL lev, int16_t stat)
{
    q_ptr->status = stat;
    q_ptr->complev = lev;
    update_playtime();
    q_ptr->comptime = current_world_ptr->play_time;
}

/*!
 * @brief クエストを達成状態にする /
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param quest_num 達成状態にしたいクエストのID
 */
void complete_quest(player_type *player_ptr, QUEST_IDX quest_num)
{
    quest_type *const q_ptr = &quest[quest_num];

    switch (q_ptr->type) {
    case QUEST_TYPE_RANDOM:
        if (record_rand_quest)
            exe_write_diary(player_ptr, DIARY_RAND_QUEST_C, quest_num, nullptr);
        break;
    default:
        if (record_fix_quest)
            exe_write_diary(player_ptr, DIARY_FIX_QUEST_C, quest_num, nullptr);
        break;
    }

    record_quest_final_status(q_ptr, player_ptr->lev, QUEST_STATUS_COMPLETED);

    if (q_ptr->flags & QUEST_FLAG_SILENT)
        return;

    play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_QUEST_CLEAR);
    msg_print(_("クエストを達成した！", "You just completed your quest!"));
    msg_print(nullptr);
}

/*!
 * @brief 特定のアーティファクトを入手した際のクエスト達成処理 /
 * Check for "Quest" completion when a quest monster is killed or charmed.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 入手したオブジェクトの構造体参照ポインタ
 */
void check_find_art_quest_completion(player_type *player_ptr, object_type *o_ptr)
{
    /* Check if completed a quest */
    for (QUEST_IDX i = 0; i < max_q_idx; i++) {
        if ((quest[i].type == QUEST_TYPE_FIND_ARTIFACT) && (quest[i].status == QUEST_STATUS_TAKEN) && (quest[i].k_idx == o_ptr->name1)) {
            complete_quest(player_ptr, i);
        }
    }
}

/*!
 * @brief クエストの導入メッセージを表示する / Discover quest
 * @param q_idx 開始されたクエストのID
 */
void quest_discovery(QUEST_IDX q_idx)
{
    quest_type *q_ptr = &quest[q_idx];
    monster_race *r_ptr = &r_info[q_ptr->r_idx];
    MONSTER_NUMBER q_num = q_ptr->max_num;

    if (!q_idx)
        return;

    GAME_TEXT name[MAX_NLEN];
    strcpy(name, (r_ptr->name.c_str()));

    msg_print(find_quest[rand_range(0, 4)]);
    msg_print(nullptr);

    if (q_num != 1) {
#ifdef JP
#else
        plural_aux(name);
#endif
        msg_format(_("注意しろ！この階は%d体の%sによって守られている！", "Be warned, this level is guarded by %d %s!"), q_num, name);
        return;
    }

    bool is_random_quest_skipped = (r_ptr->flags1 & RF1_UNIQUE) != 0;
    is_random_quest_skipped &= r_ptr->max_num == 0;
    if (!is_random_quest_skipped) {
        msg_format(_("注意せよ！この階は%sによって守られている！", "Beware, this level is protected by %s!"), name);
        return;
    }

    msg_print(_("この階は以前は誰かによって守られていたようだ…。", "It seems that this level was protected by someone before..."));
    record_quest_final_status(q_ptr, 0, QUEST_STATUS_FINISHED);
}

/*!
 * @brief 新しく入ったダンジョンの階層に固定されている一般のクエストを探し出しIDを返す。
 * / Hack -- Check if a level is a "quest" level
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param level 検索対象になる階
 * @return クエストIDを返す。該当がない場合0を返す。
 */
QUEST_IDX quest_number(player_type *player_ptr, DEPTH level)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->inside_quest)
        return (floor_ptr->inside_quest);

    for (QUEST_IDX i = 0; i < max_q_idx; i++) {
        if (quest[i].status != QUEST_STATUS_TAKEN)
            continue;

        if ((quest[i].type == QUEST_TYPE_KILL_LEVEL) && !(quest[i].flags & QUEST_FLAG_PRESET) && (quest[i].level == level)
            && (quest[i].dungeon == player_ptr->dungeon_idx))
            return i;
    }

    return random_quest_number(player_ptr, level);
}

/*!
 * @brief 新しく入ったダンジョンの階層に固定されているランダムクエストを探し出しIDを返す。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param level 検索対象になる階
 * @return クエストIDを返す。該当がない場合0を返す。
 */
QUEST_IDX random_quest_number(player_type *player_ptr, DEPTH level)
{
    if (player_ptr->dungeon_idx != DUNGEON_ANGBAND)
        return 0;

    for (QUEST_IDX i = MIN_RANDOM_QUEST; i < MAX_RANDOM_QUEST + 1; i++) {
        if ((quest[i].type == QUEST_TYPE_RANDOM) && (quest[i].status == QUEST_STATUS_TAKEN) && (quest[i].level == level)
            && (quest[i].dungeon == DUNGEON_ANGBAND)) {
            return i;
        }
    }

    return 0;
}

/*!
 * @brief クエスト階層から離脱する際の処理
 * @param player_ptr プレーヤーへの参照ポインタ
 */
void leave_quest_check(player_type *player_ptr)
{
    leaving_quest = player_ptr->current_floor_ptr->inside_quest;
    if (!leaving_quest)
        return;

    quest_type *const q_ptr = &quest[leaving_quest];
    bool is_one_time_quest = ((q_ptr->flags & QUEST_FLAG_ONCE) || (q_ptr->type == QUEST_TYPE_RANDOM)) && (q_ptr->status == QUEST_STATUS_TAKEN);
    if (!is_one_time_quest)
        return;

    record_quest_final_status(q_ptr, player_ptr->lev, QUEST_STATUS_FAILED);

    /* Additional settings */
    switch (q_ptr->type) {
    case QUEST_TYPE_TOWER:
        quest[QUEST_TOWER1].status = QUEST_STATUS_FAILED;
        quest[QUEST_TOWER1].complev = player_ptr->lev;
        break;
    case QUEST_TYPE_FIND_ARTIFACT:
        a_info[q_ptr->k_idx].gen_flags.reset(TRG::QUESTITEM);
        break;
    case QUEST_TYPE_RANDOM:
        r_info[q_ptr->r_idx].flags1 &= ~(RF1_QUESTOR);

        /* Floor of random quest will be blocked */
        prepare_change_floor_mode(player_ptr, CFM_NO_RETURN);
        break;
    }

    /* Record finishing a quest */
    if (q_ptr->type == QUEST_TYPE_RANDOM) {
        if (record_rand_quest)
            exe_write_diary(player_ptr, DIARY_RAND_QUEST_F, leaving_quest, nullptr);
        return;
    }

    if (record_fix_quest)
        exe_write_diary(player_ptr, DIARY_FIX_QUEST_F, leaving_quest, nullptr);
}

/*!
 * @brief 「塔」クエストの各階層から離脱する際の処理
 */
void leave_tower_check(player_type *player_ptr)
{
    leaving_quest = player_ptr->current_floor_ptr->inside_quest;
    bool is_leaving_from_tower = leaving_quest != 0;
    is_leaving_from_tower &= quest[leaving_quest].type == QUEST_TYPE_TOWER;
    is_leaving_from_tower &= quest[QUEST_TOWER1].status != QUEST_STATUS_COMPLETED;
    if (!is_leaving_from_tower)
        return;
    if (quest[leaving_quest].type != QUEST_TYPE_TOWER)
        return;

    quest[QUEST_TOWER1].status = QUEST_STATUS_FAILED;
    quest[QUEST_TOWER1].complev = player_ptr->lev;
    update_playtime();
    quest[QUEST_TOWER1].comptime = current_world_ptr->play_time;
}

/*! 
 * @brief Player enters a new quest
 */
void exe_enter_quest(player_type *player_ptr, QUEST_IDX quest_idx)
{
    if (quest[quest_idx].type != QUEST_TYPE_RANDOM)
        player_ptr->current_floor_ptr->dun_level = 1;
    player_ptr->current_floor_ptr->inside_quest = quest_idx;

    player_ptr->leaving = true;
}

/*!
 * @brief クエスト入り口にプレイヤーが乗った際の処理 / Do building commands
 * @param player_ptr プレーヤーへの参照ポインタ
 */
void do_cmd_quest(player_type *player_ptr)
{
    if (player_ptr->wild_mode)
        return;

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    if (!cave_has_flag_bold(player_ptr->current_floor_ptr, player_ptr->y, player_ptr->x, FF::QUEST_ENTER)) {
        msg_print(_("ここにはクエストの入口はない。", "You see no quest level here."));
        return;
    }

    msg_print(_("ここにはクエストへの入口があります。", "There is an entry of a quest."));
    if (!get_check(_("クエストに入りますか？", "Do you enter? ")))
        return;
    if (is_echizen(player_ptr))
        msg_print(_("『とにかく入ってみようぜぇ。』", "\"Let's go in anyway.\""));
    else if (is_chargeman(player_ptr))
        msg_print(_("『全滅してやるぞ！』", "\"I'll annihilate THEM!\""));

    player_ptr->oldpy = 0;
    player_ptr->oldpx = 0;
    leave_quest_check(player_ptr);

    exe_enter_quest(player_ptr, player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].special);
}
