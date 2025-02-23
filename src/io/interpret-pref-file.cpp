/*!
 * @brief prefファイルの内容を解釈しメモリに展開する
 * @date 2020/03/01
 * @author Hourier
 */

#include "io/interpret-pref-file.h"
#include "birth/character-builder.h"
#include "cmd-io/macro-util.h"
#include "game-option/game-option-page.h"
#include "game-option/option-flags.h"
#include "game-option/option-types-table.h"
#include "io/gf-descriptions.h"
#include "io/input-key-requester.h"
#include "io/macro-configurations-store.h"
#include "io/tokenizer.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "term/gameterm.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"

std::optional<std::string> histpref_buf;

/*!
 * @brief 生い立ちメッセージの内容をバッファに加える。 / Hook function for reading the histpref.prf file.
 */
static void add_history_from_pref_line(std::string_view t)
{
    if (!histpref_buf) {
        return;
    }

    histpref_buf->append(t);
}

/*!
 * @brief Rトークンの解釈 / Process "R:<num>:<a>/<c>" -- attr/char for monster races
 * @param buf バッファ
 * @return 解釈に成功したか否か
 */
static bool interpret_r_token(char *buf)
{
    char *zz[16];
    if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) {
        return false;
    }

    const auto i = std::stoi(zz[0], nullptr, 0);
    const auto n1 = static_cast<uint8_t>(std::stoi(zz[1], nullptr, 0));
    const auto n2 = static_cast<char>(std::stoi(zz[2], nullptr, 0));
    auto &monraces = MonraceList::get_instance();
    if (i >= std::ssize(monraces)) {
        return false;
    }

    auto &monrace = monraces.get_monrace(i2enum<MonraceId>(i));
    /* Allow TERM_DARK text */
    if (n1 || (!(n2 & 0x80) && n2)) {
        monrace.symbol_config.color = n1;
    }

    if (n2) {
        monrace.symbol_config.character = n2;
    }

    return true;
}

/*!
 * @brief Kトークンの解釈 / Process "K:<num>:<a>/<c>"  -- attr/char for object kinds
 * @param buf バッファ
 * @return 解釈に成功したか否か
 */
static bool interpret_k_token(char *buf)
{
    char *zz[16];
    if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) {
        return false;
    }

    const auto i = static_cast<short>(std::stoi(zz[0], nullptr, 0));
    const auto color = static_cast<uint8_t>(std::stoi(zz[1], nullptr, 0));
    const auto character = static_cast<char>(std::stoi(zz[2], nullptr, 0));
    auto &baseitems = BaseitemList::get_instance();
    if (i >= static_cast<int>(baseitems.size())) {
        return false;
    }

    /* Allow TERM_DARK text */
    auto &baseitem = baseitems.get_baseitem(i);
    if ((color > 0) || (((character & 0x80) == 0) && (character != '\0'))) {
        baseitem.symbol_config.color = color;
    }

    if (character != '\0') {
        baseitem.symbol_config.character = character;
    }

    return true;
}

/*!
 * @brief トークン数によって地形の文字形と色を決定する
 * @param i 地形種別
 * @param num トークン数
 * @param zz トークン内容
 */
static void decide_feature_type(int i, int num, char **zz)
{
    auto &terrain = TerrainList::get_instance().get_terrain(static_cast<short>(i));
    const auto color_token = static_cast<uint8_t>(std::stoi(zz[1], nullptr, 0));
    const auto character_token = static_cast<char>(std::stoi(zz[2], nullptr, 0));
    const auto has_character_token = character_token != '\0';

    /* Allow TERM_DARK text */
    if ((color_token > 0) || (!(character_token & 0x80) && has_character_token)) {
        terrain.symbol_configs[F_LIT_STANDARD].color = color_token;
    }

    if (has_character_token) {
        terrain.symbol_configs[F_LIT_STANDARD].character = character_token;
    }

    switch (num) {
    case 3: {
        /* No lighting support */
        const auto &symbol = terrain.symbol_configs.at(F_LIT_STANDARD);
        for (auto j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++) {
            terrain.symbol_configs[j] = symbol;
        }

        return;
    }
    case 4:
        terrain.reset_lighting();
        return;
    case F_LIT_MAX * 2 + 1:
        /* Use desired lighting */
        for (auto j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++) {
            const auto color = static_cast<uint8_t>(std::stoi(zz[j * 2 + 1], nullptr, 0));
            const auto character = static_cast<char>(std::stoi(zz[j * 2 + 2], nullptr, 0));
            const auto has_character = character != '\0';
            auto &symbol = terrain.symbol_configs[j];

            /* Allow TERM_DARK text */
            if ((color != 0) || (!(character & 0x80) && has_character)) {
                symbol.color = color;
            }

            if (has_character) {
                symbol.character = character;
            }
        }

        return;
    default:
        return;
    }
}

