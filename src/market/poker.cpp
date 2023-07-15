#include "market/poker.h"
#include "io/input-key-acceptor.h"
#include "system/angband.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include <iterator>
#include <numeric>

/*!
 * ポーカーの現在の手札ID
 */
static int cards[5];

#define ODDS_5A 3000 /*!< ファイブエースの役倍率 */
#define ODDS_5C 400 /*!< ファイブカードの役倍率 */
#define ODDS_RF 200 /*!< ロイヤルストレートフラッシュの役倍率 */
#define ODDS_SF 80 /*!< ストレートフラッシュの役倍率 */
#define ODDS_4C 16 /*!< フォアカードの役倍率 */
#define ODDS_FH 12 /*!< フルハウスの役倍率 */
#define ODDS_FL 8 /*!< フラッシュの役倍率 */
#define ODDS_ST 4 /*!< ストレートの役倍率 */
#define ODDS_3C 1 /*!< スリーカードの役倍率 */
#define ODDS_2P 1 /*!< ツーペアの役倍率 */

/*! @note
 * kpoker no (tyuto-hannpa na)pakuri desu...
 * joker ha shineru node haitte masen.
 *
 * TODO: donataka! tsukutte!
 *  - agatta yaku no kiroku (like DQ).
 *  - kakkoii card no e.
 *  - sousa-sei no koujyo.
 *  - code wo wakariyasuku.
 *  - double up.
 *  - Joker... -- done.
 *
 * 9/13/2000 --Koka
 * 9/15/2000 joker wo jissou. soreto, code wo sukosi kakikae. --Habu
 */

#define SUIT_OF(card) ((card) / 13) /*!< トランプカードのスートを返す */
#define NUM_OF(card) ((card) % 13) /*!< トランプカードの番号を返す */
#define IS_JOKER(card) ((card) == 52) /*!< トランプカードがジョーカーかどうかを返す */

/*!
 * @brief ポーカーの山札を切る
 * @param deck デッキの配列
 */
static void reset_deck(int (&deck)[53])
{
    std::iota(std::begin(deck), std::end(deck), 0);
    rand_shuffle(std::begin(deck), std::end(deck));
}

/*!
 * @brief ポーカープレイ中にジョーカーを持っているかの判定を返す
 * @param なし
 * @return ジョーカーを持っているか
 */
static bool has_joker(void)
{
    for (int i = 0; i < 5; i++) {
        if (IS_JOKER(cards[i])) {
            return true;
        }
    }
    return false;
}

/*!
 * @brief ポーカーの手札に該当の番号の札があるかを返す
 * @param num 探したいカードの番号
 * @return 該当の番号が手札にあるか。
 */
static bool find_card_num(int num)
{
    for (int i = 0; i < 5; i++) {
        if (NUM_OF(cards[i]) == num && !IS_JOKER(cards[i])) {
            return true;
        }
    }

    return false;
}

/*!
 * @brief ポーカーの手札がフラッシュ役を得ているかを帰す
 * @param なし
 * @return 役の判定結果
 */
static bool poker_hand_check_flush(void)
{
    bool joker_is_used = false;

    int suit = IS_JOKER(cards[0]) ? SUIT_OF(cards[1]) : SUIT_OF(cards[0]);
    for (int i = 0; i < 5; i++) {
        if (SUIT_OF(cards[i]) == suit) {
            continue;
        }

        if (has_joker() && !joker_is_used) {
            joker_is_used = true;
        } else {
            return false;
        }
    }

    return true;
}

/*!
 * @brief ポーカーの手札がストレートを含んだ高位の役を得ているかを帰す
 * @param なし
 * @return 役の判定結果 0…ストレート、フラッシュいずれもなし/1…ストレートのみ/2…ストレートフラッシュ/3…ロイヤルストレートフラッシュ
 */
