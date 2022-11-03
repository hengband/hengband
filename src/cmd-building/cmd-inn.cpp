#include "cmd-building/cmd-inn.h"
#include "cmd-item/cmd-magiceat.h"
#include "core/turn-compensator.h"
#include "game-option/birth-options.h"
#include "io/write-diary.h"
#include "market/bounty.h"
#include "market/building-actions-table.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/magic-eater-data-type.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/digestion-processor.h"
#include "player/eldritch-horror.h"
#include "status/bad-status-setter.h"
#include "store/rumor.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-poison.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 宿屋で食事を摂る
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 満腹ならFALSE、そうでないならTRUE
 */
static bool buy_food(PlayerType *player_ptr)
{
    if (player_ptr->food >= PY_FOOD_FULL) {
        msg_print(_("今は満腹だ。", "You are full now."));
        return false;
    }

    msg_print(_("バーテンはいくらかの食べ物とビールをくれた。", "The barkeep gives you some gruel and a beer."));
    (void)set_food(player_ptr, PY_FOOD_MAX - 1);
    return true;
}

/*!
 * @brief 健康体しか宿屋に泊めない処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 毒でも切り傷でもないならTRUE、そうでないならFALSE
 */
static bool is_healthy_stay(PlayerType *player_ptr)
{
    const auto effects = player_ptr->effects();
    if (!effects->poison()->is_poisoned() && !effects->cut()->is_cut()) {
        return true;
    }

    msg_print(_("あなたに必要なのは部屋ではなく、治療者です。", "You need a healer, not a room."));
    msg_print(nullptr);
    msg_print(_("すみません、でもうちで誰かに死なれちゃ困りますんで。", "Sorry, but I don't want anyone dying in here."));
    return false;
}

#ifdef JP
static bool is_player_undead(PlayerType *player_ptr)
{
    return PlayerRace(player_ptr, true).life() == PlayerRaceLifeType::UNDEAD;
}
#endif

/*!
 * @brief 宿屋に泊まったことを日記に残す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param prev_hour 宿屋に入った直後のゲーム内時刻
 */
static void write_diary_stay_inn(PlayerType *player_ptr, int prev_hour)
{
    if ((prev_hour >= 6) && (prev_hour < 18)) {
        concptr stay_message = _(is_player_undead(player_ptr) ? "宿屋に泊まった。" : "日が暮れるまで宿屋で過ごした。", "stayed during the day at the inn.");
        exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, stay_message);
        return;
    }

    concptr stay_message = _(is_player_undead(player_ptr) ? "夜が明けるまで宿屋で過ごした。" : "宿屋に泊まった。", "stayed overnight at the inn.");
    exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, stay_message);
}

/*!
 * @brief 宿泊によってゲーム内ターンを経過させる
 * @param なし
 */
static void pass_game_turn_by_stay(void)
{
    int32_t oldturn = w_ptr->game_turn;
    w_ptr->game_turn = (w_ptr->game_turn / (TURNS_PER_TICK * TOWN_DAWN / 2) + 1) * (TURNS_PER_TICK * TOWN_DAWN / 2);
    if (w_ptr->dungeon_turn >= w_ptr->dungeon_turn_limit) {
        return;
    }

    constexpr auto stay_magnificant = 10;
    w_ptr->dungeon_turn += std::min<int>((w_ptr->game_turn - oldturn), TURNS_PER_TICK * 250) * stay_magnificant;
    if (w_ptr->dungeon_turn > w_ptr->dungeon_turn_limit) {
        w_ptr->dungeon_turn = w_ptr->dungeon_turn_limit;
    }
}

/*!
 * @brief 悪夢モードなら悪夢を見せる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 悪夢モードならばTRUE
 */
static bool has_a_nightmare(PlayerType *player_ptr)
{
    if (!ironman_nightmare) {
        return false;
    }

    msg_print(_("眠りに就くと恐ろしい光景が心をよぎった。", "Horrible visions flit through your mind as you sleep."));

    while (true) {
        sanity_blast(player_ptr, nullptr, false);
        if (!one_in_(3)) {
            break;
        }
    }

    msg_print(_("あなたは絶叫して目を覚ました。", "You awake screaming."));
    exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, _("悪夢にうなされてよく眠れなかった。", "had a nightmare."));
    return true;
}

