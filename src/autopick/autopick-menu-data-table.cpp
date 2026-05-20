/*!
 * @brief 自動拾いのユーティリティ
 * @date 2020/04/25
 * @author Hourier
 */

#include "autopick/autopick-menu-data-table.h"
#include "autopick/autopick-commands-table.h"
#include "autopick/autopick-keys-table.h"
#include "locale/language-switcher.h"
#include "util/int-char-converter.h"

/*
 * These MN_* preprocessor macros were formerly defined in
 * autopick-editor-table.h.  Fold them in here to satisfy two requirements:
 * 1) The platforms using configure/automake only preprocess .c files (.h
 * files are not touched) with nkf to modify the encoding of Japanese
 * characters.
 * 2) The file scope initializer for menu_data requires constant expressions
 * that can be evaluated at compile time.
 */
constexpr auto MN_QUIT = _("セーブ無しで終了", "Quit without save");
constexpr auto MN_SAVEQUIT = _("セーブして終了", "Save & Quit");
constexpr auto MN_REVERT = _("全ての変更を破棄", "Revert all changes");
constexpr auto MN_HELP = _("ヘルプ", "Help");

constexpr auto MN_MOVE = _("カーソル移動", "Move cursor");
constexpr auto MN_LEFT = _("左       (←矢印キー)", "Left              (Left Arrow key)");
constexpr auto MN_DOWN = _("下       (↓矢印キー)", "Down              (Down Arrow key)");
constexpr auto MN_UP = _("上       (↑矢印キー)", "Up                (Up Arrow key)");
constexpr auto MN_RIGHT = _("右       (→矢印キー)", "Right             (Right Arrow key)");
constexpr auto MN_BOL = _("行頭     (Homeキー)", "Beginning of line (Home key)");
constexpr auto MN_EOL = _("行末     (Endキー)", "End of line       (End key)");
constexpr auto MN_PGUP = _("上ページ (PageUpキー)", "Page up           (PageUp key)");
constexpr auto MN_PGDOWN = _("下ページ (PageDownキー)", "Page down         (PageDown key)");
constexpr auto MN_TOP = _("先頭行   (Ctrl+Homeキー)", "Top               (Ctrl+Home key)");
constexpr auto MN_BOTTOM = _("最終行   (Ctrl+Endキー)", "Bottom            (Ctrl+End key)");

constexpr auto MN_EDIT = _("編集", "Edit");
constexpr auto MN_CUT = _("カット", "Cut");
constexpr auto MN_COPY = _("コピー", "Copy");
constexpr auto MN_PASTE = _("ペースト", "Paste");
constexpr auto MN_BLOCK = _("選択範囲の指定", "Select block");
constexpr auto MN_KILL_LINE = _("行の残りを削除", "Kill rest of line");
constexpr auto MN_DELETE_CHAR = _("1文字削除", "Delete character");
constexpr auto MN_BACKSPACE = _("バックスペース", "Backspace");
constexpr auto MN_RETURN = _("改行", "Return");

constexpr auto MN_SEARCH = _("検索", "Search");
constexpr auto MN_SEARCH_STR = _("文字列で検索", "Search by string");
constexpr auto MN_SEARCH_FORW = _("前方へ再検索", "Search foward");
constexpr auto MN_SEARCH_BACK = _("後方へ再検索", "Search backward");
constexpr auto MN_SEARCH_OBJ = _("アイテムを選択して検索", "Search by inventory object");
constexpr auto MN_SEARCH_DESTROYED = _("自動破壊されたアイテムで検索", "Search by destroyed object");

constexpr auto MN_INSERT = _("色々挿入", "Insert...");
constexpr auto MN_INSERT_OBJECT = _("選択したアイテムの名前を挿入", "Insert name of choosen object");
constexpr auto MN_INSERT_DESTROYED = _("自動破壊されたアイテムの名前を挿入", "Insert name of destroyed object");
constexpr auto MN_INSERT_BLOCK = _("条件分岐ブロックの例を挿入", "Insert conditional block");
constexpr auto MN_INSERT_MACRO = _("マクロ定義を挿入", "Insert a macro definition");
constexpr auto MN_INSERT_KEYMAP = _("キーマップ定義を挿入", "Insert a keymap definition");

