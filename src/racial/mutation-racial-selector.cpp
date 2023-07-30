#include "racial/mutation-racial-selector.h"
#include "cmd-action/cmd-spell.h"
#include "locale/japanese.h"
#include "mutation/mutation-flag-types.h"
#include "racial/racial-util.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"

void select_mutation_racial(PlayerType *player_ptr, rc_type *rc_ptr)
{
    rpi_type rpi;
    if (player_ptr->muta.has(PlayerMutationType::SPIT_ACID)) {
        rpi = rpi_type(_("酸の唾", "Spit Acid"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl);
        rpi.text = _("酸のボールを放つ", "Fires a boll of acid.");
        rpi.min_level = 9;
        rpi.cost = 9;
        rpi.stat = A_DEX;
        rpi.fail = 15;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::SPIT_ACID));
    }

    if (player_ptr->muta.has(PlayerMutationType::BR_FIRE)) {
        rpi = rpi_type(_("炎のブレス", "Fire Breath"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 2);
        rpi.text = _("火炎のブレスを放つ", "Fires a breath of fire.");
        rpi.min_level = 20;
        rpi.cost = rc_ptr->lvl;
        rpi.stat = A_CON;
        rpi.fail = 18;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::BR_FIRE));
    }

    if (player_ptr->muta.has(PlayerMutationType::HYPN_GAZE)) {
        rpi = rpi_type(_("催眠睨み", "Hypnotic Gaze"));
        rpi.info = format("%s%d", KWD_POWER, rc_ptr->lvl);
        rpi.text = _("モンスター1体をペットにする。抵抗されると無効。", "Attempts to charm a monster.");
        rpi.min_level = 12;
        rpi.cost = 12;
        rpi.stat = A_CHR;
        rpi.fail = 18;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::HYPN_GAZE));
    }

    if (player_ptr->muta.has(PlayerMutationType::TELEKINES)) {
        rpi = rpi_type(_("念動力", "Telekinesis"));
#ifdef JP
        rpi.info = format("最大重量: %d.%dkg", lb_to_kg_integer(rc_ptr->lvl * 10), lb_to_kg_fraction(rc_ptr->lvl * 10));
#else
        rpi.info = format("max wgt %d", rc_ptr->lvl * 10);
#endif
        rpi.text = _("アイテムを自分の足元へ移動させる。", "Pulls a distant item close to you.");
        rpi.min_level = 9;
        rpi.cost = 9;
        rpi.stat = A_WIS;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::TELEKINES));
    }

    if (player_ptr->muta.has(PlayerMutationType::VTELEPORT)) {
        rpi = rpi_type(_("テレポート", "Teleport"));
        rpi.info = format("%s%d", KWD_SPHERE, 10 + rc_ptr->lvl * 4);
        rpi.text = _("遠距離のテレポートをする。", "Teleports you a long distance.");
        rpi.min_level = 7;
        rpi.cost = 7;
        rpi.stat = A_WIS;
        rpi.fail = 15;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::VTELEPORT));
    }

    if (player_ptr->muta.has(PlayerMutationType::MIND_BLST)) {
        rpi = rpi_type(_("精神攻撃", "Mind Blast"));
        rpi.info = format("%s%dd%d", KWD_DAM, 3 + (rc_ptr->lvl - 1) / 5, 3);
        rpi.text = _("モンスター1体に精神攻撃を行う。", "Deals a PSI damage to a monster.");
        rpi.min_level = 5;
        rpi.cost = 3;
        rpi.stat = A_WIS;
        rpi.fail = 15;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::MIND_BLST));
    }

    if (player_ptr->muta.has(PlayerMutationType::RADIATION)) {
        rpi = rpi_type(_("放射能", "Emit Radiation"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 2);
        rpi.text = _("自分を中心とする放射性廃棄物のボールを放つ。", "Burst nuke ball.");
        rpi.min_level = 15;
        rpi.cost = 15;
        rpi.stat = A_CON;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::RADIATION));
    }

    if (player_ptr->muta.has(PlayerMutationType::VAMPIRISM)) {
        rpi = rpi_type(_("吸血", "Vampiric Drain"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 2);
        rpi.text = _("隣接したモンスター1体から生命力を吸い取る。吸い取った生命力によって満腹度があがる。",
            "Drains and transfers HP from a monster near by you. You will also gain nutritional sustenance from this.");
        rpi.min_level = 2;
        rpi.cost = 1 + (rc_ptr->lvl / 3);
        rpi.stat = A_CON;
        rpi.fail = 9;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::VAMPIRISM));
    }

    if (player_ptr->muta.has(PlayerMutationType::SMELL_MET)) {
        rpi = rpi_type(_("金属嗅覚", "Smell Metal"));
        rpi.text = _("周辺の鉱物を感知する。", "Detects all treasurely walls in your vicinity.");
        rpi.min_level = 3;
        rpi.cost = 2;
        rpi.stat = A_INT;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::SMELL_MET));
    }

    if (player_ptr->muta.has(PlayerMutationType::SMELL_MON)) {
        rpi = rpi_type(_("敵臭嗅覚", "Smell Monsters"));
        rpi.text = _("近くの全ての見えるモンスターを感知する。", "Detects all monsters in your vicinity unless invisible.");
        rpi.min_level = 5;
        rpi.cost = 4;
        rpi.stat = A_INT;
        rpi.fail = 15;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::SMELL_MON));
    }

    if (player_ptr->muta.has(PlayerMutationType::BLINK)) {
        rpi = rpi_type(_("ショート・テレポート", "Blink"));
        rpi.info = format("%s%d", KWD_SPHERE, 10);
        rpi.text = _("近距離のテレポートをする。", "Teleports you a short distance.");
        rpi.min_level = 3;
        rpi.cost = 3;
        rpi.stat = A_WIS;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::BLINK));
    }

    if (player_ptr->muta.has(PlayerMutationType::EAT_ROCK)) {
        rpi = rpi_type(_("岩食い", "Eat Rock"));
        rpi.text = _("対象の壁を食料のように食べる。", "Eats a wall as rations.");
        rpi.min_level = 8;
        rpi.cost = 12;
        rpi.stat = A_CON;
        rpi.fail = 18;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::EAT_ROCK));
    }

    if (player_ptr->muta.has(PlayerMutationType::SWAP_POS)) {
        rpi = rpi_type(_("位置交換", "Swap Position"));
        rpi.info = format("%s%d", KWD_SPHERE, 10 + rc_ptr->lvl * 3 / 2);
        rpi.text = _("対象のモンスター1体を位置を交換する。", "Swaps positions between you and a target monster.");
        rpi.min_level = 15;
        rpi.cost = 12;
        rpi.stat = A_DEX;
        rpi.fail = 16;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::SWAP_POS));
    }

    if (player_ptr->muta.has(PlayerMutationType::SHRIEK)) {
        rpi = rpi_type(_("叫び", "Shriek"));
        rpi.text = _("自分を中心とする轟音のボールを放ち、周辺の敵を怒らせる。", "Bursts a sound ball and aggravates all monsters around you. ");
        rpi.min_level = 20;
        rpi.cost = 14;
        rpi.stat = A_CON;
        rpi.fail = 16;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::SHRIEK));
    }

    if (player_ptr->muta.has(PlayerMutationType::ILLUMINE)) {
        rpi = rpi_type(_("照明", "Illuminate"));
        rpi.text = _("光源が照らしている範囲か部屋全体を永久に明るくする。", "Lights up nearby area and the inside of a room permanently.");
        rpi.min_level = 3;
        rpi.cost = 2;
        rpi.stat = A_INT;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::ILLUMINE));
    }

    if (player_ptr->muta.has(PlayerMutationType::DET_CURSE)) {
        rpi = rpi_type(_("呪い感知", "Detect Curses"));
        rpi.text = _("持ち物及び装備の中の呪われたアイテムを感知する。", "Feels curses of objects in your inventory.");
        rpi.min_level = 7;
        rpi.cost = 14;
        rpi.stat = A_WIS;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::DET_CURSE));
    }

    if (player_ptr->muta.has(PlayerMutationType::BERSERK)) {
        rpi = rpi_type(_("狂戦士化", "Berserk"));
        rpi.info = format("%s%d+d%d", KWD_DURATION, 25, 25);
        rpi.text = _("狂戦士化し、恐怖を除去する。防御力が少し低下する。", "Gives a bonus to hit and HP, immunity to fear for a while. But decreases AC.");
        rpi.min_level = 8;
        rpi.cost = 8;
        rpi.stat = A_STR;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::BERSERK));
    }

    if (player_ptr->muta.has(PlayerMutationType::POLYMORPH)) {
        rpi = rpi_type(_("変身", "Polymorph"));
        rpi.text = _("別の種族に変身する。", "Morphs you to another race.");
        rpi.min_level = 18;
        rpi.cost = 20;
        rpi.stat = A_CON;
        rpi.fail = 18;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::POLYMORPH));
    }

    if (player_ptr->muta.has(PlayerMutationType::MIDAS_TCH)) {
        rpi = rpi_type(_("ミダスの手", "Midas Touch"));
        rpi.text = _("アイテム1つをお金に変える。", "Turns an item into 1/3 of its value in gold.");
        rpi.min_level = 10;
        rpi.cost = 5;
        rpi.stat = A_INT;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::MIDAS_TCH));
    }

    if (player_ptr->muta.has(PlayerMutationType::GROW_MOLD)) {
        rpi = rpi_type(_("カビ発生", "Grow Mold"));
        rpi.text = _("モルドを召喚する。", "Summons molds.");
        rpi.min_level = 1;
        rpi.cost = 6;
        rpi.stat = A_CON;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::GROW_MOLD));
    }

    if (player_ptr->muta.has(PlayerMutationType::RESIST)) {
        rpi = rpi_type(_("エレメント耐性", "Resist Elements"));
        rpi.info = format("%s%d+d%d", KWD_DURATION, 20, 20);
        rpi.text = _("1つ以上の元素の一時耐性を得る。", "Gains one or more resitances for elements.");
        rpi.min_level = 10;
        rpi.cost = 12;
        rpi.stat = A_CON;
        rpi.fail = 12;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::RESIST));
    }

    if (player_ptr->muta.has(PlayerMutationType::EARTHQUAKE)) {
        rpi = rpi_type(_("地震", "Earthquake"));
        rpi.info = format("%s%d", KWD_SPHERE, 10);
        rpi.text = _("周囲のダンジョンを揺らし、壁と床をランダムに入れ変える。", "Shakes dungeon structure, and results in random swapping of floors and walls.");
        rpi.min_level = 12;
        rpi.cost = 12;
        rpi.stat = A_STR;
        rpi.fail = 16;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::EARTHQUAKE));
    }

    if (player_ptr->muta.has(PlayerMutationType::EAT_MAGIC)) {
        rpi = rpi_type(_("魔力食い", "Eat Magic"));
        rpi.info = format("%s%d", KWD_POWER, rc_ptr->lvl * 2);
        rpi.text = _("魔法道具から魔力を吸収してMPを回復する。", "Absorbs mana from a magic device to heal your SP.");
        rpi.min_level = 17;
        rpi.cost = 1;
        rpi.stat = A_WIS;
        rpi.fail = 15;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::EAT_MAGIC));
    }

    if (player_ptr->muta.has(PlayerMutationType::WEIGH_MAG)) {
        rpi = rpi_type(_("魔力感知", "Weigh Magic"));
        rpi.text = _("現在得ている一時的な魔法の能力を知る。", "Reports all temporary magical abilities.");
        rpi.min_level = 6;
        rpi.cost = 6;
        rpi.stat = A_INT;
        rpi.fail = 10;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::WEIGH_MAG));
    }

    if (player_ptr->muta.has(PlayerMutationType::STERILITY)) {
        rpi = rpi_type(_("増殖阻止", "Sterilize"));
        rpi.text = _("この階の増殖するモンスターが増殖できなくなる。", "Prevents any breeders on current level from breeding.");
        rpi.min_level = 12;
        rpi.cost = 23;
        rpi.stat = A_CHR;
        rpi.fail = 15;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::STERILITY));
    }

    if (player_ptr->muta.has(PlayerMutationType::HIT_AND_AWAY)) {
        rpi = rpi_type(_("ヒット＆アウェイ", "Panic Hit"));
        rpi.info = format("%s%d", KWD_SPHERE, 30);
        rpi.text = _("対象のモンスターを攻撃したあと短距離テレポートする。", "Attacks a monster then tereports you a short range.");
        rpi.min_level = 10;
        rpi.cost = 12;
        rpi.stat = A_DEX;
        rpi.fail = 14;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::HIT_AND_AWAY));
    }

    if (player_ptr->muta.has(PlayerMutationType::DAZZLE)) {
        rpi = rpi_type(_("眩惑", "Dazzle"));
        rpi.info = format("%s%d", KWD_POWER, rc_ptr->lvl * 4);
        rpi.text = _("周辺のモンスターを混乱・朦朧・恐怖させようとする。抵抗されると無効。",
            "Tries to make all monsters in your sight confused, stunned, scared.");
        rpi.min_level = 7;
        rpi.cost = 15;
        rpi.stat = A_CHR;
        rpi.fail = 8;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::DAZZLE));
    }

    if (player_ptr->muta.has(PlayerMutationType::LASER_EYE)) {
        rpi = rpi_type(_("レーザー・アイ", "Laser Eye"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 2);
        rpi.text = _("閃光のビームを放つ。", "Fires a beam of light.");
        rpi.min_level = 7;
        rpi.cost = 10;
        rpi.stat = A_WIS;
        rpi.fail = 9;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::LASER_EYE));
    }

    if (player_ptr->muta.has(PlayerMutationType::RECALL)) {
        rpi = rpi_type(_("帰還", "Recall"));
        rpi.text = _("地上にいるときはダンジョンの最深階へ、ダンジョンにいるときは地上へと移動する。",
            "Recalls player from dungeon to town or from town to the deepest level of dungeon.");
        rpi.min_level = 17;
        rpi.cost = 50;
        rpi.stat = A_INT;
        rpi.fail = 16;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::RECALL));
    }

    if (player_ptr->muta.has(PlayerMutationType::BANISH)) {
        rpi = rpi_type(_("邪悪消滅", "Banish Evil"));
        rpi.info = format("%s%d", KWD_POWER, 50 + rc_ptr->lvl);
        rpi.text = _("邪悪なモンスター1体を階から消滅させる。", "Erase a evil monster from current level.");
        rpi.min_level = 25;
        rpi.cost = 25;
        rpi.stat = A_WIS;
        rpi.fail = 18;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::BANISH));
    }

    if (player_ptr->muta.has(PlayerMutationType::COLD_TOUCH)) {
        rpi = rpi_type(_("凍結の手", "Cold Touch"));
        rpi.info = format("%s%d", KWD_DAM, rc_ptr->lvl * 2);
        rpi.text = _("冷気のビームを放つ。", "Fires a beam of cold.");
        rpi.min_level = 2;
        rpi.cost = 2;
        rpi.stat = A_CON;
        rpi.fail = 11;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::COLD_TOUCH));
    }

    if (player_ptr->muta.has(PlayerMutationType::LAUNCHER)) {
        rpi = rpi_type(_("アイテム投げ", "Throw Object"));
        rpi.text = _("強力な威力で物を投げる。", "Throws an object strongly.");
        rpi.min_level = 1;
        rpi.cost = rc_ptr->lvl;
        rpi.stat = A_STR;
        rpi.fail = 6;
        rc_ptr->add_power(rpi, enum2i(PlayerMutationType::LAUNCHER));
    }
}
