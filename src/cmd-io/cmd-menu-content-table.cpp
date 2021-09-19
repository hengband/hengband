#include "cmd-io/cmd-menu-content-table.h"
#include "player-info/class-types.h"
#include "util/int-char-converter.h"

special_menu_content special_menu_info[MAX_SPECIAL_MENU_NUM] = {
    { _("超能力/特殊能力", "MindCraft/Special"), 0, 0, MENU_CLASS, CLASS_MINDCRAFTER },
    { _("ものまね/特殊能力", "Imitation/Special"), 0, 0, MENU_CLASS, CLASS_IMITATOR },
    { _("歌/特殊能力", "Song/Special"), 0, 0, MENU_CLASS, CLASS_BARD },
    { _("必殺技/特殊能力", "Technique/Special"), 0, 0, MENU_CLASS, CLASS_SAMURAI },
    { _("練気術/魔法/特殊能力", "Mind/Magic/Special"), 0, 0, MENU_CLASS, CLASS_FORCETRAINER },
    { _("技/特殊能力", "BrutalPower/Special"), 0, 0, MENU_CLASS, CLASS_BERSERKER },
    { _("技術/特殊能力", "Technique/Special"), 0, 0, MENU_CLASS, CLASS_SMITH },
    { _("鏡魔法/特殊能力", "MirrorMagic/Special"), 0, 0, MENU_CLASS, CLASS_MIRROR_MASTER },
    { _("忍術/特殊能力", "Ninjutsu/Special"), 0, 0, MENU_CLASS, CLASS_NINJA },
    { _("広域マップ(<)", "Enter global map(<)"), 2, 6, MENU_WILD, false },
    { _("通常マップ(>)", "Enter local map(>)"), 2, 7, MENU_WILD, true },
    { "", 0, 0, 0, 0 },
};

