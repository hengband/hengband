#include "util/string-processor.h"
#include "util/int-char-converter.h"

/*!
 * 10進数から16進数への変換テーブル /
 * Global array for converting numbers to uppercase hecidecimal digit
 * This array can also be used to convert a number to an octal digit
 */
const char hexsym[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

int max_macrotrigger = 0; /*!< 現在登録中のマクロ(トリガー)の数 */
concptr macro_template = nullptr; /*!< Angband設定ファイルのT: タグ情報から読み込んだ長いTコードを処理するために利用する文字列ポインタ */
concptr macro_modifier_chr; /*!< &x# で指定されるマクロトリガーに関する情報を記録する文字列ポインタ */
concptr macro_modifier_name[MAX_MACRO_MOD]; /*!< マクロ上で取り扱う特殊キーを文字列上で表現するためのフォーマットを記録した文字列ポインタ配列 */
concptr macro_trigger_name[MAX_MACRO_TRIG]; /*!< マクロのトリガーコード */
concptr macro_trigger_keycode[2][MAX_MACRO_TRIG]; /*!< マクロの内容 */

/*
 * Convert a decimal to a single digit octal number
 */
static char octify(uint i)
{
    return hexsym[i % 8];
}

/*
 * Convert a decimal to a single digit hex number
 */
static char hexify(uint i)
{
    return hexsym[i % 16];
}

/*
 * Convert a octal-digit into a decimal
 */
static int deoct(char c)
{
    if (isdigit(c)) {
        return D2I(c);
    }
    return 0;
}

/*
 * Convert a hexidecimal-digit into a decimal
 */
static int dehex(char c)
{
    if (isdigit(c)) {
        return D2I(c);
    }
    if (islower(c)) {
        return A2I(c) + 10;
    }
    if (isupper(c)) {
        return A2I(tolower(c)) + 10;
    }
    return 0;
}

static char force_upper(char a)
{
    return islower(a) ? toupper(a) : a;
}

static int angband_stricmp(concptr a, concptr b)
{
    for (concptr s1 = a, s2 = b; true; s1++, s2++) {
        char z1 = force_upper(*s1);
        char z2 = force_upper(*s2);
        if (z1 < z2) {
            return -1;
        }
        if (z1 > z2) {
            return 1;
        }
        if (!z1) {
            return 0;
        }
    }
}

static int angband_strnicmp(concptr a, concptr b, int n)
{
    for (concptr s1 = a, s2 = b; n > 0; s1++, s2++, n--) {
        char z1 = force_upper(*s1);
        char z2 = force_upper(*s2);
        if (z1 < z2) {
            return -1;
        }
        if (z1 > z2) {
            return 1;
        }
        if (!z1) {
            return 0;
        }
    }

    return 0;
}

static void trigger_text_to_ascii(char **bufptr, concptr *strptr)
{
    char *s = *bufptr;
    concptr str = *strptr;
    bool mod_status[MAX_MACRO_MOD];

    int i, len = 0;
    int shiftstatus = 0;
    concptr key_code;

    if (macro_template == nullptr) {
        return;
    }

    for (i = 0; macro_modifier_chr[i]; i++) {
        mod_status[i] = false;
    }
    str++;

    /* Examine modifier keys */
    while (true) {
        for (i = 0; macro_modifier_chr[i]; i++) {
            len = strlen(macro_modifier_name[i]);

            if (!angband_strnicmp(str, macro_modifier_name[i], len)) {
                break;
            }
        }

        if (!macro_modifier_chr[i]) {
            break;
        }
        str += len;
        mod_status[i] = true;
        if ('S' == macro_modifier_chr[i]) {
            shiftstatus = 1;
        }
    }

    for (i = 0; i < max_macrotrigger; i++) {
        len = strlen(macro_trigger_name[i]);
        if (!angband_strnicmp(str, macro_trigger_name[i], len) && ']' == str[len]) {
            break;
        }
    }

    if (i == max_macrotrigger) {
        str = angband_strchr(str, ']');
        if (str) {
            *s++ = (char)31;
            *s++ = '\r';
            *bufptr = s;
            *strptr = str; /* where **strptr == ']' */
        }

        return;
    }

    key_code = macro_trigger_keycode[shiftstatus][i];
    str += len;

    *s++ = (char)31;
    for (i = 0; macro_template[i]; i++) {
        char ch = macro_template[i];
        switch (ch) {
        case '&':
            for (int j = 0; macro_modifier_chr[j]; j++) {
                if (mod_status[j]) {
                    *s++ = macro_modifier_chr[j];
                }
            }

            break;
        case '#':
            strcpy(s, key_code);
            s += strlen(key_code);
            break;
        default:
            *s++ = ch;
            break;
        }
    }

    *s++ = '\r';

    *bufptr = s;
    *strptr = str; /* where **strptr == ']' */
    return;
}

/*
 * Hack -- convert a printable string into real ascii
 *
 * I have no clue if this function correctly handles, for example,
 * parsing "\xFF" into a (signed) char.  Whoever thought of making
 * the "sign" of a "char" undefined is a complete moron.  Oh well.
 */
void text_to_ascii(char *buf, std::string_view sv, size_t bufsize)
{
    char *s = buf;
    auto buffer_end = s + bufsize;
    auto str = sv.data();
    constexpr auto step_size = 1;
    while (*str && (s + step_size < buffer_end)) {
        if (*str == '\\') {
            str++;
            if (!(*str)) {
                break;
            }

            if (*str == '[') {
                trigger_text_to_ascii(&s, &str);
            } else {
                if (*str == 'x') {
                    *s = 16 * (char)dehex(*++str);
                    *s++ += (char)dehex(*++str);
                } else if (*str == '\\') {
                    *s++ = '\\';
                } else if (*str == '^') {
                    *s++ = '^';
                } else if (*str == 's') {
                    *s++ = ' ';
                } else if (*str == 'e') {
                    *s++ = ESCAPE;
                } else if (*str == 'b') {
                    *s++ = '\b';
                } else if (*str == 'n') {
                    *s++ = '\n';
                } else if (*str == 'r') {
                    *s++ = '\r';
                } else if (*str == 't') {
                    *s++ = '\t';
                } else if (*str == '0') {
                    *s = 8 * (char)deoct(*++str);
                    *s++ += (char)deoct(*++str);
                } else if (*str == '1') {
                    *s = 64 + 8 * (char)deoct(*++str);
                    *s++ += (char)deoct(*++str);
                } else if (*str == '2') {
                    *s = 64 * 2 + 8 * (char)deoct(*++str);
                    *s++ += (char)deoct(*++str);
                } else if (*str == '3') {
                    *s = 64 * 3 + 8 * (char)deoct(*++str);
                    *s++ += (char)deoct(*++str);
                }
            }

            str++;
        } else if (*str == '^') {
            str++;
            *s++ = (*str++ & 037);
        } else {
            *s++ = *str++;
        }
    }

    *s = '\0';
}

static bool trigger_ascii_to_text(char **bufptr, concptr *strptr)
{
    char *s = *bufptr;
    concptr str = *strptr;
    char key_code[100];
    int i;
    if (macro_template == nullptr) {
        return false;
    }

    *s++ = '\\';
    *s++ = '[';

    concptr tmp;
    for (i = 0; macro_template[i]; i++) {
        char ch = macro_template[i];

        switch (ch) {
        case '&':
            while ((tmp = angband_strchr(macro_modifier_chr, *str)) != 0) {
                int j = (int)(tmp - macro_modifier_chr);
                tmp = macro_modifier_name[j];
                while (*tmp) {
                    *s++ = *tmp++;
                }
                str++;
            }

            break;
        case '#': {
            int j;
            for (j = 0; *str && *str != '\r'; j++) {
                key_code[j] = *str++;
            }
            key_code[j] = '\0';
            break;
        }
        default:
            if (ch != *str) {
                return false;
            }
            str++;
        }
    }

    if (*str++ != '\r') {
        return false;
    }

    for (i = 0; i < max_macrotrigger; i++) {
        if (!angband_stricmp(key_code, macro_trigger_keycode[0][i]) || !angband_stricmp(key_code, macro_trigger_keycode[1][i])) {
            break;
        }
    }

    if (i == max_macrotrigger) {
        return false;
    }

    tmp = macro_trigger_name[i];
    while (*tmp) {
        *s++ = *tmp++;
    }

    *s++ = ']';

    *bufptr = s;
    *strptr = str;
    return true;
}

/*
 * Hack -- convert a string into a printable form
 */
void ascii_to_text(char *buf, std::string_view sv, size_t bufsize)
{
    char *s = buf;
    auto buffer_end = s + bufsize;
    auto str = sv.data();
    constexpr auto step_size = 4;
    while (*str && (s + step_size < buffer_end)) {
        byte i = (byte)(*str++);
        if (i == 31) {
            if (!trigger_ascii_to_text(&s, &str)) {
                *s++ = '^';
                *s++ = '_';
            }
        } else {
            if (i == ESCAPE) {
                *s++ = '\\';
                *s++ = 'e';
            } else if (i == ' ') {
                *s++ = '\\';
                *s++ = 's';
            } else if (i == '\b') {
                *s++ = '\\';
                *s++ = 'b';
            } else if (i == '\t') {
                *s++ = '\\';
                *s++ = 't';
            } else if (i == '\n') {
                *s++ = '\\';
                *s++ = 'n';
            } else if (i == '\r') {
                *s++ = '\\';
                *s++ = 'r';
            } else if (i == '^') {
                *s++ = '\\';
                *s++ = '^';
            } else if (i == '\\') {
                *s++ = '\\';
                *s++ = '\\';
            } else if (i < 32) {
                *s++ = '^';
                *s++ = i + 64;
            } else if (i < 127) {
                *s++ = i;
            } else if (i < 64) {
                *s++ = '\\';
                *s++ = '0';
                *s++ = octify(i / 8);
                *s++ = octify(i % 8);
            } else {
                *s++ = '\\';
                *s++ = 'x';
                *s++ = hexify(i / 16);
                *s++ = hexify(i % 16);
            }
        }
    }

    *s = '\0';
}

/*
 * The angband_strcpy() function copies up to 'bufsize'-1 characters from 'src'
 * to 'buf' and NUL-terminates the result.  The 'buf' and 'src' strings may
 * not overlap.
 *
 * angband_strcpy() returns strlen(src).  This makes checking for truncation
 * easy.  Example: if (angband_strcpy(buf, src, sizeof(buf)) >= sizeof(buf)) ...;
 *
 * This function should be equivalent to the strlcpy() function in BSD.
 */
size_t angband_strcpy(char *buf, std::string_view src, size_t bufsize)
{
#ifdef JP
    char *d = buf;
    const char *s = src.data();
    size_t len = 0;

    if (bufsize > 0) {
        /* reserve for NUL termination */
        bufsize--;

        /* Copy as many bytes as will fit */
        while (*s && (len < bufsize)) {
            if (iskanji(*s)) {
                if (len + 1 >= bufsize || !*(s + 1)) {
                    break;
                }
                *d++ = *s++;
                *d++ = *s++;
                len += 2;
            } else {
                *d++ = *s++;
                len++;
            }
        }
        *d = '\0';
    }

    while (*s++) {
        len++;
    }
    return len;

#else
    auto len = src.length();
    if (bufsize == 0) {
        return len;
    }

    if (len >= bufsize) {
        len = bufsize - 1;
    }

    (void)src.copy(buf, len);
    buf[len] = '\0';
    return src.length();
#endif
}

/*
 * The angband_strcat() tries to append a string to an existing NUL-terminated string.
 * It never writes more characters into the buffer than indicated by 'bufsize' and
 * NUL-terminates the buffer.  The 'buf' and 'src' strings may not overlap.
 *
 * angband_strcat() returns strlen(buf) + strlen(src).  This makes checking for
 * truncation easy.  Example:
 * if (angband_strcat(buf, src, sizeof(buf)) >= sizeof(buf)) ...;
 *
 * This function should be equivalent to the strlcat() function in BSD.
 */
size_t angband_strcat(char *buf, std::string_view src, size_t bufsize)
{
    size_t dlen = strlen(buf);
    if (dlen < bufsize - 1) {
        return dlen + angband_strcpy(buf + dlen, src, bufsize - dlen);
    } else {
        return dlen + src.length();
    }
}

/*
 * A copy of ANSI strstr()
 *
 * angband_strstr() can handle Kanji strings correctly.
 */
char *angband_strstr(const char *haystack, std::string_view needle)
{
    std::string_view haystack_view(haystack);
    auto l1 = haystack_view.length();
    auto l2 = needle.length();
    if (l1 < l2) {
        return nullptr;
    }

    for (size_t i = 0; i <= l1 - l2; i++) {
        const auto part = haystack_view.substr(i);
        if (part.starts_with(needle)) {
            return const_cast<char *>(haystack) + i;
        }

#ifdef JP
        if (iskanji(*(haystack + i))) {
            i++;
        }
#endif
    }

    return nullptr;
}

/*
 * A copy of ANSI strchr()
 *
 * angband_strchr() can handle Kanji strings correctly.
 */
char *angband_strchr(concptr ptr, char ch)
{
    for (; *ptr != '\0'; ptr++) {
        if (*ptr == ch) {
            return (char *)ptr;
        }

#ifdef JP
        if (iskanji(*ptr)) {
            ptr++;
        }
#endif
    }

    return nullptr;
}

/*!
 * @brief 左側の空白を除去
 * @param p char型のポインタ
 * @return 除去後のポインタ
 */
char *ltrim(char *p)
{
    while (p[0] == ' ') {
        p++;
    }
    return p;
}

/*!
 * @brief 右側の空白を除去
 * @param p char型のポインタ
 * @return 除去後のポインタ
 */
char *rtrim(char *p)
{
    int i = strlen(p) - 1;
    while (p[i] == ' ') {
        p[i--] = '\0';
    }
    return p;
}

/*!
 * @brief 文字列の後方から一致するかどうか比較する
 * @param s1 比較元文字列ポインタ
 * @param s2 比較先文字列ポインタ
 * @param len 比較する長さ
 * @return 等しい場合は0、p1が大きい場合は-1、p2が大きい場合は1
 * @details
 * strncmpの後方から比較する版
 */
int strrncmp(const char *s1, const char *s2, int len)
{
    int i;
    int l1 = strlen(s1);
    int l2 = strlen(s2);

    for (i = 1; i <= len; i++) {
        int p1 = l1 - i;
        int p2 = l2 - i;

        if (l1 != l2) {
            if (p1 < 0) {
                return -1;
            }
            if (p2 < 0) {
                return 1;
            }
        } else {
            if (p1 < 0) {
                return 0;
            }
        }

        if (s1[p1] < s2[p2]) {
            return -1;
        }
        if (s1[p1] > s2[p2]) {
            return -1;
        }
    }

    return 0;
}

/*
 * @brief マルチバイト文字のダメ文字('\')を考慮しつつ文字列比較を行う
 * @param src 比較元の文字列
 * @param find 比較したい文字列
 */
bool str_find(const std::string &src, std::string_view find)
{
    return angband_strstr(src.data(), find) != nullptr;
}

/**
 * @brief 文字列の両端の空白を削除する
 *
 * 文字列 str の両端にある空白(スペースおよびタブ)を削除し、
 * 削除した文字列を std::string 型のオブジェクトとして返す。
 * 文字列全体が空白の場合は空文字列を返す。
 *
 * @param str 操作の対象とする文字列
 * @return std::string strの両端の空白を削除した文字列
 */
std::string str_trim(std::string_view str)
{
    const auto start_pos = str.find_first_not_of(" \t");
    const auto end_pos = str.find_last_not_of(" \t");

    if (start_pos == std::string_view::npos || end_pos == std::string_view::npos) {
        return std::string();
    }

    return std::string(str.substr(start_pos, end_pos - start_pos + 1));
}

/**
 * @brief 文字列の右端の空白を削除する
 *
 * 文字列 str の右端にある空白(スペースおよびタブ)を削除し、
 * 削除した文字列を std::string 型のオブジェクトとして返す。
 * 文字列全体が空白の場合は空文字列を返す。
 *
 * @param str 操作の対象とする文字列
 * @return std::string strの右端の空白を削除した文字列
 */
std::string str_rtrim(std::string_view str)
{
    const auto end_pos = str.find_last_not_of(" \t");

    if (end_pos == std::string_view::npos) {
        return std::string();
    }

    return std::string(str.substr(0, end_pos + 1));
}

/**
 * @brief 文字列の左端の空白を削除する
 *
 * 文字列 str の左端にある空白(スペースおよびタブ)を削除し、
 * 削除した文字列を std::string 型のオブジェクトとして返す。
 * 文字列全体が空白の場合は空文字列を返す。
 *
 * @param str 操作の対象とする文字列
 * @return std::string strの左端の空白を削除した文字列
 */
std::string str_ltrim(std::string_view str)
{
    const auto start_pos = str.find_first_not_of(" \t");

    if (start_pos == std::string_view::npos) {
        return std::string();
    }

    return std::string(str.substr(start_pos));
}

/**
 * @brief 文字列を指定した文字で分割する
 *
 * 文字列 str を delim で指定した文字で分割し、分割した文字列を要素とする配列を
 * std::vector<std::string> 型のオブジェクトとして返す。
 *
 * @param str 操作の対象とする文字列
 * @param delim 文字列を分割する文字
 * @param trim trueの場合、分割した文字列の両端の空白を削除する
 * @param num 1以上の場合、事前にvectorサイズを予約しておく(速度向上)
 * @return std::vector<std::string> 分割した文字列を要素とする配列
 */
std::vector<std::string> str_split(std::string_view str, char delim, bool trim, int num)
{
    std::vector<std::string> result;
    if (num > 0) {
        result.reserve(num);
    }

    auto make_str = [trim](std::string_view sv) { return trim ? str_trim(sv) : std::string(sv); };

    while (true) {
        bool found = false;
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == delim) {
                result.push_back(make_str(str.substr(0, i)));
                str.remove_prefix(i + 1);
                found = true;
                break;
            }
#ifdef JP
            if (iskanji(str[i])) {
                ++i;
            }
#endif
        }
        if (!found) {
            result.push_back(make_str(str));
            return result;
        }
    }
}

