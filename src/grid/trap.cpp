#include "grid/trap.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-io/cmd-save.h"
#include "core/disturbance.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/floor-mode-changer.h"
#include "game-option/birth-options.h"
#include "game-option/special-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "info-reader/feature-reader.h"
#include "io/files-util.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-mirror-master.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-util.h"
#include "player-info/class-info.h"
#include "player/eldritch-horror.h"
#include "player/player-damage.h"
#include "player/player-personality-types.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spell-types.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/element-resistance.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

static int16_t normal_traps[MAX_NORMAL_TRAPS];

/*!
 * @brief 箱のトラップテーブル
 * @details
 * <pre>
 * Each chest has a certain set of traps, determined by pval
 * Each chest has a "pval" from 1 to the chest level (max 55)
 * If the "pval" is negative then the trap has been disarmed
 * The "pval" of a chest determines the quality of its treasure
 * Note that disarming a trap on a chest also removes the lock.
 * </pre>
 */
const int chest_traps[64] = {
    0, /* 0 == empty */
    (CHEST_POISON),
    (CHEST_LOSE_STR),
    (CHEST_LOSE_CON),
    (CHEST_LOSE_STR),
    (CHEST_LOSE_CON), /* 5 == best small wooden */
    0,
    (CHEST_ALARM),
    (CHEST_ALARM),
    (CHEST_LOSE_STR),
    (CHEST_LOSE_CON),
    (CHEST_POISON),
    (CHEST_SCATTER),
    (CHEST_LOSE_STR | CHEST_LOSE_CON),
    (CHEST_LOSE_STR | CHEST_LOSE_CON),
    (CHEST_SUMMON), /* 15 == best large wooden */
    0,
    (CHEST_ALARM),
    (CHEST_SCATTER),
    (CHEST_PARALYZE),
    (CHEST_LOSE_STR | CHEST_LOSE_CON),
    (CHEST_SUMMON),
    (CHEST_PARALYZE),
    (CHEST_LOSE_STR),
    (CHEST_LOSE_CON),
    (CHEST_EXPLODE), /* 25 == best small iron */
    0,
    (CHEST_E_SUMMON),
    (CHEST_POISON | CHEST_LOSE_CON),
    (CHEST_LOSE_STR | CHEST_LOSE_CON),
    (CHEST_EXPLODE | CHEST_SUMMON),
    (CHEST_BIRD_STORM),
    (CHEST_POISON | CHEST_SUMMON),
    (CHEST_E_SUMMON | CHEST_ALARM),
    (CHEST_EXPLODE),
    (CHEST_EXPLODE | CHEST_SUMMON), /* 35 == best large iron */
    0,
    (CHEST_SUMMON | CHEST_ALARM),
    (CHEST_EXPLODE),
    (CHEST_EXPLODE | CHEST_SUMMON),
    (CHEST_EXPLODE | CHEST_SUMMON),
    (CHEST_POISON | CHEST_PARALYZE),
    (CHEST_EXPLODE),
    (CHEST_BIRD_STORM),
    (CHEST_EXPLODE | CHEST_E_SUMMON | CHEST_ALARM),
    (CHEST_H_SUMMON), /* 45 == best small steel */
    0,
    (CHEST_EXPLODE | CHEST_SUMMON | CHEST_ALARM),
    (CHEST_BIRD_STORM),
    (CHEST_RUNES_OF_EVIL),
    (CHEST_EXPLODE | CHEST_SUMMON | CHEST_ALARM),
    (CHEST_BIRD_STORM | CHEST_ALARM),
    (CHEST_H_SUMMON | CHEST_ALARM),
    (CHEST_RUNES_OF_EVIL),
    (CHEST_H_SUMMON | CHEST_SCATTER | CHEST_ALARM),
    (CHEST_RUNES_OF_EVIL | CHEST_EXPLODE), /* 55 == best large steel */
    (CHEST_EXPLODE | CHEST_SUMMON),
    (CHEST_EXPLODE | CHEST_SUMMON),
    (CHEST_EXPLODE | CHEST_SUMMON),
    (CHEST_EXPLODE | CHEST_SUMMON),
    (CHEST_EXPLODE | CHEST_SUMMON),
    (CHEST_EXPLODE | CHEST_SUMMON),
    (CHEST_EXPLODE | CHEST_SUMMON),
    (CHEST_EXPLODE | CHEST_SUMMON),
};

