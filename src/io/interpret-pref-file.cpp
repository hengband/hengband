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
#include "locale/language-switcher.h"
#include "system/baseitem/baseitem-config.h"
#include "system/baseitem/baseitem-configs.h"
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
#include <algorithm>
#include <array>
#include <iterator>
#include <stdexcept>

tl::optional<std::string> histpref_buf;

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
static bool interpret_r_token(std::string_view buf)
{
    const auto tokens = tokenize(buf.substr(2), 3);
    if (tokens.size() != 3) {
        return false;
    }

    const auto i = std::stoi(tokens[0], nullptr, 0);
    const auto n1 = static_cast<uint8_t>(std::stoi(tokens[1], nullptr, 0));
    const auto n2 = static_cast<char>(std::stoi(tokens[2], nullptr, 0));
    auto &monraces = MonraceList::get_instance();
    if ((i < 0) || (i >= std::ssize(monraces))) {
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
static bool interpret_k_token(std::string_view buf)
{
    const auto tokens = tokenize(buf.substr(2), 3);
    if (tokens.size() != 3) {
        return false;
    }

    const auto i = std::stoi(tokens[0], nullptr, 0);
    const auto color = static_cast<uint8_t>(std::stoi(tokens[1], nullptr, 0));
    const auto character = static_cast<char>(std::stoi(tokens[2], nullptr, 0));
    auto &baseitem_configs = BaseitemConfigs::get_instance();

    // short へ変換する前に確かめる (範囲外の ID は get_config() が例外を送出し、short への切り詰めで範囲内に見える値もある)
    if ((i < 0) || (i >= static_cast<int>(baseitem_configs.size()))) {
        return false;
    }

    /* Allow TERM_DARK text */
    auto &baseitem_config = baseitem_configs.get_config(static_cast<short>(i));
    if ((color > 0) || (((character & 0x80) == 0) && (character != '\0'))) {
        baseitem_config.update_color(color);
    }

    if (character != '\0') {
        baseitem_config.update_character(character);
    }

    return true;
}

/*!
 * @brief トークン数によって地形の文字形と色を決定する
 * @param i 地形種別
 * @param num トークン数
 * @param tokens トークン内容
 * @details
 * 途中のトークンの変換に失敗したときに一部の設定だけが書き換わらないよう、
 * 先に全てのトークンを変換してから書き込む。変換に失敗した場合は std::stoi() の例外がそのまま伝わる。
 */
static void decide_feature_type(int i, int num, const std::vector<std::string> &tokens)
{
    const auto num_symbols = (num == F_LIT_MAX * 2 + 1) ? F_LIT_MAX : 1;
    std::array<DisplaySymbol, F_LIT_MAX> symbols{};
    for (auto j = 0; j < num_symbols; j++) {
        const auto color = static_cast<uint8_t>(std::stoi(tokens[j * 2 + 1], nullptr, 0));
        const auto character = static_cast<char>(std::stoi(tokens[j * 2 + 2], nullptr, 0));
        symbols[j] = DisplaySymbol(color, character);
    }

    auto &terrain = TerrainList::get_instance().get_terrain(static_cast<short>(i));
    const auto update_symbol = [&terrain](int lighting, const DisplaySymbol &symbol) {
        auto &symbol_config = terrain.symbol_configs[lighting];
        const auto has_character = symbol.character != '\0';

        /* Allow TERM_DARK text */
        if ((symbol.color != 0) || (!(symbol.character & 0x80) && has_character)) {
            symbol_config.color = symbol.color;
        }

        if (has_character) {
            symbol_config.character = symbol.character;
        }
    };

    update_symbol(F_LIT_STANDARD, symbols[0]);
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
            update_symbol(j, symbols[j]);
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
static bool interpret_f_token(std::string_view buf)
{
    const auto tokens = tokenize(buf.substr(2), F_LIT_MAX * 2 + 1);
    const auto num = tokens.size();
    if ((num != 3) && (num != 4) && (num != F_LIT_MAX * 2 + 1)) {
        return false;
    }

    if ((num == 4) && tokens[3] != "LIT") {
        return false;
    }

    const auto i = std::stoi(tokens[0], nullptr, 0);
    if ((i < 0) || (i >= static_cast<int>(TerrainList::get_instance().size()))) {
        return false;
    }

    decide_feature_type(i, num, tokens);
    return true;
}

/*!
 * @brief Fトークンの解釈 / Process "S:<num>:<a>/<c>" -- attr/char for special things
 * @param buf バッファ
 * @return 解釈に成功したか否か
 */
static bool interpret_s_token(std::string_view buf)
{
    const auto tokens = tokenize(buf.substr(2), 3);
    if (tokens.size() != 3) {
        return false;
    }

    const auto num = std::stoi(tokens[0], nullptr, 0);
    const auto color = static_cast<uint8_t>(std::stoi(tokens[1], nullptr, 0));
    const auto character = static_cast<char>(std::stoi(tokens[2], nullptr, 0));
    if ((num < 0) || (num >= std::ssize(ds_bolt))) {
        return false;
    }

    ds_bolt[num] = DisplaySymbol(color, character);
    return true;
}

/*!
 * @brief Uトークンの解釈 / Process "U:<tv>:<a>/<c>" -- attr/char for unaware items
 * @param buf バッファ
 * @return 解釈に成功したか否か
 */
static bool interpret_u_token(std::string_view buf)
{
    const auto tokens = tokenize(buf.substr(2), 3);
    if (tokens.size() != 3) {
        return false;
    }

    const auto tval = i2enum<ItemKindType>(std::stoi(tokens[0], nullptr, 0));
    const auto color = static_cast<uint8_t>(std::stoi(tokens[1], nullptr, 0));
    const auto character = static_cast<char>(std::stoi(tokens[2], nullptr, 0));
    for (auto &baseitem : BaseitemList::get_instance()) {
        if (baseitem.is_valid() && (baseitem.bi_key.tval() == tval)) {
            if (color) {
                baseitem.init_color(color);
            }

            if (character) {
                baseitem.init_character(character);
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
static bool interpret_e_token(std::string_view buf)
{
    const auto tokens = tokenize(buf.substr(2), 2);
    if (tokens.size() != 2) {
        return false;
    }

    // 128 以上の番号は従来どおり 128 で割った余りを使う。負の番号は余りも負になり配列の範囲外を指すので弾く
    const auto tval = std::stoi(tokens[0], nullptr, 0);
    const auto color = static_cast<uint8_t>(std::stoi(tokens[1], nullptr, 0));
    if (tval < 0) {
        return false;
    }

    const auto num = tval % std::ssize(tval_to_attr);
    if (color > 0) {
        tval_to_attr[num] = color;
    }
    return true;
}

/*!
 * @brief Pトークンの解釈 / Process "P:<str>" -- normal macro
 * @param buf バッファ
 * @return エラーコード
 */
static int interpret_p_token(std::string_view buf)
{
    char tmp[1024];
    text_to_ascii(tmp, buf.substr(2), sizeof(tmp));
    return macro_add(tmp, macro_buffers.data());
}

/*!
 * @brief Cトークンの解釈 / Process "C:<str>" -- create keymap
 * @param buf バッファ
 * @return 解釈に成功したか否か
 */
static bool interpret_c_token(std::string_view buf)
{
    const auto tokens = tokenize(buf.substr(2), 2);
    if (tokens.size() != 2) {
        return false;
    }

    const auto mode = i2enum<KeymapMode>(std::stoi(tokens[0], nullptr, 0));
    if ((mode < KeymapMode::ORIGINAL) || (mode > KeymapMode::ROGUE)) {
        return false;
    }

    char tmp[1024];
    text_to_ascii(tmp, tokens[1], sizeof(tmp));
    if (!tmp[0] || tmp[1]) {
        return false;
    }

    const auto i = static_cast<uint8_t>(tmp[0]);
    keymap_actions_map.at(mode).at(i) = macro_buffers.data();
    return true;
}

/*!
 * @brief Vトークンの解釈 / Process "V:<num>:<kv>:<rv>:<gv>:<bv>" -- visual info
 * @param buf バッファ
 * @return 解釈に成功したか否か
 */
static bool interpret_v_token(std::string_view buf)
{
    const auto tokens = tokenize(buf.substr(2), 5);
    if (tokens.size() != 5) {
        return false;
    }

    const auto num = std::stoi(tokens[0], nullptr, 0);
    if ((num < 0) || (num >= std::ssize(angband_color_table))) {
        return false;
    }

    // 途中のトークンの変換に失敗したときに一部の値だけが書き換わらないよう、先に全て変換する
    std::array<uint8_t, std::size(angband_color_table[0])> values{};
    for (size_t j = 0; j < values.size(); j++) {
        values[j] = static_cast<uint8_t>(std::stoi(tokens[j + 1], nullptr, 0));
    }

    std::copy(values.begin(), values.end(), std::begin(angband_color_table[num]));
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
static void interpret_xy_token(PlayerType *player_ptr, std::string_view buf)
{
    const auto &world = AngbandWorld::get_instance();
    for (auto &option : option_info) {
        if (option.text != buf.substr(2)) {
            continue;
        }

        int os = option.flag_position;
        int ob = option.offset;
        if ((player_ptr->playing || world.character_xtra) && (GameOptionPage::BIRTH == option.page) && !world.wizard) {
            msg_print(_("初期オプションは変更できません! '{}'", "Birth options can not be changed! '{}'"), buf);
            msg_erase();
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

    msg_print(_("オプションの名前が正しくありません： {}", "Ignored invalid option: {}"), buf);
    msg_erase();
}

/*!
 * @brief Zトークンの解釈 / Process "Z:<type>:<str>" -- set spell color
 * @param line トークン1行
 * @param tokens トークン保管文字列
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
 * @param tokens トークン保管文字列
 * @return 解釈に成功したか否か
 */
static bool decide_template_modifier(size_t num_tokens, const std::vector<std::string> &tokens)
{
    // 修飾キーの数とトークン数が合わない行は、既存の定義を消さずに失敗とする
    // (空のテンプレートは全ての定義を消す指定なので、トークン数を問わない)
    const auto zz_length = std::min(MAX_MACRO_MOD, tokens[1].length());
    if (!tokens[0].empty() && (2 + zz_length != num_tokens)) {
        return false;
    }

    if (macro_template) {
        // 修飾キーの文字列は全体を保持するが、名前は MAX_MACRO_MOD 個までしか持たない
        const size_t macro_modifier_length = macro_modifier_chr ? std::min(macro_modifier_chr->length(), macro_modifier_names.size()) : 0;
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

    if (tokens[0].empty()) {
        return true;
    }

    macro_template = tokens[0];
    macro_modifier_chr = tokens[1];
    for (size_t i = 0; i < zz_length; i++) {
        macro_modifier_names[i] = tokens[2 + i];
    }

    return true;
}

/*!
 * @brief Tトークンの解釈 / Process "T:<trigger>:<keycode>:<shift-keycode>" for 2 or 3 tokens
 * @param tok トークン数
 * @param tokens トークン保管文字列
 * @return 解釈に成功したか否か
 */
static bool interpret_macro_keycodes(int tok, const std::vector<std::string> &tokens)
{
    if (max_macrotrigger >= MAX_MACRO_TRIG) {
        msg_print(_("マクロトリガーの設定が多すぎます!", "Too many macro triggers!"));
        return false;
    }

    auto m = max_macrotrigger;
    max_macrotrigger++;
    std::string t;
    t.reserve(tokens[0].size());
    std::string_view s = tokens[0];
    while (!s.empty()) {
        if (s.starts_with('\\')) {
            s.remove_prefix(1);
            if (s.empty()) {
                break;
            }
        }

        t.push_back(s[0]);
        s.remove_prefix(1);
    }

    macro_trigger_names[m] = std::move(t);
    macro_trigger_keycodes.at(ShiftStatus::OFF).at(m) = tokens[1];
    if (tok == 3) {
        macro_trigger_keycodes.at(ShiftStatus::ON).at(m) = tokens[2];
        return true;
    }

    macro_trigger_keycodes.at(ShiftStatus::ON).at(m) = tokens[1];
    return true;
}

/*!
 * @brief Tトークンの個数調査 (解釈はサブルーチンで) / Initialize macro trigger names and a template
 * @param buf バッファ
 * @return 解釈に成功したか否か
 * @todo 2.2.1r時点のコードからトークン数0～1の場合もエラーコード0だが、1であるべきでは？
 */
static bool interpret_t_token(std::string_view buf)
{
    const auto tokens = tokenize(buf.substr(2), 2 + MAX_MACRO_MOD);
    const auto size = tokens.size();
    if (size >= 4) {
        return decide_template_modifier(size, tokens);
    }

    if (size < 2) {
        return true;
    }

    return interpret_macro_keycodes(size, tokens);
}

/*!
 * @brief 設定ファイルの1行を、先頭の文字に応じて解釈する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param buf データテキスト (2文字目が「:」であること)
 * @return 解釈に成功したら0、失敗したら0以外
 * @details 数値の変換に失敗した場合などは例外が伝わる。呼び出し元の interpret_pref_file() で捕捉する。
 */
static int interpret_pref_line(PlayerType *player_ptr, std::string_view buf)
{
    switch (buf[0]) {
    case 'H':
        /* Process "H:<history>" */
        add_history_from_pref_line(buf.substr(2));
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
        text_to_ascii(macro_buffers.data(), buf.substr(2), macro_buffers.size());
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
 *
 * 数値として解釈できないトークンや存在しない ID を指定した行は、例外を送出せず解釈の失敗として扱う。
 * ゲーム内の「"」コマンドでユーザーが入力した行もそのまま渡されるためである。
 */
int interpret_pref_file(PlayerType *player_ptr, std::string_view buf)
{
    if ((buf.length() < 2) || (buf[1] != ':')) {
        return 1;
    }

    // std::stoi() の変換失敗 (invalid_argument / out_of_range) と、
    // 存在しない ID を .at() で参照したとき (out_of_range) は解釈の失敗として扱う
    try {
        return interpret_pref_line(player_ptr, buf);
    } catch (const std::invalid_argument &) {
        return 1;
    } catch (const std::out_of_range &) {
        return 1;
    }
}
