#include "util/quarks.h"

/*
 * The number of quarks
 */
STR_OFFSET quark__num;

/*
 * The pointers to the quarks [QUARK_MAX]
 */
concptr *quark__str;

/*
 * Initialize the quark array
 */
void quark_init(void)
{
    C_MAKE(quark__str, QUARK_MAX, concptr);
    quark__str[1] = string_make("");
    quark__num = 2;
}

/*
 * Add a new "quark" to the set of quarks.
 */
uint16_t quark_add(concptr str)
{
    uint16_t i;
    for (i = 1; i < quark__num; i++) {
        if (streq(quark__str[i], str))
            return (i);
    }

    if (quark__num == QUARK_MAX)
        return 1;

    quark__num = i + 1;
    quark__str[i] = string_make(str);
    return (i);
}

/*
 * This function looks up a quark
 */
concptr quark_str(STR_OFFSET i)
{
    concptr q;

    /* Return nullptr for an invalid index */
    if ((i < 1) || (i >= quark__num))
        return nullptr;

    /* Access the quark */
    q = quark__str[i];

    /* Return the quark */
    return (q);
}
