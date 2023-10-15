#include "player/player-damage.h"
#include "autopick/autopick-pref-processor.h"
#include "avatar/avatar.h"
#include "blue-magic/blue-magic-checker.h"
#include "cmd-io/cmd-process-screen.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/quest.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/game-play-options.h"
#include "game-option/input-options.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "inventory/inventory-damage.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "io/report.h"
#include "io/write-diary.h"
#include "main/music-definitions-table.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "market/arena-info-table.h"
#include "mind/mind-mirror-master.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-armor.h"
#include "object/item-tester-hooker.h"
#include "object/object-broken.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/race-types.h"
#include "player-info/samurai-data-type.h"
#include "player/player-personality-types.h"
#include "player/player-status-flags.h"
#include "player/player-status-resist.h"
#include "player/player-status.h"
#include "player/race-info-table.h"
#include "player/special-defense-types.h"
#include "racial/racial-android.h"
#include "save/save.h"
#include "status/base-status.h"
#include "status/element-resistance.h"
#include "system/building-type-definition.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/player-paralysis.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <sstream>
#include <string>

using dam_func = int (*)(PlayerType *player_ptr, int dam, std::string_view kb_str, bool aura);

/*!
 * @brief 酸攻撃による装備のAC劣化処理 /
 * Acid has hit the player, attempt to affect some armor.
 * @param 酸を浴びたキャラクタへの参照ポインタ
 * @return 装備による軽減があったならTRUEを返す
 * @details
 * 免疫があったらそもそもこの関数は実行されない (確実に錆びない).
 * Note that the "base armor" of an object never changes.
 * If any armor is damaged (or resists), the player takes less damage.
 */
static bool acid_minus_ac(PlayerType *player_ptr)
{
    constexpr static auto candidates = {
        INVEN_MAIN_HAND,
        INVEN_SUB_HAND,
        INVEN_BODY,
        INVEN_OUTER,
        INVEN_ARMS,
        INVEN_HEAD,
        INVEN_FEET,
    };

    const auto slot = rand_choice(candidates);
    auto *o_ptr = &player_ptr->inventory_list[slot];

    if ((o_ptr == nullptr) || !o_ptr->is_valid() || !o_ptr->is_protector()) {
        return false;
    }

    const auto item_name = describe_flavor(player_ptr, o_ptr, OD_OMIT_PREFIX | OD_NAME_ONLY);
    const auto item_flags = o_ptr->get_flags();
    if (o_ptr->ac + o_ptr->to_a <= 0) {
        msg_format(_("%sは既にボロボロだ！", "Your %s is already fully corroded!"), item_name.data());
        return false;
    }

    if (item_flags.has(TR_IGNORE_ACID)) {
        msg_format(_("しかし%sには効果がなかった！", "Your %s is unaffected!"), item_name.data());
        return true;
    }

    msg_format(_("%sが酸で腐食した！", "Your %s is corroded!"), item_name.data());
    o_ptr->to_a--;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::PLAYER,
    };
    rfu.set_flags(flags_swrf);
    calc_android_exp(player_ptr);
    return true;
}

/*!
 * @brief 酸属性によるプレイヤー損害処理 /
 * Hurt the player with Acid
 * @param player_ptr 酸を浴びたキャラクタへの参照ポインタ
 * @param dam 基本ダメージ量
 * @param kb_str ダメージ原因記述
 * @param monspell 原因となったモンスター特殊攻撃ID
 * @param aura オーラよるダメージが原因ならばTRUE
 * @return 修正HPダメージ量
 * @details 酸オーラは存在しないが関数ポインタのために引数だけは用意している
 */
int acid_dam(PlayerType *player_ptr, int dam, std::string_view kb_str, bool aura)
{
    int inv = (dam < 30) ? 1 : (dam < 60) ? 2
                                          : 3;
    bool double_resist = is_oppose_acid(player_ptr);
    dam = dam * calc_acid_damage_rate(player_ptr) / 100;
    if (dam <= 0) {
        return 0;
    }

    if (aura || !check_multishadow(player_ptr)) {
        if ((!(double_resist || has_resist_acid(player_ptr))) && one_in_(CHANCE_ABILITY_SCORE_DECREASE)) {
            (void)do_dec_stat(player_ptr, A_CHR);
        }

        if (acid_minus_ac(player_ptr)) {
            dam = (dam + 1) / 2;
        }
    }

    int get_damage = take_hit(player_ptr, aura ? DAMAGE_NOESCAPE : DAMAGE_ATTACK, dam, kb_str);
    if (!aura && !(double_resist && has_resist_acid(player_ptr))) {
        inventory_damage(player_ptr, BreakerAcid(), inv);
    }

    return get_damage;
}