/**
 * @brief 文字列を指定した文字数で分割する
 *
 * 文字列 str を len で指定した文字数で分割し、分割した文字列を要素とする配列を
 * std::vector<std::string> 型のオブジェクトとして返す。
 * 全角文字は2文字分として扱い、全角文字の前半で分割されてしまわないように処理される。
 *
 * @param str 操作の対象とする文字列
 * @param len 分割する文字数
 * @return std::vector<std::string> 分割した文字列を要素とする配列
 */
std::vector<std::string> str_separate(std::string_view str, size_t len)
{
    std::vector<std::string> result;

    while (!str.empty()) {
        result.push_back(str_substr(str, 0, len));
        str.remove_prefix(result.back().size());
    }

    return result;
}

/**
 * @brief 文字列から指定した文字を取り除く
 *
 * 文字列 str から文字列 erase_chars に含まれる文字をすべて削除し、
 * 削除した文字列を std::string 型のオブジェクトとして返す。
 *
 * @param str 操作の対象とする文字列
 * @param erase_chars 削除する文字を指定する文字列
 * @return std::string 指定した文字をすべて削除した文字列
 */
std::string str_erase(std::string str, std::string_view erase_chars)
{
    for (auto it = str.begin(); it != str.end();) {
        if (erase_chars.find(*it) != std::string_view::npos) {
            it = str.erase(it);
            continue;
        }
#ifdef JP
        if (iskanji(*it)) {
            ++it;
        }
#endif
        ++it;
    }

    return str;
}