/*!
 * @brief Fトークンの解釈 / Process "F:<num>:<a>/<c>" -- attr/char for terrain features
 * @param buf バッファ
 * @return 解釈に成功したか否か
 * @details
 * "F:<num>:<a>/<c>"
 * "F:<num>:<a>/<c>:LIT"
 * "F:<num>:<a>/<c>:<la>/<lc>:<da>/<dc>"
 */
static bool interpret_f_token(char *buf)
{
    char *zz[16];
    int num = tokenize(buf + 2, F_LIT_MAX * 2 + 1, zz, TOKENIZE_CHECKQUOTE);

    if ((num != 3) && (num != 4) && (num != F_LIT_MAX * 2 + 1)) {
        return false;
    }

    if ((num == 4) && !streq(zz[3], "LIT")) {
        return false;
    }

    int i = (int)strtol(zz[0], nullptr, 0);
    if (i >= static_cast<int>(TerrainList::get_instance().size())) {
        return false;
    }

    decide_feature_type(i, num, zz);
    return true;
}

/*!
 * @brief Fトークンの解釈 / Process "S:<num>:<a>/<c>" -- attr/char for special things
 * @param buf バッファ
 * @return 解釈に成功したか否か
 */
static bool interpret_s_token(char *buf)
{
    char *zz[16];
    if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) {
        return false;
    }

    int j = (byte)strtol(zz[0], nullptr, 0);
    TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], nullptr, 0);
    auto n2 = static_cast<char>(strtol(zz[2], nullptr, 0));
    misc_to_attr[j] = n1;
    misc_to_char[j] = n2;
    return true;
}

/*!
 * @brief Uトークンの解釈 / Process "U:<tv>:<a>/<c>" -- attr/char for unaware items
 * @param buf バッファ
 * @return 解釈に成功したか否か
 */
static bool interpret_u_token(char *buf)
{
    char *zz[16];
    if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) {
        return false;
    }

    const auto tval = i2enum<ItemKindType>(std::stoi(zz[0], nullptr, 0));
    const auto n1 = static_cast<uint8_t>(std::stoi(zz[1], nullptr, 0));
    const auto n2 = static_cast<char>(strtol(zz[2], nullptr, 0));
    for (auto &baseitem : BaseitemList::get_instance()) {
        if (baseitem.is_valid() && (baseitem.bi_key.tval() == tval)) {
            if (n1) {
                baseitem.symbol_definition.color = n1;
            }

            if (n2) {
                baseitem.symbol_definition.character = n2;
            }
        }
    }

    return true;
}

/*!
 * @brief Eトークンの解釈 / Process "E:<tv>:<a>" -- attribute for inventory objects
 * @param buf バッファ
 * @return 解釈に成功したか否か
 */
static bool interpret_e_token(char *buf)
{
    char *zz[16];
    if (tokenize(buf + 2, 2, zz, TOKENIZE_CHECKQUOTE) != 2) {
        return false;
    }

    int j = (byte)strtol(zz[0], nullptr, 0) % 128;
    TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], nullptr, 0);
    if (n1) {
        tval_to_attr[j] = n1;
    }
    return true;
}

/*!
 * @brief Pトークンの解釈 / Process "P:<str>" -- normal macro
 * @param buf バッファ
 * @return エラーコード
 */
static int interpret_p_token(char *buf)
{
    char tmp[1024];
    text_to_ascii(tmp, buf + 2, sizeof(tmp));
    return macro_add(tmp, macro_buffers.data());
}

/*!
 * @brief Cトークンの解釈 / Process "C:<str>" -- create keymap
 * @param buf バッファ
 * @return 解釈に成功したか否か
 */
