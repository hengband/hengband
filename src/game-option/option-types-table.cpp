#include "game-option/option-types-table.h"
#include "game-option/auto-destruction-options.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/disturbance-options.h"
#include "game-option/game-play-options.h"
#include "game-option/input-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "system/game-option-types.h"

/*!
 * @brief オプションテーブル /
 * Available Options
 */
const std::vector<option_type> option_info = {
    /*** Input Options ***/
    { &rogue_like_commands, false, OPT_PAGE_INPUT, 0, 0, "rogue_like_commands", _("ローグ風キー配置を使用する", "Rogue-like commands") },

    { &always_pickup, false, OPT_PAGE_INPUT, 0, 5, "always_pickup", _("常にアイテムを拾う", "Pick things up by default") },

    { &carry_query_flag, false, OPT_PAGE_INPUT, 0, 3, "carry_query_flag", _("アイテムを拾う前に確認する", "Prompt before picking things up") },

    { &quick_messages, true, OPT_PAGE_INPUT, 0, 1, "quick_messages", _("クイック・メッセージを使用する", "Activate quick messages") },

    { &auto_more, false, OPT_PAGE_INPUT, 2, 6, "auto_more", _("キー待ちしないで連続でメッセージを表示する", "Automatically clear '-more-' prompts") },

    { &skip_more, false, OPT_PAGE_INPUT, 2, 18, "skip_more", _("キー待ちしないで全てのメッセージを読み飛ばす", "Automatically skip all messages.") },

    { &command_menu, true, OPT_PAGE_INPUT, 2, 7, "command_menu", _("メニューによりコマンド選択を有効にする", "Enable command selection menu") },

    { &other_query_flag, false, OPT_PAGE_INPUT, 0, 2, "other_query_flag", _("床上のアイテムを使用するときに確認する", "Prompt for floor item selection") },

    { &use_old_target, false, OPT_PAGE_INPUT, 0, 4, "use_old_target", _("常に以前のターゲットを指定する", "Use old target by default") },

    { &always_repeat, true, OPT_PAGE_INPUT, 0, 6, "always_repeat", _("コマンド自動繰り返し", "Repeat obvious commands") },

    { &confirm_destroy, false, OPT_PAGE_INPUT, 5, 3, "confirm_destroy",
        _("「無価値」なアイテムを破壊する時確認する", "Prompt for destruction of known worthless items") },

    { &confirm_wear, true, OPT_PAGE_INPUT, 5, 4, "confirm_wear", _("呪われた物を装備する時確認する", "Confirm to wear/wield known cursed items") },

    { &confirm_quest, true, OPT_PAGE_INPUT, 1, 9, "confirm_quest", _("クエストを諦めて階段で逃げる前に確認する", "Prompt before exiting a quest level") },

    { &target_pet, false, OPT_PAGE_INPUT, 2, 5, "target_pet", _("ペットをターゲットにする", "Allow targeting pets") },

    { &easy_open, true, OPT_PAGE_INPUT, 5, 7, "easy_open", _("自動的にドアを開ける", "Automatically open doors") },

    { &easy_disarm, true, OPT_PAGE_INPUT, 5, 8, "easy_disarm", _("自動的に罠を解除する", "Automatically disarm traps") },

    { &easy_floor, true, OPT_PAGE_INPUT, 5, 9, "easy_floor", _("床上で重なったアイテムをリストする", "Display floor stacks in a list") },

    { &use_command, false, OPT_PAGE_INPUT, 5, 10, "use_command", _("「使う(a)」コマンドでアイテムを何でも使える", "Allow unified use command") },

    { &over_exert, false, OPT_PAGE_INPUT, 0, 29, "over_exert", _("MPが足りなくても魔法に挑戦する", "Allow casting spells when short of mana") },

    { &numpad_as_cursorkey, true, OPT_PAGE_INPUT, 2, 31, "numpad_as_cursorkey",
        _("エディタ内でテンキーをカーソルキーとして使う", "Use numpad keys as cursor keys in editor mode") },

    /*** Map Screen Options ***/
    { &center_player, false, OPT_PAGE_MAPSCREEN, 5, 11, "center_player", _("常にプレイヤーを中心に置く(*遅い*)", "Center map while walking (*slow*)") },

    { &center_running, true, OPT_PAGE_MAPSCREEN, 5, 12, "center_running", _("走っている時でも中心に置く", "Centering even while running") },

    { &view_yellow_lite, true, OPT_PAGE_MAPSCREEN, 1, 28, "view_yellow_lite", _("明かりの範囲を特別な色で表示する", "Use special colors for torch-lit grids") },

    { &view_bright_lite, true, OPT_PAGE_MAPSCREEN, 1, 29, "view_bright_lite", _("視界の範囲を特別な色で表示する", "Use special colors for 'viewable' grids") },

    { &view_granite_lite, true, OPT_PAGE_MAPSCREEN, 1, 30, "view_granite_lite", _("壁を特別な色で表示する(重い)", "Use special colors for wall grids (slow)") },

    { &view_special_lite, true, OPT_PAGE_MAPSCREEN, 1, 31, "view_special_lite",
        _("床を特別な色で表示する(重い)", "Use special colors for floor grids (slow)") },

    { &view_perma_grids, true, OPT_PAGE_MAPSCREEN, 1, 6, "view_perma_grids", _("明るい場所はそのままにする", "Map remembers all perma-lit grids") },

    { &view_torch_grids, false, OPT_PAGE_MAPSCREEN, 1, 7, "view_torch_grids", _("明かりで照らした場所はそのままにする", "Map remembers all torch-lit grids") },

    { &view_unsafe_grids, true, OPT_PAGE_MAPSCREEN, 1, 8, "view_unsafe_grids", _("トラップ感知済みでない場所を表示する", "Map marked by detect traps") },

    { &view_hidden_walls, true, OPT_PAGE_MAPSCREEN, 1, 2, "view_hidden_walls", _("壁の中に囲まれた壁を表示する", "Map walls hidden in other walls") },

    { &view_unsafe_walls, false, OPT_PAGE_MAPSCREEN, 1, 1, "view_unsafe_walls", _("トラップ未感知の壁の中に囲まれた壁を表示する", "Map hidden walls not marked by detect traps") },

    { &view_reduce_view, false, OPT_PAGE_MAPSCREEN, 1, 17, "view_reduce_view", _("街では視野を狭くする", "Reduce view-radius in town") },

    { &fresh_before, true, OPT_PAGE_MAPSCREEN, 1, 23, "fresh_before", _("連続コマンド中に画面を再描画し続ける", "Flush output while in repeated command") },

    { &fresh_after, false, OPT_PAGE_MAPSCREEN, 1, 24, "fresh_after", _("コマンド後に画面を常に再描画し続ける", "Flush output after monster's move") },

    { &fresh_once, false, OPT_PAGE_MAPSCREEN, 1, 10, "fresh_once", _("キー入力毎に一度だけ画面を再描画する", "Flush output once per key input") },

    { &fresh_message, false, OPT_PAGE_MAPSCREEN, 1, 25, "fresh_message", _("メッセージの後に画面を再描画する", "Flush output after every message") },

    { &hilite_player, false, OPT_PAGE_MAPSCREEN, 1, 27, "hilite_player", _("プレイヤーにカーソルを合わせる", "Highlight the player with the cursor") },

    { &display_path, true, OPT_PAGE_MAPSCREEN, 2, 8, "display_path", _("魔法や矢の軌跡を表示する", "Display actual path before shooting") },

    /*** Text Display Options ***/
    { &plain_descriptions, true, OPT_PAGE_TEXT, 5, 1, "plain_descriptions", _("アイテムの記述を簡略にする", "Plain object descriptions") },

    { &plain_pickup, false, OPT_PAGE_TEXT, 6, 6, "plain_pickup", _("「拾った」メッセージを簡略化する", "Plain pickup messages(japanese only)") },

    { &always_show_list, true, OPT_PAGE_TEXT, 4, 0, "always_show_list", _("選択時には常に一覧を表示する", "Always show list when choosing items") },

    { &depth_in_feet, false, OPT_PAGE_TEXT, 0, 7, "depth_in_feet", _("ダンジョンの深さをフィートで表示する", "Show dungeon level in feet") },

    { &show_labels, true, OPT_PAGE_TEXT, 0, 10, "show_labels", _("装備一覧で装備場所を表示する", "Show labels in object listings") },

    { &show_weights, true, OPT_PAGE_TEXT, 0, 11, "show_weights", _("アイテム一覧で重量を表示する", "Show weights in object listings") },

    { &show_item_graph, true, OPT_PAGE_TEXT, 2, 0, "show_item_graph", _("アイテムのシンボルを表示する", "Show items graphics") },

    { &equippy_chars, true, OPT_PAGE_TEXT, 1, 12, "equippy_chars", _("ステータスに文字で装備を表示する", "Display 'equippy' chars") },

    { &display_mutations, false, OPT_PAGE_TEXT, 5, 0, "display_mutations", _("'C'コマンドで突然変異を表示する", "Display mutations in 'C'haracter Display") },

    { &compress_savefile, false, OPT_PAGE_TEXT, 1, 26, "compress_savefile", _("セーブ・ファイル中のメッセージを圧縮する", "Compress messages in savefiles") },

    { &abbrev_extra, false, OPT_PAGE_TEXT, 2, 10, "abbrev_extra",
        _("アイテムに追加耐性/能力の略称を刻む", "Describe obj's extra resistances by abbreviation") },

    { &abbrev_all, true, OPT_PAGE_TEXT, 2, 11, "abbrev_all", _("アイテムに全ての耐性/能力の略称を刻む", "Describe obj's all resistances by abbreviation") },

    { &exp_need, false, OPT_PAGE_TEXT, 2, 12, "exp_need", _("次のレベルに必要な経験値を表示する", "Show the experience needed for next level") },

    { &ignore_unview, false, OPT_PAGE_TEXT, 2, 13, "ignore_unview", _("視界外のモンスターの行動を表示しない", "Ignore out-of-sight monster behavior") },

    { &show_ammo_detail, true, OPT_PAGE_TEXT, 2, 14, "show_ammo_detail", _("矢弾のダメージの説明を表示する", "Show description of ammo damage") },

    { &show_ammo_no_crit, false, OPT_PAGE_TEXT, 2, 15, "show_ammo_no_crit",
        _("会心を考慮しない場合の矢弾のダメージを表示する", "Show ammo damage with no critical") },

    { &show_ammo_crit_ratio, false, OPT_PAGE_TEXT, 2, 16, "show_ammo_crit_ratio", _("矢弾の会心発生率を表示する", "Show critical ratio of ammo") },

    { &show_actual_value, true, OPT_PAGE_TEXT, 2, 17, "show_actual_value", _("技能値等に実値を並記する", "Show actual values of skills or etc.") },

    /*** Game-Play ***/
    { &stack_force_notes, true, OPT_PAGE_GAMEPLAY, 0, 8, "stack_force_notes", _("異なる銘のアイテムをまとめる", "Merge inscriptions when stacking") },

    { &stack_force_costs, false, OPT_PAGE_GAMEPLAY, 0, 9, "stack_force_costs", _("異なる割引表示のアイテムをまとめる", "Merge discounts when stacking") },

    { &expand_list, true, OPT_PAGE_GAMEPLAY, 1, 5, "expand_list", _("「一覧」コマンドを拡張する", "Expand the power of the list commands") },

    { &small_levels, true, OPT_PAGE_GAMEPLAY, 0, 30, "small_levels", _("非常に小さいフロアの生成を可能にする", "Allow unusually small dungeon levels") },

    { &always_small_levels, false, OPT_PAGE_GAMEPLAY, 2, 3, "always_small_levels",
        _("常に非常に小さいフロアを生成する", "Always create unusually small dungeon levels") },

    { &empty_levels, true, OPT_PAGE_GAMEPLAY, 0, 31, "empty_levels", _("空っぽの「アリーナ」レベルの生成を可能にする", "Allow empty 'arena' levels") },

    { &bound_walls_perm, true, OPT_PAGE_GAMEPLAY, 2, 1, "bound_walls_perm", _("ダンジョンの外壁を永久岩にする", "Boundary walls become 'permanent wall'") },

    { &last_words, true, OPT_PAGE_GAMEPLAY, 0, 28, "last_words", _("キャラクターが死んだ時遺言をのこす", "Leave last words when your character dies") },

    { &auto_dump, false, OPT_PAGE_GAMEPLAY, 4, 5, "auto_dump", _("自動的にキャラクターの記録をファイルに書き出す", "Dump a character record automatically") },

#ifdef WORLD_SCORE
    { &send_score, true, OPT_PAGE_GAMEPLAY, 4, 6, "send_score", _("スコアサーバにスコアを送る", "Send score dump to the world score server") },
#else
    { &send_score, false, OPT_PAGE_HIDE, 4, 6, "send_score", _("スコアサーバにスコアを送る", "Send score dump to the world score server") },
#endif

    { &auto_debug_save, true, OPT_PAGE_GAMEPLAY, 4, 7, "auto_debug_save", _("デバッグ用セーブデータを自動生成する", "Create a debug save automatically") },

    { &allow_debug_opts, false, OPT_PAGE_GAMEPLAY, 6, 11, "allow_debug_opts", _("デバッグ/詐欺オプションを許可する", "Allow use of debug/cheat options") },

    /*** Disturbance ***/

    { &find_ignore_stairs, false, OPT_PAGE_DISTURBANCE, 0, 16, "find_ignore_stairs", _("階段は通過する", "Run past stairs") },

    { &find_ignore_doors, true, OPT_PAGE_DISTURBANCE, 0, 17, "find_ignore_doors", _("ドアは通過する", "Run through open doors") },

    { &find_cut, false, OPT_PAGE_DISTURBANCE, 0, 18, "find_cut", _("曲り角を斜めに最短距離で通過する", "Run past known corners") },

    { &check_abort, true, OPT_PAGE_DISTURBANCE, 1, 18, "check_abort", _("連続コマンドはキー入力で中断する", "Check for user abort while in repeated command") },

    { &flush_failure, true, OPT_PAGE_DISTURBANCE, 1, 20, "flush_failure", _("様々なミス発生時に入力をクリアする", "Flush input on various failures") },

    { &flush_disturb, false, OPT_PAGE_DISTURBANCE, 1, 21, "flush_disturb", _("障害発生時に入力をクリアする", "Flush input whenever disturbed") },

    { &disturb_move, false, OPT_PAGE_DISTURBANCE, 0, 20, "disturb_move", _("どこのモンスターが動いても行動を中止する", "Disturb whenever any monster moves") },

    { &disturb_high, false, OPT_PAGE_DISTURBANCE, 1, 3, "disturb_high",
        _("レベルの高いモンスターが動いたら行動を中止する", "Disturb whenever high-level monster moves") },

    { &disturb_near, true, OPT_PAGE_DISTURBANCE, 0, 21, "disturb_near",
        _("視界内のモンスターが動いたら行動を中止する", "Disturb whenever viewable monster moves") },

    { &disturb_pets, false, OPT_PAGE_DISTURBANCE, 5, 6, "disturb_pets", _("視界内のペットが動いたら行動を中止する", "Disturb when visible pets move") },

    { &disturb_panel, true, OPT_PAGE_DISTURBANCE, 0, 22, "disturb_panel", _("画面スクロール時に行動を中止する", "Disturb whenever map panel changes") },

    { &disturb_state, true, OPT_PAGE_DISTURBANCE, 0, 23, "disturb_state",
        _("自分のステータス変化時に行動を中止する", "Disturb whenever player state changes") },

    { &disturb_minor, true, OPT_PAGE_DISTURBANCE, 0, 24, "disturb_minor", _("些細なことが起きても行動を中止する", "Disturb whenever boring things happen") },

    { &ring_bell, false, OPT_PAGE_DISTURBANCE, 0, 14, "ring_bell", _("エラー時にビープ音を鳴らす", "Audible bell (on errors, etc)") },

    { &disturb_trap_detect, true, OPT_PAGE_DISTURBANCE, 0, 27, "disturb_trap_detect",
        _("トラップ感知範囲外に出る直前に行動を中止する", "Disturb when leaving trap detected area") },

    { &alert_trap_detect, true, OPT_PAGE_DISTURBANCE, 0, 25, "alert_trap_detect",
        _("トラップ感知範囲外に出る直前に警告する", "Alert when leaving trap detected area") },

    /*** Birth Options ***/
    { &smart_learn, true, OPT_PAGE_BIRTH, 1, 14, "smart_learn", _("モンスターは失敗を学習する(*)", "Monsters learn from their mistakes (*)") },

    { &smart_cheat, false, OPT_PAGE_BIRTH, 1, 15, "smart_cheat", _("モンスターはプレイヤーの弱みを突く(*)", "Monsters exploit players weaknesses (*)") },

    { &vanilla_town, false, OPT_PAGE_BIRTH, 6, 0, "vanilla_town", _("元祖の街/クエストと荒野なし", "Use 'vanilla' town without quests and wilderness") },

    { &lite_town, false, OPT_PAGE_BIRTH, 6, 1, "lite_town", _("小規模な街/荒野なし", "Use 'lite' town without a wilderness") },

    { &ironman_shops, false, OPT_PAGE_BIRTH, 6, 2, "ironman_shops", _("(鉄人用)店を使用しない(*)", "Stores are permanently closed (*)") },

    { &ironman_small_levels, false, OPT_PAGE_BIRTH, 6, 3, "ironman_small_levels",
        _("(鉄人用)常に非常に小さいフロアを生成(*)", "Always create unusually small dungeon levels (*)") },

    { &ironman_downward, false, OPT_PAGE_BIRTH, 6, 4, "ironman_downward", _("(鉄人用)帰還と上り階段なし(*)", "Disable recall and use of up stairs (*)") },

    { &ironman_empty_levels, false, OPT_PAGE_BIRTH, 6, 8, "ironman_empty_levels",
        _("(鉄人用)常に空っぽのアリーナレベルを生成(*)", "Always create empty 'arena' levels (*)") },

    { &ironman_rooms, false, OPT_PAGE_BIRTH, 6, 12, "ironman_rooms", _("(鉄人用)常に普通でない部屋を生成する(*)", "Always generate very unusual rooms (*)") },

    { &ironman_nightmare, false, OPT_PAGE_BIRTH, 6, 18, "ironman_nightmare",
        _("(鉄人用)悪夢モード(これは全く不条理です！)(*)", "Nightmare mode(it isn't even remotely fair!)(*)") },

    { &left_hander, false, OPT_PAGE_BIRTH, 6, 13, "left_hander", _("左利きである", "Left-Hander") },

    { &preserve_mode, true, OPT_PAGE_BIRTH, 6, 14, "preserve_mode", _("伝説のアイテムを取り逃しても再生成される(*)", "Preserve artifacts (*)") },

    { &autoroller, true, OPT_PAGE_BIRTH, 6, 15, "autoroller", _("能力値にオートローラー使用(*)", "Allow use of autoroller for stats (*)") },

    { &autochara, false, OPT_PAGE_BIRTH, 6, 16, "autochara", _("体格/地位にオートローラー使用", "Autoroll for weight, height and social status") },

    { &powerup_home, true, OPT_PAGE_BIRTH, 4, 3, "powerup_home", _("我が家を拡張する(*)", "Increase capacity of your home (*)") },

    { &keep_savefile, true, OPT_PAGE_BIRTH, 4, 4, "keep_savefile", _("同一のセーブファイルでゲームを開始する", "Start game with same savefile thet is loaded") },

    /*** Easy Object Auto-Destroyer ***/
    { &destroy_items, false, OPT_PAGE_AUTODESTROY, 7, 0, "destroy_items", _("アイテムの簡易自動破壊を使用する", "Use easy auto-destroyer") },

    { &destroy_feeling, false, OPT_PAGE_AUTODESTROY, 7, 8, "destroy_feeling", _("簡易鑑定したとき自動破壊を適用する", "Apply auto-destroy as sense feeling") },

    { &destroy_identify, false, OPT_PAGE_AUTODESTROY, 7, 9, "destroy_identify", _("鑑定したとき自動破壊を適用する", "Apply auto-destroy as identify an item") },

    { &leave_worth, true, OPT_PAGE_AUTODESTROY, 7, 2, "leave_worth", _("価値があるアイテムは壊さない", "Auto-destroyer leaves known worthy items") },

    { &leave_equip, false, OPT_PAGE_AUTODESTROY, 7, 3, "leave_equip", _("武器/防具は壊さない", "Auto-destroyer leaves weapons and armour") },

    { &leave_chest, true, OPT_PAGE_AUTODESTROY, 7, 7, "leave_chest", _("開封されていない箱は壊さない", "Auto-destroyer leaves closed chests") },

    { &leave_wanted, true, OPT_PAGE_AUTODESTROY, 7, 4, "leave_wanted", _("賞金首の死体/骨は壊さない", "Auto-destroyer leaves wanted corpses") },

    { &leave_corpse, false, OPT_PAGE_AUTODESTROY, 7, 5, "leave_corpse", _("死体/骨は壊さない", "Auto-destroyer leaves corpses and skeletons") },

    { &leave_junk, false, OPT_PAGE_AUTODESTROY, 7, 6, "leave_junk", _("がらくたは壊さない", "Auto-destroyer leaves junk") },

    { &leave_special, true, OPT_PAGE_AUTODESTROY, 7, 1, "leave_special",
        _("種族/職業で特別に必要なアイテムは壊さない", "Auto-destroyer leaves items your race/class needs") },

    /*** Play-record Options ***/
    { &record_fix_art, true, OPT_PAGE_PLAYRECORD, 4, 11, "record_fix_art", _("固定アーティファクトの入手を記録する", "Record fixed artifacts") },

    { &record_rand_art, true, OPT_PAGE_PLAYRECORD, 4, 12, "record_rand_art", _("ランダムアーティファクトの入手を記録する", "Record random artifacts") },

    { &record_destroy_uniq, true, OPT_PAGE_PLAYRECORD, 4, 13, "record_destroy_uniq",
        _("ユニークモンスターを倒したときを記録する", "Record when destroy unique monster") },

    { &record_fix_quest, true, OPT_PAGE_PLAYRECORD, 4, 14, "record_fix_quest", _("固定クエストの達成を記録する", "Record fixed quests") },

    { &record_rand_quest, true, OPT_PAGE_PLAYRECORD, 4, 15, "record_rand_quest", _("ランダムクエストの達成を記録する", "Record random quests") },

    { &record_maxdepth, true, OPT_PAGE_PLAYRECORD, 4, 16, "record_maxdepth", _("最深階を更新したときに記録する", "Record movements to deepest level") },

    { &record_stair, true, OPT_PAGE_PLAYRECORD, 4, 17, "record_stair", _("階の移動を記録する", "Record recall and stair movements") },

    { &record_buy, true, OPT_PAGE_PLAYRECORD, 4, 18, "record_buy", _("アイテムの購入を記録する", "Record purchased items") },

    { &record_sell, false, OPT_PAGE_PLAYRECORD, 4, 19, "record_sell", _("アイテムの売却を記録する", "Record sold items") },

    { &record_danger, true, OPT_PAGE_PLAYRECORD, 4, 20, "record_danger", _("ピンチになったときを記録する", "Record hitpoint warning") },

    { &record_arena, true, OPT_PAGE_PLAYRECORD, 4, 21, "record_arena", _("アリーナでの勝利を記録する", "Record arena victories") },

    { &record_ident, true, OPT_PAGE_PLAYRECORD, 4, 22, "record_ident", _("未判明のアイテムの識別を記録する", "Record first identified items") },

    { &record_named_pet, false, OPT_PAGE_PLAYRECORD, 4, 23, "record_named_pet", _("名前つきペットの情報を記録する", "Record information about named pets") },

    /*** End of Table ***/
    { nullptr, 0, 0, 0, 0, nullptr, nullptr }
};

