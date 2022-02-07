#include "birth/birth-select-realm.h"
#include "birth/birth-explanations-table.h"
#include "birth/birth-util.h"
#include "core/asking-player.h"
#include "io/input-key-acceptor.h"
#include "mind/mind-elementalist.h"
#include "realm/realm-names-table.h"
#include "player-base/player-class.h"
#include "player/player-realm.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/buffer-shaper.h"
#include "util/int-char-converter.h"

static const byte REALM_SELECT_CANCEL = 255;

typedef struct birth_realm_type {
    int cs;
    int n;
    char p2;
    char sym[VALID_REALM];
    char buf[80];
    int picks[VALID_REALM];
    char cur[80];
    int k;
    int os;
} birth_realm_type;

static byte count_realm_selection(const uint32_t choices, int *count)
{
    byte auto_select = REALM_NONE;
    if (choices & CH_LIFE) {
        (*count)++;
        auto_select = REALM_LIFE;
    }

    if (choices & CH_SORCERY) {
        (*count)++;
        auto_select = REALM_SORCERY;
    }

    if (choices & CH_NATURE) {
        (*count)++;
        auto_select = REALM_NATURE;
    }

    if (choices & CH_CHAOS) {
        (*count)++;
        auto_select = REALM_CHAOS;
    }

    if (choices & CH_DEATH) {
        (*count)++;
        auto_select = REALM_DEATH;
    }

    if (choices & CH_TRUMP) {
        (*count)++;
        auto_select = REALM_TRUMP;
    }

    if (choices & CH_ARCANE) {
        (*count)++;
        auto_select = REALM_ARCANE;
    }

    if (choices & CH_ENCHANT) {
        (*count)++;
        auto_select = REALM_CRAFT;
    }

    if (choices & CH_DAEMON) {
        (*count)++;
        auto_select = REALM_DAEMON;
    }

    if (choices & CH_CRUSADE) {
        (*count)++;
        auto_select = REALM_CRUSADE;
    }

    if (choices & CH_MUSIC) {
        (*count)++;
        auto_select = REALM_MUSIC;
    }

    if (choices & CH_HISSATSU) {
        (*count)++;
        auto_select = REALM_HISSATSU;
    }

    if (choices & CH_HEX) {
        (*count)++;
        auto_select = REALM_HEX;
    }

    return auto_select;
}

static birth_realm_type *initialize_birth_realm_type(birth_realm_type *birth_realm_ptr)
{
    birth_realm_ptr->cs = 0;
    birth_realm_ptr->n = 0;
    birth_realm_ptr->p2 = ')';
    for (int i = 0; i < VALID_REALM; i++)
        birth_realm_ptr->picks[i] = 0;

    birth_realm_ptr->k = -1;
    return birth_realm_ptr;
}

static void impose_first_realm(PlayerType *player_ptr, uint32_t *choices)
{
    if (player_ptr->realm2 == REALM_SELECT_CANCEL)
        return;

    if (!PlayerClass(player_ptr).equals(PlayerClassType::PRIEST))
        return;

    if (is_good_realm(player_ptr->realm1)) {
        *choices &= ~(CH_DEATH | CH_DAEMON);
    } else {
        *choices &= ~(CH_LIFE | CH_CRUSADE);
    }
}

static void analyze_realms(const PlayerType *player_ptr, const uint32_t choices, birth_realm_type *birth_realm_ptr)
{
    for (int i = 0; i < 32; i++) {
        if ((choices & (1UL << i)) == 0)
            continue;

        if (player_ptr->realm1 == i + 1) {
            if (player_ptr->realm2 == REALM_SELECT_CANCEL)
                birth_realm_ptr->cs = birth_realm_ptr->n;
            else
                continue;
        }

        if (player_ptr->realm2 == i + 1)
            birth_realm_ptr->cs = birth_realm_ptr->n;

        birth_realm_ptr->sym[birth_realm_ptr->n] = I2A(birth_realm_ptr->n);

        sprintf(birth_realm_ptr->buf, "%c%c %s", birth_realm_ptr->sym[birth_realm_ptr->n], birth_realm_ptr->p2, realm_names[i + 1]);
        put_str(birth_realm_ptr->buf, 12 + (birth_realm_ptr->n / 5), 2 + 15 * (birth_realm_ptr->n % 5));
        birth_realm_ptr->picks[birth_realm_ptr->n++] = i + 1;
    }
}

