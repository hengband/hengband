#include "spell-realm/spells-trump.h"
#include "avatar/avatar.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "mutation/mutation-investor-remover.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/base-status.h"
#include "status/buff-setter.h"
#include "status/experience.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * @brief トランプ領域の「シャッフル」の効果をランダムに決めて処理する。
 * @param caster_ptr プレーヤーへの参照ポインタ
 */
void cast_shuffle(player_type *caster_ptr)
{
    PLAYER_LEVEL plev = caster_ptr->lev;
    DIRECTION dir;
    int die;
    int vir = virtue_number(caster_ptr, V_CHANCE);
    int i;

    if ((caster_ptr->pclass == CLASS_ROGUE) || (caster_ptr->pclass == CLASS_HIGH_MAGE) || (caster_ptr->pclass == CLASS_SORCERER))
        die = (randint1(110)) + plev / 5;
    else
        die = randint1(120);

    if (vir) {
        if (caster_ptr->virtues[vir - 1] > 0) {
            while (randint1(400) < caster_ptr->virtues[vir - 1])
                die++;
        } else {
            while (randint1(400) < (0 - caster_ptr->virtues[vir - 1]))
                die--;
        }
    }

    msg_print(_("あなたはカードを切って一枚引いた...", "You shuffle the deck and draw a card..."));

    if (die < 30) {
        chg_virtue(caster_ptr, V_CHANCE, 1);
    }

    floor_type *floor_ptr = caster_ptr->current_floor_ptr;
    if (die < 7) {
        msg_print(_("なんてこった！《死》だ！", "Oh no! It's Death!"));

        for (i = 0; i < randint1(3); i++) {
            activate_hi_summon(caster_ptr, caster_ptr->y, caster_ptr->x, false);
        }

        return;
    }

    if (die < 14) {
        msg_print(_("なんてこった！《悪魔》だ！", "Oh no! It's the Devil!"));
        summon_specific(caster_ptr, 0, caster_ptr->y, caster_ptr->x, floor_ptr->dun_level, SUMMON_DEMON, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
        return;
    }

    if (die < 18) {
        int count = 0;
        msg_print(_("なんてこった！《吊られた男》だ！", "Oh no! It's the Hanged Man."));
        activate_ty_curse(caster_ptr, false, &count);
        return;
    }

    if (die < 22) {
        msg_print(_("《不調和の剣》だ。", "It's the swords of discord."));
        aggravate_monsters(caster_ptr, 0);
        return;
    }

    if (die < 26) {
        msg_print(_("《愚者》だ。", "It's the Fool."));
        do_dec_stat(caster_ptr, A_INT);
        do_dec_stat(caster_ptr, A_WIS);
        return;
    }

    if (die < 30) {
        msg_print(_("奇妙なモンスターの絵だ。", "It's the picture of a strange monster."));
        trump_summoning(caster_ptr, 1, false, caster_ptr->y, caster_ptr->x, (floor_ptr->dun_level * 3 / 2),
            static_cast<summon_type>(SUMMON_UNIQUE + randint1(6)), PM_ALLOW_GROUP | PM_ALLOW_UNIQUE);
        return;
    }

    if (die < 33) {
        msg_print(_("《月》だ。", "It's the Moon."));
        unlite_area(caster_ptr, 10, 3);
        return;
    }

    if (die < 38) {
        msg_print(_("《運命の輪》だ。", "It's the Wheel of Fortune."));
        wild_magic(caster_ptr, randint0(32));
        return;
    }

    if (die < 40) {
        msg_print(_("テレポート・カードだ。", "It's a teleport trump card."));
        teleport_player(caster_ptr, 10, TELEPORT_PASSIVE);
        return;
    }

    if (die < 42) {
        msg_print(_("《正義》だ。", "It's Justice."));
        set_blessed(caster_ptr, caster_ptr->lev, false);
        return;
    }

    if (die < 47) {
        msg_print(_("テレポート・カードだ。", "It's a teleport trump card."));
        teleport_player(caster_ptr, 100, TELEPORT_PASSIVE);
        return;
    }

    if (die < 52) {
        msg_print(_("テレポート・カードだ。", "It's a teleport trump card."));
        teleport_player(caster_ptr, 200, TELEPORT_PASSIVE);
        return;
    }

    if (die < 60) {
        msg_print(_("《塔》だ。", "It's the Tower."));
        wall_breaker(caster_ptr);
        return;
    }

    if (die < 72) {
        msg_print(_("《節制》だ。", "It's Temperance."));
        sleep_monsters_touch(caster_ptr);
        return;
    }

    if (die < 80) {
        msg_print(_("《塔》だ。", "It's the Tower."));
        earthquake(caster_ptr, caster_ptr->y, caster_ptr->x, 5, 0);
        return;
    }

    if (die < 82) {
        msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
        trump_summoning(caster_ptr, 1, true, caster_ptr->y, caster_ptr->x, (floor_ptr->dun_level * 3 / 2), SUMMON_MOLD, 0L);
        return;
    }

    if (die < 84) {
        msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
        trump_summoning(caster_ptr, 1, true, caster_ptr->y, caster_ptr->x, (floor_ptr->dun_level * 3 / 2), SUMMON_BAT, 0L);
        return;
    }

    if (die < 86) {
        msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
        trump_summoning(caster_ptr, 1, true, caster_ptr->y, caster_ptr->x, (floor_ptr->dun_level * 3 / 2), SUMMON_VORTEX, 0L);
        return;
    }

    if (die < 88) {
        msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
        trump_summoning(caster_ptr, 1, true, caster_ptr->y, caster_ptr->x, (floor_ptr->dun_level * 3 / 2), SUMMON_COIN_MIMIC, 0L);
        return;
    }

    if (die < 96) {
        msg_print(_("《恋人》だ。", "It's the Lovers."));

        if (get_aim_dir(caster_ptr, &dir)) {
            charm_monster(caster_ptr, dir, MIN(caster_ptr->lev, 20));
        }

        return;
    }

    if (die < 101) {
        msg_print(_("《隠者》だ。", "It's the Hermit."));
        wall_stone(caster_ptr);
        return;
    }

    if (die < 111) {
        msg_print(_("《審判》だ。", "It's Judgement."));
        roll_hitdice(caster_ptr, SPOP_NONE);
        lose_all_mutations(caster_ptr);
        return;
    }

    if (die < 120) {
        msg_print(_("《太陽》だ。", "It's the Sun."));
        chg_virtue(caster_ptr, V_KNOWLEDGE, 1);
        chg_virtue(caster_ptr, V_ENLIGHTEN, 1);
        wiz_lite(caster_ptr, false);
        return;
    }

    msg_print(_("《世界》だ。", "It's the World."));
    if (caster_ptr->exp >= PY_MAX_EXP) {
        return;
    }

    int32_t ee = (caster_ptr->exp / 25) + 1;
    if (ee > 5000)
        ee = 5000;
    msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));
    gain_exp(caster_ptr, ee);
}

void become_living_trump(player_type *creature_ptr)
{
    /* 1/7 Teleport control and 6/7 Random teleportation (uncontrolled) */
    MUTATION_IDX mutation = one_in_(7) ? 12 : 77;
    if (gain_mutation(creature_ptr, mutation))
        msg_print(_("あなたは生きているカードに変わった。", "You have turned into a Living Trump."));
}
