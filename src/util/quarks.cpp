#include "util/quarks.h"

#include <string>
#include <vector>

namespace {
/*!
 * @brief 銘情報の最大数 / Maximum number of "quarks"
 * @note
 * Default: assume at most 512 different inscriptions are used<br>
 * Was 512... 256 quarks added for random artifacts<br>
 */
constexpr auto QUARK_MAX = 768;

/*
 * The pointers to the quarks [QUARK_MAX]
 */
std::vector<std::string> quark__str;
}

/*
 * Initialize the quark array
 */
void quark_init(void)
{
    //! @note [0]は使用しない、[1]は空文字列固定
    quark__str.assign(2, {});
    quark__str[1] = "";
}

/*
 * Add a new "quark" to the set of quarks.
 */
uint16_t quark_add(concptr str)
{
    for (uint16_t i = 1; i < quark__str.size(); i++) {
        if (streq(quark__str[i], str))
            return (i);
    }

    if (quark__str.size() >= QUARK_MAX)
        return 1;

    quark__str.emplace_back(str);
    return quark__str.size() - 1;
}

/*
 * This function looks up a quark
 */
concptr quark_str(STR_OFFSET i)
{
    concptr q;

    /* Return nullptr for an invalid index */
    if ((i < 1) || (i >= quark__str.size()))
        return nullptr;

    /* Access the quark */
    q = quark__str[i].data();

    /* Return the quark */
    return (q);
}
