#include "cmd-building/cmd-inn.h"
#include "cmd-item/cmd-magiceat.h"
#include "core/turn-compensator.h"
#include "game-option/birth-options.h"
#include "io/write-diary.h"
#include "market/bounty.h"
#include "market/building-actions-table.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/digestion-processor.h"
#include "player/eldritch-horror.h"
#include "status/bad-status-setter.h"
#include "store/rumor.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 宿屋で食事を摂る
 * @param customer_ptr プレーヤーへの参照ポインタ
 * @return 満腹ならFALSE、そうでないならTRUE
 */
static bool buy_food(player_type *customer_ptr)
{
    if (customer_ptr->food >= PY_FOOD_FULL) {
        msg_print(_("今は満腹だ。", "You are full now."));
        return false;
    }

    msg_print(_("バーテンはいくらかの食べ物とビールをくれた。", "The barkeep gives you some gruel and a beer."));
    (void)set_food(customer_ptr, PY_FOOD_MAX - 1);
    return true;
}

/*!
 * @brief 健康体しか宿屋に泊めない処理
 * @param customer_ptr プレーヤーへの参照ポインタ
 * @return 毒でも切り傷でもないならTRUE、そうでないならFALSE
 */
static bool is_healthy_stay(player_type *customer_ptr)
{
    if (!customer_ptr->poisoned && !customer_ptr->cut)
        return true;

    msg_print(_("あなたに必要なのは部屋ではなく、治療者です。", "You need a healer, not a room."));
    msg_print(nullptr);
    msg_print(_("すみません、でもうちで誰かに死なれちゃ困りますんで。", "Sorry, but I don't want anyone dying in here."));
    return false;
}

#ifdef JP
static bool is_player_undead(player_type *customer_ptr)
{
    return player_race_life(customer_ptr, true) == PlayerRaceLife::UNDEAD;
}
#endif

/*!
 * @brief 宿屋に泊まったことを日記に残す
 * @param customer_ptr プレーヤーへの参照ポインタ
 * @param prev_hour 宿屋に入った直後のゲーム内時刻
 */
static void write_diary_stay_inn(player_type *customer_ptr, int prev_hour)
{
    if ((prev_hour >= 6) && (prev_hour < 18)) {
        concptr stay_message = _(is_player_undead(customer_ptr) ? "宿屋に泊まった。" : "日が暮れるまで宿屋で過ごした。", "stayed during the day at the inn.");
        exe_write_diary(customer_ptr, DIARY_DESCRIPTION, 0, stay_message);
        return;
    }

    concptr stay_message = _(is_player_undead(customer_ptr) ? "夜が明けるまで宿屋で過ごした。" : "宿屋に泊まった。", "stayed overnight at the inn.");
    exe_write_diary(customer_ptr, DIARY_DESCRIPTION, 0, stay_message);
}

/*!
 * @brief 宿泊によってゲーム内ターンを経過させる
 * @param なし
 */
static void pass_game_turn_by_stay(void)
{
    int32_t oldturn = current_world_ptr->game_turn;
    current_world_ptr->game_turn = (current_world_ptr->game_turn / (TURNS_PER_TICK * TOWN_DAWN / 2) + 1) * (TURNS_PER_TICK * TOWN_DAWN / 2);
    if (current_world_ptr->dungeon_turn >= current_world_ptr->dungeon_turn_limit)
        return;

    current_world_ptr->dungeon_turn += MIN((current_world_ptr->game_turn - oldturn), TURNS_PER_TICK * 250) * INN_DUNGEON_TURN_ADJ;
    if (current_world_ptr->dungeon_turn > current_world_ptr->dungeon_turn_limit)
        current_world_ptr->dungeon_turn = current_world_ptr->dungeon_turn_limit;
}

/*!
 * @brief 悪夢モードなら悪夢を見せる
 * @param customer_ptr プレーヤーへの参照ポインタ
 * @return 悪夢モードならばTRUE
 */
static bool has_a_nightmare(player_type *customer_ptr)
{
    if (!ironman_nightmare)
        return false;

    msg_print(_("眠りに就くと恐ろしい光景が心をよぎった。", "Horrible visions flit through your mind as you sleep."));

    while (true) {
        sanity_blast(customer_ptr, nullptr, false);
        if (!one_in_(3))
            break;
    }

    msg_print(_("あなたは絶叫して目を覚ました。", "You awake screaming."));
    exe_write_diary(customer_ptr, DIARY_DESCRIPTION, 0, _("悪夢にうなされてよく眠れなかった。", "had a nightmare."));
    return true;
}

