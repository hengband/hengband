/*!
 * @file learnt-power-getter.cpp
 * @brief 青魔法の処理実行定義
 */

#include "blue-magic/learnt-power-getter.h"
#include "blue-magic/blue-magic-checker.h"
#include "blue-magic/learnt-info.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/text-display-options.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "mind/mind-blue-mage.h"
#include "monster-race/race-ability-flags.h"
#include "mspell/monster-power-table.h"
#include "player-base/player-class.h"
#include "player-info/bluemage-data-type.h"
#include "player/player-status-table.h"
#include "realm/realm-types.h"
#include "spell/spell-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "util/enum-converter.h"
#include "util/flag-group.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <algorithm>
#include <iterator>
#include <optional>
#include <vector>

/*!
 * @brief コマンド反復チェック
 * @return 反復可能な青魔法があればそれを返す。なければ std::nullopt を返す。
 */
static std::optional<MonsterAbilityType> check_blue_magic_repeat()
{
    COMMAND_CODE code;
    if (!repeat_pull(&code)) {
        return std::nullopt;
    }

    if (auto spell = static_cast<MonsterAbilityType>(code);
        monster_powers.find(spell) != monster_powers.end()) {
        return spell;
    }

    return std::nullopt;
}

/*!
 * @brief 青魔法のタイプをコマンドメニューにより選択する
 *
 * @return 選択した青魔法のタイプ
 * 選択をキャンセルした場合は std::nullopt
 */
static std::optional<BlueMagicType> select_blue_magic_type_by_menu()
{
    auto menu_line = 1;
    std::optional<BlueMagicType> type;

    screen_save();

    while (!type) {
        prt(format(_(" %s ボルト", " %s bolt"), (menu_line == 1) ? _("》", "> ") : "  "), 2, 14);
        prt(format(_(" %s ボール", " %s ball"), (menu_line == 2) ? _("》", "> ") : "  "), 3, 14);
        prt(format(_(" %s ブレス", " %s breath"), (menu_line == 3) ? _("》", "> ") : "  "), 4, 14);
        prt(format(_(" %s 召喚", " %s sommoning"), (menu_line == 4) ? _("》", "> ") : "  "), 5, 14);
        prt(format(_(" %s その他", " %s others"), (menu_line == 5) ? _("》", "> ") : "  "), 6, 14);
        prt(_("どの種類の魔法を使いますか？", "use which type of magic? "), 0, 0);

        auto choice = inkey();
        switch (choice) {
        case ESCAPE:
        case 'z':
        case 'Z':
            screen_load();
            return std::nullopt;
            break;
        case '2':
        case 'j':
        case 'J':
            menu_line++;
            break;
        case '8':
        case 'k':
        case 'K':
            menu_line += 4;
            break;
        case '\r':
        case 'x':
        case 'X':
            type = i2enum<BlueMagicType>(menu_line);
            break;
        }

        if (menu_line > 5) {
            menu_line -= 5;
        }
    }

    screen_load();

    return type;
}

/*!
 * @brief 青魔法のタイプを記号により選択する
 *
 * @return 選択した青魔法のタイプ
 * 選択をキャンセルした場合は std::nullopt
 */
static std::optional<BlueMagicType> select_blue_magic_kind_by_symbol()
{
    constexpr auto candidate_desc = _("[A]ボルト, [B]ボール, [C]ブレス, [D]召喚, [E]その他:",
        "[A] bolt, [B] ball, [C] breath, [D] summoning, [E] others:");
    while (true) {
        const auto command = input_command(candidate_desc, true);
        if (!command) {
            return std::nullopt;
        }

        switch (*command) {
        case 'A':
        case 'a':
            return BlueMagicType::BOLT;
        case 'B':
        case 'b':
            return BlueMagicType::BALL;
        case 'C':
        case 'c':
            return BlueMagicType::BREATH;
        case 'D':
        case 'd':
            return BlueMagicType::SUMMON;
        case 'E':
        case 'e':
            return BlueMagicType::OTHER;
        default:
            break;
        }
    }

    return std::nullopt;
}

/*!
 * @brief 指定したタイプの青魔法のリストを(覚えていないものも含め)返す
 *
 * @param bluemage_data 青魔道士の固有データへの参照
 * @param type 青魔法のタイプ
 * @return 指定したタイプの青魔法のリストを(覚えていないものも含め)返す
 * 但し、そのタイプの魔法を1つも覚えていない場合は std::nullopt を返す
 */