static bool interpret_c_token(char *buf)
{
    char *zz[16];
    if (tokenize(buf + 2, 2, zz, TOKENIZE_CHECKQUOTE) != 2) {
        return false;
    }

    int mode = strtol(zz[0], nullptr, 0);
    if ((mode < 0) || (mode >= KEYMAP_MODES)) {
        return false;
    }

    char tmp[1024];
    text_to_ascii(tmp, zz[1], sizeof(tmp));
    if (!tmp[0] || tmp[1]) {
        return false;
    }

    int i = (byte)(tmp[0]);
    string_free(keymap_act[mode][i]);
    keymap_act[mode][i] = string_make(macro_buffers.data());
    return true;
}

/*!
 * @brief Vトークンの解釈 / Process "V:<num>:<kv>:<rv>:<gv>:<bv>" -- visual info
 * @param buf バッファ
 * @return 解釈に成功したか否か
 */
static bool interpret_v_token(char *buf)
{
    char *zz[16];
    if (tokenize(buf + 2, 5, zz, TOKENIZE_CHECKQUOTE) != 5) {
        return false;
    }

    int i = (byte)strtol(zz[0], nullptr, 0);
    angband_color_table[i][0] = (byte)strtol(zz[1], nullptr, 0);
    angband_color_table[i][1] = (byte)strtol(zz[2], nullptr, 0);
    angband_color_table[i][2] = (byte)strtol(zz[3], nullptr, 0);
    angband_color_table[i][3] = (byte)strtol(zz[4], nullptr, 0);
    return true;
}

/*!
 * @brief X/Yトークンの解釈
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param buf バッファ
 * @details
 * Process "X:<str>" -- turn option off
 * Process "Y:<str>" -- turn option on
 * オプションの名前が正しくない時も、パース自体は続行する (V2以前からの仕様)
 */
static void interpret_xy_token(PlayerType *player_ptr, char *buf)
{
    const auto &world = AngbandWorld::get_instance();
    for (auto &option : option_info) {
        if (option.text != buf + 2) {
            continue;
        }

        int os = option.flag_position;
        int ob = option.offset;
        if ((player_ptr->playing || world.character_xtra) && (GameOptionPage::BIRTH == option.page) && !world.wizard) {
            msg_format(_("初期オプションは変更できません! '%s'", "Birth options can not be changed! '%s'"), buf);
            msg_print(nullptr);
            return;
        }

        if (buf[0] == 'X') {
            g_option_flags[os] &= ~(1UL << ob);
            *option.value = false;
            return;
        }

        g_option_flags[os] |= (1UL << ob);
        *option.value = true;
        return;
    }

    msg_format(_("オプションの名前が正しくありません： %s", "Ignored invalid option: %s"), buf);
    msg_print(nullptr);
}

/*!
 * @brief Zトークンの解釈 / Process "Z:<type>:<str>" -- set spell color
 * @param line トークン1行
 * @param zz トークン保管文字列
 * @return 解釈に成功したか
 */
static bool interpret_z_token(std::string_view line)
{
    constexpr auto num_splits = 3;
    const auto splits = str_split(line, ':', false, num_splits);
    if (splits.size() != num_splits) {
        return false;
    }

    for (const auto &[name, num] : gf_descriptions) {
        if (name != splits[1]) {
            continue;
        }

        gf_colors[num] = splits[2];
        return true;
    }

    return false;
}

/*!
 * @brief Tトークンの解釈 / Process "T:<template>:<modifier chr>:<modifier name>:..." for 4 tokens
 * @param num_tokens トークン数
 * @param zz トークン保管文字列
 * @return 解釈に成功したか否か
 */
static bool decide_template_modifier(size_t num_tokens, char **zz)
{
    if (macro_template) {
        const size_t macro_modifier_length = macro_modifier_chr ? macro_modifier_chr->length() : 0;
        macro_template.reset();
        macro_modifier_chr.reset();
        for (size_t i = 0; i < macro_modifier_length; i++) {
            macro_modifier_names[i] = "";
        }

        for (size_t i = 0; i < max_macrotrigger; i++) {
            macro_trigger_names[i] = "";
            macro_trigger_keycodes.at(ShiftStatus::OFF).at(i) = "";
            macro_trigger_keycodes.at(ShiftStatus::ON).at(i) = "";
        }

        max_macrotrigger = 0;
    }

    if (*zz[0] == '\0') {
        return true;
    }

    size_t zz_length = strlen(zz[1]);
    zz_length = std::min(MAX_MACRO_MOD, zz_length);
    if (2 + zz_length != num_tokens) {
        return false;
    }

    macro_template = zz[0];
    macro_modifier_chr = zz[1];
    for (size_t i = 0; i < zz_length; i++) {
        macro_modifier_names[i] = zz[2 + i];
    }

    return true;
}