static std::pair<size_t, size_t> adjust_substr_pos(std::string_view sv, size_t pos, size_t n)
{
    const auto start = std::min(pos, sv.length());
    const auto end = n == std::string_view::npos ? sv.length() : std::min(pos + n, sv.length());

#ifdef JP
    auto seek_pos = 0U;
    while (seek_pos < start) {
        if (iskanji(sv[seek_pos])) {
            ++seek_pos;
        }
        ++seek_pos;
    }
    const auto mb_pos = seek_pos;

    while (seek_pos < end) {
        if (iskanji(sv[seek_pos])) {
            if (seek_pos == end - 1) {
                break;
            }
            ++seek_pos;
        }
        ++seek_pos;
    }
    const auto mb_n = seek_pos - mb_pos;

    return { mb_pos, mb_n };
#else
    return { start, end - start };
#endif
}

/*!
 * @brief 2バイト文字を考慮して部分文字列を取得する
 *
 * 引数で与えられた文字列から pos バイト目から n バイトの部分文字列を取得する。
 * 但し、以下の通り2バイト文字の途中で分断されないようにする。
 * - 開始位置 pos バイト目が2バイト文字の後半バイトの場合は pos+1 バイト目を開始位置とする。
 * - 終了位置 pos+n バイト目が2バイト文字の前半バイトの場合は pos+n-1 バイト目を終了位置とする。
 *
 * @param sv 文字列
 * @param pos 部分文字列の開始位置
 * @param n 部分文字列の長さ
 * @return 部分文字列
 */
