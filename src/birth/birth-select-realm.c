#include "system/angband.h"
#include "birth/birth-select-realm.h"
#include "term/gameterm.h"
#include "birth/birth-explanations-table.h"
#include "birth/birth-util.h"

static byte count_realm_selection(s32b choices, int* count)
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

/*!
 * @brief プレイヤーの魔法領域を選択する / Choose from one of the available magical realms
 * @param choices 選択可能な魔法領域のビット配列
 * @param count 選択可能な魔法領域を返すポインタ群。
 * @return 選択した魔法領域のID
 */
static byte select_realm(player_type* creature_ptr, s32b choices, int* count)
{
    byte auto_select = count_realm_selection(choices, count);
    clear_from(10);

    /* Auto-select the realm */
    if ((*count) < 2)
        return auto_select;

    /* Constraint to the 1st realm */
    if (creature_ptr->realm2 != 255) {
        if (creature_ptr->pclass == CLASS_PRIEST) {
            if (is_good_realm(creature_ptr->realm1)) {
                choices &= ~(CH_DEATH | CH_DAEMON);
            } else {
                choices &= ~(CH_LIFE | CH_CRUSADE);
            }
        }
    }

    /* Extra info */
    put_str(_("注意：魔法の領域の選択によりあなたが習得する呪文のタイプが決まります。",
                "Note: The realm of magic will determine which spells you can learn."),
        23, 5);

    int cs = 0;
    int n = 0;
    char p2 = ')';
    char sym[VALID_REALM];
    char buf[80];
    int picks[VALID_REALM] = { 0 };
    for (int i = 0; i < 32; i++) {
        /* Analize realms */
        if (choices & (1L << i)) {
            if (creature_ptr->realm1 == i + 1) {
                if (creature_ptr->realm2 == 255)
                    cs = n;
                else
                    continue;
            }
            if (creature_ptr->realm2 == i + 1)
                cs = n;

            sym[n] = I2A(n);

            sprintf(buf, "%c%c %s", sym[n], p2, realm_names[i + 1]);
            put_str(buf, 12 + (n / 5), 2 + 15 * (n % 5));
            picks[n++] = i + 1;
        }
    }

    char cur[80];
    sprintf(cur, "%c%c %s", '*', p2, _("ランダム", "Random"));

    /* Get a realm */
    int k = -1;
    int os = n;
    while (TRUE) {
        /* Move Cursol */
        if (cs != os) {
            c_put_str(TERM_WHITE, cur, 12 + (os / 5), 2 + 15 * (os % 5));

            if (cs == n) {
                sprintf(cur, "%c%c %s", '*', p2, _("ランダム", "Random"));
            } else {
                sprintf(cur, "%c%c %s", sym[cs], p2, realm_names[picks[cs]]);
                sprintf(buf, "%s", realm_names[picks[cs]]);
                c_put_str(TERM_L_BLUE, buf, 3, 40);
                prt(_("の特徴", ": Characteristic"), 3, 40 + strlen(buf));
                prt(realm_subinfo[technic2magic(picks[cs]) - 1], 4, 40);
            }
            c_put_str(TERM_YELLOW, cur, 12 + (cs / 5), 2 + 15 * (cs % 5));
            os = cs;
        }

        if (k >= 0)
            break;

        sprintf(buf, _("領域を選んで下さい(%c-%c) ('='初期オプション設定): ", "Choose a realm (%c-%c) ('=' for options): "), sym[0], sym[n - 1]);

        put_str(buf, 10, 10);
        char c = inkey();
        if (c == 'Q')
            birth_quit();

        if (c == 'S')
            return 255;

        if (c == ' ' || c == '\r' || c == '\n') {
            if (cs == n) {
                k = randint0(n);
                break;
            } else {
                k = cs;
                break;
            }
        }

        if (c == '*') {
            k = randint0(n);
            break;
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
            if (cs < n)
                cs++;
        }

        if (c == '2') {
            if ((cs + 5) <= n)
                cs += 5;
        }

        k = (islower(c) ? A2I(c) : -1);
        if ((k >= 0) && (k < n)) {
            cs = k;
            continue;
        }

        k = (isupper(c) ? (26 + c - 'A') : -1);
        if ((k >= 26) && (k < n)) {
            cs = k;
            continue;
        } else
            k = -1;

        if (c == '?') {
            show_help(creature_ptr, _("jmagic.txt#MagicRealms", "magic.txt#MagicRealms"));
        } else if (c == '=') {
            screen_save();
            do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth option((*)s effect score)"));

            screen_load();
        } else if (c != '2' && c != '4' && c != '6' && c != '8')
            bell();
    }

    clear_from(10);
    return (byte)(picks[k]);
}

