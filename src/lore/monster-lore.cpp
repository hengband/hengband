/*!
 * @brief モンスターの思い出表示に必要なフラグ類の処理
 * @date 2020/06/09
 * @author Hourier
 */

#include "lore/monster-lore.h"
#include "game-option/cheat-options.h"
#include "lore/lore-calculator.h"
#include "lore/lore-util.h"
#include "lore/magic-types-setter.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "monster-race/race-misc-flags.h"
#include "monster-race/race-sex-const.h"
#include "player-ability/player-ability-types.h"
#include "system/angband.h"
#include "system/monster-race-info.h"
#include "term/term-color-types.h"
#include "view/display-lore-attacks.h"
#include "view/display-lore-drops.h"
#include "view/display-lore-magics.h"
#include "view/display-lore-status.h"
#include "view/display-lore.h"
#include <algorithm>

static void set_msex_flags(lore_type *lore_ptr)
{
    lore_ptr->msex = MSEX_NONE;
    if (is_male(*(lore_ptr->r_ptr))) {
        lore_ptr->msex = MSEX_MALE;
    }
    if (is_female(*(lore_ptr->r_ptr))) {
        lore_ptr->msex = MSEX_FEMALE;
    }
}

static void set_flags1(lore_type *lore_ptr)
{
    if (lore_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        lore_ptr->kind_flags.set(MonsterKindType::UNIQUE);
    }

    if (lore_ptr->r_ptr->misc_flags.has(MonsterMiscType::QUESTOR)) {
        lore_ptr->misc_flags.set(MonsterMiscType::QUESTOR);
    }

    if (lore_ptr->r_ptr->misc_flags.has(MonsterMiscType::HAS_FRIENDS)) {
        lore_ptr->misc_flags.set(MonsterMiscType::HAS_FRIENDS);
    }

    if (lore_ptr->r_ptr->misc_flags.has(MonsterMiscType::ESCORT)) {
        lore_ptr->misc_flags.set(MonsterMiscType::ESCORT);
    }

    if (lore_ptr->r_ptr->misc_flags.has(MonsterMiscType::MORE_ESCORT)) {
        lore_ptr->misc_flags.set(MonsterMiscType::MORE_ESCORT);
    }
}

static void set_race_flags(lore_type *lore_ptr)
{
    if (!lore_ptr->r_ptr->r_tkills && !lore_ptr->know_everything) {
        return;
    }

    if (lore_ptr->r_ptr->kind_flags.has(MonsterKindType::ORC)) {
        lore_ptr->kind_flags.set(MonsterKindType::ORC);
    }

    if (lore_ptr->r_ptr->kind_flags.has(MonsterKindType::TROLL)) {
        lore_ptr->kind_flags.set(MonsterKindType::TROLL);
    }

    if (lore_ptr->r_ptr->kind_flags.has(MonsterKindType::GIANT)) {
        lore_ptr->kind_flags.set(MonsterKindType::GIANT);
    }

    if (lore_ptr->r_ptr->kind_flags.has(MonsterKindType::DRAGON)) {
        lore_ptr->kind_flags.set(MonsterKindType::DRAGON);
    }

    if (lore_ptr->r_ptr->kind_flags.has(MonsterKindType::DEMON)) {
        lore_ptr->kind_flags.set(MonsterKindType::DEMON);
    }

    if (lore_ptr->r_ptr->kind_flags.has(MonsterKindType::UNDEAD)) {
        lore_ptr->kind_flags.set(MonsterKindType::UNDEAD);
    }

    if (lore_ptr->r_ptr->kind_flags.has(MonsterKindType::EVIL)) {
        lore_ptr->kind_flags.set(MonsterKindType::EVIL);
    }

    if (lore_ptr->r_ptr->kind_flags.has(MonsterKindType::GOOD)) {
        lore_ptr->kind_flags.set(MonsterKindType::GOOD);
    }

    if (lore_ptr->r_ptr->kind_flags.has(MonsterKindType::ANIMAL)) {
        lore_ptr->kind_flags.set(MonsterKindType::ANIMAL);
    }

    if (lore_ptr->r_ptr->kind_flags.has(MonsterKindType::AMBERITE)) {
        lore_ptr->kind_flags.set(MonsterKindType::AMBERITE);
    }

    if (lore_ptr->r_ptr->kind_flags.has(MonsterKindType::HUMAN)) {
        lore_ptr->kind_flags.set(MonsterKindType::HUMAN);
    }

    if (lore_ptr->r_ptr->kind_flags.has(MonsterKindType::QUANTUM)) {
        lore_ptr->kind_flags.set(MonsterKindType::QUANTUM);
    }

    if (lore_ptr->r_ptr->misc_flags.has(MonsterMiscType::FORCE_DEPTH)) {
        lore_ptr->misc_flags.set(MonsterMiscType::FORCE_DEPTH);
    }

    if (lore_ptr->r_ptr->misc_flags.has(MonsterMiscType::FORCE_MAXHP)) {
        lore_ptr->misc_flags.set(MonsterMiscType::FORCE_MAXHP);
    }
}