static int poker_hand_check_straight(void)
{
    int lowest = 99;
    bool joker_is_used = false;
    bool straight = false;

    for (int i = 0; i < 5; i++) {
        if (NUM_OF(cards[i]) < lowest && !IS_JOKER(cards[i])) {
            lowest = NUM_OF(cards[i]);
        }
    }

    if (poker_hand_check_flush()) {
        int i;
        if (lowest == 0) {
            for (i = 0; i < 4; i++) {
                if (!find_card_num(9 + i)) {
                    if (has_joker() && !joker_is_used) {
                        joker_is_used = true;
                    } else {
                        break;
                    }
                }
            }

            if (i == 4) {
                return 3;
            }
        }

        if (lowest == 9) {
            for (i = 0; i < 3; i++) {
                if (!find_card_num(10 + i)) {
                    break;
                }
            }

            if (i == 3 && has_joker()) {
                return 3;
            }
        }
    }

    joker_is_used = false;

    if (lowest == 0) {
        int i;
        for (i = 0; i < 4; i++) {
            if (!find_card_num(9 + i)) {
                if (has_joker() && !joker_is_used) {
                    joker_is_used = true;
                } else {
                    break;
                }
            }
        }

        if (i == 4) {
            straight = true;
        }
    }

    joker_is_used = false;

    int i;
    for (i = 0; i < 5; i++) {
        if (!find_card_num(lowest + i)) {
            if (has_joker() && !joker_is_used) {
                joker_is_used = true;
            } else {
                break;
            } /* None */
        }
    }

    if (i == 5) {
        straight = true;
    }

    if (straight && poker_hand_check_flush()) {
        return 2;
    } /* Straight Flush */
    else if (straight) {
        return 1;
    } /* Only Straight */
    else {
        return 0;
    }
}

/*!
 * @brief ポーカーのペア役の状態を返す。
 * @param なし
 * @return 0:nopair 1:1 pair 2:2 pair 3:3 cards 4:full house 6:4cards
 */
static int poker_hand_check_pair(void)
{
    int matching = 0;
    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) {
            if (IS_JOKER(cards[i]) || IS_JOKER(cards[j])) {
                continue;
            }
            if (NUM_OF(cards[i]) == NUM_OF(cards[j])) {
                matching++;
            }
        }
    }

    if (has_joker()) {
        switch (matching) {
        case 0:
            matching = 1;
            break;
        case 1:
            matching = 3;
            break;
        case 2:
            matching = 4;
            break;
        case 3:
            matching = 6;
            break;
        case 6:
            matching = 7;
            break;
        default:
            /* don't reach */
            break;
        }
    }

    return matching;
}

/*!
 * @brief ポーカーの役をチェックし、その結果を画面に表示しつつ結果を返す。
 * @return 役のID
 */
static int poker_hand_check(void)
{
    prt("                            ", 4, 3);

    switch (poker_hand_check_straight()) {
    case 3: /* RF! */
        c_put_str(TERM_YELLOW, _("ロイヤルストレートフラッシュ", "Royal Flush"), 4, 3);
        return ODDS_RF;
    case 2: /* SF! */
        c_put_str(TERM_YELLOW, _("ストレートフラッシュ", "Straight Flush"), 4, 3);
        return ODDS_SF;
    case 1:
        c_put_str(TERM_YELLOW, _("ストレート", "Straight"), 4, 3);
        return ODDS_ST;
    default:
        /* Not straight -- fall through */
        break;
    }

    if (poker_hand_check_flush()) {
        c_put_str(TERM_YELLOW, _("フラッシュ", "Flush"), 4, 3);
        return ODDS_FL;
    }

    switch (poker_hand_check_pair()) {
    case 1:
        c_put_str(TERM_YELLOW, _("ワンペア", "One pair"), 4, 3);
        return 0;
    case 2:
        c_put_str(TERM_YELLOW, _("ツーペア", "Two pair"), 4, 3);
        return ODDS_2P;
    case 3:
        c_put_str(TERM_YELLOW, _("スリーカード", "Three of a kind"), 4, 3);
        return ODDS_3C;
    case 4:
        c_put_str(TERM_YELLOW, _("フルハウス", "Full house"), 4, 3);
        return ODDS_FH;
    case 6:
        c_put_str(TERM_YELLOW, _("フォーカード", "Four of a kind"), 4, 3);
        return ODDS_4C;
    case 7:
        if (!NUM_OF(cards[0]) && !NUM_OF(cards[1])) {
            c_put_str(TERM_YELLOW, _("ファイブエース", "Five ace"), 4, 3);
            return ODDS_5A;
        } else {
            c_put_str(TERM_YELLOW, _("ファイブカード", "Five of a kind"), 4, 3);
            return ODDS_5C;
        }
    default:
        break;
    }

    return 0;
}

/*!
 * @brief ポーカーの捨てる/残すインターフェイスの表示を更新する。
 * @param hoge カーソルの現在位置
 * @param kaeruka カードの捨てる/残すフラグ配列
 */