static std::optional<std::vector<MonsterAbilityType>> sweep_learnt_spells(const bluemage_data_type &bluemage_data, BlueMagicType type)
{
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;
    set_rf_masks(ability_flags, type);

    if (bluemage_data.learnt_blue_magics.has_none_of(ability_flags)) {
        msg_print(_("その種類の魔法は覚えていない！", "You don't know any spell of this type."));
        return std::nullopt;
    }

    std::vector<MonsterAbilityType> blue_magics;
    EnumClassFlagGroup<MonsterAbilityType>::get_flags(ability_flags, std::back_inserter(blue_magics));

    return blue_magics;
}

/*!
 * @brief 入力されたキーに従いコマンドメニューで選択中の青魔法を切り替える
 *
 * @param key 入力されたキー
 * @param menu_line 選択中の青魔法の行
 * @param bluemage_data 青魔道士の固有データへの参照
 * @param blue_magics 青魔法のリスト(覚えていないものも含まれているが、カーソル移動時に選択をスキップする)
 * @return 選択確定キーが入力された場合は true、そうでなければ false
 */
static bool switch_blue_magic_choice(const char key, int &menu_line, const bluemage_data_type &bluemage_data, const std::vector<MonsterAbilityType> blue_magics)
{
    const auto &learnt_blue_magics = bluemage_data.learnt_blue_magics;
    const int blue_magics_count = blue_magics.size();

    switch (key) {
    case '8':
    case 'k':
    case 'K':
        do {
            menu_line += (blue_magics_count - 1);
            if (menu_line > blue_magics_count) {
                menu_line -= blue_magics_count;
            }
        } while (learnt_blue_magics.has_not(blue_magics[menu_line - 1]));
        return false;

    case '2':
    case 'j':
    case 'J':
        do {
            menu_line++;
            if (menu_line > blue_magics_count) {
                menu_line -= blue_magics_count;
            }
        } while (learnt_blue_magics.has_not(blue_magics[menu_line - 1]));
        return false;

    case '6':
    case 'l':
    case 'L':
        menu_line = blue_magics_count;
        while (learnt_blue_magics.has_not(blue_magics[menu_line - 1])) {
            menu_line--;
        }

        return false;

    case '4':
    case 'h':
    case 'H':
        menu_line = 1;
        while (learnt_blue_magics.has_not(blue_magics[menu_line - 1])) {
            menu_line++;
        }

        return false;

    case 'x':
    case 'X':
    case '\r':
        return true;

    default:
        return false;
    }
}

/*!
 * @brief 青魔法の失敗率を計算する
 *
 * @param mp 失敗率を計算する青魔法に対応した monster_power 構造体への参照
 * @param need_mana 青魔法を使うのに必要なMP
 * @return int 失敗率(%)を返す
 */
int calculate_blue_magic_failure_probability(PlayerType *player_ptr, const monster_power &mp, int need_mana)
{
    auto chance = mp.fail;
    if (player_ptr->lev > mp.level) {
        chance -= 3 * (player_ptr->lev - mp.level);
    } else {
        chance += (mp.level - player_ptr->lev);
    }

    chance -= 3 * (adj_mag_stat[player_ptr->stat_index[A_INT]] - 1);
    chance = mod_spell_chance_1(player_ptr, chance);
    if (need_mana > player_ptr->csp) {
        chance += 5 * (need_mana - player_ptr->csp);
    }

    PERCENTAGE minfail = adj_mag_fail[player_ptr->stat_index[A_INT]];
    if (chance < minfail) {
        chance = minfail;
    }

    auto player_stun = player_ptr->effects()->stun();
    chance += player_stun->get_magic_chance_penalty();
    if (chance > 95) {
        chance = 95;
    }

    chance = mod_spell_chance_2(player_ptr, chance);

    return chance;
}

/*!
 * @brief 青魔法リストに表示する青魔法の先頭のヘッダを出力する
 * 記号で選択する場合、" 記号)" が出力される
 * コマンドメニューで選択する場合、選択中の青魔法にはカーソルが、そうでない青魔法には空白が出力される
 *
 * @param buf 出力するバッファ
 * @param buf_size バッファのサイズ
 * @param index 選択する青魔法のリスト上の番号
 * @param menu_line コマンドメニューで選択する場合、選択中の行。記号で選択する場合は使用されない。
 */
