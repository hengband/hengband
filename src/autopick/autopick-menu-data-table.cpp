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
#include <map>

namespace {
const std::map<EditorCommandId, std::string_view> menu_labels = {
    { EditorCommandId::QUIT, _("セーブ無しで終了", "Quit without save") },
    { EditorCommandId::SAVEQUIT, _("セーブして終了", "Save & Quit") },
    { EditorCommandId::REVERT, _("全ての変更を破棄", "Revert all changes") },
    { EditorCommandId::HELP, _("ヘルプ", "Help") },
    { EditorCommandId::LEFT, _("左       (←矢印キー)", "Left              (Left Arrow key)") },
    { EditorCommandId::DOWN, _("下       (↓矢印キー)", "Down              (Down Arrow key)") },
    { EditorCommandId::UP, _("上       (↑矢印キー)", "Up                (Up Arrow key)") },
    { EditorCommandId::RIGHT, _("右       (→矢印キー)", "Right             (Right Arrow key)") },
    { EditorCommandId::BOL, _("行頭     (Homeキー)", "Beginning of line (Home key)") },
    { EditorCommandId::EOL, _("行末     (Endキー)", "End of line       (End key)") },
    { EditorCommandId::PGUP, _("上ページ (PageUpキー)", "Page up           (PageUp key)") },
    { EditorCommandId::PGDOWN, _("下ページ (PageDownキー)", "Page down         (PageDown key)") },
    { EditorCommandId::TOP, _("先頭行   (Ctrl+Homeキー)", "Top               (Ctrl+Home key)") },
    { EditorCommandId::BOTTOM, _("最終行   (Ctrl+Endキー)", "Bottom            (Ctrl+End key)") },
    { EditorCommandId::CUT, _("カット", "Cut") },
    { EditorCommandId::COPY, _("コピー", "Copy") },
    { EditorCommandId::PASTE, _("ペースト", "Paste") },
    { EditorCommandId::BLOCK, _("選択範囲の指定", "Select block") },
    { EditorCommandId::KILL_LINE, _("行の残りを削除", "Kill rest of line") },
    { EditorCommandId::DELETE_CHAR, _("1文字削除", "Delete character") },
    { EditorCommandId::BACKSPACE, _("バックスペース", "Backspace") },
    { EditorCommandId::RETURN, _("改行", "Return") },
    { EditorCommandId::SEARCH_STR, _("文字列で検索", "Search by string") },
    { EditorCommandId::SEARCH_FORW, _("前方へ再検索", "Search foward") },
    { EditorCommandId::SEARCH_BACK, _("後方へ再検索", "Search backward") },
    { EditorCommandId::SEARCH_OBJ, _("アイテムを選択して検索", "Search by inventory object") },
    { EditorCommandId::SEARCH_DESTROYED, _("自動破壊されたアイテムで検索", "Search by destroyed object") },
    { EditorCommandId::INSERT_OBJECT, _("選択したアイテムの名前を挿入", "Insert name of choosen object") },
    { EditorCommandId::INSERT_DESTROYED, _("自動破壊されたアイテムの名前を挿入", "Insert name of destroyed object") },
    { EditorCommandId::INSERT_BLOCK, _("条件分岐ブロックの例を挿入", "Insert conditional block") },
    { EditorCommandId::INSERT_MACRO, _("マクロ定義を挿入", "Insert a macro definition") },
    { EditorCommandId::INSERT_KEYMAP, _("キーマップ定義を挿入", "Insert a keymap definition") },
    { EditorCommandId::CL_AUTOPICK, _("「 」 (自動拾い)", "' ' (Auto pick)") },
    { EditorCommandId::CL_DESTROY, _("「!」 (自動破壊)", "'!' (Auto destroy)") },
    { EditorCommandId::CL_LEAVE, _("「~」 (放置)", "'~' (Leave it on the floor)") },
    { EditorCommandId::CL_QUERY, _("「;」 (確認して拾う)", "';' (Query to pick up)") },
    { EditorCommandId::CL_NO_DISP, _("「(」 (マップコマンドで表示しない)", "'(' (No display on the large map)") },
    { EditorCommandId::OK_RARE, _("レアな (装備)", "rare (equipment)") },
    { EditorCommandId::OK_COMMON, _("ありふれた (装備)", "common (equipment)") },
    { EditorCommandId::OK_BOOSTED, _("ダイス目の違う (武器)", "dice boosted (weapons)") },
    { EditorCommandId::OK_MORE_DICE, _("ダイス目 # 以上の (武器)", "more than # dice (weapons)") },
    { EditorCommandId::OK_MORE_BONUS, _("修正値 # 以上の (指輪等)", "more bonus than # (rings etc.)") },
    { EditorCommandId::OK_WANTED, _("賞金首の (死体)", "wanted (corpse)") },
    { EditorCommandId::OK_UNIQUE, _("ユニーク・モンスターの (死体)", "unique (corpse)") },
    { EditorCommandId::OK_HUMAN, _("人間の (死体)", "human (corpse)") },
    { EditorCommandId::OK_UNREADABLE, _("読めない (魔法書)", "unreadable (spellbooks)") },
    { EditorCommandId::OK_REALM1, _("第一領域の (魔法書)", "realm1 (spellbooks)") },
    { EditorCommandId::OK_REALM2, _("第二領域の (魔法書)", "realm2 (spellbooks)") },
    { EditorCommandId::OK_FIRST, _("1冊目の (魔法書)", "first (spellbooks)") },
    { EditorCommandId::OK_SECOND, _("2冊目の (魔法書)", "second (spellbooks)") },
    { EditorCommandId::OK_THIRD, _("3冊目の (魔法書)", "third (spellbooks)") },
    { EditorCommandId::OK_FOURTH, _("4冊目の (魔法書)", "fourth (spellbooks)") },
};
}

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
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::HELP), 0, tl::nullopt, EditorCommandId::HELP);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::QUIT), 0, KTRL('q'), EditorCommandId::QUIT);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::SAVEQUIT), 0, KTRL('w'), EditorCommandId::SAVEQUIT);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::REVERT), 0, KTRL('z'), EditorCommandId::REVERT);

    this->menu_data.emplace_back(_("編集", "Edit"), 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::CUT), 1, KTRL('x'), EditorCommandId::CUT);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::COPY), 1, KTRL('c'), EditorCommandId::COPY);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::PASTE), 1, KTRL('v'), EditorCommandId::PASTE);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::BLOCK), 1, KTRL('g'), EditorCommandId::BLOCK);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::KILL_LINE), 1, KTRL('k'), EditorCommandId::KILL_LINE);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::DELETE_CHAR), 1, KTRL('d'), EditorCommandId::DELETE_CHAR);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::BACKSPACE), 1, KTRL('h'), EditorCommandId::BACKSPACE);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::RETURN), 1, KTRL('j'), EditorCommandId::RETURN);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::RETURN), 1, KTRL('m'), EditorCommandId::RETURN);

    this->menu_data.emplace_back(_("検索", "Search"), 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::SEARCH_STR), 1, KTRL('s'), EditorCommandId::SEARCH_STR);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::SEARCH_FORW), 1, tl::nullopt, EditorCommandId::SEARCH_FORW);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::SEARCH_BACK), 1, KTRL('r'), EditorCommandId::SEARCH_BACK);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::SEARCH_OBJ), 1, KTRL('y'), EditorCommandId::SEARCH_OBJ);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::SEARCH_DESTROYED), 1, tl::nullopt, EditorCommandId::SEARCH_DESTROYED);

    this->menu_data.emplace_back(_("カーソル移動", "Move cursor"), 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::LEFT), 1, KTRL('b'), EditorCommandId::LEFT);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::DOWN), 1, KTRL('n'), EditorCommandId::DOWN);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::UP), 1, KTRL('p'), EditorCommandId::UP);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::RIGHT), 1, KTRL('f'), EditorCommandId::RIGHT);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::BOL), 1, KTRL('a'), EditorCommandId::BOL);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::EOL), 1, KTRL('e'), EditorCommandId::EOL);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::PGUP), 1, KTRL('o'), EditorCommandId::PGUP);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::PGDOWN), 1, KTRL('l'), EditorCommandId::PGDOWN);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::TOP), 1, KTRL('t'), EditorCommandId::TOP);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::BOTTOM), 1, KTRL('u'), EditorCommandId::BOTTOM);

    this->menu_data.emplace_back(_("色々挿入", "Insert..."), 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::INSERT_OBJECT), 1, KTRL('i'), EditorCommandId::INSERT_OBJECT);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::INSERT_DESTROYED), 1, tl::nullopt, EditorCommandId::INSERT_DESTROYED);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::INSERT_BLOCK), 1, tl::nullopt, EditorCommandId::INSERT_BLOCK);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::INSERT_MACRO), 1, tl::nullopt, EditorCommandId::INSERT_MACRO);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::INSERT_KEYMAP), 1, tl::nullopt, EditorCommandId::INSERT_KEYMAP);

    this->menu_data.emplace_back(_("形容詞(一般)の選択", "Adjective (general)"), 0, tl::nullopt, tl::nullopt);
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
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::OK_RARE), 1, tl::nullopt, EditorCommandId::OK_RARE);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::OK_COMMON), 1, tl::nullopt, EditorCommandId::OK_COMMON);

    this->menu_data.emplace_back(_("形容詞(特殊)の選択", "Adjective (special)"), 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::OK_BOOSTED), 1, tl::nullopt, EditorCommandId::OK_BOOSTED);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::OK_MORE_DICE), 1, tl::nullopt, EditorCommandId::OK_MORE_DICE);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::OK_MORE_BONUS), 1, tl::nullopt, EditorCommandId::OK_MORE_BONUS);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::OK_WANTED), 1, tl::nullopt, EditorCommandId::OK_WANTED);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::OK_UNIQUE), 1, tl::nullopt, EditorCommandId::OK_UNIQUE);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::OK_HUMAN), 1, tl::nullopt, EditorCommandId::OK_HUMAN);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::OK_UNREADABLE), 1, tl::nullopt, EditorCommandId::OK_UNREADABLE);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::OK_REALM1), 1, tl::nullopt, EditorCommandId::OK_REALM1);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::OK_REALM2), 1, tl::nullopt, EditorCommandId::OK_REALM2);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::OK_FIRST), 1, tl::nullopt, EditorCommandId::OK_FIRST);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::OK_SECOND), 1, tl::nullopt, EditorCommandId::OK_SECOND);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::OK_THIRD), 1, tl::nullopt, EditorCommandId::OK_THIRD);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::OK_FOURTH), 1, tl::nullopt, EditorCommandId::OK_FOURTH);

    this->menu_data.emplace_back(_("名詞の選択", "Keywords (noun)"), 0, tl::nullopt, tl::nullopt);
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

    this->menu_data.emplace_back(_("拾い/破壊/放置の選択", "Command letter"), 0, tl::nullopt, tl::nullopt);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::CL_AUTOPICK), 1, tl::nullopt, EditorCommandId::CL_AUTOPICK);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::CL_DESTROY), 1, tl::nullopt, EditorCommandId::CL_DESTROY);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::CL_LEAVE), 1, tl::nullopt, EditorCommandId::CL_LEAVE);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::CL_QUERY), 1, tl::nullopt, EditorCommandId::CL_QUERY);
    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::CL_NO_DISP), 1, tl::nullopt, EditorCommandId::CL_NO_DISP);

    this->menu_data.emplace_back(menu_labels.at(EditorCommandId::DELETE_CHAR), tl::nullopt, DELETE, EditorCommandId::DELETE_CHAR);
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
