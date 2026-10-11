/*!
 * @brief アリーナの状態解除を、本体の終了処理とメモリ上の端末で検証する。
 * @details 各ケースを独立プロセスで実行する。通常のユニットテストには登録しない。
 */
#include "avatar/avatar.h"
#include "game-option/input-options.h"
#include "game-option/text-display-options.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "locale/character-encoding.h"
#include "main/angband-initializer.h"
#include "market/arena-entry.h"
#include "market/arena.h"
#include "market/building-actions-table.h"
#include "market/melee-arena.h"
#include "mspell/mspell-dispel.h"
#include "mspell/mspell-result.h"
#include "mspell/mspell-util.h"
#include "player-info/bard-data-type.h"
#include "player-info/class-info.h"
#include "player-info/force-trainer-data-type.h"
#include "player-info/ninja-data-type.h"
#include "player-info/race-info.h"
#include "player-info/spell-hex-data-type.h"
#include "player/attack-defense-types.h"
#include "player/player-personality.h"
#include "player/race-info-table.h"
#include "realm/realm-song-numbers.h"
#include "realm/realm-types.h"
#include "spell-realm/spells-hex.h"
#include "status/buff-setter.h"
#include "system/angband-system.h"
#include "system/building-type-definition.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/term-color-types.h"
#include "term/z-term.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#ifndef _WIN32
#include "main-unix/unix-user-ids.h"
#include <unistd.h>
#endif

namespace {
std::string input;
size_t input_pos = 0;
int more_count = 0;
bool passed = true;

errr event_hook(int action, int)
{
    if (action == TERM_XTRA_EVENT) {
        if (is_waiting_for_more()) {
            ++more_count;
            term_key_push(' ');
        } else if (input_pos < input.size()) {
            term_key_push(input[input_pos++]);
        } else {
            // 入力の食い違いを無限待ちにせず失敗として報告する。
            std::cerr << "Unexpected input request\n";
            std::exit(1);
        }
    }
    return 0;
}

void check(bool condition, const char *description)
{
    if (!condition) {
        passed = false;
        std::cerr << "FAIL: " << description << '\n';
    }
}

int count_message(std::u8string_view text)
{
    // プローブの文字列が本体と異なる実行文字コードでも比較できるようにする。
    const std::string_view expected(reinterpret_cast<const char *>(text.data()), text.size());
    auto count = 0;
    for (auto age = 0; age < message_num(); ++age) {
        const auto message = sys_to_utf8(*message_str(age));
        if (message && *message == expected) {
            ++count;
        }
    }
    return count;
}
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        std::cerr << "Usage: arena-status-probe <lib-dir> <scratch-dir> <case>\n";
        return 2;
    }

    const std::string mode(argv[3]);
    PlayerType player;
    if (mode.ends_with("force")) {
        player.pclass = PlayerClassType::FORCETRAINER;
        player.realm1 = RealmType::LIFE;
    } else if (mode.ends_with("ninja")) {
        player.pclass = PlayerClassType::NINJA;
    } else if (mode.starts_with("song-")) {
        player.pclass = PlayerClassType::BARD;
        player.realm1 = RealmType::MUSIC;
    } else if (mode.starts_with("hex-")) {
        player.pclass = PlayerClassType::HIGH_MAGE;
        player.realm1 = RealmType::HEX;
    }
    p_ptr = &player;
#ifndef _WIN32
    auto &ids = UnixUserIds::get_instance();
    ids.set_user_id(getuid());
    ids.set_effective_user_id(geteuid());
    ids.set_effective_group_id(getegid());
