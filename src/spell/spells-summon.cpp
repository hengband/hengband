#include "spell/spells-summon.h"
#include "avatar/avatar.h"
#include "effect/spells-effect-util.h"
#include "floor/floor-object.h"
#include "floor/line-of-sight.h"
#include "game-option/birth-options.h"
#include "hpmp/hp-mp-processor.h"
#include "inventory/inventory-object.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-specific-bolt.h"
#include "spell/spell-types.h"
#include "spell/spells-diceroll.h"
#include "spell/spells-status.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "sv-definition/sv-other-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief トランプ魔法独自の召喚処理を行う / Handle summoning and failure of trump spells
 * @param num summon_specific()関数を呼び出す回数
 * @param pet ペット化として召喚されるか否か
 * @param y 召喚位置のy座標
 * @param x 召喚位置のx座標
 * @param lev 召喚レベル
 * @param type 召喚条件ID
 * @param mode モンスター生成条件フラグ
 * @return モンスターが（敵対も含めて）召還されたならばTRUEを返す。
 */
bool trump_summoning(player_type *player_ptr, int num, bool pet, POSITION y, POSITION x, DEPTH lev, summon_type type, BIT_FLAGS mode)
{
    /* Default level */
    PLAYER_LEVEL plev = player_ptr->lev;
    if (!lev)
        lev = plev * 2 / 3 + randint1(plev / 2);

    MONSTER_IDX who;
    if (pet) {
        /* Become pet */
        mode |= PM_FORCE_PET;

        /* Only sometimes allow unique monster */
        if (mode & PM_ALLOW_UNIQUE) {
            /* Forbid often */
            if (randint1(50 + plev) >= plev / 10)
                mode &= ~PM_ALLOW_UNIQUE;
        }

        /* Player is who summons */
        who = -1;
    } else {
        /* Prevent taming, allow unique monster */
        mode |= PM_NO_PET;

        /* Behave as if they appear by themselfs */
        who = 0;
    }

    bool success = false;
    for (int i = 0; i < num; i++) {
        if (summon_specific(player_ptr, who, y, x, lev, type, mode))
            success = true;
    }

    if (!success) {
        msg_print(_("誰もあなたのカードの呼び声に答えない。", "Nobody answers to your Trump call."));
    }

    return success;
}

bool cast_summon_demon(player_type *player_ptr, int power)
{
    uint32_t flg = 0L;
    bool pet = !one_in_(3);
    if (pet)
        flg |= PM_FORCE_PET;
    else
        flg |= PM_NO_PET;
    if (!(pet && (player_ptr->lev < 50)))
        flg |= PM_ALLOW_GROUP;

    if (!summon_specific(player_ptr, (pet ? -1 : 0), player_ptr->y, player_ptr->x, power, SUMMON_DEMON, flg))
        return true;

    msg_print(_("硫黄の悪臭が充満した。", "The area fills with a stench of sulphur and brimstone."));
    if (pet) {
        msg_print(_("「ご用でございますか、ご主人様」", "'What is thy bidding... Master?'"));
    } else {
        msg_print(_("「卑しき者よ、我は汝の下僕にあらず！ お前の魂を頂くぞ！」", "'NON SERVIAM! Wretch! I shall feast on thy mortal soul!'"));
    }

    return true;
}

bool cast_summon_undead(player_type *player_ptr, int power)
{
    bool pet = one_in_(3);
    summon_type type = (player_ptr->lev > 47 ? SUMMON_HI_UNDEAD : SUMMON_UNDEAD);

    BIT_FLAGS mode = 0L;
    if (!pet || ((player_ptr->lev > 24) && one_in_(3)))
        mode |= PM_ALLOW_GROUP;
    if (pet)
        mode |= PM_FORCE_PET;
    else
        mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);

    if (summon_specific(player_ptr, (pet ? -1 : 0), player_ptr->y, player_ptr->x, power, type, mode)) {
        msg_print(_("冷たい風があなたの周りに吹き始めた。それは腐敗臭を運んでいる...",
            "Cold winds begin to blow around you, carrying with them the stench of decay..."));
        if (pet)
            msg_print(_("古えの死せる者共があなたに仕えるため土から甦った！", "Ancient, long-dead forms arise from the ground to serve you!"));
        else
            msg_print(_("死者が甦った。眠りを妨げるあなたを罰するために！", "'The dead arise... to punish you for disturbing them!'"));
    }
    return true;
}