std::string str_substr(std::string_view sv, size_t pos, size_t n)
{
    const auto [mb_pos, mb_n] = adjust_substr_pos(sv, pos, n);
    return std::string(sv.substr(mb_pos, mb_n));
}

/*!
 * @brief 2バイト文字を考慮して部分文字列を取得する
 *
 * 引数で与えられた文字列を pos バイト目から n バイトの部分文字列にして返す。
 * 但し、以下の通り2バイト文字の途中で分断されないようにする。
 * - 開始位置 pos バイト目が2バイト文字の後半バイトの場合は pos+1 バイト目を開始位置とする。
 * - 終了位置 pos+n バイト目が2バイト文字の前半バイトの場合は pos+n-1 バイト目を終了位置とする。
 *
 * @param str 文字列
 * @param pos 部分文字列の開始位置
 * @param n 部分文字列の長さ
 * @return 部分文字列
 */
std::string str_substr(std::string &&str, size_t pos, size_t n)
{
    const auto [mb_pos, mb_n] = adjust_substr_pos(str, pos, n);
    str.erase(mb_pos + mb_n);
    str.erase(0, mb_pos);
    return std::move(str);
}

std::string str_substr(const char *str, size_t pos, size_t n)
{
    return str_substr(std::string_view(str), pos, n);
}