constexpr auto MN_COMMAND_LETTER = _("拾い/破壊/放置の選択", "Command letter");
constexpr auto MN_CL_AUTOPICK = _("「 」 (自動拾い)", "' ' (Auto pick)");
constexpr auto MN_CL_DESTROY = _("「!」 (自動破壊)", "'!' (Auto destroy)");
constexpr auto MN_CL_LEAVE = _("「~」 (放置)", "'~' (Leave it on the floor)");
constexpr auto MN_CL_QUERY = _("「;」 (確認して拾う)", "';' (Query to pick up)");
constexpr auto MN_CL_NO_DISP = _("「(」 (マップコマンドで表示しない)", "'(' (No display on the large map)");

constexpr auto MN_ADJECTIVE_GEN = _("形容詞(一般)の選択", "Adjective (general)");
constexpr auto MN_RARE = _("レアな (装備)", "rare (equipment)");
constexpr auto MN_COMMON = _("ありふれた (装備)", "common (equipment)");

constexpr auto MN_ADJECTIVE_SPECIAL = _("形容詞(特殊)の選択", "Adjective (special)");
constexpr auto MN_BOOSTED = _("ダイス目の違う (武器)", "dice boosted (weapons)");
constexpr auto MN_MORE_DICE = _("ダイス目 # 以上の (武器)", "more than # dice (weapons)");
constexpr auto MN_MORE_BONUS = _("修正値 # 以上の (指輪等)", "more bonus than # (rings etc.)");
constexpr auto MN_WANTED = _("賞金首の (死体)", "wanted (corpse)");
constexpr auto MN_UNIQUE = _("ユニーク・モンスターの (死体)", "unique (corpse)");
constexpr auto MN_HUMAN = _("人間の (死体)", "human (corpse)");
constexpr auto MN_UNREADABLE = _("読めない (魔法書)", "unreadable (spellbooks)");
constexpr auto MN_REALM1 = _("第一領域の (魔法書)", "realm1 (spellbooks)");
constexpr auto MN_REALM2 = _("第二領域の (魔法書)", "realm2 (spellbooks)");
constexpr auto MN_FIRST = _("1冊目の (魔法書)", "first (spellbooks)");
constexpr auto MN_SECOND = _("2冊目の (魔法書)", "second (spellbooks)");
constexpr auto MN_THIRD = _("3冊目の (魔法書)", "third (spellbooks)");
constexpr auto MN_FOURTH = _("4冊目の (魔法書)", "fourth (spellbooks)");

constexpr auto MN_NOUN = _("名詞の選択", "Keywords (noun)");

constexpr char DELETE = 0x7F;

CommandMenuData CommandMenuData::instance{};

CommandMenuData &CommandMenuData::get_instance()
{
    return instance;
}