static void move_birth_realm_cursor(birth_realm_type *birth_realm_ptr)
{
    if (birth_realm_ptr->cs == birth_realm_ptr->os)
        return;

    c_put_str(TERM_WHITE, birth_realm_ptr->cur, 12 + (birth_realm_ptr->os / 5), 2 + 15 * (birth_realm_ptr->os % 5));

    if (birth_realm_ptr->cs == birth_realm_ptr->n) {
        sprintf(birth_realm_ptr->cur, "%c%c %s", '*', birth_realm_ptr->p2, _("ランダム", "Random"));
    } else {
        sprintf(birth_realm_ptr->cur, "%c%c %s", birth_realm_ptr->sym[birth_realm_ptr->cs], birth_realm_ptr->p2,
            realm_names[birth_realm_ptr->picks[birth_realm_ptr->cs]]);
        sprintf(birth_realm_ptr->buf, "%s", realm_names[birth_realm_ptr->picks[birth_realm_ptr->cs]]);
        c_put_str(TERM_L_BLUE, birth_realm_ptr->buf, 3, 40);
        prt(_("の特徴", ": Characteristic"), 3, 40 + strlen(birth_realm_ptr->buf));
        prt(realm_subinfo[technic2magic(birth_realm_ptr->picks[birth_realm_ptr->cs]) - 1].data(), 4, 40);
    }

    c_put_str(TERM_YELLOW, birth_realm_ptr->cur, 12 + (birth_realm_ptr->cs / 5), 2 + 15 * (birth_realm_ptr->cs % 5));
    birth_realm_ptr->os = birth_realm_ptr->cs;
}

static void interpret_realm_select_key(birth_realm_type *birth_realm_ptr, char c)
{
    if (c == 'Q')
        birth_quit();

    if (c == '8') {
        if (birth_realm_ptr->cs >= 5)
            birth_realm_ptr->cs -= 5;
    }

    if (c == '4') {
        if (birth_realm_ptr->cs > 0)
            birth_realm_ptr->cs--;
    }

    if (c == '6') {
        if (birth_realm_ptr->cs < birth_realm_ptr->n)
            birth_realm_ptr->cs++;
    }

    if (c == '2') {
        if ((birth_realm_ptr->cs + 5) <= birth_realm_ptr->n)
            birth_realm_ptr->cs += 5;
    }
}

static bool get_a_realm(PlayerType *player_ptr, birth_realm_type *birth_realm_ptr)
{
    birth_realm_ptr->os = birth_realm_ptr->n;
    while (true) {
        move_birth_realm_cursor(birth_realm_ptr);
        if (birth_realm_ptr->k >= 0)
            break;

        sprintf(birth_realm_ptr->buf, _("領域を選んで下さい(%c-%c) ('='初期オプション設定): ", "Choose a realm (%c-%c) ('=' for options): "),
            birth_realm_ptr->sym[0], birth_realm_ptr->sym[birth_realm_ptr->n - 1]);

        put_str(birth_realm_ptr->buf, 10, 10);
        char c = inkey();
        interpret_realm_select_key(birth_realm_ptr, c);
        if (c == 'S')
            return true;

        if (c == ' ' || c == '\r' || c == '\n') {
            if (birth_realm_ptr->cs == birth_realm_ptr->n) {
                birth_realm_ptr->k = randint0(birth_realm_ptr->n);
                break;
            } else {
                birth_realm_ptr->k = birth_realm_ptr->cs;
                break;
            }
        }

        if (c == '*') {
            birth_realm_ptr->k = randint0(birth_realm_ptr->n);
            break;
        }

        birth_realm_ptr->k = (islower(c) ? A2I(c) : -1);
        if ((birth_realm_ptr->k >= 0) && (birth_realm_ptr->k < birth_realm_ptr->n)) {
            birth_realm_ptr->cs = birth_realm_ptr->k;
            continue;
        }

        birth_realm_ptr->k = (isupper(c) ? (26 + c - 'A') : -1);
        if ((birth_realm_ptr->k >= 26) && (birth_realm_ptr->k < birth_realm_ptr->n)) {
            birth_realm_ptr->cs = birth_realm_ptr->k;
            continue;
        } else
            birth_realm_ptr->k = -1;

        birth_help_option(player_ptr, c, BK_REALM);
    }

    return false;
}

/*!
 * @brief プレイヤーの魔法領域を選択する / Choose from one of the available magical realms
 * @param choices 選択可能な魔法領域のビット配列
 * @param count 選択可能な魔法領域を返すポインタ群。
 * @return 選択した魔法領域のID
 * @details 領域数が0 (戦士等)or 1 (観光客等)なら自動での値を返す
 */