/*!
 * @brief タグに従って、基本トラップテーブルを初期化する / Initialize arrays for normal traps
 */
void init_normal_traps(void)
{
    int cur_trap = 0;

    normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_TRAPDOOR");
    normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_PIT");
    normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_SPIKED_PIT");
    normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_POISON_PIT");
    normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_TY_CURSE");
    normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_TELEPORT");
    normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_FIRE");
    normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_ACID");
    normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_SLOW");
    normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_LOSE_STR");
    normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_LOSE_DEX");
    normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_LOSE_CON");
    normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_BLIND");
    normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_CONFUSE");
    normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_POISON");
    normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_SLEEP");
    normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_TRAPS");
    normal_traps[cur_trap++] = f_tag_to_index_in_init("TRAP_ALARM");
}

/*!
 * @brief 基本トラップをランダムに選択する /
 * Get random trap
 * @return 選択したトラップのID
 * @details
 * This routine should be redone to reflect trap "level".\n
 * That is, it does not make sense to have spiked pits at 50 feet.\n
 * Actually, it is not this routine, but the "trap instantiation"\n
 * code, which should also check for "trap doors" on quest levels.\n
 */
FEAT_IDX choose_random_trap(player_type *trapped_ptr)
{
    FEAT_IDX feat;

    /* Pick a trap */
    floor_type *floor_ptr = trapped_ptr->current_floor_ptr;
    while (true) {
        /* Hack -- pick a trap */
        feat = normal_traps[randint0(MAX_NORMAL_TRAPS)];

        /* Accept non-trapdoors */
        if (f_info[feat].flags.has_not(FF::MORE))
            break;

        /* Hack -- no trap doors on special levels */
        if (floor_ptr->inside_arena || quest_number(trapped_ptr, floor_ptr->dun_level))
            continue;

        /* Hack -- no trap doors on the deepest level */
        if (floor_ptr->dun_level >= d_info[floor_ptr->dungeon_idx].maxdepth)
            continue;

        break;
    }

    return feat;
}

/*!
 * @brief マスに存在する隠しトラップを公開する /
 * Disclose an invisible trap
 * @param player
 * @param y 秘匿したいマスのY座標
 * @param x 秘匿したいマスのX座標
 */
void disclose_grid(player_type *trapped_ptr, POSITION y, POSITION x)
{
    grid_type *g_ptr = &trapped_ptr->current_floor_ptr->grid_array[y][x];

    if (g_ptr->cave_has_flag(FF::SECRET)) {
        /* No longer hidden */
        cave_alter_feat(trapped_ptr, y, x, FF::SECRET);
    } else if (g_ptr->mimic) {
        /* No longer hidden */
        g_ptr->mimic = 0;

        note_spot(trapped_ptr, y, x);
        lite_spot(trapped_ptr, y, x);
    }
}

/*!
 * @brief マスをトラップを配置する /
 * The location must be a legal, naked, floor grid.
 * @param y 配置したいマスのY座標
 * @param x 配置したいマスのX座標
 * @return
 * Note that all traps start out as "invisible" and "untyped", and then\n
 * when they are "discovered" (by detecting them or setting them off),\n
 * the trap is "instantiated" as a visible, "typed", trap.\n
 */
void place_trap(player_type *trapped_ptr, POSITION y, POSITION x)
{
    floor_type *floor_ptr = trapped_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];

    /* Paranoia -- verify location */
    if (!in_bounds(floor_ptr, y, x))
        return;

    /* Require empty, clean, floor grid */
    if (!cave_clean_bold(floor_ptr, y, x))
        return;

    /* Place an invisible trap */
    g_ptr->mimic = g_ptr->feat;
    g_ptr->feat = choose_random_trap(trapped_ptr);
}