void CommandMenuData::initialize()
{
    if (!this->menu_data.empty()) {
        return;
    }

    this->menu_data.reserve(100);
    this->menu_data.emplace_back(MN_HELP, 0, tl::nullopt, EditorCommandId::HELP);
    this->menu_data.emplace_back(MN_QUIT, 0, KTRL('q'), EditorCommandId::QUIT);
    this->menu_data.emplace_back(MN_SAVEQUIT, 0, KTRL('w'), EditorCommandId::SAVEQUIT);
    this->menu_data.emplace_back(MN_REVERT, 0, KTRL('z'), EditorCommandId::REVERT);

    this->menu_data.emplace_back(MN_EDIT, 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(MN_CUT, 1, KTRL('x'), EditorCommandId::CUT);
    this->menu_data.emplace_back(MN_COPY, 1, KTRL('c'), EditorCommandId::COPY);
    this->menu_data.emplace_back(MN_PASTE, 1, KTRL('v'), EditorCommandId::PASTE);
    this->menu_data.emplace_back(MN_BLOCK, 1, KTRL('g'), EditorCommandId::BLOCK);
    this->menu_data.emplace_back(MN_KILL_LINE, 1, KTRL('k'), EditorCommandId::KILL_LINE);
    this->menu_data.emplace_back(MN_DELETE_CHAR, 1, KTRL('d'), EditorCommandId::DELETE_CHAR);
    this->menu_data.emplace_back(MN_BACKSPACE, 1, KTRL('h'), EditorCommandId::BACKSPACE);
    this->menu_data.emplace_back(MN_RETURN, 1, KTRL('j'), EditorCommandId::RETURN);
    this->menu_data.emplace_back(MN_RETURN, 1, KTRL('m'), EditorCommandId::RETURN);

    this->menu_data.emplace_back(MN_SEARCH, 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(MN_SEARCH_STR, 1, KTRL('s'), EditorCommandId::SEARCH_STR);
    this->menu_data.emplace_back(MN_SEARCH_FORW, 1, tl::nullopt, EditorCommandId::SEARCH_FORW);
    this->menu_data.emplace_back(MN_SEARCH_BACK, 1, KTRL('r'), EditorCommandId::SEARCH_BACK);
    this->menu_data.emplace_back(MN_SEARCH_OBJ, 1, KTRL('y'), EditorCommandId::SEARCH_OBJ);
    this->menu_data.emplace_back(MN_SEARCH_DESTROYED, 1, tl::nullopt, EditorCommandId::SEARCH_DESTROYED);

    this->menu_data.emplace_back(MN_MOVE, 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(MN_LEFT, 1, KTRL('b'), EditorCommandId::LEFT);
    this->menu_data.emplace_back(MN_DOWN, 1, KTRL('n'), EditorCommandId::DOWN);
    this->menu_data.emplace_back(MN_UP, 1, KTRL('p'), EditorCommandId::UP);
    this->menu_data.emplace_back(MN_RIGHT, 1, KTRL('f'), EditorCommandId::RIGHT);
    this->menu_data.emplace_back(MN_BOL, 1, KTRL('a'), EditorCommandId::BOL);
    this->menu_data.emplace_back(MN_EOL, 1, KTRL('e'), EditorCommandId::EOL);
    this->menu_data.emplace_back(MN_PGUP, 1, KTRL('o'), EditorCommandId::PGUP);
    this->menu_data.emplace_back(MN_PGDOWN, 1, KTRL('l'), EditorCommandId::PGDOWN);
    this->menu_data.emplace_back(MN_TOP, 1, KTRL('t'), EditorCommandId::TOP);
    this->menu_data.emplace_back(MN_BOTTOM, 1, KTRL('u'), EditorCommandId::BOTTOM);

    this->menu_data.emplace_back(MN_INSERT, 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(MN_INSERT_OBJECT, 1, KTRL('i'), EditorCommandId::INSERT_OBJECT);
    this->menu_data.emplace_back(MN_INSERT_DESTROYED, 1, tl::nullopt, EditorCommandId::INSERT_DESTROYED);
    this->menu_data.emplace_back(MN_INSERT_BLOCK, 1, tl::nullopt, EditorCommandId::INSERT_BLOCK);
    this->menu_data.emplace_back(MN_INSERT_MACRO, 1, tl::nullopt, EditorCommandId::INSERT_MACRO);
    this->menu_data.emplace_back(MN_INSERT_KEYMAP, 1, tl::nullopt, EditorCommandId::INSERT_KEYMAP);

    this->menu_data.emplace_back(MN_ADJECTIVE_GEN, 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::UNAWARE), 1, tl::nullopt, EditorCommandId::IK_UNAWARE);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::UNIDENTIFIED), 1, tl::nullopt, EditorCommandId::IK_UNIDENTIFIED);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::IDENTIFIED), 1, tl::nullopt, EditorCommandId::IK_IDENTIFIED);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::STAR_IDENTIFIED), 1, tl::nullopt, EditorCommandId::IK_STAR_IDENTIFIED);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::COLLECTING), 1, tl::nullopt, EditorCommandId::OK_COLLECTING);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::ARTIFACT), 1, tl::nullopt, EditorCommandId::OK_ARTIFACT);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::EGO), 1, tl::nullopt, EditorCommandId::OK_EGO);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::GOOD), 1, tl::nullopt, EditorCommandId::OK_GOOD);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::NAMELESS), 1, tl::nullopt, EditorCommandId::OK_NAMELESS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::AVERAGE), 1, tl::nullopt, EditorCommandId::OK_AVERAGE);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::WORTHLESS), 1, tl::nullopt, EditorCommandId::OK_WORTHLESS);
    this->menu_data.emplace_back(MN_RARE, 1, tl::nullopt, EditorCommandId::OK_RARE);
    this->menu_data.emplace_back(MN_COMMON, 1, tl::nullopt, EditorCommandId::OK_COMMON);

    this->menu_data.emplace_back(MN_ADJECTIVE_SPECIAL, 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(MN_BOOSTED, 1, tl::nullopt, EditorCommandId::OK_BOOSTED);
    this->menu_data.emplace_back(MN_MORE_DICE, 1, tl::nullopt, EditorCommandId::OK_MORE_DICE);
    this->menu_data.emplace_back(MN_MORE_BONUS, 1, tl::nullopt, EditorCommandId::OK_MORE_BONUS);
    this->menu_data.emplace_back(MN_WANTED, 1, tl::nullopt, EditorCommandId::OK_WANTED);
    this->menu_data.emplace_back(MN_UNIQUE, 1, tl::nullopt, EditorCommandId::OK_UNIQUE);
    this->menu_data.emplace_back(MN_HUMAN, 1, tl::nullopt, EditorCommandId::OK_HUMAN);
    this->menu_data.emplace_back(MN_UNREADABLE, 1, tl::nullopt, EditorCommandId::OK_UNREADABLE);
    this->menu_data.emplace_back(MN_REALM1, 1, tl::nullopt, EditorCommandId::OK_REALM1);
    this->menu_data.emplace_back(MN_REALM2, 1, tl::nullopt, EditorCommandId::OK_REALM2);
    this->menu_data.emplace_back(MN_FIRST, 1, tl::nullopt, EditorCommandId::OK_FIRST);
    this->menu_data.emplace_back(MN_SECOND, 1, tl::nullopt, EditorCommandId::OK_SECOND);
    this->menu_data.emplace_back(MN_THIRD, 1, tl::nullopt, EditorCommandId::OK_THIRD);
    this->menu_data.emplace_back(MN_FOURTH, 1, tl::nullopt, EditorCommandId::OK_FOURTH);

    this->menu_data.emplace_back(MN_NOUN, 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::WEAPONS), 1, tl::nullopt, EditorCommandId::KK_WEAPONS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::FAVORITE_WEAPONS), 1, tl::nullopt, EditorCommandId::KK_FAVORITE_WEAPONS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::ARMORS), 1, tl::nullopt, EditorCommandId::KK_ARMORS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::MISSILES), 1, tl::nullopt, EditorCommandId::KK_MISSILES);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::DEVICES), 1, tl::nullopt, EditorCommandId::KK_DEVICES);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::LIGHTS), 1, tl::nullopt, EditorCommandId::KK_LIGHTS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::JUNKS), 1, tl::nullopt, EditorCommandId::KK_JUNKS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::CORPSES), 1, tl::nullopt, EditorCommandId::KK_CORPSES);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::SPELLBOOKS), 1, tl::nullopt, EditorCommandId::KK_SPELLBOOKS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::SHIELDS), 1, tl::nullopt, EditorCommandId::KK_SHIELDS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::BOWS), 1, tl::nullopt, EditorCommandId::KK_BOWS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::RINGS), 1, tl::nullopt, EditorCommandId::KK_RINGS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::AMULETS), 1, tl::nullopt, EditorCommandId::KK_AMULETS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::SUITS), 1, tl::nullopt, EditorCommandId::KK_SUITS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::CLOAKS), 1, tl::nullopt, EditorCommandId::KK_CLOAKS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::HELMS), 1, tl::nullopt, EditorCommandId::KK_HELMS);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::GLOVES), 1, tl::nullopt, EditorCommandId::KK_GLOVES);
    this->menu_data.emplace_back(autopick_keys.at(AutopickKey::BOOTS), 1, tl::nullopt, EditorCommandId::KK_BOOTS);

    this->menu_data.emplace_back(MN_COMMAND_LETTER, 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(MN_CL_AUTOPICK, 1, tl::nullopt, EditorCommandId::CL_AUTOPICK);
    this->menu_data.emplace_back(MN_CL_DESTROY, 1, tl::nullopt, EditorCommandId::CL_DESTROY);
    this->menu_data.emplace_back(MN_CL_LEAVE, 1, tl::nullopt, EditorCommandId::CL_LEAVE);
    this->menu_data.emplace_back(MN_CL_QUERY, 1, tl::nullopt, EditorCommandId::CL_QUERY);
    this->menu_data.emplace_back(MN_CL_NO_DISP, 1, tl::nullopt, EditorCommandId::CL_NO_DISP);

    this->menu_data.emplace_back(MN_DELETE_CHAR, tl::nullopt, DELETE, EditorCommandId::DELETE_CHAR);
}

const CommandMenuDatum &CommandMenuData::get_datum(size_t num) const
{
    return this->menu_data.at(num);
}

/*!
 * @brief Find a command by 'key'.
 */
EditorCommandId CommandMenuData::get_com_id(char key) const
{
    for (const auto &menu_datum : this->menu_data) {
        if (menu_datum.key == key) {
            return menu_datum.com_id.value_or(EditorCommandId::NONE);
        }
    }

    return EditorCommandId::NONE;
}