bool cast_summon_hound(player_type *player_ptr, int power)
{
    BIT_FLAGS mode = PM_ALLOW_GROUP;
    bool pet = !one_in_(5);
    if (pet)
        mode |= PM_FORCE_PET;
    else
        mode |= PM_NO_PET;

    if (summon_specific(player_ptr, (pet ? -1 : 0), player_ptr->y, player_ptr->x, power, SUMMON_HOUND, mode)) {
        if (pet)
            msg_print(_("ハウンドがあなたの下僕として出現した。", "A group of hounds appear as your servants."));
        else
            msg_print(_("ハウンドはあなたに牙を向けている！", "A group of hounds appear as your enemies!"));
    }

    return true;
}

bool cast_summon_elemental(player_type *player_ptr, int power)
{
    bool pet = one_in_(3);
    BIT_FLAGS mode = 0L;
    if (!(pet && (player_ptr->lev < 50)))
        mode |= PM_ALLOW_GROUP;
    if (pet)
        mode |= PM_FORCE_PET;
    else
        mode |= PM_NO_PET;

    if (summon_specific(player_ptr, (pet ? -1 : 0), player_ptr->y, player_ptr->x, power, SUMMON_ELEMENTAL, mode)) {
        msg_print(_("エレメンタルが現れた...", "An elemental materializes..."));
        if (pet)
            msg_print(_("あなたに服従しているようだ。", "It seems obedient to you."));
        else
            msg_print(_("それをコントロールできなかった！", "You fail to control it!"));
    }

    return true;
}

bool cast_summon_octopus(player_type *player_ptr)
{
    BIT_FLAGS mode = PM_ALLOW_GROUP;
    bool pet = !one_in_(5);
    if (pet)
        mode |= PM_FORCE_PET;
    if (summon_named_creature(player_ptr, 0, player_ptr->y, player_ptr->x, MON_JIZOTAKO, mode)) {
        if (pet)
            msg_print(_("蛸があなたの下僕として出現した。", "A group of octopuses appear as your servants."));
        else
            msg_print(_("蛸はあなたを睨んでいる！", "A group of octopuses appear as your enemies!"));
    }

    return true;
}

/*!
 * @brief 悪魔領域のグレーターデーモン召喚を処理する / Daemon spell Summon Greater Demon
 * @return 処理を実行したならばTRUEを返す。
 */
bool cast_summon_greater_demon(player_type *player_ptr)
{
    concptr q = _("どの死体を捧げますか? ", "Sacrifice which corpse? ");
    concptr s = _("捧げられる死体を持っていない。", "You have nothing to scrifice.");
    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), FuncItemTester(&object_type::is_offerable));
    if (!o_ptr)
        return false;

    PLAYER_LEVEL plev = player_ptr->lev;
    int summon_lev = plev * 2 / 3 + r_info[o_ptr->pval].level;

    if (summon_specific(player_ptr, -1, player_ptr->y, player_ptr->x, summon_lev, SUMMON_HI_DEMON, (PM_ALLOW_GROUP | PM_FORCE_PET))) {
        msg_print(_("硫黄の悪臭が充満した。", "The area fills with a stench of sulphur and brimstone."));
        msg_print(_("「ご用でございますか、ご主人様」", "'What is thy bidding... Master?'"));
        vary_item(player_ptr, item, -1);
    } else {
        msg_print(_("悪魔は現れなかった。", "No Greater Demon arrives."));
    }

    return true;
}

/*!
 * @brief 同族召喚(援軍)処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param level 召喚基準レベル
 * @param y 召喚先Y座標
 * @param x 召喚先X座標
 * @param mode 召喚オプション
 * @return ターンを消費した場合TRUEを返す
 */