/*!
 * @brief プレイヤーへのトラップ命中判定 /
 * Determine if a trap affects the player.
 * @param power 基本回避難度
 * @return トラップが命中した場合TRUEを返す。
 * @details
 * Always miss 5% of the time, Always hit 5% of the time.
 * Otherwise, match trap power against player armor.
 */
static int check_hit_from_monster_to_player(player_type *target_ptr, int power)
{
    int k;
    ARMOUR_CLASS ac;

    /* Percentile dice */
    k = randint0(100);

    /* Hack -- 5% hit, 5% miss */
    if (k < 10)
        return (k < 5);

    if (target_ptr->pseikaku == PERSONALITY_LAZY)
        if (one_in_(20))
            return true;

    /* Paranoia -- No power */
    if (power <= 0)
        return false;

    /* Total armor */
    ac = target_ptr->ac + target_ptr->to_a;

    /* Power competes against Armor */
    if (randint1(power) > ((ac * 3) / 4))
        return true;

    /* Assume miss */
    return false;
}

/*!
 * @brief 落とし穴系トラップの判定とプレイヤーの被害処理
 * @param trap_feat_type トラップの種別ID
 */
static void hit_trap_pit(player_type *trapped_ptr, enum trap_type trap_feat_type)
{
    HIT_POINT dam;
    concptr trap_name = "";
    concptr spike_name = "";

    switch (trap_feat_type) {
    case TRAP_PIT:
        trap_name = _("落とし穴", "a pit trap");
        break;
    case TRAP_SPIKED_PIT:
        trap_name = _("スパイクが敷かれた落とし穴", "a spiked pit");
        spike_name = _("スパイク", "spikes");
        break;
    case TRAP_POISON_PIT:
        trap_name = _("スパイクが敷かれた落とし穴", "a spiked pit");
        spike_name = _("毒を塗られたスパイク", "poisonous spikes");
        break;
    default:
        return;
    }

    if (trapped_ptr->levitation) {
        msg_format(_("%sを飛び越えた。", "You fly over %s."), trap_name);
        return;
    }

    msg_format(_("%sに落ちてしまった！", "You have fallen into %s!"), trap_name);

    /* Base damage */
    dam = damroll(2, 6);

    /* Extra spike damage */
    if ((trap_feat_type == TRAP_SPIKED_PIT || trap_feat_type == TRAP_POISON_PIT) && one_in_(2)) {
        msg_format(_("%sが刺さった！", "You are impaled on %s!"), spike_name);

        dam = dam * 2;
        (void)set_cut(trapped_ptr, trapped_ptr->cut + randint1(dam));

        if (trap_feat_type == TRAP_POISON_PIT) {
            if (has_resist_pois(trapped_ptr) || is_oppose_pois(trapped_ptr)) {
                msg_print(_("しかし毒の影響はなかった！", "The poison does not affect you!"));
            } else {
                dam = dam * 2;
                (void)set_poisoned(trapped_ptr, trapped_ptr->poisoned + randint1(dam));
            }
        }
    }

    take_hit(trapped_ptr, DAMAGE_NOESCAPE, dam, trap_name);
}

/*!
 * @brief ダーツ系トラップ（通常ダメージ）の判定とプレイヤーの被害処理
 * @return ダーツが命中した場合TRUEを返す
 */
static bool hit_trap_dart(player_type *target_ptr)
{
    bool hit = false;

    if (check_hit_from_monster_to_player(target_ptr, 125)) {
        msg_print(_("小さなダーツが飛んできて刺さった！", "A small dart hits you!"));
        take_hit(target_ptr, DAMAGE_ATTACK, damroll(1, 4), _("ダーツの罠", "a dart trap"));
        if (!check_multishadow(target_ptr))
            hit = true;
    } else {
        msg_print(_("小さなダーツが飛んできた！が、運良く当たらなかった。", "A small dart barely misses you."));
    }

    return hit;
}