/*!
 * @brief 電撃属性によるプレイヤー損害処理 /
 * Hurt the player with electricity
 * @param player_ptr 電撃を浴びたキャラクタへの参照ポインタ
 * @param dam 基本ダメージ量
 * @param kb_str ダメージ原因記述
 * @param monspell 原因となったモンスター特殊攻撃ID
 * @param aura オーラよるダメージが原因ならばTRUE
 * @return 修正HPダメージ量
 */
int elec_dam(PlayerType *player_ptr, int dam, std::string_view kb_str, bool aura)
{
    int inv = (dam < 30) ? 1 : (dam < 60) ? 2
                                          : 3;
    bool double_resist = is_oppose_elec(player_ptr);

    dam = dam * calc_elec_damage_rate(player_ptr) / 100;

    if (dam <= 0) {
        return 0;
    }

    if (aura || !check_multishadow(player_ptr)) {
        if ((!(double_resist || has_resist_elec(player_ptr))) && one_in_(CHANCE_ABILITY_SCORE_DECREASE)) {
            (void)do_dec_stat(player_ptr, A_DEX);
        }
    }

    int get_damage = take_hit(player_ptr, aura ? DAMAGE_NOESCAPE : DAMAGE_ATTACK, dam, kb_str);
    if (!aura && !(double_resist && has_resist_elec(player_ptr))) {
        inventory_damage(player_ptr, BreakerElec(), inv);
    }

    return get_damage;
}

/*!
 * @brief 火炎属性によるプレイヤー損害処理 /
 * Hurt the player with Fire
 * @param player_ptr 火炎を浴びたキャラクタへの参照ポインタ
 * @param dam 基本ダメージ量
 * @param kb_str ダメージ原因記述
 * @param monspell 原因となったモンスター特殊攻撃ID
 * @param aura オーラよるダメージが原因ならばTRUE
 * @return 修正HPダメージ量
 */
int fire_dam(PlayerType *player_ptr, int dam, std::string_view kb_str, bool aura)
{
    int inv = (dam < 30) ? 1 : (dam < 60) ? 2
                                          : 3;
    bool double_resist = is_oppose_fire(player_ptr);

    /* Totally immune */
    if (has_immune_fire(player_ptr) || (dam <= 0)) {
        return 0;
    }

    dam = dam * calc_fire_damage_rate(player_ptr) / 100;
    if (aura || !check_multishadow(player_ptr)) {
        if ((!(double_resist || has_resist_fire(player_ptr))) && one_in_(CHANCE_ABILITY_SCORE_DECREASE)) {
            (void)do_dec_stat(player_ptr, A_STR);
        }
    }

    int get_damage = take_hit(player_ptr, aura ? DAMAGE_NOESCAPE : DAMAGE_ATTACK, dam, kb_str);
    if (!aura && !(double_resist && has_resist_fire(player_ptr))) {
        inventory_damage(player_ptr, BreakerFire(), inv);
    }

    return get_damage;
}

/*!
 * @brief 冷気属性によるプレイヤー損害処理 /
 * Hurt the player with Cold
 * @param player_ptr 冷気を浴びたキャラクタへの参照ポインタ
 * @param dam 基本ダメージ量
 * @param kb_str ダメージ原因記述
 * @param monspell 原因となったモンスター特殊攻撃ID
 * @param aura オーラよるダメージが原因ならばTRUE
 * @return 修正HPダメージ量
 */
int cold_dam(PlayerType *player_ptr, int dam, std::string_view kb_str, bool aura)
{
    int inv = (dam < 30) ? 1 : (dam < 60) ? 2
                                          : 3;
    bool double_resist = is_oppose_cold(player_ptr);
    if (has_immune_cold(player_ptr) || (dam <= 0)) {
        return 0;
    }

    dam = dam * calc_cold_damage_rate(player_ptr) / 100;
    if (aura || !check_multishadow(player_ptr)) {
        if ((!(double_resist || has_resist_cold(player_ptr))) && one_in_(CHANCE_ABILITY_SCORE_DECREASE)) {
            (void)do_dec_stat(player_ptr, A_STR);
        }
    }

    int get_damage = take_hit(player_ptr, aura ? DAMAGE_NOESCAPE : DAMAGE_ATTACK, dam, kb_str);
    if (!aura && !(double_resist && has_resist_cold(player_ptr))) {
        inventory_damage(player_ptr, BreakerCold(), inv);
    }

    return get_damage;
}