bool summon_kin_player(player_type *player_ptr, DEPTH level, POSITION y, POSITION x, BIT_FLAGS mode)
{
    bool pet = (bool)(mode & PM_FORCE_PET);
    if (!pet)
        mode |= PM_NO_PET;
    return summon_specific(player_ptr, (pet ? -1 : 0), y, x, level, SUMMON_KIN, mode);
}

/*!
 * @brief サイバーデーモンの召喚
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param who 召喚主のモンスターID(0ならばプレイヤー)
 * @param y 召喚位置Y座標
 * @param x 召喚位置X座標
 * @return 作用が実際にあった場合TRUEを返す
 */
int summon_cyber(player_type *player_ptr, MONSTER_IDX who, POSITION y, POSITION x)
{
    /* Summoned by a monster */
    BIT_FLAGS mode = PM_ALLOW_GROUP;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (who > 0) {
        monster_type *m_ptr = &floor_ptr->m_list[who];
        if (is_pet(m_ptr))
            mode |= PM_FORCE_PET;
    }

    int max_cyber = (easy_band ? 1 : (floor_ptr->dun_level / 50) + randint1(2));
    if (max_cyber > 4)
        max_cyber = 4;

    int count = 0;
    for (int i = 0; i < max_cyber; i++) {
        count += summon_specific(player_ptr, who, y, x, 100, SUMMON_CYBER, mode);
    }

    return count;
}

void mitokohmon(player_type *player_ptr)
{
    int count = 0;
    concptr sukekakusan = "";
    if (summon_named_creature(player_ptr, 0, player_ptr->y, player_ptr->x, MON_SUKE, PM_FORCE_PET)) {
        msg_print(_("『助さん』が現れた。", "Suke-san apperars."));
        sukekakusan = "Suke-san";
        count++;
    }

    if (summon_named_creature(player_ptr, 0, player_ptr->y, player_ptr->x, MON_KAKU, PM_FORCE_PET)) {
        msg_print(_("『格さん』が現れた。", "Kaku-san appears."));
        sukekakusan = "Kaku-san";
        count++;
    }

    if (!count) {
        for (int i = player_ptr->current_floor_ptr->m_max - 1; i > 0; i--) {
            monster_type *m_ptr;
            m_ptr = &player_ptr->current_floor_ptr->m_list[i];
            if (!monster_is_valid(m_ptr))
                continue;
            if (!((m_ptr->r_idx == MON_SUKE) || (m_ptr->r_idx == MON_KAKU)))
                continue;
            if (!los(player_ptr, m_ptr->fy, m_ptr->fx, player_ptr->y, player_ptr->x))
                continue;
            if (!projectable(player_ptr, m_ptr->fy, m_ptr->fx, player_ptr->y, player_ptr->x))
                continue;
            count++;
            break;
        }
    }

    if (count == 0) {
        msg_print(_("しかし、何も起きなかった。", "Nothing happens."));
        return;
    }

    msg_format(
        _("「者ども、ひかえおろう！！！このお方をどなたとこころえる。」", "%^s says 'WHO do you think this person is! Bow your head, down to your knees!'"),
        sukekakusan);
    sukekaku = true;
    stun_monsters(player_ptr, 120);
    confuse_monsters(player_ptr, 120);
    turn_monsters(player_ptr, 120);
    stasis_monsters(player_ptr, 120);
    sukekaku = false;
}

/*!
 * @brief HI_SUMMON(上級召喚)処理発動
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 召喚位置Y座標
 * @param x 召喚位置X座標
 * @param can_pet プレイヤーのペットとなる可能性があるならばTRUEにする
 * @return 作用が実際にあった場合TRUEを返す
 * @todo 引数にPOSITION x/yは必要か？ 要調査
 */