/*!
 * @brief 体調を元に戻す
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void back_to_health(PlayerType *player_ptr)
{
    BadStatusSetter bss(player_ptr);
    (void)bss.set_blindness(0);
    (void)bss.set_confusion(0);
    player_ptr->effects()->stun()->reset();
    player_ptr->chp = player_ptr->mhp;
    player_ptr->csp = player_ptr->msp;
}

/*!
 * @brief 魔道具術師の取り込んだ魔法をすべて完全に回復した状態にする
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void charge_magic_eating_energy(PlayerType *player_ptr)
{
    auto magic_eater_data = PlayerClass(player_ptr).get_specific_data<magic_eater_data_type>();
    if (!magic_eater_data) {
        return;
    }

    for (auto tval : { ItemKindType::STAFF, ItemKindType::WAND }) {
        for (auto &item : magic_eater_data->get_item_group(tval)) {
            item.charge = item.count * EATER_CHARGE;
        }
    }
    for (auto &item : magic_eater_data->get_item_group(ItemKindType::ROD)) {
        item.charge = 0;
    }
}

/*!
 * @brief リフレッシュ結果を画面に表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param prev_hour 宿屋に入った直後のゲーム内時刻
 */
static void display_stay_result(PlayerType *player_ptr, int prev_hour)
{
    if ((prev_hour >= 6) && (prev_hour < 18)) {
#if JP
        char refresh_message_jp[50];
        sprintf(refresh_message_jp, "%s%s%s", "あなたはリフレッシュして目覚め、", is_player_undead(player_ptr) ? "夜" : "夕方", "を迎えた。");
        msg_print(refresh_message_jp);
#else
        msg_print("You awake refreshed for the evening.");
#endif
        concptr awake_message = _(is_player_undead(player_ptr) ? "すがすがしい夜を迎えた。" : "夕方を迎えた。", "awoke refreshed.");
        exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, awake_message);
        return;
    }

    msg_print(_("あなたはリフレッシュして目覚め、新たな日を迎えた。", "You awake refreshed for the new day."));
    concptr awake_message = _(is_player_undead(player_ptr) ? "すがすがしい朝を迎えた。" : "朝を迎えた。", "awoke refreshed.");
    exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, awake_message);
}

/*!
 * @brief 宿屋への宿泊実行処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 泊まれたらTRUE
 */
static bool stay_inn(PlayerType *player_ptr)
{
    if (!is_healthy_stay(player_ptr)) {
        return false;
    }

    int prev_day, prev_hour, prev_min;
    extract_day_hour_min(player_ptr, &prev_day, &prev_hour, &prev_min);
    write_diary_stay_inn(player_ptr, prev_hour);

    pass_game_turn_by_stay();
    prevent_turn_overflow(player_ptr);

    if ((prev_hour >= 18) && (prev_hour <= 23)) {
        determine_daily_bounty(player_ptr, false); /* Update daily bounty */
        exe_write_diary(player_ptr, DIARY_DIALY, 0, nullptr);
    }

    player_ptr->chp = player_ptr->mhp;
    if (has_a_nightmare(player_ptr)) {
        return true;
    }

    back_to_health(player_ptr);
    charge_magic_eating_energy(player_ptr);

    display_stay_result(player_ptr, prev_hour);
    return true;
}

/*!
 * @brief 宿屋を利用する
 * @param player_ptr プレイヤーへの参照ポインタ
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
bool inn_comm(PlayerType *player_ptr, int cmd)
{
    switch (cmd) {
    case BACT_FOOD:
        return buy_food(player_ptr);
    case BACT_REST:
        return stay_inn(player_ptr);
    case BACT_RUMORS:
        display_rumor(player_ptr, true);
        return true;
    default:
        //!< @todo リファクタリング前のコードもTRUEだった、FALSEにすべきでは.
        return true;
    }
}
