#include "racial/mutation-racial-selector.h"
#include "cmd-action/cmd-spell.h"
#include "mutation/mutation-flag-types.h"
#include "racial/racial-util.h"

void select_mutation_racial(player_type *creature_ptr, rc_type *rc_ptr)
{
    rpi_type rpi;

    if (creature_ptr->muta.has(MUTA::SPIT_ACID)) {
        rpi = rc_ptr->make_power(_("酸の唾", "Spit Acid"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl);
        rpi.min_level = 9;
        rpi.cost = 9;
        rpi.stat = A_DEX;
        rpi.fail = 15;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::SPIT_ACID));
    }

    if (creature_ptr->muta.has(MUTA::BR_FIRE)) {
        rpi = rc_ptr->make_power(_("炎のブレス", "Fire Breath"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl);
        rpi.min_level = 20;
        rpi.cost = rc_ptr->lvl;
        rpi.stat = A_CON;
        rpi.fail = 18;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::BR_FIRE));
    }

    if (creature_ptr->muta.has(MUTA::HYPN_GAZE)) {
        rpi = rc_ptr->make_power(_("催眠睨み", "Hypnotic Gaze"));
        rpi.info = format("%s%d", KWD_POWER, rc_ptr->lvl);
        rpi.min_level = 12;
        rpi.cost = 12;
        rpi.stat = A_CHR;
        rpi.fail = 18;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::HYPN_GAZE));
    }

    if (creature_ptr->muta.has(MUTA::TELEKINES)) {
        rpi = rc_ptr->make_power(_("念動力", "Telekinesis"));
#ifdef JP
        rpi.info = format("最大重量: %d.%dkg", lbtokg1(rc_ptr->lvl * 10), lbtokg2(rc_ptr->lvl * 10));
#else
        rpi.info = format("max wgt %d", rc_ptr->lvl * 10);
#endif
        rpi.min_level = 9;
        rpi.cost = 9;
        rpi.stat = A_WIS;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::TELEKINES));
    }

    if (creature_ptr->muta.has(MUTA::VTELEPORT)) {
        rpi = rc_ptr->make_power(_("テレポート", "Teleport"));
        rpi.info = format("%s%d", KWD_SPHERE, 10 + rc_ptr->lvl * 4);
        rpi.min_level = 7;
        rpi.cost = 7;
        rpi.stat = A_WIS;
        rpi.fail = 15;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::VTELEPORT));
    }

    if (creature_ptr->muta.has(MUTA::MIND_BLST)) {
        rpi = rc_ptr->make_power(_("精神攻撃", "Mind Blast"));
        rpi.info = format("%s%dd%", KWD_DAM, 3 + (rc_ptr->lvl - 1) / 5, 3);
        rpi.min_level = 5;
        rpi.cost = 3;
        rpi.stat = A_WIS;
        rpi.fail = 15;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::MIND_BLST));
    }

    if (creature_ptr->muta.has(MUTA::RADIATION)) {
        rpi = rc_ptr->make_power(_("放射能", "Emit Radiation"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 2);
        rpi.min_level = 15;
        rpi.cost = 15;
        rpi.stat = A_CON;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::RADIATION));
    }

    if (creature_ptr->muta.has(MUTA::VAMPIRISM)) {
        rpi = rc_ptr->make_power(_("吸血", "Vampiric Drain"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 2);
        rpi.min_level = 2;
        rpi.cost = 1 + (rc_ptr->lvl / 3);
        rpi.stat = A_CON;
        rpi.fail = 9;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::VAMPIRISM));
    }

    if (creature_ptr->muta.has(MUTA::SMELL_MET)) {
        rpi = rc_ptr->make_power(_("金属嗅覚", "Smell Metal"));
        rpi.min_level = 3;
        rpi.cost = 2;
        rpi.stat = A_INT;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::SMELL_MET));
    }

    if (creature_ptr->muta.has(MUTA::SMELL_MON)) {
        rpi = rc_ptr->make_power(_("敵臭嗅覚", "Smell Monsters"));
        rpi.min_level = 5;
        rpi.cost = 4;
        rpi.stat = A_INT;
        rpi.fail = 15;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::SMELL_MON));
    }

    if (creature_ptr->muta.has(MUTA::BLINK)) {
        rpi = rc_ptr->make_power(_("ショート・テレポート", "Blink"));
        rpi.info = format("%s%d", KWD_SPHERE, 10);
        rpi.min_level = 3;
        rpi.cost = 3;
        rpi.stat = A_WIS;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::BLINK));
    }

    if (creature_ptr->muta.has(MUTA::EAT_ROCK)) {
        rpi = rc_ptr->make_power(_("岩食い", "Eat Rock"));
        rpi.min_level = 8;
        rpi.cost = 12;
        rpi.stat = A_CON;
        rpi.fail = 18;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::EAT_ROCK));
    }

    if (creature_ptr->muta.has(MUTA::SWAP_POS)) {
        rpi = rc_ptr->make_power(_("位置交換", "Swap Position"));
        rpi.info = format("%s%d", KWD_SPHERE, 10 + rc_ptr->lvl * 3 / 2);
        rpi.min_level = 15;
        rpi.cost = 12;
        rpi.stat = A_DEX;
        rpi.fail = 16;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::SWAP_POS));
    }

    if (creature_ptr->muta.has(MUTA::SHRIEK)) {
        rpi = rc_ptr->make_power(_("叫び", "Shriek"));
        rpi.min_level = 20;
        rpi.cost = 14;
        rpi.stat = A_CON;
        rpi.fail = 16;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::SHRIEK));
    }

    if (creature_ptr->muta.has(MUTA::ILLUMINE)) {
        rpi = rc_ptr->make_power(_("照明", "Illuminate"));
        rpi.min_level = 3;
        rpi.cost = 2;
        rpi.stat = A_INT;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::ILLUMINE));
    }

    if (creature_ptr->muta.has(MUTA::DET_CURSE)) {
        rpi = rc_ptr->make_power(_("呪い感知", "Detect Curses"));
        rpi.min_level = 7;
        rpi.cost = 14;
        rpi.stat = A_WIS;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::DET_CURSE));
    }

    if (creature_ptr->muta.has(MUTA::BERSERK)) {
        rpi = rc_ptr->make_power(_("狂戦士化", "Berserk"));
        rpi.info = format("%s%d+d%d", KWD_DURATION, 25, 25);
        rpi.min_level = 8;
        rpi.cost = 8;
        rpi.stat = A_STR;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::BERSERK));
    }

    if (creature_ptr->muta.has(MUTA::POLYMORPH)) {
        rpi = rc_ptr->make_power(_("変身", "Polymorph"));
        rpi.min_level = 18;
        rpi.cost = 20;
        rpi.stat = A_CON;
        rpi.fail = 18;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::POLYMORPH));
    }

    if (creature_ptr->muta.has(MUTA::MIDAS_TCH)) {
        rpi = rc_ptr->make_power(_("ミダスの手", "Midas Touch"));
        rpi.min_level = 10;
        rpi.cost = 5;
        rpi.stat = A_INT;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::MIDAS_TCH));
    }

    if (creature_ptr->muta.has(MUTA::GROW_MOLD)) {
        rpi = rc_ptr->make_power(_("カビ発生", "Grow Mold"));
        rpi.min_level = 1;
        rpi.cost = 6;
        rpi.stat = A_CON;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::GROW_MOLD));
    }

    if (creature_ptr->muta.has(MUTA::RESIST)) {
        rpi = rc_ptr->make_power(_("エレメント耐性", "Resist Elements"));
        rpi.info = format("%s%d+d%d", KWD_DURATION, 20, 20);
        rpi.min_level = 10;
        rpi.cost = 12;
        rpi.stat = A_CON;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::RESIST));
    }

    if (creature_ptr->muta.has(MUTA::EARTHQUAKE)) {
        rpi = rc_ptr->make_power(_("地震", "Earthquake"));
        rpi.info = format("%s%d", KWD_SPHERE, 10);
        rpi.min_level = 12;
        rpi.cost = 12;
        rpi.stat = A_STR;
        rpi.fail = 16;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::EARTHQUAKE));
    }

    if (creature_ptr->muta.has(MUTA::EAT_MAGIC)) {
        rpi = rc_ptr->make_power(_("魔力食い", "Eat Magic"));
        rpi.info = format("%s%d", KWD_POWER, rc_ptr->lvl * 2);
        rpi.min_level = 17;
        rpi.cost = 1;
        rpi.stat = A_WIS;
        rpi.fail = 15;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::EAT_MAGIC));
    }

    if (creature_ptr->muta.has(MUTA::WEIGH_MAG)) {
        rpi = rc_ptr->make_power(_("魔力感知", "Weigh Magic"));
        rpi.min_level = 6;
        rpi.cost = 6;
        rpi.stat = A_INT;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::WEIGH_MAG));
    }

    if (creature_ptr->muta.has(MUTA::STERILITY)) {
        rpi = rc_ptr->make_power(_("増殖阻止", "Sterilize"));
        rpi.min_level = 12;
        rpi.cost = 23;
        rpi.stat = A_CHR;
        rpi.fail = 15;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::STERILITY));
    }

    if (creature_ptr->muta.has(MUTA::HIT_AND_AWAY)) {
        rpi = rc_ptr->make_power(_("ヒット＆アウェイ", "Panic Hit"));
        rpi.info = format("%s%d", KWD_SPHERE, 30);
        rpi.min_level = 10;
        rpi.cost = 12;
        rpi.stat = A_DEX;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::HIT_AND_AWAY));
    }

    if (creature_ptr->muta.has(MUTA::DAZZLE)) {
        rpi = rc_ptr->make_power(_("眩惑", "Dazzle"));
        rpi.info = format("%s%d", KWD_POWER, rc_ptr->lvl * 4);
        rpi.min_level = 7;
        rpi.cost = 15;
        rpi.stat = A_CHR;
        rpi.fail = 8;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::DAZZLE));
    }

    if (creature_ptr->muta.has(MUTA::LASER_EYE)) {
        rpi = rc_ptr->make_power(_("レーザー・アイ", "Laser Eye"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 2);
        rpi.min_level = 7;
        rpi.cost = 10;
        rpi.stat = A_WIS;
        rpi.fail = 9;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::LASER_EYE));
    }

    if (creature_ptr->muta.has(MUTA::RECALL)) {
        rpi = rc_ptr->make_power(_("帰還", "Recall"));
        rpi.min_level = 17;
        rpi.cost = 50;
        rpi.stat = A_INT;
        rpi.fail = 16;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::RECALL));
    }

    if (creature_ptr->muta.has(MUTA::BANISH)) {
        rpi = rc_ptr->make_power(_("邪悪消滅", "Banish Evil"));
        rpi.info = format("%s%d", KWD_POWER, 50 + rc_ptr->lvl);
        rpi.min_level = 25;
        rpi.cost = 25;
        rpi.stat = A_WIS;
        rpi.fail = 18;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::BANISH));
    }

    if (creature_ptr->muta.has(MUTA::COLD_TOUCH)) {
        rpi = rc_ptr->make_power(_("凍結の手", "Cold Touch"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 2);
        rpi.min_level = 2;
        rpi.cost = 2;
        rpi.stat = A_CON;
        rpi.fail = 11;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::COLD_TOUCH));
    }

    if (creature_ptr->muta.has(MUTA::LAUNCHER)) {
        rpi = rc_ptr->make_power(_("アイテム投げ", "Throw Object"));
        rpi.min_level = 1;
        rpi.cost = rc_ptr->lvl;
        rpi.stat = A_STR;
        rpi.fail = 6;
        rc_ptr->add_power(rpi, static_cast<int>(MUTA::LAUNCHER));
    }
}
