#include "game-option/option-types-table.h"
#include "game-option/auto-destruction-options.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/disturbance-options.h"
#include "game-option/game-option-page.h"
#include "game-option/game-play-options.h"
#include "game-option/input-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "locale/language-switcher.h"
#include "util/flag-group.h"
#include <stdexcept>
#include <util/enum-converter.h>
#include <utility>

GameOption::GameOption(bool *value, bool norm, GameOptionType type, std::string &&text, std::string &&description, const tl::optional<GameOptionPage> &page)
    : GameOption(value, norm, static_cast<uint8_t>(enum2i(type) / 32), static_cast<uint8_t>(enum2i(type) % 32), std::move(text), std::move(description), page)
{
}

GameOption::GameOption(bool *value, bool norm, uint8_t set, uint8_t bits, std::string &&text, std::string &&description, const tl::optional<GameOptionPage> &page)
    : value(value)
    , default_value(norm)
    , flag_position(set)
    , offset(bits)
    , text(std::move(text))
    , description(std::move(description))
    , page(page)
{
}

namespace {

std::vector<GameOption> validate_option_info(std::vector<GameOption> options)
{
    EnumClassFlagGroup<GameOptionType> used_flags;
    for (const auto &option : options) {
        const auto type_val = option.flag_position * 32 + option.offset;
        if (type_val >= enum2i(GameOptionType::MAX)) {
            throw std::logic_error("game option flag position out of range: " + option.text);
        }

        const auto type = static_cast<GameOptionType>(type_val);
        if (used_flags.has(type)) {
            throw std::logic_error("duplicated game option flag: " + option.text);
        }

        used_flags.set(type);
    }

    return options;
}

}

/*!
 * @brief オプションテーブル /
 * Available Options
 */