/*!
 * @brief 体調を元に戻す
 * @param customer_ptr プレーヤーへの参照ポインタ
 */
static void back_to_health(player_type *customer_ptr)
{
    set_blind(customer_ptr, 0);
    set_confused(customer_ptr, 0);
    customer_ptr->stun = 0;
    customer_ptr->chp = customer_ptr->mhp;
    customer_ptr->csp = customer_ptr->msp;
}

/*!
 * @brief 魔力喰いの残り回数回復(本当？ 要調査)
 * @param customer_ptr プレーヤーへの参照ポインタ
 */
static void charge_magic_eating_energy(player_type *customer_ptr)
{
    if (customer_ptr->pclass != CLASS_MAGIC_EATER)
        return;

    int i;
    for (i = 0; i < 72; i++) {
        customer_ptr->magic_num1[i] = customer_ptr->magic_num2[i] * EATER_CHARGE;
    }

    for (; i < MAX_SPELLS; i++) {
        customer_ptr->magic_num1[i] = 0;
    }
}

/*!
 * @brief リフレッシュ結果を画面に表示する
 * @param customer_ptr プレーヤーへの参照ポインタ
 * @param prev_hour 宿屋に入った直後のゲーム内時刻
 */
static void display_stay_result(player_type *customer_ptr, int prev_hour)
{
    if ((prev_hour >= 6) && (prev_hour < 18)) {
#if JP
        char refresh_message_jp[50];
        sprintf(refresh_message_jp, "%s%s%s", "あなたはリフレッシュして目覚め、", is_player_undead(customer_ptr) ? "夜" : "夕方", "を迎えた。");
        msg_print(refresh_message_jp);
#else
        msg_print("You awake refreshed for the evening.");
#endif
        concptr awake_message = _(is_player_undead(customer_ptr) ? "すがすがしい夜を迎えた。" : "夕方を迎えた。", "awoke refreshed.");
        exe_write_diary(customer_ptr, DIARY_DESCRIPTION, 0, awake_message);
        return;
    }

    msg_print(_("あなたはリフレッシュして目覚め、新たな日を迎えた。", "You awake refreshed for the new day."));
    concptr awake_message = _(is_player_undead(customer_ptr) ? "すがすがしい朝を迎えた。" : "朝を迎えた。", "awoke refreshed.");
    exe_write_diary(customer_ptr, DIARY_DESCRIPTION, 0, awake_message);
}

/*!
 * @brief 宿屋への宿泊実行処理
 * @param customer_ptr プレーヤーへの参照ポインタ
 * @return 泊まれたらTRUE
 */
static bool stay_inn(player_type *customer_ptr)
{
    if (!is_healthy_stay(customer_ptr))
        return false;

    int prev_day, prev_hour, prev_min;
    extract_day_hour_min(customer_ptr, &prev_day, &prev_hour, &prev_min);
    write_diary_stay_inn(customer_ptr, prev_hour);

    pass_game_turn_by_stay();
    prevent_turn_overflow(customer_ptr);

    if ((prev_hour >= 18) && (prev_hour <= 23)) {
        determine_daily_bounty(customer_ptr, false); /* Update daily bounty */
        exe_write_diary(customer_ptr, DIARY_DIALY, 0, nullptr);
    }

    customer_ptr->chp = customer_ptr->mhp;
    if (has_a_nightmare(customer_ptr))
        return true;

    back_to_health(customer_ptr);
    charge_magic_eating_energy(customer_ptr);

    display_stay_result(customer_ptr, prev_hour);
    return true;
}

/*!
 * @brief 宿屋を利用する
 * @param customer_ptr プレーヤーへの参照ポインタ
 * @param cmd 宿屋の利用施設ID
 * @return 施設の利用が実際に行われたらTRUE
 * @details inn commands
 * Note that resting for the night was a perfect way to avoid player
 * ghosts in the town *if* you could only make it to the inn in time (-:
 * Now that the ghosts are temporarily disabled in 2.8.X, this function
 * will not be that useful.  I will keep it in the hopes the player
 * ghost code does become a reality again. Does help to avoid filthy urchins.
 * Resting at night is also a quick way to restock stores -KMW-
 * @todo 悪夢を見る前後に全回復しているが、何か意図がある？
 */
bool inn_comm(player_type *customer_ptr, int cmd)
{
    switch (cmd) {
    case BACT_FOOD:
        return buy_food(customer_ptr);
    case BACT_REST:
        return stay_inn(customer_ptr);
    case BACT_RUMORS:
        display_rumor(customer_ptr, true);
        return true;
    default:
        //!< @todo リファクタリング前のコードもTRUEだった、FALSEにすべきでは.
        return true;
    }
}