/*!
 * @brief ダーツ系トラップ（通常ダメージ＋能力値減少）の判定とプレイヤーの被害処理
 * @param stat 低下する能力値ID
 */
static void hit_trap_lose_stat(player_type *target_ptr, int stat)
{
    if (hit_trap_dart(target_ptr)) {
        do_dec_stat(target_ptr, stat);
    }
}

/*!
 * @brief ダーツ系トラップ（通常ダメージ＋減速）の判定とプレイヤーの被害処理
 */
static void hit_trap_slow(player_type *target_ptr)
{
    if (hit_trap_dart(target_ptr)) {
        set_slow(target_ptr, target_ptr->slow + randint0(20) + 20, false);
    }
}

/*!
 * @brief ダーツ系トラップ（通常ダメージ＋状態異常）の判定とプレイヤーの被害処理
 * @param trap_message メッセージの補完文字列
 * @param resist 状態異常に抵抗する判定が出たならTRUE
 * @param set_status 状態異常を指定する関数ポインタ
 * @param turn_aux 状態異常の追加ターン量
 */
static void hit_trap_set_abnormal_status_p(player_type *trapped_ptr, concptr trap_message, bool resist, bool (*set_status)(player_type *, IDX), IDX turn_aux)
{
    msg_print(trap_message);
    if (!resist) {
        set_status(trapped_ptr, turn_aux);
    }
}

/*!
 * @brief プレイヤーへのトラップ作動処理メインルーチン /
 * Handle player hitting a real trap
 * @param break_trap 作動後のトラップ破壊が確定しているならばTRUE
 * @todo cmd-save.h への依存あり。コールバックで何とかしたい
 */
