#include "player/player-damage.h"
#include "autopick/autopick-pref-processor.h"
#include "avatar/avatar.h"
#include "blue-magic/blue-magic-checker.h"
#include "cmd-io/cmd-process-screen.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon.h"
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
#include "object/object-flags.h"
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
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"

using dam_func = int (*)(PlayerType *player_ptr, int dam, concptr kb_str, bool aura);

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
    ObjectType *o_ptr = nullptr;
    switch (randint1(7)) {
    case 1:
        o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND];
        break;
    case 2:
        o_ptr = &player_ptr->inventory_list[INVEN_SUB_HAND];
        break;
    case 3:
        o_ptr = &player_ptr->inventory_list[INVEN_BODY];
        break;
    case 4:
        o_ptr = &player_ptr->inventory_list[INVEN_OUTER];
        break;
    case 5:
        o_ptr = &player_ptr->inventory_list[INVEN_ARMS];
        break;
    case 6:
        o_ptr = &player_ptr->inventory_list[INVEN_HEAD];
        break;
    case 7:
        o_ptr = &player_ptr->inventory_list[INVEN_FEET];
        break;
    }

    if ((o_ptr == nullptr) || (o_ptr->k_idx == 0) || !o_ptr->is_armour())
        return false;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, OD_OMIT_PREFIX | OD_NAME_ONLY);
    auto flgs = object_flags(o_ptr);
    if (o_ptr->ac + o_ptr->to_a <= 0) {
        msg_format(_("%sは既にボロボロだ！", "Your %s is already fully corroded!"), o_name);
        return false;
    }

    if (flgs.has(TR_IGNORE_ACID)) {
        msg_format(_("しかし%sには効果がなかった！", "Your %s is unaffected!"), o_name);
        return true;
    }

    msg_format(_("%sが酸で腐食した！", "Your %s is corroded!"), o_name);
    o_ptr->to_a--;
    player_ptr->update |= PU_BONUS;
    player_ptr->window_flags |= PW_EQUIP | PW_PLAYER;
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
int acid_dam(PlayerType *player_ptr, int dam, concptr kb_str, bool aura)
{
    int inv = (dam < 30) ? 1 : (dam < 60) ? 2
                                          : 3;
    bool double_resist = is_oppose_acid(player_ptr);
    dam = dam * calc_acid_damage_rate(player_ptr) / 100;
    if (dam <= 0)
        return 0;

    if (aura || !check_multishadow(player_ptr)) {
        if ((!(double_resist || has_resist_acid(player_ptr))) && one_in_(HURT_CHANCE))
            (void)do_dec_stat(player_ptr, A_CHR);

        if (acid_minus_ac(player_ptr))
            dam = (dam + 1) / 2;
    }

    int get_damage = take_hit(player_ptr, aura ? DAMAGE_NOESCAPE : DAMAGE_ATTACK, dam, kb_str);
    if (!aura && !(double_resist && has_resist_acid(player_ptr)))
        inventory_damage(player_ptr, BreakerAcid(), inv);

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
int elec_dam(PlayerType *player_ptr, int dam, concptr kb_str, bool aura)
{
    int inv = (dam < 30) ? 1 : (dam < 60) ? 2
                                          : 3;
    bool double_resist = is_oppose_elec(player_ptr);

    dam = dam * calc_elec_damage_rate(player_ptr) / 100;

    if (dam <= 0)
        return 0;

    if (aura || !check_multishadow(player_ptr)) {
        if ((!(double_resist || has_resist_elec(player_ptr))) && one_in_(HURT_CHANCE))
            (void)do_dec_stat(player_ptr, A_DEX);
    }

    int get_damage = take_hit(player_ptr, aura ? DAMAGE_NOESCAPE : DAMAGE_ATTACK, dam, kb_str);
    if (!aura && !(double_resist && has_resist_elec(player_ptr)))
        inventory_damage(player_ptr, BreakerElec(), inv);

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
int fire_dam(PlayerType *player_ptr, int dam, concptr kb_str, bool aura)
{
    int inv = (dam < 30) ? 1 : (dam < 60) ? 2
                                          : 3;
    bool double_resist = is_oppose_fire(player_ptr);

    /* Totally immune */
    if (has_immune_fire(player_ptr) || (dam <= 0))
        return 0;

    dam = dam * calc_fire_damage_rate(player_ptr) / 100;
    if (aura || !check_multishadow(player_ptr)) {
        if ((!(double_resist || has_resist_fire(player_ptr))) && one_in_(HURT_CHANCE))
            (void)do_dec_stat(player_ptr, A_STR);
    }

    int get_damage = take_hit(player_ptr, aura ? DAMAGE_NOESCAPE : DAMAGE_ATTACK, dam, kb_str);
    if (!aura && !(double_resist && has_resist_fire(player_ptr)))
        inventory_damage(player_ptr, BreakerFire(), inv);

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
int cold_dam(PlayerType *player_ptr, int dam, concptr kb_str, bool aura)
{
    int inv = (dam < 30) ? 1 : (dam < 60) ? 2
                                          : 3;
    bool double_resist = is_oppose_cold(player_ptr);
    if (has_immune_cold(player_ptr) || (dam <= 0))
        return 0;

    dam = dam * calc_cold_damage_rate(player_ptr) / 100;
    if (aura || !check_multishadow(player_ptr)) {
        if ((!(double_resist || has_resist_cold(player_ptr))) && one_in_(HURT_CHANCE))
            (void)do_dec_stat(player_ptr, A_STR);
    }

    int get_damage = take_hit(player_ptr, aura ? DAMAGE_NOESCAPE : DAMAGE_ATTACK, dam, kb_str);
    if (!aura && !(double_resist && has_resist_cold(player_ptr)))
        inventory_damage(player_ptr, BreakerCold(), inv);

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
int take_hit(PlayerType *player_ptr, int damage_type, int damage, concptr hit_from)
{
    int old_chp = player_ptr->chp;

    char death_message[1024];
    char tmp[1024];

    int warning = (player_ptr->mhp * hitpoint_warn / 10);
    if (player_ptr->is_dead)
        return 0;

    if (player_ptr->sutemi)
        damage *= 2;
    if (PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::IAI))
        damage += (damage + 4) / 5;

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
                if ((damage == 0) && one_in_(2))
                    damage = 1;
            }
        }

        if (PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::MUSOU)) {
            damage /= 2;
            if ((damage == 0) && one_in_(2))
                damage = 1;
        }
    }

    player_ptr->chp -= damage;
    if (player_ptr->chp < -9999)
        player_ptr->chp = -9999;
    if (damage_type == DAMAGE_GENO && player_ptr->chp < 0) {
        damage += player_ptr->chp;
        player_ptr->chp = 0;
    }

    player_ptr->redraw |= PR_HP;
    player_ptr->window_flags |= PW_PLAYER;

    if (damage_type != DAMAGE_GENO && player_ptr->chp == 0) {
        chg_virtue(player_ptr, V_SACRIFICE, 1);
        chg_virtue(player_ptr, V_CHANCE, 2);
    }

    if (player_ptr->chp < 0 && !cheat_immortal) {
        bool android = PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID);

        /* 死んだ時に強制終了して死を回避できなくしてみた by Habu */
        if (!cheat_save && !save_player(player_ptr, SAVE_TYPE_CLOSE_GAME))
            msg_print(_("セーブ失敗！", "death save failed!"));

        sound(SOUND_DEATH);
        chg_virtue(player_ptr, V_SACRIFICE, 10);
        handle_stuff(player_ptr);
        player_ptr->leaving = true;
        if (!cheat_immortal)
            player_ptr->is_dead = true;
        if (player_ptr->current_floor_ptr->inside_arena) {
            concptr m_name = r_info[arena_info[player_ptr->arena_number].r_idx].name.c_str();
            msg_format(_("あなたは%sの前に敗れ去った。", "You are beaten by %s."), m_name);
            msg_print(nullptr);
            if (record_arena)
                exe_write_diary(player_ptr, DIARY_ARENA, -1 - player_ptr->arena_number, m_name);
        } else {
            auto q_idx = quest_number(player_ptr, player_ptr->current_floor_ptr->dun_level);
            bool seppuku = streq(hit_from, "Seppuku");
            bool winning_seppuku = w_ptr->total_winner && seppuku;

            play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_GAMEOVER);

