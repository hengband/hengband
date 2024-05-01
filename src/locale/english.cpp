#include "locale/english.h"
#include "system/angband-exceptions.h"
#include "system/angband.h"
#include "util/string-processor.h"

#ifndef JP

namespace {
std::string replace_tail(std::string_view name, size_t replace_num, std::string_view replace_str)
{
    if (name.length() < replace_num) {
        THROW_EXCEPTION(std::logic_error, "replace_num is too large");
    }

    std::string result(name.substr(0, name.length() - replace_num));
    result.append(replace_str);
    return result;
}
}

/*!
 * @brief 英単語、句、説を複数形を変換する / Pluralize a monster name
 * @param name 変換する文字列
 */
std::string pluralize(std::string_view name)
{
    /// @note 英語版のみの処理なのでマルチバイト文字は考慮しなくてよい
    /// contains() はC++23以降

    if (name.find("Disembodied hand") != std::string_view::npos) {
        return "Disembodied hands that strangled people";
    }

    if (name.find("Colour out of space") != std::string_view::npos) {
        return "Colours out of space";
    }

    if (name.find("stairway to hell") != std::string_view::npos) {
        return "stairways to hell";
    }

    if (name.find("Dweller on the threshold") != std::string_view::npos) {
        return "Dwellers on the threshold";
    }

    if (auto it = name.find(" of "); it != std::string_view::npos) {
        std::string formar_part(name.begin(), name.begin() + it);
        std::string latter_part(name.begin() + it, name.end());

        if (formar_part.back() == 's') {
            formar_part.append("es");
        } else {
            formar_part.push_back('s');
        }

        return formar_part + latter_part;
    }

    if (name.find("coins") != std::string_view::npos) {
        return std::string("piles of ").append(name);
    }

    if (name.find("Manes") != std::string_view::npos) {
        return std::string(name);
    }

    if (name.ends_with("y") && !(name.length() >= 2 && is_a_vowel(name[name.length() - 2]))) {
        return replace_tail(name, 1, "ies");
    }

    if (name.ends_with("ouse")) {
        return replace_tail(name, 4, "ice");
    }

    if (name.ends_with("ous")) {
        return replace_tail(name, 3, "i");
    }

    if (name.ends_with("us")) {
        return replace_tail(name, 2, "i");
    }

    if (name.ends_with("kelman")) {
        return replace_tail(name, 6, "kelmen");
    }

    if (name.ends_with("wordsman")) {
        return replace_tail(name, 8, "wordsmen");
    }

    if (name.ends_with("oodsman")) {
        return replace_tail(name, 7, "oodsmen");
    }

    if (name.ends_with("eastman")) {
        return replace_tail(name, 7, "eastmen");
    }

    if (name.ends_with("izardman")) {
        return replace_tail(name, 8, "izardmen");
    }

    if (name.ends_with("geist")) {
        return replace_tail(name, 5, "geister");
    }

    if (name.ends_with("ex")) {
        return replace_tail(name, 2, "ices");
    }

    if (name.ends_with("lf")) {
        return replace_tail(name, 2, "lves");
    }

    if (name.ends_with("ch") ||
        name.ends_with("sh") ||
        name.ends_with("nx") ||
        name.ends_with("s") ||
        name.ends_with("o")) {
        return std::string(name).append("es");
    }

    return std::string(name).append("s");
}

/*
 * Check a char for "vowel-hood"
 */
bool is_a_vowel(int ch)
{
    switch (ch) {
    case 'a':
    case 'e':
    case 'i':
    case 'o':
    case 'u':
    case 'A':
    case 'E':
    case 'I':
    case 'O':
    case 'U':
        return true;
    }

    return false;
}

#endif // !JP