/*
 * Decreases players hit points and sets death flag if necessary
 *
 * Invulnerability needs to be changed into a "shield"
 *
 * Hack -- this function allows the user to save (or quit)
 * the game when he dies, since the "You die." message is shown before
 * setting the player to "dead".
 */
int take_hit(PlayerType *player_ptr, int damage_type, int damage, std::string_view hit_from)
{
    int old_chp = player_ptr->chp;
    int warning = (player_ptr->mhp * hitpoint_warn / 10);
    if (player_ptr->is_dead) {
        return 0;
    }

    if (player_ptr->sutemi) {
        damage *= 2;
    }
    if (PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::IAI)) {
        damage += (damage + 4) / 5;
    }

    if (damage_type != DAMAGE_USELIFE) {
        disturb(player_ptr, true, true);
        if (auto_more) {
            player_ptr->now_damaged = true;
        }
    }

    if ((damage_type != DAMAGE_USELIFE) && (damage_type != DAMAGE_LOSELIFE)) {
        if (is_invuln(player_ptr) && (damage < 9000)) {
            if (damage_type == DAMAGE_FORCE) {
                msg_print(_("バリアが切り裂かれた！", "The attack cuts your shield of invulnerability open!"));
            } else if (one_in_(PENETRATE_INVULNERABILITY)) {
                msg_print(_("無敵のバリアを破って攻撃された！", "The attack penetrates your shield of invulnerability!"));
            } else {
                return 0;
            }
        }

        if (check_multishadow(player_ptr)) {
            if (damage_type == DAMAGE_FORCE) {
                msg_print(_("幻影もろとも体が切り裂かれた！", "The attack hits Shadow together with you!"));
            } else if (damage_type == DAMAGE_ATTACK) {
                msg_print(_("攻撃は幻影に命中し、あなたには届かなかった。", "The attack hits Shadow, but you are unharmed!"));
                return 0;
            }
        }

        if (player_ptr->wraith_form) {
            if (damage_type == DAMAGE_FORCE) {
                msg_print(_("半物質の体が切り裂かれた！", "The attack cuts through your ethereal body!"));
            } else {
                damage /= 2;
                if ((damage == 0) && one_in_(2)) {
                    damage = 1;
                }
            }
        }

        if (PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::MUSOU)) {
            damage /= 2;
            if ((damage == 0) && one_in_(2)) {
                damage = 1;
            }
        }
    }

    player_ptr->chp -= damage;
    if (player_ptr->chp < -9999) {
        player_ptr->chp = -9999;
    }
    if (damage_type == DAMAGE_GENO && player_ptr->chp < 0) {
        damage += player_ptr->chp;
        player_ptr->chp = 0;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::HP);
    rfu.set_flag(SubWindowRedrawingFlag::PLAYER);

    if (damage_type != DAMAGE_GENO && player_ptr->chp == 0) {
        chg_virtue(player_ptr, Virtue::SACRIFICE, 1);
        chg_virtue(player_ptr, Virtue::CHANCE, 2);
    }

    if (player_ptr->chp < 0 && !cheat_immortal) {
        bool android = PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID);

        /* 死んだ時に強制終了して死を回避できなくしてみた by Habu */
        if (!cheat_save && !save_player(player_ptr, SaveType::CLOSE_GAME)) {
            msg_print(_("セーブ失敗！", "death save failed!"));
        }

        sound(SOUND_DEATH);
        chg_virtue(player_ptr, Virtue::SACRIFICE, 10);
        handle_stuff(player_ptr);
        player_ptr->leaving = true;
        if (!cheat_immortal) {
            player_ptr->is_dead = true;
        }

        const auto &floor = *player_ptr->current_floor_ptr;
        if (floor.inside_arena) {
            const auto &m_name = monraces_info[arena_info[player_ptr->arena_number].r_idx].name;
            msg_format(_("あなたは%sの前に敗れ去った。", "You are beaten by %s."), m_name.data());
            msg_print(nullptr);
            if (record_arena) {
                exe_write_diary(player_ptr, DiaryKind::ARENA, -1 - player_ptr->arena_number, m_name);
            }
        } else {
            const auto q_idx = floor.get_quest_id();
            const auto seppuku = hit_from == "Seppuku";
            const auto winning_seppuku = w_ptr->total_winner && seppuku;

            play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_GAMEOVER);

#ifdef WORLD_SCORE
            screen_dump = make_screen_dump(player_ptr);