/*!
 * @brief Tトークンの解釈 / Process "T:<trigger>:<keycode>:<shift-keycode>" for 2 or 3 tokens
 * @param tok トークン数
 * @param zz トークン保管文字列
 * @return 解釈に成功したか否か
 */
static bool interpret_macro_keycodes(int tok, char **zz)
{
    if (max_macrotrigger >= MAX_MACRO_TRIG) {
        msg_print(_("マクロトリガーの設定が多すぎます!", "Too many macro triggers!"));
        return false;
    }

    auto m = max_macrotrigger;
    max_macrotrigger++;
    std::string t;
    const auto *s = zz[0];
    while (*s != '\0') {
        if ('\\' == *s) {
            s++;
        }

        t.push_back(*s++);
    }

    macro_trigger_names[m] = std::move(t);
    macro_trigger_keycodes.at(ShiftStatus::OFF).at(m) = zz[1];
    if (tok == 3) {
        macro_trigger_keycodes.at(ShiftStatus::ON).at(m) = zz[2];
        return true;
    }

    macro_trigger_keycodes.at(ShiftStatus::ON).at(m) = zz[1];
    return true;
}

/*!
 * @brief Tトークンの個数調査 (解釈はサブルーチンで) / Initialize macro trigger names and a template
 * @param buf バッファ
 * @return 解釈に成功したか否か
 * @todo 2.2.1r時点のコードからトークン数0～1の場合もエラーコード0だが、1であるべきでは？
 */
static bool interpret_t_token(char *buf)
{
    char *zz[16];
    int tok = tokenize(buf + 2, 2 + MAX_MACRO_MOD, zz, 0);
    if (tok >= 4) {
        return decide_template_modifier(tok, zz);
    }
    if (tok < 2) {
        return true;
    }

    return interpret_macro_keycodes(tok, zz);
}

/*!
 * @brief 設定ファイルの各行から各種テキスト情報を取得する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param buf データテキストの参照ポインタ
 * @return 解釈に成功したか否か
 * @details
 * <pre>
 * Each "action" line has an "action symbol" in the first column,
 * followed by a colon, followed by some command specific info,
 * usually in the form of "tokens" separated by colons or slashes.
 * Blank lines, lines starting with white space, and lines starting
 * with pound signs ("#") are ignored (as comments).
 * Note the use of "tokenize()" to allow the use of both colons and
 * slashes as delimeters, while still allowing final tokens which
 * may contain any characters including "delimiters".
 * Note the use of "strtol()" to allow all "integers" to be encoded
 * in decimal, hexidecimal, or octal form.
 * Note that "monster zero" is used for the "player" attr/char, "object
 * zero" will be used for the "stack" attr/char, and "feature zero" is
 * used for the "nothing" attr/char.
 * </pre>
 */
int interpret_pref_file(PlayerType *player_ptr, char *buf)
{
    if (buf[1] != ':') {
        return 1;
    }

    switch (buf[0]) {
    case 'H':
        /* Process "H:<history>" */
        add_history_from_pref_line(buf + 2);
        return 0;
    case 'R':
        return interpret_r_token(buf) ? 0 : 1;
    case 'K':
        return interpret_k_token(buf) ? 0 : 1;
    case 'F':
        return interpret_f_token(buf) ? 0 : 1;
    case 'S':
        return interpret_s_token(buf) ? 0 : 1;
    case 'U':
        return interpret_u_token(buf) ? 0 : 1;
    case 'E':
        return interpret_e_token(buf) ? 0 : 1;
    case 'A':
        /* Process "A:<str>" -- save an "action" for later */
        text_to_ascii(macro_buffers.data(), buf + 2, macro_buffers.size());
        return 0;
    case 'P':
        return interpret_p_token(buf);
    case 'C':
        return interpret_c_token(buf) ? 0 : 1;
    case 'V':
        return interpret_v_token(buf) ? 0 : 1;
    case 'X':
    case 'Y':
        interpret_xy_token(player_ptr, buf);
        return 0;
    case 'Z':
        return interpret_z_token(buf) ? 0 : 1;
    case 'T':
        return interpret_t_token(buf) ? 0 : 1;
    default:
        return 1;
    }
}