int activate_hi_summon(player_type *player_ptr, POSITION y, POSITION x, bool can_pet)
{
    BIT_FLAGS mode = PM_ALLOW_GROUP;
    bool pet = false;
    if (can_pet) {
        if (one_in_(4)) {
            mode |= PM_FORCE_FRIENDLY;
        } else {
            mode |= PM_FORCE_PET;
            pet = true;
        }
    }

    if (!pet)
        mode |= PM_NO_PET;

    DEPTH dungeon_level = player_ptr->current_floor_ptr->dun_level;
    DEPTH summon_lev = (pet ? player_ptr->lev * 2 / 3 + randint1(player_ptr->lev / 2) : dungeon_level);
    int count = 0;
    for (int i = 0; i < (randint1(7) + (dungeon_level / 40)); i++) {
        switch (randint1(25) + (dungeon_level / 20)) {
        case 1:
        case 2:
            count += summon_specific(player_ptr, (pet ? -1 : 0), y, x, summon_lev, SUMMON_ANT, mode);
            break;
        case 3:
        case 4:
            count += summon_specific(player_ptr, (pet ? -1 : 0), y, x, summon_lev, SUMMON_SPIDER, mode);
            break;
        case 5:
        case 6:
            count += summon_specific(player_ptr, (pet ? -1 : 0), y, x, summon_lev, SUMMON_HOUND, mode);
            break;
        case 7:
        case 8:
            count += summon_specific(player_ptr, (pet ? -1 : 0), y, x, summon_lev, SUMMON_HYDRA, mode);
            break;
        case 9:
        case 10:
            count += summon_specific(player_ptr, (pet ? -1 : 0), y, x, summon_lev, SUMMON_ANGEL, mode);
            break;
        case 11:
        case 12:
            count += summon_specific(player_ptr, (pet ? -1 : 0), y, x, summon_lev, SUMMON_UNDEAD, mode);
            break;
        case 13:
        case 14:
            count += summon_specific(player_ptr, (pet ? -1 : 0), y, x, summon_lev, SUMMON_DRAGON, mode);
            break;
        case 15:
        case 16:
            count += summon_specific(player_ptr, (pet ? -1 : 0), y, x, summon_lev, SUMMON_DEMON, mode);
            break;
        case 17:
            if (can_pet)
                break;
            count += summon_specific(player_ptr, (pet ? -1 : 0), y, x, summon_lev, SUMMON_AMBERITES, (mode | PM_ALLOW_UNIQUE));
            break;
        case 18:
        case 19:
            if (can_pet)
                break;
            count += summon_specific(player_ptr, (pet ? -1 : 0), y, x, summon_lev, SUMMON_UNIQUE, (mode | PM_ALLOW_UNIQUE));
            break;
        case 20:
        case 21:
            if (!can_pet)
                mode |= PM_ALLOW_UNIQUE;
            count += summon_specific(player_ptr, (pet ? -1 : 0), y, x, summon_lev, SUMMON_HI_UNDEAD, mode);
            break;
        case 22:
        case 23:
            if (!can_pet)
                mode |= PM_ALLOW_UNIQUE;
            count += summon_specific(player_ptr, (pet ? -1 : 0), y, x, summon_lev, SUMMON_HI_DRAGON, mode);
            break;
        case 24:
            count += summon_specific(player_ptr, (pet ? -1 : 0), y, x, 100, SUMMON_CYBER, mode);
            break;
        default:
            if (!can_pet)
                mode |= PM_ALLOW_UNIQUE;
            count += summon_specific(player_ptr, (pet ? -1 : 0), y, x, pet ? summon_lev : (((summon_lev * 3) / 2) + 5), SUMMON_NONE, mode);
        }
    }

    return count;
}

/*!
 * @brief 「悪霊召喚」のランダムな効果を決定して処理する。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向ID
 */