static byte select_realm(PlayerType *player_ptr, uint32_t choices, int *count)
{
    byte auto_select = count_realm_selection(choices, count);
    clear_from(10);
    if ((*count) < 2)
        return auto_select;

    impose_first_realm(player_ptr, &choices);
    put_str(_("注意：魔法の領域の選択によりあなたが習得する呪文のタイプが決まります。", "Note: The realm of magic will determine which spells you can learn."),
        23, 5);

    birth_realm_type tmp_birth_realm;
    birth_realm_type *birth_realm_ptr = initialize_birth_realm_type(&tmp_birth_realm);
    analyze_realms(player_ptr, choices, birth_realm_ptr);
    sprintf(birth_realm_ptr->cur, "%c%c %s", '*', birth_realm_ptr->p2, _("ランダム", "Random"));
    if (get_a_realm(player_ptr, birth_realm_ptr))
        return REALM_SELECT_CANCEL;

    clear_from(10);
    return (byte)(birth_realm_ptr->picks[birth_realm_ptr->k]);
}

static void cleanup_realm_selection_window(void)
{
    clear_from(10);
    put_str("                                   ", 3, 40);
    put_str("                                   ", 4, 40);
    put_str("                                   ", 5, 40);
}

/*!
 * @brief 選んだ魔法領域で本当に良いか問い合わせる
 * @param count 魔法領域の数
 * @return 選んだ魔法領域で良ければTRUE、再選択ならばFALSE
 */
static bool check_realm_selection(PlayerType *player_ptr, int count)
{
    if (count < 2) {
        prt(_("何かキーを押してください", "Hit any key."), 0, 0);
        (void)inkey();
        prt("", 0, 0);
        return true;
    } else if (get_check_strict(player_ptr, _("よろしいですか？", "Are you sure? "), CHECK_DEFAULT_Y))
        return true;

    return false;
}

/*!
 * @brief 選択した魔法領域の解説を表示する / Choose the magical realms
 * @return ユーザが魔法領域の確定を選んだらTRUEを返す。
 */
bool get_player_realms(PlayerType *player_ptr)
{
    /* Clean up infomation of modifications */
    put_str("                                   ", 3, 40);
    put_str("                                   ", 4, 40);
    put_str("                                   ", 5, 40);
    put_str("                                   ", 6, 40);

    /* Select the first realm */
    player_ptr->realm1 = REALM_NONE;
    player_ptr->realm2 = REALM_SELECT_CANCEL;

    if (PlayerClass(player_ptr).equals(PlayerClassType::ELEMENTALIST)) {
        player_ptr->element = select_element_realm(player_ptr);
        if (player_ptr->element == REALM_SELECT_CANCEL)
            return false;

        put_str(_("魔法        :", "Magic       :"), 6, 1);
        c_put_str(TERM_L_BLUE, get_element_title(player_ptr->element), 6, 15);
        player_ptr->realm2 = REALM_NONE;
        return true;
    }

    while (true) {
        char temp[80 * 10];
        int count = 0;
        player_ptr->realm1 = select_realm(player_ptr, realm_choices1[enum2i(player_ptr->pclass)], &count);
        if (player_ptr->realm1 == REALM_SELECT_CANCEL)
            return false;
        if (!player_ptr->realm1)
            break;

        cleanup_realm_selection_window();
        shape_buffer(realm_explanations[technic2magic(player_ptr->realm1) - 1].data(), 74, temp, sizeof(temp));
        concptr t = temp;
        for (int i = 0; i < 10; i++) {
            if (t[0] == 0)
                break;
            else {
                prt(t, 12 + i, 3);
                t += strlen(t) + 1;
            }
        }

        if (check_realm_selection(player_ptr, count))
            break;
    }

    /* Select the second realm */
    player_ptr->realm2 = REALM_NONE;
    if (player_ptr->realm1 == REALM_NONE)
        return true;

    /* Print the realm */
    put_str(_("魔法        :", "Magic       :"), 6, 1);
    c_put_str(TERM_L_BLUE, realm_names[player_ptr->realm1], 6, 15);

    /* Select the second realm */
    while (true) {
        char temp[80 * 8];
        int count = 0;
        player_ptr->realm2 = select_realm(player_ptr, realm_choices2[enum2i(player_ptr->pclass)], &count);

        if (player_ptr->realm2 == REALM_SELECT_CANCEL)
            return false;
        if (!player_ptr->realm2)
            break;

        cleanup_realm_selection_window();
        shape_buffer(realm_explanations[technic2magic(player_ptr->realm2) - 1].data(), 74, temp, sizeof(temp));
        concptr t = temp;
        for (int i = 0; i < A_MAX; i++) {
            if (t[0] == 0)
                break;
            else {
                prt(t, 12 + i, 3);
                t += strlen(t) + 1;
            }
        }

        if (check_realm_selection(player_ptr, count))
            break;
    }

    if (player_ptr->realm2) {
        /* Print the realm */
        c_put_str(TERM_L_BLUE, format("%s, %s", realm_names[player_ptr->realm1], realm_names[player_ptr->realm2]), 6, 15);
    }

    return true;
}