static void display_kaeruka(int hoge, int kaeruka[])
{
    char col = TERM_WHITE;
    for (int i = 0; i < 5; i++) {
        if (i == hoge) {
            col = TERM_YELLOW;
        } else if (kaeruka[i]) {
            col = TERM_WHITE;
        } else {
            col = TERM_L_BLUE;
        }

        if (kaeruka[i]) {
            c_put_str(col, _("かえる", "Change"), 14, 5 + i * 16);
        } else {
            c_put_str(col, _("のこす", " Stay "), 14, 5 + i * 16);
        }
    }

    if (hoge > 4) {
        col = TERM_YELLOW;
    } else {
        col = TERM_WHITE;
    }
    c_put_str(col, _("決定", "Sure"), 16, 38);

    if (hoge < 5) {
        move_cursor(14, 5 + hoge * 16);
    } else {
        move_cursor(16, 38);
    }
}

/*!
 * @brief ポーカーの手札を表示する
 * @param なし
 * @todo _() でまとめる
 */
static void display_cards(void)
{
    char suitcolor[4] = { TERM_YELLOW, TERM_L_RED, TERM_L_BLUE, TERM_L_GREEN };
#ifdef JP
    concptr suit[4] = { "★", "●", "¶", "†" };
    concptr card_grph[13][7] = { { "Ａ   %s     ",
                                     "     変     ",
                                     "     愚     ",
                                     "     蛮     ",
                                     "     怒     ",
                                     "     %s     ",
                                     "          Ａ" },
        { "２          ",
            "     %s     ",
            "            ",
            "            ",
            "            ",
            "     %s     ",
            "          ２" },
        { "３          ",
            "     %s     ",
            "            ",
            "     %s     ",
            "            ",
            "     %s     ",
            "          ３" },
        { "４          ",
            "   %s  %s   ",
            "            ",
            "            ",
            "            ",
            "   %s  %s   ",
            "          ４" },
        { "５          ",
            "   %s  %s   ",
            "            ",
            "     %s     ",
            "            ",
            "   %s  %s   ",
            "          ５" },
        { "６          ",
            "   %s  %s   ",
            "            ",
            "   %s  %s   ",
            "            ",
            "   %s  %s   ",
            "          ６" },
        { "７          ",
            "   %s  %s   ",
            "     %s     ",
            "   %s  %s   ",
            "            ",
            "   %s  %s   ",
            "          ７" },
        { "８          ",
            "   %s  %s   ",
            "     %s     ",
            "   %s  %s   ",
            "     %s     ",
            "   %s  %s   ",
            "          ８" },
        { "９ %s  %s   ",
            "            ",
            "   %s  %s   ",
            "     %s     ",
            "   %s  %s   ",
            "            ",
            "   %s  %s ９" },
        { "10 %s  %s   ",
            "     %s     ",
            "   %s  %s   ",
            "            ",
            "   %s  %s   ",
            "     %s     ",
            "   %s  %s 10" },
        { "Ｊ   Λ     ",
            "%s   ||     ",
            "     ||     ",
            "     ||     ",
            "     ||     ",
            "   |=亜=| %s",
            "     目   Ｊ" },
        { "Ｑ ######   ",
            "%s#      #  ",
            "  # ++++ #  ",
            "  # +==+ #  ",
            "   # ++ #   ",
            "    #  #  %s",
            "     ##   Ｑ" },
        { "Ｋ          ",
            "%s ｀⌒´   ",
            "  γγγλ  ",
            "  ο ο ι  ",
            "   υ    ∂ ",
            "    σ ノ %s",
            "          Ｋ" } };
    concptr joker_grph[7] = { "            ",
        "     Ｊ     ",
        "     Ｏ     ",
        "     Ｋ     ",
        "     Ｅ     ",
        "     Ｒ     ",
        "            " };

#else

    concptr suit[4] = { "[]", "qp", "<>", "db" };
    concptr card_grph[13][7] = { { "A    %s     ",
                                     "     He     ",
                                     "     ng     ",
                                     "     ba     ",
                                     "     nd     ",
                                     "     %s     ",
                                     "           A" },
        { "2           ",
            "     %s     ",
            "            ",
            "            ",
            "            ",
            "     %s     ",
            "           2" },
        { "3           ",
            "     %s     ",
            "            ",
            "     %s     ",
            "            ",
            "     %s     ",
            "           3" },
        { "4           ",
            "   %s  %s   ",
            "            ",
            "            ",
            "            ",
            "   %s  %s   ",
            "           4" },
        { "5           ",
            "   %s  %s   ",
            "            ",
            "     %s     ",
            "            ",
            "   %s  %s   ",
            "           5" },
        { "6           ",
            "   %s  %s   ",
            "            ",
            "   %s  %s   ",
            "            ",
            "   %s  %s   ",
            "           6" },
        { "7           ",
            "   %s  %s   ",
            "     %s     ",
            "   %s  %s   ",
            "            ",
            "   %s  %s   ",
            "           7" },
        { "8           ",
            "   %s  %s   ",
            "     %s     ",
            "   %s  %s   ",
            "     %s     ",
            "   %s  %s   ",
            "           8" },
        { "9  %s  %s   ",
            "            ",
            "   %s  %s   ",
            "     %s     ",
            "   %s  %s   ",
            "            ",
            "   %s  %s  9" },
        { "10 %s  %s   ",
            "     %s     ",
            "   %s  %s   ",
            "            ",
            "   %s  %s   ",
            "     %s     ",
            "   %s  %s 10" },
        { "J    /\\     ",
            "%s   ||     ",
            "     ||     ",
            "     ||     ",
            "     ||     ",
            "   |=HH=| %s",
            "     ][    J" },
        { "Q  ######   ",
            "%s#      #  ",
            "  # ++++ #  ",
            "  # +==+ #  ",
            "   # ++ #   ",
            "    #  #  %s",
            "     ##    Q" },
        { "K           ",
            "%s _'~~`_   ",
            "   jjjjj$&  ",
            "   q q uu   ",
            "   c    &   ",
            "    v__/  %s",
            "           K" } };
    concptr joker_grph[7] = { "            ",
        "     J      ",
        "     O      ",
        "     K      ",
        "     E      ",
        "     R      ",
        "            " };
#endif

    for (int i = 0; i < 5; i++) {
        prt(_("┏━━━━━━┓", " +------------+ "), 5, i * 16);
    }

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 7; j++) {
            prt(_("┃", " |"), j + 6, i * 16);
            if (IS_JOKER(cards[i])) {
                c_put_str(TERM_VIOLET, joker_grph[j], j + 6, 2 + i * 16);
            } else {
                c_put_str(suitcolor[SUIT_OF(cards[i])], format(card_grph[NUM_OF(cards[i])][j], suit[SUIT_OF(cards[i])], suit[SUIT_OF(cards[i])]), j + 6, 2 + i * 16);
            }
            prt(_("┃", "| "), j + 6, i * 16 + 14);
        }
    }

    for (int i = 0; i < 5; i++) {
        prt(_("┗━━━━━━┛", " +------------+ "), 13, i * 16);
    }
}