static void close_blue_magic_name(char *buf, size_t buf_size, int index, int menu_line)
{
    if (!use_menu) {
        snprintf(buf, buf_size, "  %c)", I2A(index));
        return;
    }

    if (index == menu_line - 1) {
        snprintf(buf, buf_size, _("  》", "  > "));
    } else {
        snprintf(buf, buf_size, "    ");
    }
}

/*!
 * @brief 使用できる青魔法のリストを表示する
 *
 * @param menu_line 選択中の青魔法の行
 * @param bluemage_data 青魔道士の固有データへの参照
 * @param blue_magics 青魔法のリスト(覚えていないものも含まれているが、覚えていないものは表示をスキップする)
 */
static void describe_blue_magic_name(PlayerType *player_ptr, int menu_line, const bluemage_data_type &bluemage_data, const std::vector<MonsterAbilityType> &blue_magics)
{
    constexpr TERM_LEN y_base = 1;
    constexpr TERM_LEN x_base = 18;
    prt("", y_base, x_base);
    put_str(_("名前", "Name"), y_base, x_base + 5);
    put_str(_("MP 失率 効果", "SP Fail Info"), y_base, x_base + 33);
    for (auto i = 0U; i < blue_magics.size(); ++i) {
        prt("", y_base + i + 1, x_base);
        const auto &spell = blue_magics[i];
        if (bluemage_data.learnt_blue_magics.has_not(spell)) {
            continue;
        }

        const auto &mp = monster_powers.at(spell);
        auto need_mana = mod_need_mana(player_ptr, mp.smana, 0, REALM_NONE);
        auto chance = calculate_blue_magic_failure_probability(player_ptr, mp, need_mana);
        char header[80];
        close_blue_magic_name(header, sizeof(header), i, menu_line);
        const auto info = learnt_info(player_ptr, spell);
        const auto psi_desc = format("%s %-26s %3d %3d%%%s", header, mp.name, need_mana, chance, info.data());
        prt(psi_desc, y_base + i + 1, x_base);
    }

    prt("", y_base + blue_magics.size() + 1, x_base);
}

/*!
 * @brief 青魔法を唱えるか確認する
 *
 * @param spell 唱える青魔法
 * @return 唱えるなら ture、キャンセルするなら false を返す
 */
static bool confirm_cast_blue_magic(MonsterAbilityType spell)
{
    char tmp_val[160];
    (void)strnfmt(tmp_val, 78, _("%sの魔法を唱えますか？", "Use %s? "), monster_powers.at(spell).name);
    return input_check(tmp_val);
}

/*!
 * @brief 唱える青魔法を記号により選択する
 *
 * @param bluemage_data 青魔道士の固有データへの参照
 * @param blue_magics 青魔法のリスト(覚えていないものも含まれているが、覚えていない物は候補に出ず選択できない)
 * @return 選択した青魔法。選択をキャンセルした場合は std::nullopt
 */
static std::optional<MonsterAbilityType> select_learnt_spells_by_symbol(PlayerType *player_ptr, const bluemage_data_type &bluemage_data, std::vector<MonsterAbilityType> spells)
{
    constexpr auto fmt = _("(%c-%c, '*'で一覧, ESC) どの%sを唱えますか？", "(%c-%c, *=List, ESC=exit) Use which %s? ");
    const auto prompt = format(fmt, I2A(0), I2A(spells.size() - 1), _("魔法", "magic"));

    bool first_show_list = always_show_list;
    auto show_list = false;
    std::optional<MonsterAbilityType> selected_spell;

    while (!selected_spell) {
        auto choice = '\0';
        if (!first_show_list) {
            const auto choice_opt = input_command(prompt, true);
            if (!choice_opt) {
                break;
            }

            choice = *choice_opt;
        }

        if (first_show_list || (choice == ' ') || (choice == '*') || (choice == '?')) {
            // 選択する青魔法一覧の表示/非表示切り替え
            first_show_list = false;
            show_list = !show_list;
            if (show_list) {
                screen_save();
                describe_blue_magic_name(player_ptr, 0, bluemage_data, spells);
            } else {
                screen_load();
            }

            continue;
        }

        auto confirm = isupper(choice) != 0;
        uint index = A2I(tolower(choice));
        if (spells.size() <= index || bluemage_data.learnt_blue_magics.has_not(spells[index])) {
            bell();
            continue;
        }

        if (confirm && !confirm_cast_blue_magic(spells[index])) {
            continue;
        }

        selected_spell = spells[index];
    }

    if (show_list) {
        screen_load();
    }

    return selected_spell;
}