menu_content menu_info[MAX_COMMAND_MENU_NUM][MAX_COMMAND_PER_SCREEN] = {
    {
        { _("魔法/特殊能力", "Magic/Special"), 1, false },
        { _("行動", "Action"), 2, false },
        { _("道具(使用)", "Items(use)"), 3, false },
        { _("道具(その他)", "Items(other)"), 4, false },
        { _("装備", "Equip"), 5, false },
        { _("扉/箱", "Door/Box"), 6, false },
        { _("情報", "Information"), 7, false },
        { _("設定", "Options"), 8, false },
        { _("その他", "Other commands"), 9, false },
        { "", 0, false },
    },

    { { _("使う(m)", "Use(m)"), 'm', true }, { _("調べる(b/P)", "See tips(b/P)"), 'b', true }, { _("覚える(G)", "Study(G)"), 'G', true },
        { _("特殊能力を使う(U/O)", "Special abilities(U/O)"), 'U', true }, { "", 0, false }, { "", 0, false }, { "", 0, false }, { "", 0, false },
        { "", 0, false }, { "", 0, false } },

    { { _("休息する(R)", "Rest(R)"), 'R', true }, { _("トラップ解除(D)", "Disarm a trap(D)"), 'D', true }, { _("探す(s)", "Search(s)"), 's', true },
        { _("周りを調べる(l/x)", "Look(l/x)"), 'l', true }, { _("ターゲット指定(*)", "Target(*)"), '*', true }, { _("穴を掘る(T/^t)", "Dig(T/^t)"), 'T', true },
        { _("階段を上る(<)", "Go up stairs(<)"), '<', true }, { _("階段を下りる(>)", "Go down stairs(>)"), '>', true },
        { _("ペットに命令する(p)", "Command pets(p)"), 'p', true }, { _("探索モードのON/OFF(S/#)", "Search mode ON/OFF(S/#)"), 'S', true } },

    { { _("読む(r)", "Read a scroll(r)"), 'r', true }, { _("飲む(q)", "Drink a potion(q)"), 'q', true }, { _("杖を使う(u/Z)", "Use a staff(u/Z)"), 'u', true },
        { _("魔法棒で狙う(a/z)", "Aim a wand(a/z)"), 'a', true }, { _("ロッドを振る(z/a)", "Zap a rod(z/a)"), 'z', true },
        { _("始動する(A)", "Activate equipped item(A)"), 'A', true }, { _("食べる(E)", "Eat(E)"), 'E', true },
        { _("飛び道具で撃つ(f/t)", "Fire missile weapon(f/t)"), 'f', true }, { _("投げる(v)", "Throw an item(v)"), 'v', true }, { "", 0, false } },

    { { _("拾う(g)", "Get items(g)"), 'g', true }, { _("落とす(d)", "Drop an item(d)"), 'd', true }, { _("壊す(k/^d)", "Destroy an item(k/^d)"), 'k', true },
        { _("銘を刻む({)", "Inscribe an item({)"), '{', true }, { _("銘を消す(})", "Uninscribe an item(})"), '}', true },
        { _("調査(I)", "Uninscribe an item(})"), 'I', true }, { _("アイテム一覧(i)", "Inventory list(i)"), 'i', true }, { "", 0, false }, { "", 0, false },
        { "", 0, false } },

    { { _("装備する(w)", "Wear(w)"), 'w', true }, { _("装備を外す(t/T)", "Take off(t/T)"), 't', true }, { _("燃料を補給(F)", "Refuel(F)"), 'F', true },
        { _("装備一覧(e)", "Equipment list(e)"), 'e', true }, { "", 0, false }, { "", 0, false }, { "", 0, false }, { "", 0, false }, { "", 0, false },
        { "", 0, false } },

    { { _("開ける(o)", "Open(o)"), 'o', true }, { _("閉じる(c)", "Close(c)"), 'c', true }, { _("体当たりする(B/f)", "Bash a door(B/f)"), 'B', true },
        { _("くさびを打つ(j/S)", "Jam a door(j/S)"), 'j', true }, { "", 0, false }, { "", 0, false }, { "", 0, false }, { "", 0, false }, { "", 0, false },
        { "", 0, false } },

    { { _("ダンジョンの全体図(M)", "Full map(M)"), 'M', true }, { _("位置を確認(L/W)", "Map(L/W)"), 'L', true },
        { _("階の雰囲気(^f)", "Level feeling(^f)"), KTRL('F'), true }, { _("ステータス(C)", "Character status(C)"), 'C', true },
        { _("文字の説明(/)", "Identify symbol(/)"), '/', true }, { _("メッセージ履歴(^p)", "Show prev messages(^p)"), KTRL('P'), true },
        { _("現在の時刻(^t/')", "Current time(^t/')"), KTRL('T'), true }, { _("現在の知識(~)", "Various information(~)"), '~', true },
        { _("プレイ記録(|)", "Play record menu(|)"), '|', true }, { "", 0, false } },

    { { _("オプション(=)", "Set options(=)"), '=', true }, { _("マクロ(@)", "Interact with macros(@)"), '@', true },
        { _("画面表示(%)", "Interact w/ visuals(%)"), '%', true }, { _("カラー(&)", "Interact with colors(&)"), '&', true },
        { _("設定変更コマンド(\")", "Enter a user pref(\")"), '\"', true }, { _("自動拾いをロード($)", "Reload auto-pick pref($)"), '$', true },
        { _("システム(!)", "System(!)"), '!', true }, { "", 0, false }, { "", 0, false }, { "", 0, false } },

    { { _("セーブ&中断(^x)", "Save and quit(^x)"), KTRL('X'), true }, { _("セーブ(^s)", "Save(^s)"), KTRL('S'), true },
        { _("ヘルプ(?)", "Help(out-of-date)(?)"), '?', true }, { _("再描画(^r)", "Redraw(^r)"), KTRL('R'), true }, { _("メモ(:)", "Take note(:)"), ':', true },
        { _("記念撮影())", "Dump screen dump(()"), ')', true }, { _("記念撮影の表示(()", "Load screen dump())"), '(', true },
        { _("バージョン情報(V)", "Version info(V)"), 'V', true }, { _("引退する(Q)", "Quit(Q)"), 'Q', true }, { "", 0, false } },
};