/*!
 * @brief ポーカーの1プレイルーチン。
 * @return 1プレイの役の結果
 */
int do_poker(void)
{
    int is_put[5];

    bool done = false;
    bool decision = true;
    bool draw = true;

    int deck[53];
    reset_deck(deck);

    int deck_ptr = 0;
    for (int i = 0; i < 5; i++) {
        cards[i] = deck[deck_ptr++];
        is_put[i] = 0;
    }

    prt(_("残すカードを決めて下さい(方向で移動, スペースで選択)。", "Keep which cards (direction keys move; space selects)? "), 0, 0);

    display_cards();
    poker_hand_check();

    int k = 2;
    char cmd;
    while (!done) {
        if (draw) {
            display_kaeruka(k + decision * 5, is_put);
        }

        draw = false;
        cmd = inkey();
        switch (cmd) {
        case '6':
        case 'l':
        case 'L':
        case KTRL('F'):
            if (!decision) {
                k = (k + 1) % 5;
            } else {
                k = 0;
                decision = false;
            }

            draw = true;
            break;
        case '4':
        case 'h':
        case 'H':
        case KTRL('B'):
            if (!decision) {
                k = (k + 4) % 5;
            } else {
                k = 4;
                decision = false;
            }

            draw = true;
            break;
        case '2':
        case 'j':
        case 'J':
        case KTRL('N'):
            if (!decision) {
                decision = true;
                draw = true;
            }

            break;
        case '8':
        case 'k':
        case 'K':
        case KTRL('P'):
            if (decision) {
                decision = false;
                draw = true;
            }

            break;
        case ' ':
        case '\r':
            if (decision) {
                done = true;
            } else {
                is_put[k] = !is_put[k];
                draw = true;
            }

            break;
        default:
            break;
        }
    }

    prt("", 0, 0);

    for (int i = 0; i < 5; i++) {
        if (is_put[i] == 1) {
            cards[i] = deck[deck_ptr++];
        }
    }

    display_cards();

    return poker_hand_check();
}