#ifdef WORLD_SCORE
            screen_dump = make_screen_dump(player_ptr);
#endif
            if (seppuku) {
                strcpy(player_ptr->died_from, hit_from);
#ifdef JP
                if (!winning_seppuku)
                    strcpy(player_ptr->died_from, "切腹");
#endif
            } else {
                char dummy[1024];
#ifdef JP
                sprintf(dummy, "%s%s%s",
                    !player_ptr->paralyzed ? ""
                    : player_ptr->free_act ? "彫像状態で"
                                           : "麻痺状態で",
                    player_ptr->hallucinated ? "幻覚に歪んだ" : "", hit_from);
#else
                sprintf(dummy, "%s%s", hit_from, !player_ptr->paralyzed ? "" : " while helpless");
#endif
                angband_strcpy(player_ptr->died_from, dummy, sizeof player_ptr->died_from);
            }

            w_ptr->total_winner = false;
            if (winning_seppuku) {
                add_retired_class(player_ptr->pclass);
                exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, _("勝利の後切腹した。", "committed seppuku after the winning."));
            } else {
                char buf[20];

                if (player_ptr->current_floor_ptr->inside_arena)
                    strcpy(buf, _("アリーナ", "in the Arena"));
                else if (!is_in_dungeon(player_ptr))
                    strcpy(buf, _("地上", "on the surface"));
                else if (inside_quest(q_idx) && (quest_type::is_fixed(q_idx) && !((q_idx == QuestId::OBERON) || (q_idx == QuestId::SERPENT))))
                    strcpy(buf, _("クエスト", "in a quest"));
                else
                    sprintf(buf, _("%d階", "level %d"), (int)player_ptr->current_floor_ptr->dun_level);

                sprintf(tmp, _("%sで%sに殺された。", "killed by %s %s."), buf, player_ptr->died_from);
                exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, tmp);
            }

            exe_write_diary(player_ptr, DIARY_GAMESTART, 1, _("-------- ゲームオーバー --------", "--------   Game  Over   --------"));
            exe_write_diary(player_ptr, DIARY_DESCRIPTION, 1, "\n\n\n\n");
            flush();
            if (get_check_strict(player_ptr, _("画面を保存しますか？", "Dump the screen? "), CHECK_NO_HISTORY))
                do_cmd_save_screen(player_ptr);

            flush();
            if (player_ptr->last_message)
                string_free(player_ptr->last_message);

            player_ptr->last_message = nullptr;
            if (!last_words) {
#ifdef JP
                msg_format("あなたは%sました。", android ? "壊れ" : "死に");
#else
                msg_print(android ? "You are broken." : "You die.");
#endif

                msg_print(nullptr);
            } else {
                if (winning_seppuku) {
                    get_rnd_line(_("seppuku_j.txt", "seppuku.txt"), 0, death_message);
                } else {
                    get_rnd_line(_("death_j.txt", "death.txt"), 0, death_message);
                }

                do {
#ifdef JP
                    while (!get_string(winning_seppuku ? "辞世の句: " : "断末魔の叫び: ", death_message, sizeof(death_message)))
                        ;
#else
                    while (!get_string("Last words: ", death_message, 1024))
                        ;
#endif
                } while (winning_seppuku && !get_check_strict(player_ptr, _("よろしいですか？", "Are you sure? "), CHECK_NO_HISTORY));

                if (death_message[0] == '\0') {
#ifdef JP
                    strcpy(death_message, format("あなたは%sました。", android ? "壊れ" : "死に"));
#else
                    strcpy(death_message, android ? "You are broken." : "You die.");
#endif
                } else
                    player_ptr->last_message = string_make(death_message);

#ifdef JP
                if (winning_seppuku) {
                    int i, len;
                    int w = game_term->wid;
                    int h = game_term->hgt;
                    int msg_pos_x[9] = { 5, 7, 9, 12, 14, 17, 19, 21, 23 };
                    int msg_pos_y[9] = { 3, 4, 5, 4, 5, 4, 5, 6, 4 };
                    concptr str;
                    char *str2;

                    term_clear();

                    /* 桜散る */
                    for (i = 0; i < 40; i++)
                        term_putstr(randint0(w / 2) * 2, randint0(h), 2, TERM_VIOLET, "υ");

                    str = death_message;
                    if (strncmp(str, "「", 2) == 0)
                        str += 2;

                    str2 = angband_strstr(str, "」");
                    if (str2 != nullptr)
                        *str2 = '\0';

                    i = 0;
                    while (i < 9) {
                        str2 = angband_strstr(str, " ");
                        if (str2 == nullptr)
                            len = strlen(str);
                        else
                            len = str2 - str;

                        if (len != 0) {
                            term_putstr_v(w * 3 / 4 - 2 - msg_pos_x[i] * 2, msg_pos_y[i], len, TERM_WHITE, str);
                            if (str2 == nullptr)
                                break;
                            i++;
                        }
                        str = str2 + 1;
                        if (*str == 0)
                            break;
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
        if (old_chp > warning)
            bell();

        sound(SOUND_WARN);
        if (record_danger && (old_chp > warning)) {
            if (player_ptr->hallucinated && damage_type == DAMAGE_ATTACK)
                hit_from = _("何か", "something");

            sprintf(tmp, _("%sによってピンチに陥った。", "was in a critical situation because of %s."), hit_from);
            exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, tmp);
        }

        if (auto_more)
            player_ptr->now_damaged = true;

        msg_print(_("*** 警告:低ヒット・ポイント！ ***", "*** LOW HITPOINT WARNING! ***"));
        msg_print(nullptr);
        flush();
    }

    if (player_ptr->wild_mode && !player_ptr->leaving && (player_ptr->chp < std::max(warning, player_ptr->mhp / 5)))
        change_wild_mode(player_ptr, false);

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
static void process_aura_damage(monster_type *m_ptr, PlayerType *player_ptr, bool immune, MonsterAuraType aura_flag, dam_func dam_func, concptr message)
{
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (r_ptr->aura_flags.has_not(aura_flag) || immune) {
        return;
    }

    GAME_TEXT mon_name[MAX_NLEN];
    int aura_damage = damroll(1 + (r_ptr->level / 26), 1 + (r_ptr->level / 17));
    monster_desc(player_ptr, mon_name, m_ptr, MD_WRONGDOER_NAME);
    msg_print(message);
    (*dam_func)(player_ptr, aura_damage, mon_name, true);
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
void touch_zap_player(monster_type *m_ptr, PlayerType *player_ptr)
{
    process_aura_damage(m_ptr, player_ptr, has_immune_fire(player_ptr) != 0, MonsterAuraType::FIRE, fire_dam, _("突然とても熱くなった！", "You are suddenly very hot!"));
    process_aura_damage(m_ptr, player_ptr, has_immune_cold(player_ptr) != 0, MonsterAuraType::COLD, cold_dam, _("突然とても寒くなった！", "You are suddenly very cold!"));
    process_aura_damage(m_ptr, player_ptr, has_immune_elec(player_ptr) != 0, MonsterAuraType::ELEC, elec_dam, _("電撃をくらった！", "You get zapped!"));
}