#endif
    init_file_paths(std::filesystem::absolute(argv[1]));
    const auto scratch = std::filesystem::absolute(argv[2]);
    // 初期化で作るスコア等も、普段のゲーム環境とソースから分離する。
    std::filesystem::create_directories(scratch);
    ANGBAND_DIR_APEX = scratch;
    ANGBAND_DIR_BONE = scratch;
    ANGBAND_DIR_DATA = scratch;
    ANGBAND_DIR_SAVE = scratch;
    ANGBAND_DIR_USER = scratch;
    init_angband(&player, true);
    player.lev = 1;
    player.playing = true;
    player.energy_need = 42;
    player.tsubureru = 20;
    for (auto i = 0; i < A_MAX; ++i) {
        player.stat_cur[i] = 18;
        player.stat_max[i] = 18;
        player.stat_max_max[i] = 118;
    }
    player.vir_types[0] = Virtue::VITALITY;
    rp_ptr = &race_info[enum2i(player.prace)];
    cp_ptr = &class_info.at(player.pclass);
    mp_ptr = &class_magics_info[enum2i(player.pclass)];
    ap_ptr = &personality_info[player.ppersonality];
    auto &world = AngbandWorld::get_instance();
    world.character_generated = true;
    world.character_icky_depth = 1;
    auto_more = false;
    skip_more = false;
    quick_messages = false;
    num_more = 0;
    msg_flag = false;

    term_type term;
    term_init(&term, 80, 24, 64);
    term.xtra_hook = event_hook;
    term.never_bored = true;
    term.never_fresh = true;
    angband_terms[0] = &term;
    term_activate(&term);
    term_putstr(0, 10, -1, TERM_WHITE, "facility screen");
    const auto dispel = [&] {
        auto &caster = player.current_floor_ptr->m_list[1];
        caster.r_idx = caster.ap_r_idx = MonraceId::NOBORTA;
        caster.ml = true;
        check(spell_RF4_DISPEL(1, &player, 0, MONSTER_TO_PLAYER).valid, "monster dispel executed");
    };

    if ((mode == "timed-force") || (mode == "arena-force") || (mode == "dispel-force")) {
        player.pclass = PlayerClassType::FORCETRAINER;
        auto data = std::make_shared<force_trainer_data_type>();
        player.class_specific_data = data;
        data->ki = 200;
        if (mode == "timed-force") {
            reset_tim_flags(&player);
            check(player.tsubureru == 0, "timed reset clears Kutar flattening");
            check(data->ki == 200, "timed reset must leave ki alone");
        } else if (mode == "arena-force") {
            reset_arena_buffs(&player);
            check(data->ki == 0, "arena clears ki");
        } else {
            dispel();
            check(data->ki == 0, "dispel clears ki");
            check(count_message(_(u8"練気が消えた。", u8"Your stored Force dissipates.")) == 1, "ki notification");
        }
    } else if ((mode == "timed-ninja") || (mode == "arena-ninja") || (mode == "dispel-ninja")) {
        player.pclass = PlayerClassType::NINJA;
        auto data = std::make_shared<ninja_data_type>();
        player.class_specific_data = data;
        data->kawarimi = true;
        if (mode == "timed-ninja") {
            reset_tim_flags(&player);
            check(player.tsubureru == 0, "timed reset clears Kutar flattening");
            check(data->kawarimi, "timed reset must leave kawarimi alone");
        } else {
            if (mode == "arena-ninja") {
                reset_arena_buffs(&player);
            } else {
                dispel();
                check(count_message(_(u8"変わり身の備えが消えた。", u8"You are no longer prepared to evade attacks.")) == 1, "kawarimi notification");
            }
            check(!data->kawarimi, "arena/dispel clears kawarimi");
        }
    } else if ((mode == "hex-active") || (mode == "hex-interrupted") || (mode == "hex-stop") || (mode == "hex-dispel")) {
        auto data = std::make_shared<spell_hex_data_type>();
        player.class_specific_data = data;
        data->casting_spells.set(HEX_SHOCK_CLOAK);
        data->revenge_type = SpellHexRevengeType::PATIENCE;
        data->revenge_power = 20;
        data->revenge_turn = 3;
        if (mode == "hex-interrupted") {
            SpellHex(&player).interrupt_spelling();
        } else {
            player.action = ACTION_SPELL;
        }
        if (mode == "hex-dispel") {
            dispel();
            check(data->interrupting_spells.has(HEX_SHOCK_CLOAK) && data->revenge_turn == 3 && player.tsubureru == 20, "normal dispel preserves Hex restart, revenge and flattening");
        } else if (mode == "hex-stop") {
            SpellHex(&player).stop_all_spells();
            check(data->revenge_turn == 3, "normal Hex stop preserves revenge");
        } else {
            reset_arena_buffs(&player);
            check(data->interrupting_spells.none(), "arena clears interrupted Hex");
            check(data->revenge_type == SpellHexRevengeType::NONE && data->revenge_turn == 0 && data->revenge_power == 0, "arena clears revenge");
        }
        check(data->casting_spells.none(), "active Hex stopped");
        check(player.action == ACTION_NONE, "Hex action ended");
        check(count_message(_(u8"稲妻のオーラが消え去った。", u8"The electrical aura disappeared.")) == ((mode == "hex-interrupted") || (mode == "hex-dispel") ? 0 : 1), "real Hex STOP runs exactly once");
    } else if (mode == "dispel-invuln") {
        player.invuln = 20;
        dispel();
        check(player.invuln == 0 && player.energy_need > 42, "normal dispel retains invulnerability ending energy penalty");
    } else if ((mode == "song-active") || (mode == "song-interrupted")) {
        player.pclass = PlayerClassType::BARD;
        auto data = std::make_shared<bard_data_type>();
        player.class_specific_data = data;
        data->singing_song_spell_idx = 1;
        if (mode == "song-active") {
            data->singing_song = MUSIC_BLESS;
            player.action = ACTION_SING;
        } else {
            data->interrputing_song = MUSIC_BLESS;
        }
        reset_arena_buffs(&player);
        check(data->singing_song == MUSIC_NONE && data->interrputing_song == MUSIC_NONE && data->singing_song_spell_idx == 0, "song and restart discarded");
        check(count_message(_(u8"高潔な気分が消え失せた。", u8"The prayer has expired.")) == (mode == "song-active" ? 1 : 0), "real song STOP runs exactly once");
    } else if ((mode == "ordinary") || (mode == "tsuyoshi") || (mode == "arena-normal") || (mode == "arena-silent") || (mode == "arena-metal") || (mode == "arena-cancel") || (mode == "arena-watch")) {
        player.hero = player.berserk = player.shield = player.blessed = player.invuln = player.tsubureru = 20;
        player.oppose_acid = player.oppose_elec = player.oppose_fire = player.oppose_cold = player.oppose_pois = 20;
        player.tsuyoshi = ((mode == "ordinary") || (mode == "arena-silent")) ? 0 : 20;
        const auto sequence = message_sequence();
        if ((mode == "arena-normal") || (mode == "arena-silent") || (mode == "arena-metal") || (mode == "arena-cancel")) {
            auto &entries = ArenaEntryList::get_instance();
            const auto is_normal_entry = (mode == "arena-normal") || (mode == "arena-silent");
            entries.load_current_entry(is_normal_entry ? 0 : entries.get_max_entries() + 1);
            input = is_normal_entry ? "" : (mode == "arena-cancel" ? "n" : "y");
            check(arena_comm(&player, BACT_ARENA) == (mode != "arena-cancel"), "arena entry result");
            check(player.current_floor_ptr->inside_arena == (mode != "arena-cancel"), "arena entry floor flag");
            if (mode != "arena-cancel") {
                check(!msg_flag, "ending notification acknowledged before leaving facility");
            }
        } else if (mode == "arena-watch") {
            player.au = 100;
            for (auto i = 0; i < NUM_GLADIATORS; ++i) {
                MeleeArena::get_instance().set_gladiator(i, MeleeGladiator(MonraceId::NOBORTA, 200));
            }
            input = "1\r";
            check(melee_arena_comm(&player), "spectator entry result");
            check(AngbandSystem::get_instance().is_phase_out(), "spectator flag");
        } else {
            reset_arena_buffs(&player);
        }
        if (mode == "arena-cancel") {
            check(player.tsuyoshi == 20 && player.hero == 20 && player.stat_max[A_STR] == 18 && player.virtues[0] == 0, "cancel preserves buffs and stats");
        } else {
            check(player.hero == 0 && player.berserk == 0 && player.shield == 0 && player.blessed == 0 && player.invuln == 0 && player.tsubureru == 0, "ordinary timed buffs cleared");
            check(player.oppose_acid == 0 && player.oppose_elec == 0 && player.oppose_fire == 0 && player.oppose_cold == 0 && player.oppose_pois == 0, "resistances cleared");
            check(player.energy_need == 42, "no invulnerability ending energy penalty");
            if ((mode == "ordinary") || (mode == "arena-silent")) {
                check(message_sequence() == sequence && more_count == 0, "ordinary buffs end silently without more prompts");
                check(std::string(term.scr->c[10].begin(), term.scr->c[10].end()).starts_with("facility screen"), "facility screen preserved");
            } else {
                check(player.tsuyoshi == 0 && player.stat_cur[A_STR] == 17 && player.stat_max[A_STR] == 17 && player.stat_cur[A_CON] == 17 && player.stat_max[A_CON] == 17 && player.virtues[0] == -3, "Tsuyoshi permanent ending penalty");
                if (mode == "arena-normal") {
                    check(more_count == 1, "Tsuyoshi ending notification is shown before entering arena");
                }
                reset_arena_buffs(&player);
                check(player.stat_max[A_STR] == 17 && player.stat_max[A_CON] == 17 && player.virtues[0] == -3, "ending penalty applied only once");
            }
        }
    } else {
        std::cerr << "Unknown case: " << mode << '\n';
        return 2;
    }

    std::cout << "case=" << mode << " more=" << more_count << " result=" << (passed ? "PASS" : "FAIL") << '\n';
    angband_terms[0] = nullptr;
    game_term = nullptr;
    return passed ? 0 : 1;
}