const std::vector<GameOption> option_info = validate_option_info({
    /*** Input Options ***/
    { &rogue_like_commands, false, GameOptionType::ROGUE_LIKE_COMMANDS, "rogue_like_commands", _("ローグ風キー配置を使用する", "Rogue-like commands"), GameOptionPage::INPUT },

    { &always_pickup, false, GameOptionType::ALWAYS_PICKUP, "always_pickup", _("常にアイテムを拾う", "Pick things up by default"), GameOptionPage::INPUT },

    { &carry_query_flag, false, GameOptionType::CARRY_QUERY_FLAG, "carry_query_flag", _("アイテムを拾う前に確認する", "Prompt before picking things up"), GameOptionPage::INPUT },

    { &quick_messages, true, GameOptionType::QUICK_MESSAGES, "quick_messages", _("クイック・メッセージを使用する", "Activate quick messages"), GameOptionPage::INPUT },

    { &auto_more, false, GameOptionType::AUTO_MORE, "auto_more", _("キー待ちしないで連続でメッセージを表示する", "Automatically clear '-more-' prompts"), GameOptionPage::INPUT },

    { &skip_more, false, GameOptionType::SKIP_MORE, "skip_more", _("キー待ちしないで全てのメッセージを読み飛ばす", "Automatically skip all messages."), GameOptionPage::INPUT },

    { &command_menu, true, GameOptionType::COMMAND_MENU, "command_menu", _("メニューによりコマンド選択を有効にする", "Enable command selection menu"), GameOptionPage::INPUT },

    { &other_query_flag, false, GameOptionType::OTHER_QUERY_FLAG, "other_query_flag", _("床上のアイテムを使用するときに確認する", "Prompt for floor item selection"), GameOptionPage::INPUT },

    { &use_old_target, false, GameOptionType::USE_OLD_TARGET, "use_old_target", _("常に以前のターゲットを指定する", "Use old target by default"), GameOptionPage::INPUT },

    { &always_repeat, true, GameOptionType::ALWAYS_REPEAT, "always_repeat", _("コマンド自動繰り返し", "Repeat obvious commands"), GameOptionPage::INPUT },

    { &confirm_destroy, false, GameOptionType::CONFIRM_DESTROY, "confirm_destroy",
        _("「無価値」なアイテムを破壊する時確認する", "Prompt for destruction of known worthless items"), GameOptionPage::INPUT },

    { &confirm_wear, true, GameOptionType::CONFIRM_WEAR, "confirm_wear", _("呪われた物を装備する時確認する", "Confirm to wear/wield known cursed items"), GameOptionPage::INPUT },

    { &confirm_quest, true, GameOptionType::CONFIRM_QUEST, "confirm_quest", _("クエストを諦めて階段で逃げる前に確認する", "Prompt before exiting a quest level"), GameOptionPage::INPUT },

    { &target_pet, false, GameOptionType::TARGET_PET, "target_pet", _("ペットをターゲットにする", "Allow targeting pets"), GameOptionPage::INPUT },

    { &easy_open, true, GameOptionType::EASY_OPEN, "easy_open", _("自動的にドアを開ける", "Automatically open doors"), GameOptionPage::INPUT },

    { &easy_disarm, true, GameOptionType::EASY_DISARM, "easy_disarm", _("自動的に罠を解除する", "Automatically disarm traps"), GameOptionPage::INPUT },

    { &easy_floor, true, GameOptionType::EASY_FLOOR, "easy_floor", _("床上で重なったアイテムをリストする", "Display floor stacks in a list"), GameOptionPage::INPUT },

    { &use_command, false, GameOptionType::USE_COMMAND, "use_command", _("「使う(a)」コマンドでアイテムを何でも使える", "Allow unified use command"), GameOptionPage::INPUT },

    { &over_exert, false, GameOptionType::OVER_EXERT, "over_exert", _("MPが足りなくても魔法に挑戦する", "Allow casting spells when short of mana"), GameOptionPage::INPUT },

    { &numpad_as_cursorkey, true, GameOptionType::NUMPAD_AS_CURSORKEY, "numpad_as_cursorkey",
        _("エディタ内でテンキーをカーソルキーとして使う", "Use numpad keys as cursor keys in editor mode"), GameOptionPage::INPUT },

    /*** Map Screen Options ***/
    { &center_player, false, GameOptionType::CENTER_PLAYER, "center_player", _("常にプレイヤーを中心に置く(*遅い*)", "Center map while walking (*slow*)"), GameOptionPage::MAPSCREEN },

    { &center_running, true, GameOptionType::CENTER_RUNNING, "center_running", _("走っている時でも中心に置く", "Centering even while running"), GameOptionPage::MAPSCREEN },

    { &view_yellow_lite, true, GameOptionType::VIEW_YELLOW_LITE, "view_yellow_lite", _("明かりの範囲を特別な色で表示する", "Use special colors for torch-lit grids"), GameOptionPage::MAPSCREEN },

    { &view_bright_lite, true, GameOptionType::VIEW_BRIGHT_LITE, "view_bright_lite", _("視界の範囲を特別な色で表示する", "Use special colors for 'viewable' grids"), GameOptionPage::MAPSCREEN },

    { &view_granite_lite, true, GameOptionType::VIEW_GRANITE_LITE, "view_granite_lite", _("壁を特別な色で表示する(重い)", "Use special colors for wall grids (slow)"), GameOptionPage::MAPSCREEN },

    { &view_special_lite, true, GameOptionType::VIEW_SPECIAL_LITE, "view_special_lite",
        _("床を特別な色で表示する(重い)", "Use special colors for floor grids (slow)"), GameOptionPage::MAPSCREEN },

    { &view_perma_grids, true, GameOptionType::VIEW_PERMA_GRIDS, "view_perma_grids", _("明るい場所はそのままにする", "Map remembers all perma-lit grids"), GameOptionPage::MAPSCREEN },

    { &view_torch_grids, false, GameOptionType::VIEW_TORCH_GRIDS, "view_torch_grids", _("明かりで照らした場所はそのままにする", "Map remembers all torch-lit grids"), GameOptionPage::MAPSCREEN },

    { &view_unsafe_grids, true, GameOptionType::VIEW_UNSAFE_GRIDS, "view_unsafe_grids", _("トラップ感知済みでない場所を表示する", "Map marked by detect traps"), GameOptionPage::MAPSCREEN },

    { &view_hidden_walls, true, GameOptionType::VIEW_HIDDEN_WALLS, "view_hidden_walls", _("壁の中に囲まれた壁を表示する", "Map walls hidden in other walls"), GameOptionPage::MAPSCREEN },

    { &view_unsafe_walls, false, GameOptionType::VIEW_UNSAFE_WALLS, "view_unsafe_walls", _("トラップ未感知の壁の中に囲まれた壁を表示する", "Map hidden walls not marked by detect traps"), GameOptionPage::MAPSCREEN },

    { &view_reduce_view, false, GameOptionType::VIEW_REDUCE_VIEW, "view_reduce_view", _("街では視野を狭くする", "Reduce view-radius in town"), GameOptionPage::MAPSCREEN },

    { &fresh_before, true, GameOptionType::FRESH_BEFORE, "fresh_before", _("連続コマンド中に画面を再描画し続ける", "Flush output while in repeated command"), GameOptionPage::MAPSCREEN },

    { &fresh_after, false, GameOptionType::FRESH_AFTER, "fresh_after", _("コマンド後に画面を常に再描画し続ける", "Flush output after monster's move"), GameOptionPage::MAPSCREEN },

    { &fresh_once, false, GameOptionType::FRESH_ONCE, "fresh_once", _("キー入力毎に一度だけ画面を再描画する", "Flush output once per key input"), GameOptionPage::MAPSCREEN },

    { &fresh_message, false, GameOptionType::FRESH_MESSAGE, "fresh_message", _("メッセージの後に画面を再描画する", "Flush output after every message"), GameOptionPage::MAPSCREEN },

    { &hilite_player, false, GameOptionType::HILITE_PLAYER, "hilite_player", _("プレイヤーにカーソルを合わせる", "Highlight the player with the cursor"), GameOptionPage::MAPSCREEN },

    { &display_path, true, GameOptionType::DISPLAY_PATH, "display_path", _("魔法や矢の軌跡を表示する", "Display actual path before shooting"), GameOptionPage::MAPSCREEN },

    /*** Text Display Options ***/
    { &plain_descriptions, true, GameOptionType::PLAIN_DESCRIPTIONS, "plain_descriptions", _("アイテムの記述を簡略にする", "Plain object descriptions"), GameOptionPage::TEXT },

    { &plain_pickup, false, GameOptionType::PLAIN_PICKUP, "plain_pickup", _("「拾った」メッセージを簡略化する", "Plain pickup messages(japanese only)"), GameOptionPage::TEXT },

    { &always_show_list, true, GameOptionType::ALWAYS_SHOW_LIST, "always_show_list", _("選択時には常に一覧を表示する", "Always show list when choosing items"), GameOptionPage::TEXT },

    { &depth_in_feet, false, GameOptionType::DEPTH_IN_FEET, "depth_in_feet", _("ダンジョンの深さをフィートで表示する", "Show dungeon level in feet"), GameOptionPage::TEXT },

    { &show_labels, true, GameOptionType::SHOW_LABELS, "show_labels", _("装備一覧で装備場所を表示する", "Show labels in object listings"), GameOptionPage::TEXT },

    { &show_weights, true, GameOptionType::SHOW_WEIGHTS, "show_weights", _("アイテム一覧で重量を表示する", "Show weights in object listings"), GameOptionPage::TEXT },

    { &show_item_graph, true, GameOptionType::SHOW_ITEM_GRAPH, "show_item_graph", _("アイテムのシンボルを表示する", "Show items graphics"), GameOptionPage::TEXT },

    { &equippy_chars, true, GameOptionType::EQUIPPY_CHARS, "equippy_chars", _("ステータスに文字で装備を表示する", "Display 'equippy' chars"), GameOptionPage::TEXT },

    { &display_mutations, false, GameOptionType::DISPLAY_MUTATIONS, "display_mutations", _("'C'コマンドで突然変異を表示する", "Display mutations in 'C'haracter Display"), GameOptionPage::TEXT },

    { &compress_savefile, false, GameOptionType::COMPRESS_SAVEFILE, "compress_savefile", _("セーブ・ファイル中のメッセージを圧縮する", "Compress messages in savefiles"), GameOptionPage::TEXT },

    { &abbrev_extra, false, GameOptionType::ABBREV_EXTRA, "abbrev_extra",
        _("アイテムに追加耐性/能力の略称を刻む", "Describe obj's extra resistances by abbreviation"), GameOptionPage::TEXT },

    { &abbrev_all, true, GameOptionType::ABBREV_ALL, "abbrev_all", _("アイテムに全ての耐性/能力の略称を刻む", "Describe obj's all resistances by abbreviation"), GameOptionPage::TEXT },

    { &exp_need, false, GameOptionType::EXP_NEED, "exp_need", _("次のレベルに必要な経験値を表示する", "Show the experience needed for next level"), GameOptionPage::TEXT },

    { &ignore_unview, false, GameOptionType::IGNORE_UNVIEW, "ignore_unview", _("視界外のモンスターの行動を表示しない", "Ignore out-of-sight monster behavior"), GameOptionPage::TEXT },

    { &show_ammo_detail, true, GameOptionType::SHOW_AMMO_DETAIL, "show_ammo_detail", _("矢弾のダメージの説明を表示する", "Show description of ammo damage"), GameOptionPage::TEXT },

    { &show_ammo_no_crit, false, GameOptionType::SHOW_AMMO_NO_CRIT, "show_ammo_no_crit",
        _("会心を考慮しない場合の矢弾のダメージを表示する", "Show ammo damage with no critical"), GameOptionPage::TEXT },

    { &show_ammo_crit_ratio, false, GameOptionType::SHOW_AMMO_CRIT_RATIO, "show_ammo_crit_ratio", _("矢弾の会心発生率を表示する", "Show critical ratio of ammo"), GameOptionPage::TEXT },

    { &show_actual_value, true, GameOptionType::SHOW_ACTUAL_VALUE, "show_actual_value", _("技能値等に実値を並記する", "Show actual values of skills or etc."), GameOptionPage::TEXT },

    { &show_lore_summary, true, GameOptionType::SHOW_LORE_SUMMARY, "show_lore_summary", _("モンスターの思い出を要約表記にする", " Show a summary of monster lore."), GameOptionPage::TEXT },

    /*** Game-Play ***/
    { &stack_force_notes, true, GameOptionType::STACK_FORCE_NOTES, "stack_force_notes", _("異なる銘のアイテムをまとめる", "Merge inscriptions when stacking"), GameOptionPage::GAMEPLAY },

    { &stack_force_costs, false, GameOptionType::STACK_FORCE_COSTS, "stack_force_costs", _("異なる割引表示のアイテムをまとめる", "Merge discounts when stacking"), GameOptionPage::GAMEPLAY },

    { &expand_list, true, GameOptionType::EXPAND_LIST, "expand_list", _("「一覧」コマンドを拡張する", "Expand the power of the list commands"), GameOptionPage::GAMEPLAY },

    { &allow_smallest_floor, true, GameOptionType::ALLOW_SMALLEST_FLOOR, "allow_smallest_floor", _("非常に小さいフロアの生成を可能にする", "Allow extremely small floor"), GameOptionPage::GAMEPLAY },

    { &allow_largest_floor, true, GameOptionType::ALLOW_LARGEST_FLOOR, "allow_largest_floor", _("非常に大きいフロアの生成を可能にする", "Allow extremely large floor"), GameOptionPage::GAMEPLAY },

    { &always_small_floor, false, GameOptionType::ALWAYS_SMALL_FLOOR, "always_small_floor",
        _("常に小さめのフロアを生成する", "Always generate small dungeon floor"), GameOptionPage::GAMEPLAY },

    { &always_large_floor, false, GameOptionType::ALWAYS_LARGE_FLOOR, "always_large_floor",
        _("常に大きめのフロアを生成する", "Always generate large dungeon floor"), GameOptionPage::GAMEPLAY },

    { &allow_arena_floor, true, GameOptionType::ALLOW_ARENA_FLOOR, "allow_arena_floor", _("空っぽの「アリーナ」フロアの生成を可能にする", "Allow empty 'arena' floor"), GameOptionPage::GAMEPLAY },

    { &bound_walls_perm, true, GameOptionType::BOUND_WALLS_PERM, "bound_walls_perm", _("ダンジョンの外壁を永久岩にする", "Boundary walls become 'permanent wall'"), GameOptionPage::GAMEPLAY },

    { &last_words, true, GameOptionType::LAST_WORDS, "last_words", _("キャラクターが死んだ時遺言をのこす", "Leave last words when your character dies"), GameOptionPage::GAMEPLAY },

    { &auto_dump, false, GameOptionType::AUTO_DUMP, "auto_dump", _("自動的にキャラクターの記録をファイルに書き出す", "Dump a character record automatically"), GameOptionPage::GAMEPLAY },

#ifdef WORLD_SCORE
    { &send_score, true, GameOptionType::SEND_SCORE, "send_score", _("スコアサーバにスコアを送る", "Send score dump to the world score server"), GameOptionPage::GAMEPLAY },
#else
    { &send_score, false, GameOptionType::SEND_SCORE, "send_score", _("スコアサーバにスコアを送る", "Send score dump to the world score server"), GameOptionPage::HIDE },
#endif

    { &allow_debug_opts, false, GameOptionType::ALLOW_DEBUG_OPTS, "allow_debug_opts", _("デバッグ/詐欺オプションを許可する", "Allow use of debug/cheat options"), GameOptionPage::GAMEPLAY },

    /*** Disturbance ***/

    { &find_ignore_stairs, false, GameOptionType::FIND_IGNORE_STAIRS, "find_ignore_stairs", _("階段は通過する", "Run past stairs"), GameOptionPage::DISTURBANCE },

    { &find_ignore_doors, true, GameOptionType::FIND_IGNORE_DOORS, "find_ignore_doors", _("ドアは通過する", "Run through open doors"), GameOptionPage::DISTURBANCE },

    { &find_cut, false, GameOptionType::FIND_CUT, "find_cut", _("曲り角を斜めに最短距離で通過する", "Run past known corners"), GameOptionPage::DISTURBANCE },

    { &check_abort, true, GameOptionType::CHECK_ABORT, "check_abort", _("連続コマンドはキー入力で中断する", "Check for user abort while in repeated command"), GameOptionPage::DISTURBANCE },

    { &flush_failure, true, GameOptionType::FLUSH_FAILURE, "flush_failure", _("様々なミス発生時に入力をクリアする", "Flush input on various failures"), GameOptionPage::DISTURBANCE },

    { &flush_disturb, false, GameOptionType::FLUSH_DISTURB, "flush_disturb", _("障害発生時に入力をクリアする", "Flush input whenever disturbed"), GameOptionPage::DISTURBANCE },

    { &disturb_move, false, GameOptionType::DISTURB_MOVE, "disturb_move", _("どこのモンスターが動いても行動を中止する", "Disturb whenever any monster moves"), GameOptionPage::DISTURBANCE },

    { &disturb_high, true, GameOptionType::DISTURB_HIGH, "disturb_high",
        _("レベルの高いモンスターが動いたら行動を中止する", "Disturb whenever high-level monster moves"), GameOptionPage::DISTURBANCE },

    { &disturb_unknown, true, GameOptionType::DISTURB_UNKNOWN, "disturb_unknown",
        _("レベル不明のモンスターが動いたら行動を中止する", "Disturb whenever unknown-level monster moves"), GameOptionPage::DISTURBANCE },

    { &disturb_near, true, GameOptionType::DISTURB_NEAR, "disturb_near",
        _("視界内のモンスターが動いたら行動を中止する", "Disturb whenever viewable monster moves"), GameOptionPage::DISTURBANCE },

    { &disturb_pets, false, GameOptionType::DISTURB_PETS, "disturb_pets", _("視界内のペットが動いたら行動を中止する", "Disturb when visible pets move"), GameOptionPage::DISTURBANCE },

    { &disturb_panel, true, GameOptionType::DISTURB_PANEL, "disturb_panel", _("画面スクロール時に行動を中止する", "Disturb whenever map panel changes"), GameOptionPage::DISTURBANCE },

    { &disturb_state, true, GameOptionType::DISTURB_STATE, "disturb_state",
        _("自分のステータス変化時に行動を中止する", "Disturb whenever player state changes"), GameOptionPage::DISTURBANCE },

    { &disturb_minor, true, GameOptionType::DISTURB_MINOR, "disturb_minor", _("些細なことが起きても行動を中止する", "Disturb whenever boring things happen"), GameOptionPage::DISTURBANCE },

    { &ring_bell, false, GameOptionType::RING_BELL, "ring_bell", _("エラー時にビープ音を鳴らす", "Audible bell (on errors, etc)"), GameOptionPage::DISTURBANCE },

    { &disturb_trap_detect, true, GameOptionType::DISTURB_TRAP_DETECT, "disturb_trap_detect",
        _("トラップ感知範囲外に出る直前に行動を中止する", "Disturb when leaving trap detected area"), GameOptionPage::DISTURBANCE },

    { &alert_trap_detect, true, GameOptionType::ALERT_TRAP_DETECT, "alert_trap_detect",
        _("トラップ感知範囲外に出る直前に警告する", "Alert when leaving trap detected area"), GameOptionPage::DISTURBANCE },

    /*** Birth Options ***/
    { &smart_learn, true, GameOptionType::SMART_LEARN, "smart_learn", _("モンスターは失敗を学習する(*)", "Monsters learn from their mistakes (*)"), GameOptionPage::BIRTH },

    { &smart_cheat, false, GameOptionType::SMART_CHEAT, "smart_cheat", _("モンスターはプレイヤーの弱みを突く(*)", "Monsters exploit players weaknesses (*)"), GameOptionPage::BIRTH },

    { &vanilla_town, false, GameOptionType::VANILLA_TOWN, "vanilla_town", _("元祖の街/クエストと荒野なし", "Use 'vanilla' town without quests and wilderness"), GameOptionPage::BIRTH },

    { &lite_town, false, GameOptionType::LITE_TOWN, "lite_town", _("小規模な街/荒野なし", "Use 'lite' town without a wilderness"), GameOptionPage::BIRTH },

    { &ironman_shops, false, GameOptionType::IRONMAN_SHOPS, "ironman_shops", _("(鉄人用)店を使用しない(*)", "Stores are permanently closed (*)"), GameOptionPage::BIRTH },

    { &ironman_smallest_floor, false, GameOptionType::IRONMAN_SMALLEST_FLOOR, "ironman_smallest_floor",
        _("(鉄人用)常に非常に小さいフロアを生成(*)", "Always generate the smallest dungeon floor (*)"), GameOptionPage::BIRTH },

    { &ironman_downward, false, GameOptionType::IRONMAN_DOWNWARD, "ironman_downward", _("(鉄人用)帰還と上り階段なし(*)", "Disable recall and use of up stairs (*)"), GameOptionPage::BIRTH },

    { &ironman_force_arena_floor, false, GameOptionType::IRONMAN_FORCE_ARENA_FLOOR, "ironman_force_arena_floor",
        _("(鉄人用)常に空っぽの「アリーナ」フロアを生成(*)", "Always create empty 'arena' floor (*)"), GameOptionPage::BIRTH },

    { &ironman_rooms, false, GameOptionType::IRONMAN_ROOMS, "ironman_rooms", _("(鉄人用)常に普通でない部屋を生成する(*)", "Always generate very unusual rooms (*)"), GameOptionPage::BIRTH },

    { &ironman_nightmare, false, GameOptionType::IRONMAN_NIGHTMARE, "ironman_nightmare",
        _("(鉄人用)悪夢モード(これは全く不条理です！)(*)", "Nightmare mode(it isn't even remotely fair!)(*)"), GameOptionPage::BIRTH },

    { &left_hander, false, GameOptionType::LEFT_HANDER, "left_hander", _("左利きである", "Left-Hander"), GameOptionPage::BIRTH },

    { &preserve_mode, true, GameOptionType::PRESERVE_MODE, "preserve_mode", _("伝説のアイテムを取り逃しても再生成される(*)", "Preserve artifacts (*)"), GameOptionPage::BIRTH },

    { &autoroller, true, GameOptionType::AUTOROLLER, "autoroller", _("能力値にオートローラー使用(*)", "Allow use of autoroller for stats (*)"), GameOptionPage::BIRTH },

    { &autochara, false, GameOptionType::AUTOCHARA, "autochara", _("体格/地位にオートローラー使用", "Autoroll for weight, height and social status"), GameOptionPage::BIRTH },

    { &powerup_home, true, GameOptionType::POWERUP_HOME, "powerup_home", _("我が家を拡張する(*)", "Increase capacity of your home (*)"), GameOptionPage::BIRTH },

    { &keep_savefile, true, GameOptionType::KEEP_SAVEFILE, "keep_savefile", _("同一のセーブファイルでゲームを開始する", "Start game with same savefile thet is loaded"), GameOptionPage::BIRTH },

    /*** Easy Object Auto-Destroyer ***/
    { &destroy_items, false, GameOptionType::DESTROY_ITEMS, "destroy_items", _("アイテムの簡易自動破壊を使用する", "Use easy auto-destroyer"), GameOptionPage::AUTODESTROY },

    { &destroy_feeling, false, GameOptionType::DESTROY_FEELING, "destroy_feeling", _("簡易鑑定したとき自動破壊を適用する", "Apply auto-destroy as sense feeling"), GameOptionPage::AUTODESTROY },

    { &destroy_identify, false, GameOptionType::DESTROY_IDENTIFY, "destroy_identify", _("鑑定したとき自動破壊を適用する", "Apply auto-destroy as identify an item"), GameOptionPage::AUTODESTROY },

    { &leave_worth, true, GameOptionType::LEAVE_WORTH, "leave_worth", _("価値があるアイテムは壊さない", "Auto-destroyer leaves known worthy items"), GameOptionPage::AUTODESTROY },

    { &leave_equip, false, GameOptionType::LEAVE_EQUIP, "leave_equip", _("武器/防具は壊さない", "Auto-destroyer leaves weapons and armour"), GameOptionPage::AUTODESTROY },

    { &leave_chest, true, GameOptionType::LEAVE_CHEST, "leave_chest", _("開封されていない箱は壊さない", "Auto-destroyer leaves closed chests"), GameOptionPage::AUTODESTROY },

    { &leave_wanted, true, GameOptionType::LEAVE_WANTED, "leave_wanted", _("賞金首の死体/骨は壊さない", "Auto-destroyer leaves wanted corpses"), GameOptionPage::AUTODESTROY },

    { &leave_corpse, false, GameOptionType::LEAVE_CORPSE, "leave_corpse", _("死体/骨は壊さない", "Auto-destroyer leaves corpses and skeletons"), GameOptionPage::AUTODESTROY },

    { &leave_junk, false, GameOptionType::LEAVE_JUNK, "leave_junk", _("がらくたは壊さない", "Auto-destroyer leaves junk"), GameOptionPage::AUTODESTROY },

    { &leave_special, true, GameOptionType::LEAVE_SPECIAL, "leave_special",
        _("種族/職業で特別に必要なアイテムは壊さない", "Auto-destroyer leaves items your race/class needs"), GameOptionPage::AUTODESTROY },

    /*** Play-record Options ***/
    { &record_fix_art, true, GameOptionType::RECORD_FIX_ART, "record_fix_art", _("固定アーティファクトの入手を記録する", "Record fixed artifacts"), GameOptionPage::PLAYRECORD },

    { &record_rand_art, true, GameOptionType::RECORD_RAND_ART, "record_rand_art", _("ランダムアーティファクトの入手を記録する", "Record random artifacts"), GameOptionPage::PLAYRECORD },

    { &record_destroy_uniq, true, GameOptionType::RECORD_DESTROY_UNIQ, "record_destroy_uniq",
        _("ユニークモンスターを倒したときを記録する", "Record when destroy unique monster"), GameOptionPage::PLAYRECORD },

    { &record_fix_quest, true, GameOptionType::RECORD_FIX_QUEST, "record_fix_quest", _("固定クエストの達成を記録する", "Record fixed quests"), GameOptionPage::PLAYRECORD },

    { &record_rand_quest, true, GameOptionType::RECORD_RAND_QUEST, "record_rand_quest", _("ランダムクエストの達成を記録する", "Record random quests"), GameOptionPage::PLAYRECORD },

    { &record_maxdepth, true, GameOptionType::RECORD_MAXDEPTH, "record_maxdepth", _("最深階を更新したときに記録する", "Record movements to deepest level"), GameOptionPage::PLAYRECORD },

    { &record_stair, true, GameOptionType::RECORD_STAIR, "record_stair", _("階の移動を記録する", "Record recall and stair movements"), GameOptionPage::PLAYRECORD },

    { &record_buy, true, GameOptionType::RECORD_BUY, "record_buy", _("アイテムの購入を記録する", "Record purchased items"), GameOptionPage::PLAYRECORD },

    { &record_sell, false, GameOptionType::RECORD_SELL, "record_sell", _("アイテムの売却を記録する", "Record sold items"), GameOptionPage::PLAYRECORD },

    { &record_danger, true, GameOptionType::RECORD_DANGER, "record_danger", _("ピンチになったときを記録する", "Record hitpoint warning"), GameOptionPage::PLAYRECORD },

    { &record_arena, true, GameOptionType::RECORD_ARENA, "record_arena", _("アリーナでの勝利を記録する", "Record arena victories"), GameOptionPage::PLAYRECORD },

    { &record_ident, true, GameOptionType::RECORD_IDENT, "record_ident", _("未判明のアイテムの識別を記録する", "Record first identified items"), GameOptionPage::PLAYRECORD },

    { &record_named_pet, false, GameOptionType::RECORD_NAMED_PET, "record_named_pet", _("名前つきペットの情報を記録する", "Record information about named pets"), GameOptionPage::PLAYRECORD },
});

