#include "market/play-gamble.h"
#include "avatar/avatar.h"
#include "core/asking-player.h"
#include "core/show-file.h"
#include "io/input-key-acceptor.h"
#include "market/building-actions-table.h"
#include "market/building-util.h"
#include "market/poker.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/string-processor.h"
#include "view/display-fruit.h"
#include "view/display-messages.h"

/*!
 * @brief カジノ1プレイごとのメインルーチン / gamble_comm
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param cmd プレイするゲームID
 * @return プレイ成立やルール説明のみ等ならTRUE、賭け金不足で不成立ならFALSE
 */
bool gamble_comm(PlayerType *player_ptr, int cmd)
{
    screen_save();
    if (cmd == BACT_GAMBLE_RULES) {
        (void)show_file(player_ptr, true, _("jgambling.txt", "gambling.txt"), 0, 0);
        screen_load();
        return true;
    }

    if (player_ptr->au < 1) {
        msg_print(_("おい！おまえ一文なしじゃないか！こっから出ていけ！", "Hey! You don't have gold - get out of here!"));
        msg_print(nullptr);
        screen_load();
        return false;
    }

    clear_bldg(5, 23);
    auto maxbet = player_ptr->lev * 200;
    maxbet = std::min(maxbet, player_ptr->au);
    constexpr auto prompt = _("賭け金？", "Your wager ?");
    const auto wager_opt = input_integer(prompt, 1, maxbet, 1);
    if (!wager_opt.has_value()) {
        msg_print(nullptr);
        screen_load();
        return true;
    }

    auto wager = wager_opt.value();
    if (wager > player_ptr->au) {
        msg_print(_("おい！金が足りないじゃないか！出ていけ！", "Hey! You don't have the gold - get out of here!"));
        msg_print(nullptr);
        screen_load();
        return false;
    }

    msg_print(nullptr);
    auto win = 0;
    auto odds = 0;
    auto oldgold = player_ptr->au;

    prt(format(_("ゲーム前の所持金: %9d", "Gold before game: %9d"), oldgold), 20, 2);
    prt(format(_("現在の掛け金:     %9d", "Current Wager:    %9d"), wager), 21, 2);

    while (true) {
        player_ptr->au -= wager;
        switch (cmd) {
        case BACT_IN_BETWEEN: {
            c_put_str(TERM_GREEN, _("イン・ビトイーン", "In Between"), 5, 2);

            odds = 4;
            win = 0;
            auto roll1 = randint1(10);
            auto roll2 = randint1(10);
            auto choice = randint1(10);

            prt(format(_("黒ダイス: %d        黒ダイス: %d", "Black die: %d       Black Die: %d"), roll1, roll2), 8, 3);

            prt(format(_("赤ダイス: %d", "Red die: %d"), choice), 11, 14);
            if (((choice > roll1) && (choice < roll2)) || ((choice < roll1) && (choice > roll2))) {
                win = 1;
            }

            break;
        }
        case BACT_CRAPS: {
            c_put_str(TERM_GREEN, _("クラップス", "Craps"), 5, 2);

            win = 3;
            odds = 2;
            auto roll1 = randint1(6);
            auto roll2 = randint1(6);
            auto roll3 = roll1 + roll2;
            auto choice = roll3;
            prt(format(_("１振りめ: %d %d      Total: %d", "First roll: %d %d    Total: %d"), roll1, roll2, roll3), 7, 5);
            if ((roll3 == 7) || (roll3 == 11)) {
                win = 1;
            } else if ((roll3 == 2) || (roll3 == 3) || (roll3 == 12)) {
                win = 0;
            } else {
                do {
                    msg_print(_("なにかキーを押すともう一回振ります。", "Hit any key to roll again"));

                    msg_print(nullptr);
                    roll1 = randint1(6);
                    roll2 = randint1(6);
                    roll3 = roll1 + roll2;
                    prt(format(_("出目: %d %d          合計:      %d", "Roll result: %d %d   Total:     %d"), roll1, roll2, roll3), 8, 5);
                    if (roll3 == choice) {
                        win = 1;
                    } else if (roll3 == 7) {
                        win = 0;
                    }
                } while ((win != 1) && (win != 0));
            }

            break;
        }
        case BACT_SPIN_WHEEL: {
            win = 0;
            odds = 9;
            c_put_str(TERM_GREEN, _("ルーレット", "Wheel"), 5, 2);

            prt("0  1  2  3  4  5  6  7  8  9", 7, 5);
            prt("--------------------------------", 8, 3);
            while (true) {
                const auto choice_opt = input_integer(_("何番？", "Pick a number"), 0, 9);
                if (!choice_opt.has_value()) {
                    continue;
                }

                auto choice = choice_opt.value();
                msg_print(nullptr);
                auto roll1 = randint0(10);
                prt(format(_("ルーレットは回り、止まった。勝者は %d番だ。", "The wheel spins to a stop and the winner is %d"), roll1), 13, 3);
                prt("", 9, 0);
                prt("*", 9, (3 * roll1 + 5));
                if (roll1 == choice) {
                    win = 1;
                }

                break;
            }

            break;
        }
        case BACT_DICE_SLOTS: {
            c_put_str(TERM_GREEN, _("ダイス・スロット", "Dice Slots"), 5, 2);
            c_put_str(TERM_YELLOW, _("レモン   レモン            2", "Lemon    Lemon             2"), 6, 37);
            c_put_str(TERM_YELLOW, _("レモン   レモン   レモン   5", "Lemon    Lemon    Lemon    5"), 7, 37);
            c_put_str(TERM_ORANGE, _("オレンジ オレンジ オレンジ 10", "Orange   Orange   Orange   10"), 8, 37);
            c_put_str(TERM_UMBER, _("剣       剣       剣       20", "Sword    Sword    Sword    20"), 9, 37);
            c_put_str(TERM_SLATE, _("盾       盾       盾       50", "Shield   Shield   Shield   50"), 10, 37);
            c_put_str(TERM_VIOLET, _("プラム   プラム   プラム   200", "Plum     Plum     Plum     200"), 11, 37);
            c_put_str(TERM_RED, _("チェリー チェリー チェリー 1000", "Cherry   Cherry   Cherry   1000"), 12, 37);

            win = 0;
            auto roll1 = randint1(21);
            for (auto i = 6; i > 0; i--) {
                if ((roll1 - i) < 1) {
                    roll1 = 7 - i;
                    break;
                }
                roll1 -= i;
            }
            auto roll2 = randint1(21);
            for (auto i = 6; i > 0; i--) {
                if ((roll2 - i) < 1) {
                    roll2 = 7 - i;
                    break;
                }
                roll2 -= i;
            }
            auto choice = randint1(21);
            for (auto i = 6; i > 0; i--) {
                if ((choice - i) < 1) {
                    choice = 7 - i;
                    break;
                }
                choice -= i;
            }
            put_str("/--------------------------\\", 7, 2);
            prt("\\--------------------------/", 17, 2);
            display_fruit(8, 3, roll1 - 1);
            display_fruit(8, 12, roll2 - 1);
            display_fruit(8, 21, choice - 1);
            if ((roll1 == roll2) && (roll2 == choice)) {
                win = 1;
                switch (roll1) {
                case 1:
                    odds = 5;
                    break;
                case 2:
                    odds = 10;
                    break;
                case 3:
                    odds = 20;
                    break;
                case 4:
                    odds = 50;
                    break;
                case 5:
                    odds = 200;
                    break;
                case 6:
                    odds = 1000;
                    break;
                }
            } else if ((roll1 == 1) && (roll2 == 1)) {
                win = 1;
                odds = 2;
            }

            break;
        }
        case BACT_POKER:
            win = 0;
            odds = do_poker();
            if (odds) {
                win = 1;
            }

            break;
        }

        if (win) {
            prt(_("あなたの勝ち", "YOU WON"), 16, 37);

            player_ptr->au += odds * wager;

            prt(format(_("倍率: %d", "Payoff: %d"), odds), 17, 37);
        } else {
            prt(_("あなたの負け", "You Lost"), 16, 37);
            prt("", 17, 37);
        }

        prt(format(_("現在の所持金:     %9d", "Current Gold:     %9d"), player_ptr->au), 22, 2);
        prt(_("もう一度(Y/N)？", "Again(Y/N)?"), 18, 37);

        move_cursor(18, 52);
        auto again = inkey();
        prt("", 16, 37);
        prt("", 17, 37);
        prt("", 18, 37);
        if (wager > player_ptr->au) {
            msg_print(_("おい！金が足りないじゃないか！ここから出て行け！", "Hey! You don't have the gold - get out of here!"));
            msg_print(nullptr);
            break;
        }

        if ((again == 'y') || (again == 'Y')) {
            continue;
        }

        break;
    }

    prt("", 18, 37);
    if (player_ptr->au >= oldgold) {
        msg_print(_("「今回は儲けたな！でも次はこっちが勝ってやるからな、絶対に！」", "You came out a winner! We'll win next time, I'm sure."));
        chg_virtue(player_ptr, Virtue::CHANCE, 3);
    } else {
        msg_print(_("「金をスッてしまったな、わはは！うちに帰った方がいいぜ。」", "You lost gold! Haha, better head home."));
        chg_virtue(player_ptr, Virtue::CHANCE, -3);
    }

    msg_print(nullptr);
    screen_load();
    return true;
}