/*!
 * @brief モンスターの思い出情報を表示するメインルーチン
 * Hack -- display monster information using "hooked_roff()"
 * @param r_idx モンスターの種族ID
 * @param mode 表示オプション
 * @details
 * This function should only be called with the cursor placed at the
 * left edge of the screen, on a cleared line, in which the recall is
 * to take place.  One extra blank line is left after the recall.
 */
void process_monster_lore(PlayerType *player_ptr, MonsterRaceId r_idx, monster_lore_mode mode)
{
    lore_type tmp_lore(r_idx, mode);
    lore_type *lore_ptr = &tmp_lore;

    auto is_valid_reinforcer = [](const auto &reinforce) {
        auto [r_idx, dd, ds] = reinforce;
        auto is_reinforce = MonsterRace(r_idx).is_valid();
        is_reinforce &= dd > 0;
        is_reinforce &= ds > 0;
        return is_reinforce;
    };

    lore_ptr->reinforce =
        std::find_if(
            lore_ptr->r_ptr->reinforces.begin(), lore_ptr->r_ptr->reinforces.end(),
            is_valid_reinforcer) != lore_ptr->r_ptr->reinforces.end();

    if (cheat_know || (mode == MONSTER_LORE_RESEARCH) || (mode == MONSTER_LORE_DEBUG)) {
        lore_ptr->know_everything = true;
    }

    set_flags_for_full_knowledge(lore_ptr);
    set_msex_flags(lore_ptr);
    set_flags1(lore_ptr);
    set_race_flags(lore_ptr);
    display_kill_numbers(lore_ptr);
    concptr tmp = lore_ptr->r_ptr->text.data();
    if (tmp[0]) {
        hooked_roff(tmp);
        hooked_roff("\n");
    }

    if (r_idx == MonsterRaceId::KAGE) {
        hooked_roff("\n");
        return;
    }

    if (!display_where_to_appear(lore_ptr)) {
        return;
    }

    display_monster_move(lore_ptr);
    display_monster_never_move(lore_ptr);
    if (lore_ptr->old) {
        hooked_roff(_("。", ".  "));
        lore_ptr->old = false;
    }

    display_lore_this(player_ptr, lore_ptr);
    if (lore_ptr->special_flags.has(MonsterSpecialType::DIMINISH_MAX_DAMAGE)) {
        hooked_roff(format(_("%s^は", "%s^ "), Who::who(lore_ptr->msex)));
        hook_c_roff(TERM_RED, _("一度に受けるダメージを常に自身の身体のごく一部に抑える。", "always diminishes a damage per time to a small portion of the body.  "));
    }
    display_monster_aura(lore_ptr);
    if (lore_ptr->misc_flags.has(MonsterMiscType::REFLECTING)) {
        hooked_roff(format(_("%s^は矢の呪文を跳ね返す。", "%s^ reflects bolt spells.  "), Who::who(lore_ptr->msex)));
    }

    display_monster_collective(lore_ptr);
    lore_ptr->vn = 0;
    if (lore_ptr->ability_flags.has(MonsterAbilityType::SHRIEK)) {
        lore_ptr->vp[lore_ptr->vn] = _("悲鳴で助けを求める", "shriek for help");
        lore_ptr->color[lore_ptr->vn++] = TERM_L_WHITE;
    }

    display_monster_launching(player_ptr, lore_ptr);
    if (lore_ptr->ability_flags.has(MonsterAbilityType::SPECIAL)) {
        lore_ptr->vp[lore_ptr->vn] = _("特別な行動をする", "do something");
        lore_ptr->color[lore_ptr->vn++] = TERM_VIOLET;
    }

    display_monster_sometimes(lore_ptr);
    set_breath_types(player_ptr, lore_ptr);
    display_monster_breath(lore_ptr);

    lore_ptr->vn = 0;
    set_ball_types(player_ptr, lore_ptr);
    set_particular_types(player_ptr, lore_ptr);
    set_bolt_types(player_ptr, lore_ptr);
    set_status_types(lore_ptr);
    set_teleport_types(lore_ptr);
    set_floor_types(player_ptr, lore_ptr);
    set_summon_types(lore_ptr);
    display_monster_magic_types(lore_ptr);
    display_mosnter_magic_possibility(lore_ptr);
    display_monster_hp_ac(lore_ptr);

    lore_ptr->vn = 0;
    display_monster_concrete_abilities(lore_ptr);
    display_monster_abilities(lore_ptr);
    display_monster_constitutions(lore_ptr);

    lore_ptr->vn = 0;
    display_monster_concrete_weakness(lore_ptr);
    display_monster_weakness(lore_ptr);

    lore_ptr->vn = 0;
    display_monster_concrete_resistances(lore_ptr);
    display_monster_resistances(lore_ptr);
    display_monster_evolution(lore_ptr);

    lore_ptr->vn = 0;
    display_monster_concrete_immunities(lore_ptr);
    display_monster_immunities(lore_ptr);
    display_monster_alert(lore_ptr);
    display_monster_drops(lore_ptr);
    display_monster_blows(lore_ptr);
    display_monster_guardian(lore_ptr);
}