/*!
 * チートオプションの定義テーブル / Cheating options
 */
const std::vector<option_type> cheat_info = {
    { &cheat_peek, false, 255, 0x01, 0x00, "cheat_peek", _("アイテムの生成をのぞき見る", "Peek into object creation") },

    { &cheat_hear, false, 255, 0x02, 0x00, "cheat_hear", _("モンスターの生成をのぞき見る", "Peek into monster creation") },

    { &cheat_room, false, 255, 0x04, 0x00, "cheat_room", _("ダンジョンの生成をのぞき見る", "Peek into dungeon creation") },

    { &cheat_xtra, false, 255, 0x08, 0x00, "cheat_xtra", _("その他の事をのぞき見る", "Peek into something else") },

    { &cheat_know, false, 255, 0x10, 0x00, "cheat_know", _("完全なモンスターの思い出を知る", "Know complete monster info") },

    { &cheat_live, false, 255, 0x20, 0x00, "cheat_live", _("死を回避することを可能にする", "Allow player to avoid death") },

    { &cheat_save, false, 255, 0x40, 0x00, "cheat_save", _("死んだ時セーブするか確認する", "Ask for saving death") },

    { &cheat_diary_output, false, 255, 0x80, 0x00, "cheat_diary_output", _("ウィザードログを日記に出力する", "Output wizard log to diary.") },

    { &cheat_turn, false, 255, 0x81, 0x00, "cheat_turn", _("ゲームメッセージにターン表示を行う", "Put turn in game messages.") },

    { &cheat_sight, false, 255, 0x82, 0x00, "cheat_sight", _("「見る」コマンドを拡張する。", "Expand \"L\"ook command.") },

    { &cheat_immortal, false, 255, 0x83, 0x00, "cheat_immortal", _("完全な不滅状態になる。", "Completely immortal.") }
};

/*!
 * 自動セーブオプションテーブル
 */
const std::vector<option_type> autosave_info = {
    { &autosave_l, false, 255, 0x01, 0x00, "autosave_l", _("新しい階に入る度に自動セーブする", "Autosave when entering new levels") },

    { &autosave_t, false, 255, 0x02, 0x00, "autosave_t", _("一定ターン毎に自動セーブする", "Timed autosave") },
};