/*!
 * @brief 選択した魔法領域の解説を表示する / Choose the magical realms
 * @return ユーザが魔法領域の確定を選んだらTRUEを返す。
 */
bool get_player_realms(player_type* creature_ptr)
{
    /* Clean up infomation of modifications */
    put_str("                                   ", 3, 40);
    put_str("                                   ", 4, 40);
    put_str("                                   ", 5, 40);

    /* Select the first realm */
    creature_ptr->realm1 = REALM_NONE;
    creature_ptr->realm2 = 255;
    while (TRUE) {
        char temp[80 * 10];
        concptr t;
        int count = 0;
        creature_ptr->realm1 = select_realm(creature_ptr, realm_choices1[creature_ptr->pclass], &count);

        if (255 == creature_ptr->realm1)
            return FALSE;
        if (!creature_ptr->realm1)
            break;

        /* Clean up*/
        clear_from(10);
        put_str("                                   ", 3, 40);
        put_str("                                   ", 4, 40);
        put_str("                                   ", 5, 40);

        roff_to_buf(realm_explanations[technic2magic(creature_ptr->realm1) - 1], 74, temp, sizeof(temp));
        t = temp;
        for (int i = 0; i < 10; i++) {
            if (t[0] == 0)
                break;
            else {
                prt(t, 12 + i, 3);
                t += strlen(t) + 1;
            }
        }

        if (count < 2) {
            prt(_("何かキーを押してください", "Hit any key."), 0, 0);
            (void)inkey();
            prt("", 0, 0);
            break;
        } else if (get_check_strict(_("よろしいですか？", "Are you sure? "), CHECK_DEFAULT_Y))
            break;
    }

    /* Select the second realm */
    creature_ptr->realm2 = REALM_NONE;
    if (creature_ptr->realm1 == REALM_NONE)
        return TRUE;

    /* Print the realm */
    put_str(_("魔法        :", "Magic       :"), 6, 1);
    c_put_str(TERM_L_BLUE, realm_names[creature_ptr->realm1], 6, 15);

    /* Select the second realm */
    while (TRUE) {
        char temp[80 * 8];
        concptr t;

        int count = 0;
        creature_ptr->realm2 = select_realm(creature_ptr, realm_choices2[creature_ptr->pclass], &count);

        if (255 == creature_ptr->realm2)
            return FALSE;
        if (!creature_ptr->realm2)
            break;

        /* Clean up*/
        clear_from(10);
        put_str("                                   ", 3, 40);
        put_str("                                   ", 4, 40);
        put_str("                                   ", 5, 40);

        roff_to_buf(realm_explanations[technic2magic(creature_ptr->realm2) - 1], 74, temp, sizeof(temp));
        t = temp;
        for (int i = 0; i < A_MAX; i++) {
            if (t[0] == 0)
                break;
            else {
                prt(t, 12 + i, 3);
                t += strlen(t) + 1;
            }
        }

        if (count < 2) {
            prt(_("何かキーを押してください", "Hit any key."), 0, 0);
            (void)inkey();
            prt("", 0, 0);
            break;
        } else if (get_check_strict(_("よろしいですか？", "Are you sure? "), CHECK_DEFAULT_Y))
            break;
    }

    if (creature_ptr->realm2) {
        /* Print the realm */
        c_put_str(TERM_L_BLUE, format("%s, %s", realm_names[creature_ptr->realm1], realm_names[creature_ptr->realm2]), 6, 15);
    }

    return TRUE;
}
