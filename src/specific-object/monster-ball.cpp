#include "specific-object/monster-ball.h"
#include "effect/spells-effect-util.h"
#include "floor/geometry.h"
#include "game-option/input-options.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-info.h"
#include "monster/monster-util.h"
#include "object/tval-types.h"
#include "pet/pet-util.h"
#include "racial/racial-android.h"
#include "spell-kind/spells-launcher.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "util/flag-group.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <sstream>

static void inscribe_nickname(ItemEntity &item, const CapturedMonsterType &cap_mon)
{
    if (cap_mon.nickname.empty()) {
        return;
    }

    auto &insc = item.inscription;
    if (!insc) {
        insc = "";
    }

    if (auto s = angband_strchr(insc->data(), '#'); s != nullptr) {
        insc->erase(s - insc->data());
    }

    std::stringstream nickname;
    nickname << '#' << _("", '\'') << cap_mon.nickname << _("", '\'');

    insc->append(nickname.str());
}

static bool capture_monster(PlayerType *player_ptr, ItemEntity &item)
{
    const auto old_target_pet = target_pet;
    target_pet = true;
    const auto dir = get_aim_dir(player_ptr);
    if (!dir) {
        target_pet = old_target_pet;
        return false;
    }

    target_pet = old_target_pet;
    CapturedMonsterType cap_mon;
    if (!fire_ball(player_ptr, AttributeType::CAPTURE, dir, 0, 0, &cap_mon)) {
        return true;
    }

    item.pval = enum2i(cap_mon.r_idx);
    item.captured_monster_speed = cap_mon.speed;
    item.captured_monster_current_hp = cap_mon.current_hp;
    item.captured_monster_max_hp = cap_mon.max_hp;
    inscribe_nickname(item, cap_mon);
    return true;
}

static void restore_monster_nickname(MonsterEntity &monster, ItemEntity &item)
{
    if (!item.is_inscribed()) {
        return;
    }

    auto &insc = item.inscription;
    const auto s = angband_strchr(insc->data(), '#');
    if (s == nullptr) {
        return;
    }

    std::string_view nickname = s + 1;
#ifndef JP
    if (nickname.starts_with('\'')) {
        nickname.remove_prefix(1);
        if (nickname.ends_with('\'')) {
            nickname.remove_suffix(1);
        }
    }
#endif
    monster.nickname = nickname;

    insc->erase(s - insc->data());
}

static bool release_monster(PlayerType *player_ptr, ItemEntity &item, const Direction &dir)
{
    const auto &monrace = item.get_monrace();
    const auto pos = player_ptr->get_neighbor(dir);
    if (!monster_can_enter(player_ptr, pos.y, pos.x, monrace, 0)) {
        return false;
    }

    const auto m_idx = place_specific_monster(player_ptr, pos.y, pos.x, monrace.idx, PM_FORCE_PET | PM_NO_KAGE);
    if (!m_idx) {
        return false;
    }

    auto &monster = player_ptr->current_floor_ptr->m_list[*m_idx];
    if (item.captured_monster_speed > 0) {
        monster.mspeed = item.captured_monster_speed;
    }

    if (item.captured_monster_max_hp) {
        monster.max_maxhp = item.captured_monster_max_hp;
    }

    if (item.captured_monster_current_hp > 0) {
        monster.hp = item.captured_monster_current_hp;
    }

    monster.maxhp = monster.max_maxhp;
    restore_monster_nickname(monster, item);
    item.pval = 0;
    item.captured_monster_speed = 0;
    item.captured_monster_current_hp = 0;
    item.captured_monster_max_hp = 0;
    return true;
}

bool exe_monster_capture(PlayerType *player_ptr, ItemEntity &item)
{
    if (item.bi_key.tval() != ItemKindType::CAPTURE) {
        return false;
    }

    if (item.pval == 0) {
        if (!capture_monster(player_ptr, item)) {
            return true;
        }

        calc_android_exp(player_ptr);
        return true;
    }

    const auto dir = get_direction(player_ptr);
    if (!dir) {
        return true;
    }

    if (!release_monster(player_ptr, item, dir)) {
        msg_print(_("おっと、解放に失敗した。", "Oops.  You failed to release your pet."));
    }

    calculate_upkeep(player_ptr);
    calc_android_exp(player_ptr);
    return true;
}