void cast_invoke_spirits(player_type *player_ptr, DIRECTION dir)
{
    PLAYER_LEVEL plev = player_ptr->lev;
    int die = randint1(100) + plev / 5;
    int vir = virtue_number(player_ptr, V_CHANCE);

    if (vir != 0) {
        if (player_ptr->virtues[vir - 1] > 0) {
            while (randint1(400) < player_ptr->virtues[vir - 1])
                die++;
        } else {
            while (randint1(400) < (0 - player_ptr->virtues[vir - 1]))
                die--;
        }
    }

    msg_print(_("あなたは死者たちの力を招集した...", "You call on the power of the dead..."));
    if (die < 26)
        chg_virtue(player_ptr, V_CHANCE, 1);

    if (die > 100) {
        msg_print(_("あなたはおどろおどろしい力のうねりを感じた！", "You feel a surge of eldritch force!"));
    }

    BadStatusSetter bss(player_ptr);
    if (die < 8) {
        msg_print(_("なんてこった！あなたの周りの地面から朽ちた人影が立ち上がってきた！", "Oh no! Mouldering forms rise from the earth around you!"));

        (void)summon_specific(player_ptr, 0, player_ptr->y, player_ptr->x, player_ptr->current_floor_ptr->dun_level, SUMMON_UNDEAD,
            (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
        chg_virtue(player_ptr, V_UNLIFE, 1);
    } else if (die < 14) {
        msg_print(_("名状し難い邪悪な存在があなたの心を通り過ぎて行った...", "An unnamable evil brushes against your mind..."));
        (void)bss.mod_afraidness(randint1(4) + 4);
    } else if (die < 26) {
        msg_print(_("あなたの頭に大量の幽霊たちの騒々しい声が押し寄せてきた...", "Your head is invaded by a horde of gibbering spectral voices..."));
        (void)bss.mod_confusion(randint1(4) + 4);
    } else if (die < 31) {
        poly_monster(player_ptr, dir, plev);
    } else if (die < 36) {
        fire_bolt_or_beam(player_ptr, beam_chance(player_ptr) - 10, GF_MISSILE, dir, damroll(3 + ((plev - 1) / 5), 4));
    } else if (die < 41) {
        confuse_monster(player_ptr, dir, plev);
    } else if (die < 46) {
        fire_ball(player_ptr, GF_POIS, dir, 20 + (plev / 2), 3);
    } else if (die < 51) {
        (void)lite_line(player_ptr, dir, damroll(6, 8));
    } else if (die < 56) {
        fire_bolt_or_beam(player_ptr, beam_chance(player_ptr) - 10, GF_ELEC, dir, damroll(3 + ((plev - 5) / 4), 8));
    } else if (die < 61) {
        fire_bolt_or_beam(player_ptr, beam_chance(player_ptr) - 10, GF_COLD, dir, damroll(5 + ((plev - 5) / 4), 8));
    } else if (die < 66) {
        fire_bolt_or_beam(player_ptr, beam_chance(player_ptr), GF_ACID, dir, damroll(6 + ((plev - 5) / 4), 8));
    } else if (die < 71) {
        fire_bolt_or_beam(player_ptr, beam_chance(player_ptr), GF_FIRE, dir, damroll(8 + ((plev - 5) / 4), 8));
    } else if (die < 76) {
        hypodynamic_bolt(player_ptr, dir, 75);
    } else if (die < 81) {
        fire_ball(player_ptr, GF_ELEC, dir, 30 + plev / 2, 2);
    } else if (die < 86) {
        fire_ball(player_ptr, GF_ACID, dir, 40 + plev, 2);
    } else if (die < 91) {
        fire_ball(player_ptr, GF_ICE, dir, 70 + plev, 3);
    } else if (die < 96) {
        fire_ball(player_ptr, GF_FIRE, dir, 80 + plev, 3);
    } else if (die < 101) {
        hypodynamic_bolt(player_ptr, dir, 100 + plev);
    } else if (die < 104) {
        earthquake(player_ptr, player_ptr->y, player_ptr->x, 12, 0);
    } else if (die < 106) {
        (void)destroy_area(player_ptr, player_ptr->y, player_ptr->x, 13 + randint0(5), false);
    } else if (die < 108) {
        symbol_genocide(player_ptr, plev + 50, true);
    } else if (die < 110) {
        dispel_monsters(player_ptr, 120);
    } else {
        dispel_monsters(player_ptr, 150);
        slow_monsters(player_ptr, plev);
        sleep_monsters(player_ptr, plev);
        hp_player(player_ptr, 300);
    }

    if (die < 31) {
        msg_print(
            _("陰欝な声がクスクス笑う。「もうすぐおまえは我々の仲間になるだろう。弱き者よ。」", "Sepulchral voices chuckle. 'Soon you will join us, mortal.'"));
    }
}