void hit_trap(player_type *trapped_ptr, bool break_trap)
{
    int i, num, dam;
    POSITION x = trapped_ptr->x, y = trapped_ptr->y;
    grid_type *g_ptr = &trapped_ptr->current_floor_ptr->grid_array[y][x];
    feature_type *f_ptr = &f_info[g_ptr->feat];
    enum trap_type trap_feat_type = f_ptr->flags.has(FF::TRAP) ? (enum trap_type)f_ptr->subtype : NOT_TRAP;
    concptr name = _("トラップ", "a trap");

    disturb(trapped_ptr, false, true);

    cave_alter_feat(trapped_ptr, y, x, FF::HIT_TRAP);

    /* Analyze */
    switch (trap_feat_type) {
    case TRAP_TRAPDOOR: {
        if (trapped_ptr->levitation) {
            msg_print(_("落とし戸を飛び越えた。", "You fly over a trap door."));
        } else {
            msg_print(_("落とし戸に落ちた！", "You have fallen through a trap door!"));
            if (is_echizen(trapped_ptr))
                msg_print(_("くっそ～！", ""));
            else if (is_chargeman(trapped_ptr))
                msg_print(_("ジュラル星人の仕業に違いない！", ""));

            sound(SOUND_FALL);
            dam = damroll(2, 8);
            name = _("落とし戸", "a trap door");

            take_hit(trapped_ptr, DAMAGE_NOESCAPE, dam, name);

            /* Still alive and autosave enabled */
            if (autosave_l && (trapped_ptr->chp >= 0))
                do_cmd_save_game(trapped_ptr, true);

            exe_write_diary(trapped_ptr, DIARY_DESCRIPTION, 0, _("落とし戸に落ちた", "fell through a trap door!"));
            prepare_change_floor_mode(trapped_ptr, CFM_SAVE_FLOORS | CFM_DOWN | CFM_RAND_PLACE | CFM_RAND_CONNECT);
            trapped_ptr->leaving = true;
        }
        break;
    }

    case TRAP_PIT:
    case TRAP_SPIKED_PIT:
    case TRAP_POISON_PIT: {
        hit_trap_pit(trapped_ptr, trap_feat_type);
        break;
    }

    case TRAP_TY_CURSE: {
        msg_print(_("何かがピカッと光った！", "There is a flash of shimmering light!"));
        num = 2 + randint1(3);
        for (i = 0; i < num; i++) {
            (void)summon_specific(trapped_ptr, 0, y, x, trapped_ptr->current_floor_ptr->dun_level, SUMMON_NONE, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
        }

        if (trapped_ptr->current_floor_ptr->dun_level > randint1(100)) /* No nasty effect for low levels */
        {
            bool stop_ty = false;
            int count = 0;

            do {
                stop_ty = activate_ty_curse(trapped_ptr, stop_ty, &count);
            } while (one_in_(6));
        }
        break;
    }

    case TRAP_TELEPORT: {
        msg_print(_("テレポート・トラップにひっかかった！", "You hit a teleport trap!"));
        teleport_player(trapped_ptr, 100, TELEPORT_PASSIVE);
        break;
    }

    case TRAP_FIRE: {
        msg_print(_("炎に包まれた！", "You are enveloped in flames!"));
        dam = damroll(4, 6);
        (void)fire_dam(trapped_ptr, dam, _("炎のトラップ", "a fire trap"), false);
        break;
    }

    case TRAP_ACID: {
        msg_print(_("酸が吹きかけられた！", "You are splashed with acid!"));
        dam = damroll(4, 6);
        (void)acid_dam(trapped_ptr, dam, _("酸のトラップ", "an acid trap"), false);
        break;
    }

    case TRAP_SLOW: {
        hit_trap_slow(trapped_ptr);
        break;
    }

    case TRAP_LOSE_STR: {
        hit_trap_lose_stat(trapped_ptr, A_STR);
        break;
    }

    case TRAP_LOSE_DEX: {
        hit_trap_lose_stat(trapped_ptr, A_DEX);
        break;
    }

    case TRAP_LOSE_CON: {
        hit_trap_lose_stat(trapped_ptr, A_CON);
        break;
    }

    case TRAP_BLIND: {
        hit_trap_set_abnormal_status_p(trapped_ptr, _("黒いガスに包み込まれた！", "A black gas surrounds you!"), (has_resist_blind(trapped_ptr) != 0),
            set_blind, trapped_ptr->blind + (TIME_EFFECT)randint0(50) + 25);
        break;
    }

    case TRAP_CONFUSE: {
        hit_trap_set_abnormal_status_p(trapped_ptr, _("きらめくガスに包み込まれた！", "A gas of scintillating colors surrounds you!"),
            (has_resist_conf(trapped_ptr) != 0), set_confused, trapped_ptr->confused + (TIME_EFFECT)randint0(20) + 10);
        break;
    }

    case TRAP_POISON: {
        hit_trap_set_abnormal_status_p(trapped_ptr, _("刺激的な緑色のガスに包み込まれた！", "A pungent green gas surrounds you!"),
            has_resist_pois(trapped_ptr) || is_oppose_pois(trapped_ptr), set_poisoned, trapped_ptr->poisoned + (TIME_EFFECT)randint0(20) + 10);
        break;
    }

    case TRAP_SLEEP: {
        msg_print(_("奇妙な白い霧に包まれた！", "A strange white mist surrounds you!"));
        if (!trapped_ptr->free_act) {
            msg_print(_("あなたは眠りに就いた。", "You fall asleep."));

            if (ironman_nightmare) {
                msg_print(_("身の毛もよだつ光景が頭に浮かんだ。", "A horrible vision enters your mind."));

                /* Have some nightmares */
                sanity_blast(trapped_ptr, nullptr, false);
            }
            (void)set_paralyzed(trapped_ptr, trapped_ptr->paralyzed + randint0(10) + 5);
        }
        break;
    }

    case TRAP_TRAPS: {
        msg_print(_("まばゆい閃光が走った！", "There is a bright flash of light!"));
        /* Make some new traps */
        project(trapped_ptr, 0, 1, y, x, 0, GF_MAKE_TRAP, PROJECT_HIDE | PROJECT_JUMP | PROJECT_GRID);

        break;
    }

    case TRAP_ALARM: {
        msg_print(_("けたたましい音が鳴り響いた！", "An alarm sounds!"));

        aggravate_monsters(trapped_ptr, 0);

        break;
    }

    case TRAP_OPEN: {
        msg_print(_("大音響と共にまわりの壁が崩れた！", "Suddenly, surrounding walls are opened!"));
        (void)project(trapped_ptr, 0, 3, y, x, 0, GF_DISINTEGRATE, PROJECT_GRID | PROJECT_HIDE);
        (void)project(trapped_ptr, 0, 3, y, x - 4, 0, GF_DISINTEGRATE, PROJECT_GRID | PROJECT_HIDE);
        (void)project(trapped_ptr, 0, 3, y, x + 4, 0, GF_DISINTEGRATE, PROJECT_GRID | PROJECT_HIDE);
        aggravate_monsters(trapped_ptr, 0);

        break;
    }

    case TRAP_ARMAGEDDON: {
        static int levs[10] = { 0, 0, 20, 10, 5, 3, 2, 1, 1, 1 };
        int evil_idx = 0, good_idx = 0;

        DEPTH lev;
        msg_print(_("突然天界の戦争に巻き込まれた！", "Suddenly, you are surrounded by immotal beings!"));

        /* Summon Demons and Angels */
        for (lev = trapped_ptr->current_floor_ptr->dun_level; lev >= 20; lev -= 1 + lev / 16) {
            num = levs[MIN(lev / 10, 9)];
            for (i = 0; i < num; i++) {
                POSITION x1 = rand_spread(x, 7);
                POSITION y1 = rand_spread(y, 5);

                if (!in_bounds(trapped_ptr->current_floor_ptr, y1, x1))
                    continue;

                /* Require line of projection */
                if (!projectable(trapped_ptr, trapped_ptr->y, trapped_ptr->x, y1, x1))
                    continue;

                if (summon_specific(trapped_ptr, 0, y1, x1, lev, SUMMON_ARMAGE_EVIL, (PM_NO_PET)))
                    evil_idx = hack_m_idx_ii;

                if (summon_specific(trapped_ptr, 0, y1, x1, lev, SUMMON_ARMAGE_GOOD, (PM_NO_PET))) {
                    good_idx = hack_m_idx_ii;
                }

                /* Let them fight each other */
                if (evil_idx && good_idx) {
                    monster_type *evil_ptr = &trapped_ptr->current_floor_ptr->m_list[evil_idx];
                    monster_type *good_ptr = &trapped_ptr->current_floor_ptr->m_list[good_idx];
                    evil_ptr->target_y = good_ptr->fy;
                    evil_ptr->target_x = good_ptr->fx;
                    good_ptr->target_y = evil_ptr->fy;
                    good_ptr->target_x = evil_ptr->fx;
                }
            }
        }

        break;
    }

    case TRAP_PIRANHA: {
        msg_print(_("突然壁から水が溢れ出した！ピラニアがいる！", "Suddenly, the room is filled with water with piranhas!"));

        /* Water fills room */
        fire_ball_hide(trapped_ptr, GF_WATER_FLOW, 0, 1, 10);

        /* Summon Piranhas */
        num = 1 + trapped_ptr->current_floor_ptr->dun_level / 20;
        for (i = 0; i < num; i++) {
            (void)summon_specific(trapped_ptr, 0, y, x, trapped_ptr->current_floor_ptr->dun_level, SUMMON_PIRANHAS, (PM_ALLOW_GROUP | PM_NO_PET));
        }
        break;
    }

    default:
        break;
    }

    if (break_trap && is_trap(trapped_ptr, g_ptr->feat)) {
        cave_alter_feat(trapped_ptr, y, x, FF::DISARM);
        msg_print(_("トラップを粉砕した。", "You destroyed the trap."));
    }
}