/*!
 * @brief 唱える青魔法をコマンドメニューにより選択する
 *
 * @param bluemage_data 青魔道士の固有データへの参照
 * @param blue_magics 青魔法のリスト(覚えていないものも含まれているが、覚えていない物は候補に出ず選択できない)
 * @return 選択した青魔法。選択をキャンセルした場合は std::nullopt
 */
static std::optional<MonsterAbilityType> select_learnt_spells_by_menu(PlayerType *player_ptr, const bluemage_data_type &bluemage_data, std::vector<MonsterAbilityType> spells)
{
    constexpr auto prompt = _("(ESC=中断) どの魔法を唱えますか？", "(ESC=exit) Use which magic? ");

    auto it = std::find_if(
        spells.begin(), spells.end(), [&bluemage_data](const auto &spell) { return bluemage_data.learnt_blue_magics.has(spell); });
    int menu_line = std::distance(spells.begin(), it) + 1;
    std::optional<MonsterAbilityType> selected_spell;

    screen_save();

    while (!selected_spell) {
        describe_blue_magic_name(player_ptr, menu_line, bluemage_data, spells);

        const auto choice = input_command(prompt, true);
        if (!choice) {
            break;
        }

        if (choice == '0') {
            break;
        }

        if (!switch_blue_magic_choice(*choice, menu_line, bluemage_data, spells)) {
            continue;
        }

        uint index = menu_line - 1;
        if (spells.size() <= index || bluemage_data.learnt_blue_magics.has_not(spells[index])) {
            bell();
            continue;
        }

        selected_spell = spells[index];
    }

    screen_load();

    return selected_spell;
}

/*!
 * @brief 使用可能な青魔法を選択する /
 * Allow user to choose a imitation.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sn 選択したモンスター攻撃ID、キャンセルの場合-1、不正な選択の場合-2を返す
 * @return 発動可能な魔法を選択した場合TRUE、キャンセル処理か不正な選択が行われた場合FALSEを返す。
 * @details
 * If a valid spell is chosen, saves it in '*sn' and returns TRUE\n
 * If the user hits escape, returns FALSE, and set '*sn' to -1\n
 * If there are no legal choices, returns FALSE, and sets '*sn' to -2\n
 *\n
 * The "prompt" should be "cast", "recite", or "study"\n
 * The "known" should be TRUE for cast/pray, FALSE for study\n
 *\n
 * nb: This function has a (trivial) display bug which will be obvious\n
 * when you run it. It's probably easy to fix but I haven't tried,\n
 * sorry.\n
 */
std::optional<MonsterAbilityType> get_learned_power(PlayerType *player_ptr)
{
    auto bluemage_data = PlayerClass(player_ptr).get_specific_data<bluemage_data_type>();
    if (!bluemage_data) {
        return std::nullopt;
    }

    if (auto repeat_spell = check_blue_magic_repeat();
        repeat_spell) {
        return repeat_spell;
    }

    auto type = (use_menu)
                    ? select_blue_magic_type_by_menu()
                    : select_blue_magic_kind_by_symbol();
    if (!type) {
        return std::nullopt;
    }

    auto spells = sweep_learnt_spells(*bluemage_data, *type);
    if (!spells || spells->empty()) {
        return std::nullopt;
    }

    auto selected_spell = (use_menu)
                              ? select_learnt_spells_by_menu(player_ptr, *bluemage_data, *spells)
                              : select_learnt_spells_by_symbol(player_ptr, *bluemage_data, *spells);

    RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::SPELL);
    handle_stuff(player_ptr);

    if (!selected_spell) {
        return std::nullopt;
    }

    repeat_push(static_cast<COMMAND_CODE>(*selected_spell));
    return selected_spell;
}