#endif
            if (seppuku) {
                player_ptr->died_from = hit_from;
                if (!winning_seppuku) {
                    player_ptr->died_from = _("切腹", "Seppuku");
                }
            } else {
                auto effects = player_ptr->effects();
                auto is_hallucinated = effects->hallucination()->is_hallucinated();
                auto paralysis_state = "";
                if (effects->paralysis()->is_paralyzed()) {
                    paralysis_state = player_ptr->free_act ? _("彫像状態で", " while being the statue") : _("麻痺状態で", " while paralyzed");
                }

                auto hallucintion_state = is_hallucinated ? _("幻覚に歪んだ", "hallucinatingly distorted ") : "";
#ifdef JP
                player_ptr->died_from = format("%s%s%s", paralysis_state, hallucintion_state, hit_from.data());
#else
                player_ptr->died_from = format("%s%s%s", hallucintion_state, hit_from.data(), paralysis_state);
#endif
            }

            w_ptr->total_winner = false;
            if (winning_seppuku) {
                add_retired_class(player_ptr->pclass);
                exe_write_diary(player_ptr, DiaryKind::DESCRIPTION, 0, _("勝利の後切腹した。", "committed seppuku after the winning."));
            } else {
                std::string place;

                if (floor.inside_arena) {
                    place = _("アリーナ", "in the Arena");
                } else if (!floor.is_in_dungeon()) {
                    place = _("地上", "on the surface");
                } else if (inside_quest(q_idx) && (QuestType::is_fixed(q_idx) && !((q_idx == QuestId::OBERON) || (q_idx == QuestId::SERPENT)))) {
                    place = _("クエスト", "in a quest");
                } else {
                    place = format(_("%d階", "on level %d"), static_cast<int>(floor.dun_level));
                }

#ifdef JP
                const auto note = format("%sで%sに殺された。", place.data(), player_ptr->died_from.data());
#else
                const auto note = format("killed by %s %s.", player_ptr->died_from.data(), place.data());
#endif
                exe_write_diary(player_ptr, DiaryKind::DESCRIPTION, 0, note);
            }

            exe_write_diary(player_ptr, DiaryKind::GAMESTART, 1, _("-------- ゲームオーバー --------", "--------   Game  Over   --------"));
            exe_write_diary(player_ptr, DiaryKind::DESCRIPTION, 1, "\n\n\n\n");
            flush();
            if (input_check_strict(player_ptr, _("画面を保存しますか？", "Dump the screen? "), UserCheck::NO_HISTORY)) {
                do_cmd_save_screen(player_ptr);
            }

            flush();
            player_ptr->last_message = "";
            if (!last_words) {
#ifdef JP
                msg_format("あなたは%sました。", android ? "壊れ" : "死に");
#else
                msg_print(android ? "You are broken." : "You die.");
#endif

                msg_print(nullptr);
            } else {
                std::optional<std::string> death_message_opt;
                if (winning_seppuku) {
                    death_message_opt = get_random_line(_("seppuku_j.txt", "seppuku.txt"), 0);
                } else {
                    death_message_opt = get_random_line(_("death_j.txt", "death.txt"), 0);
                }

                auto &death_message = death_message_opt.value();
                constexpr auto max_last_words = 1024;
                const auto prompt = winning_seppuku ? _("辞世の句: ", "Haiku: ") : _("断末魔の叫び: ", "Last words: ");
                while (true) {
                    const auto input_last_words = input_string(prompt, max_last_words, death_message);
                    if (!input_last_words.has_value()) {
                        continue;
                    }

                    if (input_check_strict(player_ptr, _("よろしいですか？", "Are you sure? "), UserCheck::NO_HISTORY)) {
                        death_message = input_last_words.value();
                        break;
                    }
                }

                if (death_message.empty()) {
#ifdef JP
                    death_message = format("あなたは%sました。", android ? "壊れ" : "死に");
#else
                    death_message = android ? "You are broken." : "You die.";
#endif
                } else {
                    player_ptr->last_message = death_message;
                }

#ifdef JP
                if (winning_seppuku) {
                    int i, len;
                    int w = game_term->wid;
                    int h = game_term->hgt;
                    int msg_pos_x[9] = { 5, 7, 9, 12, 14, 17, 19, 21, 23 };
                    int msg_pos_y[9] = { 3, 4, 5, 4, 5, 4, 5, 6, 4 };
                    term_clear();

                    /* 桜散る */
                    for (i = 0; i < 40; i++) {
                        term_putstr(randint0(w / 2) * 2, randint0(h), 2, TERM_VIOLET, "υ");
                    }

                    auto str = death_message.data();
                    if (strncmp(str, "「", 2) == 0) {
                        str += 2;
                    }

                    auto *str2 = angband_strstr(str, "」");
                    if (str2 != nullptr) {
                        *str2 = '\0';
                    }

                    i = 0;
                    while (i < 9) {
                        str2 = angband_strstr(str, " ");
                        if (str2 == nullptr) {
                            len = strlen(str);
                        } else {
                            len = str2 - str;
                        }

                        if (len != 0) {
                            term_putstr_v(w * 3 / 4 - 2 - msg_pos_x[i] * 2, msg_pos_y[i], len, TERM_WHITE, str);
                            if (str2 == nullptr) {
                                break;
                            }
                            i++;
                        }
                        str = str2 + 1;
                        if (*str == 0) {
                            break;
                        }
                    }

                    term_putstr(w - 1, h - 1, 1, TERM_WHITE, " ");
                    flush();
#ifdef WORLD_SCORE
                    screen_dump = make_screen_dump(player_ptr);
#endif
                    (void)inkey();
                } else
#endif
                    msg_print(death_message);
            }
        }

        return damage;
    }

    handle_stuff(player_ptr);
    if (player_ptr->chp < warning) {
        if (old_chp > warning) {
            bell();
        }

        sound(SOUND_WARN);
        if (record_danger && (old_chp > warning)) {
            if (player_ptr->effects()->hallucination()->is_hallucinated() && damage_type == DAMAGE_ATTACK) {
                hit_from = _("何か", "something");
            }

            std::stringstream ss;
            ss << _(hit_from, "was in a critical situation because of ");
            ss << _("によってピンチに陥った。", hit_from);
            ss << _("", ".");
            exe_write_diary(player_ptr, DiaryKind::DESCRIPTION, 0, ss.str());
        }

        if (auto_more) {
            player_ptr->now_damaged = true;
        }

        msg_print(_("*** 警告:低ヒット・ポイント！ ***", "*** LOW HITPOINT WARNING! ***"));
        msg_print(nullptr);
        flush();
    }

    if (player_ptr->wild_mode && !player_ptr->leaving && (player_ptr->chp < std::max(warning, player_ptr->mhp / 5))) {
        change_wild_mode(player_ptr, false);
    }

    return damage;
}

