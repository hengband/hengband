#include "cmd-io/macro-util.h"

/* Current macro action [1024] */
std::vector<char> macro__buf;

/* Array of macro patterns [MACRO_MAX] */
std::vector<std::string> macro__pat;

/* Array of macro actions [MACRO_MAX] */
std::vector<std::string> macro__act;

/* Number of active macros */
int16_t macro__num;

/* Expand macros in "get_com" or not */
bool get_com_no_macros = false;

/* Determine if any macros have ever started with a given character */
static bool macro__use[256];

/* Find the macro (if any) which exactly matches the given pattern */
int macro_find_exact(concptr pat)
{
    if (!macro__use[(byte)(pat[0])]) {
        return -1;
    }

    for (int i = 0; i < macro__num; ++i) {
        if (!streq(macro__pat[i], pat))
            continue;

        return i;
    }

    return -1;
}

/*
 * Find the first macro (if any) which contains the given pattern
 */
int macro_find_check(concptr pat)
{
    if (!macro__use[(byte)(pat[0])]) {
        return -1;
    }

    for (int i = 0; i < macro__num; ++i) {
        if (!prefix(macro__pat[i], pat))
            continue;

        return i;
    }

    return -1;
}

/*
 * Find the first macro (if any) which contains the given pattern and more
 */
int macro_find_maybe(concptr pat)
{
    if (!macro__use[(byte)(pat[0])]) {
        return -1;
    }

    for (int i = 0; i < macro__num; ++i) {
        if (!prefix(macro__pat[i], pat))
            continue;
        if (streq(macro__pat[i], pat))
            continue;

        return i;
    }

    return -1;
}

/*
 * Find the longest macro (if any) which starts with the given pattern
 */
int macro_find_ready(concptr pat)
{
    int t, n = -1, s = -1;

    if (!macro__use[(byte)(pat[0])]) {
        return -1;
    }

    for (int i = 0; i < macro__num; ++i) {
        if (!prefix(pat, macro__pat[i]))
            continue;

        t = macro__pat[i].size();
        if ((n >= 0) && (s > t))
            continue;

        n = i;
        s = t;
    }

    return n;
}

/*
 * Add a macro definition (or redefinition).
 *
 * We should use "act == nullptr" to "remove" a macro, but this might make it
 * impossible to save the "removal" of a macro definition.
 *
 * We should consider refusing to allow macros which contain existing macros,
 * or which are contained in existing macros, because this would simplify the
 * macro analysis code.
 *
 * We should consider removing the "command macro" crap, and replacing it
 * with some kind of "powerful keymap" ability, but this might make it hard
 * to change the "roguelike" option from inside the game.
 */
errr macro_add(concptr pat, concptr act)
{
    if (!pat || !act)
        return -1;
    if (strlen(pat) == 0 || strlen(act) == 0)
        return -1;

    int n = macro_find_exact(pat);
    if (n < 0) {
        n = macro__num++;
        macro__pat[n] = pat;
    }

    macro__act[n] = act;
    macro__use[(byte)(pat[0])] = true;
    return 0;
}
