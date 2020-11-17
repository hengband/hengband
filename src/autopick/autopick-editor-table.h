#pragma once

/*!
 * todo GAME_TEXT[] かconcptrを使いたかった
 * しかしコンパイルエラーで通らなかったので暫定的にdefineにした
 * 上手いことコンパイルできる方法があれば求む
 * @brief コマンド説明
 */
#define MN_QUIT _("セーブ無しで終了", "Quit without save")
#define MN_SAVEQUIT _("セーブして終了", "Save & Quit")
#define MN_REVERT _("全ての変更を破棄", "Revert all changes")
#define MN_HELP _("ヘルプ", "Help")

#define MN_MOVE _("カーソル移動", "Move cursor")
#define MN_LEFT _("左          (←矢印キー)", "Left     (Left Arrow key)")
#define MN_DOWN _("下          (↓矢印キー)", "Down     (Down Arrow key)")
#define MN_UP _("上          (↑矢印キー)", "Up       (Up Arrow key)")
#define MN_RIGHT _("右          (→矢印キー)", "Right    (Right Arrow key)")
#define MN_BOL _("行の先頭", "Beggining of line")
#define MN_EOL _("行の終端", "End of line")
#define MN_PGUP _("上のページ  (PageUpキー)", "Page up  (PageUp key)")
#define MN_PGDOWN _("下のページ  (PageDownキー)", "Page down(PageDown key)")
#define MN_TOP _("1行目へ移動 (Homeキー)", "Top      (Home key)")
#define MN_BOTTOM _("最下行へ移動(Endキー)", "Bottom   (End key)")

#define MN_EDIT _("編集", "Edit")
#define MN_CUT _("カット", "Cut")
#define MN_COPY _("コピー", "Copy")
#define MN_PASTE _("ペースト", "Paste")
#define MN_BLOCK _("選択範囲の指定", "Select block")
#define MN_KILL_LINE _("行の残りを削除", "Kill rest of line")
#define MN_DELETE_CHAR _("1文字削除", "Delete character")
#define MN_BACKSPACE _("バックスペース", "Backspace")
#define MN_RETURN _("改行", "Return")

#define MN_SEARCH _("検索", "Search")
#define MN_SEARCH_STR _("文字列で検索", "Search by string")
#define MN_SEARCH_FORW _("前方へ再検索", "Search foward")
#define MN_SEARCH_BACK _("後方へ再検索", "Search backward")
#define MN_SEARCH_OBJ _("アイテムを選択して検索", "Search by inventory object")
#define MN_SEARCH_DESTROYED _("自動破壊されたアイテムで検索", "Search by destroyed object")

#define MN_INSERT _("色々挿入", "Insert...")
#define MN_INSERT_OBJECT _("選択したアイテムの名前を挿入", "Insert name of choosen object")
#define MN_INSERT_DESTROYED _("自動破壊されたアイテムの名前を挿入", "Insert name of destroyed object")
#define MN_INSERT_BLOCK _("条件分岐ブロックの例を挿入", "Insert conditional block")
#define MN_INSERT_MACRO _("マクロ定義を挿入", "Insert a macro definition")
#define MN_INSERT_KEYMAP _("キーマップ定義を挿入", "Insert a keymap definition")

#define MN_COMMAND_LETTER _("拾い/破壊/放置の選択", "Command letter")
#define MN_CL_AUTOPICK _("「 」 (自動拾い)", "' ' (Auto pick)")
#define MN_CL_DESTROY _("「!」 (自動破壊)", "'!' (Auto destroy)")
#define MN_CL_LEAVE _("「~」 (放置)", "'~' (Leave it on the floor)")
#define MN_CL_QUERY _("「;」 (確認して拾う)", "';' (Query to pick up)")
#define MN_CL_NO_DISP _("「(」 (マップコマンドで表示しない)", "'(' (No display on the large map)")

#define MN_ADJECTIVE_GEN _("形容詞(一般)の選択", "Adjective (general)")
#define MN_RARE _("レアな (装備)", "rare (equipment)")
#define MN_COMMON _("ありふれた (装備)", "common (equipment)")

#define MN_ADJECTIVE_SPECIAL _("形容詞(特殊)の選択", "Adjective (special)")
#define MN_BOOSTED _("ダイス目の違う (武器)", "dice boosted (weapons)")
#define MN_MORE_DICE _("ダイス目 # 以上の (武器)", "more than # dice (weapons)")
#define MN_MORE_BONUS _("修正値 # 以上の (指輪等)", "more bonus than # (rings etc.)")
#define MN_WANTED _("賞金首の (死体)", "wanted (corpse)")
#define MN_UNIQUE _("ユニーク・モンスターの (死体)", "unique (corpse)")
#define MN_HUMAN _("人間の (死体)", "human (corpse)")
#define MN_UNREADABLE _("読めない (魔法書)", "unreadable (spellbooks)")
#define MN_REALM1 _("第一領域の (魔法書)", "realm1 (spellbooks)")
#define MN_REALM2 _("第二領域の (魔法書)", "realm2 (spellbooks)")
#define MN_FIRST _("1冊目の (魔法書)", "first (spellbooks)")
#define MN_SECOND _("2冊目の (魔法書)", "second (spellbooks)")
#define MN_THIRD _("3冊目の (魔法書)", "third (spellbooks)")
#define MN_FOURTH _("4冊目の (魔法書)", "fourth (spellbooks)")

#define MN_NOUN _("名詞の選択", "Keywords (noun)")