/*!
 * @brief 属性に応じた敵オーラによるプレイヤーのダメージ処理
 * @param m_ptr オーラを持つモンスターの構造体参照ポインタ
 * @param immune ダメージを回避できる免疫フラグ
 * @param flags_offset オーラフラグ配列の参照オフセット
 * @param r_flags_offset モンスターの耐性配列の参照オフセット
 * @param aura_flag オーラフラグ配列
 * @param dam_func ダメージ処理を行う関数の参照ポインタ
 * @param message オーラダメージを受けた際のメッセージ
 */
static void process_aura_damage(MonsterEntity *m_ptr, PlayerType *player_ptr, bool immune, MonsterAuraType aura_flag, dam_func dam_func, concptr message)
{
    auto *r_ptr = &m_ptr->get_monrace();
    if (r_ptr->aura_flags.has_not(aura_flag) || immune) {
        return;
    }

    int aura_damage = damroll(1 + (r_ptr->level / 26), 1 + (r_ptr->level / 17));
    msg_print(message);
    (*dam_func)(player_ptr, aura_damage, monster_desc(player_ptr, m_ptr, MD_WRONGDOER_NAME).data(), true);
    if (is_original_ap_and_seen(player_ptr, m_ptr)) {
        r_ptr->r_aura_flags.set(aura_flag);
    }

    handle_stuff(player_ptr);
}

/*!
 * @brief 敵オーラによるプレイヤーのダメージ処理
 * @param m_ptr オーラを持つモンスターの構造体参照ポインタ
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void touch_zap_player(MonsterEntity *m_ptr, PlayerType *player_ptr)
{
    constexpr auto fire_mes = _("突然とても熱くなった！", "You are suddenly very hot!");
    constexpr auto cold_mes = _("突然とても寒くなった！", "You are suddenly very cold!");
    constexpr auto elec_mes = _("電撃をくらった！", "You get zapped!");
    process_aura_damage(m_ptr, player_ptr, has_immune_fire(player_ptr) != 0, MonsterAuraType::FIRE, fire_dam, fire_mes);
    process_aura_damage(m_ptr, player_ptr, has_immune_cold(player_ptr) != 0, MonsterAuraType::COLD, cold_dam, cold_mes);
    process_aura_damage(m_ptr, player_ptr, has_immune_elec(player_ptr) != 0, MonsterAuraType::ELEC, elec_dam, elec_mes);
}
