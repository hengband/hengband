#include "cmd-io/cmd-knowledge.h"
#include "cmd-visual/cmd-draw.h"
#include "game-option/birth-options.h"
#include "grid/feature.h"
#include "io/input-key-acceptor.h"
#include "knowledge/knowledge-autopick.h"
#include "knowledge/knowledge-experiences.h"
#include "knowledge/knowledge-features.h"
#include "knowledge/knowledge-inventory.h"
#include "knowledge/knowledge-items.h"
#include "knowledge/knowledge-monsters.h"
#include "knowledge/knowledge-mutations.h"
#include "knowledge/knowledge-quests.h"
#include "knowledge/knowledge-self.h"
#include "knowledge/knowledge-uniques.h"
#include "main/sound-of-music.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

/*
 * Interact with "knowledge"
 */
void do_cmd_knowledge(PlayerType *player_ptr)
{
    int i, p = 0;
    bool need_redraw = false;
    screen_save();
    TermCenteredOffsetSetter tcos(MAIN_TERM_MIN_COLS, MAIN_TERM_MIN_ROWS);
    while (true) {
        term_clear();
        prt(format(_("%d/2 ページ", "page %d/2"), (p + 1)), 2, 65);
        prt(_("現在の知識を確認する", "Display current knowledge"), 3, 0);
        if (p == 0) {
            prt(_("(1) 既知の伝説のアイテム                 の一覧", "(1) Display known artifacts"), 6, 5);
            prt(_("(2) 既知のアイテム                       の一覧", "(2) Display known objects"), 7, 5);
            prt(_("(3) 既知の生きているユニーク・モンスター の一覧", "(3) Display remaining uniques"), 8, 5);
            prt(_("(4) 既知の撃破したユニーク・モンスター   の一覧", "(4) Display defeated uniques"), 9, 5);
            prt(_("(5) 既知のモンスター                     の一覧", "(5) Display known monster"), 10, 5);
            prt(_("(6) 倒した敵の数                         の一覧", "(6) Display kill count"), 11, 5);
            if (!vanilla_town) {
                prt(_("(7) 賞金首                               の一覧", "(7) Display wanted monsters"), 12, 5);
            }
            prt(_("(8) 現在のペット                         の一覧", "(8) Display current pets"), 13, 5);
            prt(_("(9) 我が家のアイテム                     の一覧", "(9) Display home inventory"), 14, 5);
            prt(_("(0) *鑑定*済み装備の耐性                 の一覧", "(0) Display *identified* equip."), 15, 5);
        } else {
            prt(_("(a) 地形の表示文字/タイル                の一覧", "(a) Display terrain symbols"), 6, 5);
            prt(_("(b) 自分に関する情報                     の一覧", "(b) Display about yourself"), 7, 5);
            prt(_("(c) 突然変異                             の一覧", "(c) Display mutations"), 8, 5);
            prt(_("(d) 武器の経験値                         の一覧", "(d) Display weapon proficiency"), 9, 5);
            prt(_("(e) 魔法の経験値                         の一覧", "(e) Display spell proficiency"), 10, 5);
            prt(_("(f) 技能の経験値                         の一覧", "(f) Display misc. proficiency"), 11, 5);
            prt(_("(g) プレイヤーの徳                       の一覧", "(g) Display virtues"), 12, 5);
            prt(_("(h) 入ったダンジョン                     の一覧", "(h) Display dungeons"), 13, 5);
            prt(_("(i) 実行中のクエスト                     の一覧", "(i) Display current quests"), 14, 5);
            prt(_("(k) 現在の自動拾い/破壊設定              の一覧", "(k) Display auto pick/destroy"), 15, 5);
        }

        prt(_("-続く-", "-more-"), 17, 8);
        prt(_("ESC) 抜ける", "ESC) Exit menu"), 21, 1);
        prt(_("SPACE) 次ページ", "SPACE) Next page"), 21, 30);
        prt(_("コマンド:", "Command: "), 20, 0);
        i = inkey();

        if (i == ESCAPE) {
            break;
        }
        switch (i) {
        case ' ': /* Page change */
        case '-':
            p = 1 - p;
            break;
        case '1': /* Artifacts */
            do_cmd_knowledge_artifacts(player_ptr);
            break;
        case '2': /* Objects */
            do_cmd_knowledge_objects(player_ptr, &need_redraw, false, -1);
            break;
        case '3': /* Uniques */
            do_cmd_knowledge_uniques(player_ptr, true);
            break;
        case '4': /* Uniques */
            do_cmd_knowledge_uniques(player_ptr, false);
            break;
        case '5': /* Monsters */
            do_cmd_knowledge_monsters(player_ptr, &need_redraw, false);
            break;
        case '6': /* Kill count  */
            do_cmd_knowledge_kill_count(player_ptr);
            break;
        case '7': /* wanted */
            if (!vanilla_town) {
                do_cmd_knowledge_bounty(player_ptr);
            }
            break;
        case '8': /* Pets */
            do_cmd_knowledge_pets(player_ptr);
            break;
        case '9': /* Home */
            do_cmd_knowledge_home(player_ptr);
            break;
        case '0': /* Resist list */
            do_cmd_knowledge_inventory(player_ptr);
            break;
        /* Next page */
        case 'a': /* Feature list */
        {
            IDX lighting_level = F_LIT_STANDARD;
            do_cmd_knowledge_features(&need_redraw, false, -1, &lighting_level);
            break;
        }
        case 'b': /* Max stat */
            do_cmd_knowledge_stat(player_ptr);
            break;
        case 'c': /* Mutations */
            do_cmd_knowledge_mutations(player_ptr);
            break;
        case 'd': /* weapon-exp */
            do_cmd_knowledge_weapon_exp(player_ptr);
            break;
        case 'e': /* spell-exp */
            do_cmd_knowledge_spell_exp(player_ptr);
            break;
        case 'f': /* skill-exp */
            do_cmd_knowledge_skill_exp(player_ptr);
            break;
        case 'g': /* Virtues */
            do_cmd_knowledge_virtues(player_ptr);
            break;
        case 'h': /* Dungeon */
            do_cmd_knowledge_dungeon(player_ptr);
            break;
        case 'i': /* Quests */
            do_cmd_knowledge_quests(player_ptr);
            break;
        case 'k': /* Autopick */
            do_cmd_knowledge_autopick(player_ptr);
            break;
        default: /* Unknown option */
            bell();
        }

        msg_erase();
    }

    screen_load();
    if (need_redraw) {
        do_cmd_redraw(player_ptr);
    }
}
