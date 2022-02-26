#include "io/pref-file-expressor.h"
#include "game-option/runtime-arguments.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "realm/realm-names-table.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "util/string-processor.h"

/*!
 * @brief process_pref_fileのサブルーチンとして条件分岐処理の解釈と結果を返す
 * Helper function for "process_pref_file()"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sp テキスト文字列の参照ポインタ
 * @param fp 再帰中のポインタ参照
 * @return
 * @details
 * かなり長いが、エンバグすると非常に危険な関数なので一旦放置する
 * <pre>
 * Input:
 *   v: output buffer array
 *   f: final character
 * Output:
 *   result
 * </pre>
 */
concptr process_pref_file_expr(PlayerType *player_ptr, char **sp, char *fp)
{
    char *s;
    s = (*sp);
    while (iswspace(*s)) {
        s++;
    }

    char *b;
    b = s;

    concptr v = "?o?o?";

    char b1 = '[';
    char b2 = ']';
    char f = ' ';
    static char tmp[16];
    if (*s == b1) {
        concptr p;
        concptr t;

        /* Skip b1 */
        s++;

        /* First */
        t = process_pref_file_expr(player_ptr, &s, &f);

        if (!*t) {
        } else if (streq(t, "IOR")) {
            v = "0";
            while (*s && (f != b2)) {
                t = process_pref_file_expr(player_ptr, &s, &f);
                if (*t && !streq(t, "0")) {
                    v = "1";
                }
            }
        } else if (streq(t, "AND")) {
            v = "1";
            while (*s && (f != b2)) {
                t = process_pref_file_expr(player_ptr, &s, &f);
                if (*t && streq(t, "0")) {
                    v = "0";
                }
            }
        } else if (streq(t, "NOT")) {
            v = "1";
            while (*s && (f != b2)) {
                t = process_pref_file_expr(player_ptr, &s, &f);
                if (*t && streq(t, "1")) {
                    v = "0";
                }
            }
        } else if (streq(t, "EQU")) {
            v = "0";
            if (*s && (f != b2)) {
                t = process_pref_file_expr(player_ptr, &s, &f);
            }
            while (*s && (f != b2)) {
                p = process_pref_file_expr(player_ptr, &s, &f);
                if (streq(t, p)) {
                    v = "1";
                }
            }
        } else if (streq(t, "LEQ")) {
            v = "1";
            if (*s && (f != b2)) {
                t = process_pref_file_expr(player_ptr, &s, &f);
            }
            while (*s && (f != b2)) {
                p = t;
                t = process_pref_file_expr(player_ptr, &s, &f);
                if (*t && atoi(p) > atoi(t)) {
                    v = "0";
                }
            }
        } else if (streq(t, "GEQ")) {
            v = "1";
            if (*s && (f != b2)) {
                t = process_pref_file_expr(player_ptr, &s, &f);
            }
            while (*s && (f != b2)) {
                p = t;
                t = process_pref_file_expr(player_ptr, &s, &f);
                if (*t && atoi(p) < atoi(t)) {
                    v = "0";
                }
            }
        } else {
            while (*s && (f != b2)) {
                t = process_pref_file_expr(player_ptr, &s, &f);
            }
        }

        if (f != b2) {
            v = "?x?x?";
        }

        if ((f = *s) != '\0') {
            *s++ = '\0';
        }

        *fp = f;
        *sp = s;
        return v;
    }

    /* Accept all printables except spaces and brackets */
#ifdef JP
    while (iskanji(*s) || (isprint(*s) && !angband_strchr(" []", *s))) {
        if (iskanji(*s)) {
            s++;
        }
        s++;
    }
#else
    while (isprint(*s) && !angband_strchr(" []", *s)) {
        ++s;
    }
#endif

    if ((f = *s) != '\0') {
        *s++ = '\0';
    }

    if (*b != '$') {
        v = b;
        *fp = f;
        *sp = s;
        return v;
    }

    if (streq(b + 1, "SYS")) {
        v = ANGBAND_SYS;
    } else if (streq(b + 1, "KEYBOARD")) {
        v = ANGBAND_KEYBOARD;
    } else if (streq(b + 1, "GRAF")) {
        v = ANGBAND_GRAF;
    } else if (streq(b + 1, "MONOCHROME")) {
        if (arg_monochrome) {
            v = "ON";
        } else {
            v = "OFF";
        }
    } else if (streq(b + 1, "RACE")) {
#ifdef JP
        v = rp_ptr->E_title;
#else
        v = rp_ptr->title;
#endif
    } else if (streq(b + 1, "CLASS")) {
#ifdef JP
        v = cp_ptr->E_title;
#else
        v = cp_ptr->title;
#endif
    } else if (streq(b + 1, "PLAYER")) {
        static char tmp_player_name[32];
        char *pn, *tpn;
        for (pn = player_ptr->name, tpn = tmp_player_name; *pn; pn++, tpn++) {
#ifdef JP
            if (iskanji(*pn)) {
                *(tpn++) = *(pn++);
                *tpn = *pn;
                continue;
            }
#endif
            *tpn = angband_strchr(" []", *pn) ? '_' : *pn;
        }

        *tpn = '\0';
        v = tmp_player_name;
    } else if (streq(b + 1, "REALM1")) {
#ifdef JP
        v = E_realm_names[player_ptr->realm1];
#else
        v = realm_names[player_ptr->realm1];
#endif
    } else if (streq(b + 1, "REALM2")) {
#ifdef JP
        v = E_realm_names[player_ptr->realm2];
#else
        v = realm_names[player_ptr->realm2];
#endif
    } else if (streq(b + 1, "LEVEL")) {
        sprintf(tmp, "%02d", player_ptr->lev);
        v = tmp;
    } else if (streq(b + 1, "AUTOREGISTER")) {
        if (player_ptr->autopick_autoregister) {
            v = "1";
        } else {
            v = "0";
        }
    } else if (streq(b + 1, "MONEY")) {
        sprintf(tmp, "%09ld", (long int)player_ptr->au);
        v = tmp;
    }

    *fp = f;
    *sp = s;
    return v;
}