/*!
 * チートオプションの定義テーブル / Cheating options
 */
const std::vector<GameOption> cheat_info = {
    { &cheat_peek, false, 0x01, 0x00, "cheat_peek", _("アイテムの生成をのぞき見る", "Peek into object creation") },

    { &cheat_hear, false, 0x02, 0x00, "cheat_hear", _("モンスターの生成をのぞき見る", "Peek into monster creation") },

    { &cheat_room, false, 0x04, 0x00, "cheat_room", _("ダンジョンの生成をのぞき見る", "Peek into dungeon creation") },

    { &cheat_xtra, false, 0x08, 0x00, "cheat_xtra", _("その他の事をのぞき見る", "Peek into something else") },

    { &cheat_know, false, 0x10, 0x00, "cheat_know", _("完全なモンスターの思い出を知る", "Know complete monster info") },

    { &cheat_live, false, 0x20, 0x00, "cheat_live", _("死を回避することを可能にする", "Allow player to avoid death") },

    { &cheat_save, false, 0x40, 0x00, "cheat_save", _("死んだ時セーブするか確認する", "Ask for saving death") },

    { &cheat_diary_output, false, 0x80, 0x00, "cheat_diary_output", _("ウィザードログを日記に出力する", "Output wizard log to diary.") },

    { &cheat_turn, false, 0x81, 0x00, "cheat_turn", _("ゲームメッセージにターン表示を行う", "Put turn in game messages.") },

    { &cheat_sight, false, 0x82, 0x00, "cheat_sight", _("「見る」コマンドを拡張する。", "Expand \"L\"ook command.") },

    { &cheat_immortal, false, 0x83, 0x00, "cheat_immortal", _("完全な不滅状態になる。", "Completely immortal.") }
};

/*!
 * 自動セーブオプションテーブル
 */
const std::vector<GameOption> autosave_info = {
    { &autosave_l, false, 0x01, 0x00, "autosave_l", _("新しい階に入る度に自動セーブする", "Autosave when entering new levels") },

    { &autosave_t, false, 0x02, 0x00, "autosave_t", _("一定ターン毎に自動セーブする", "Timed autosave") },
};
