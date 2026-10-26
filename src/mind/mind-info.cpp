#include "mind/mind-info.h"
#include "cmd-action/cmd-spell.h"
#include "locale/japanese.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-types.h"
#include "player-info/equipment-info.h"
#include "system/player-type-definition.h"
#include <fmt/format.h>

static std::string switch_mind_mindcrafter(PlayerType *player_ptr, const PLAYER_LEVEL plev, const int power)
{
    switch (power) {
    case 1:
        return fmt::format(" {}{}d{}", KWD_DAM, 3 + ((plev - 1) / 4), 3 + plev / 15);
    case 2:
        return fmt::format(" {}10", KWD_SPHERE);
    case 3:
        return fmt::format(" {}{}", KWD_SPHERE, plev * 5);
    case 5:
        return fmt::format(" {}{}d8", KWD_DAM, 8 + ((plev - 5) / 4));
    case 6:
        return fmt::format(" {}{}", KWD_DURATION, plev);
    case 8:
        if (plev < 25) {
            return fmt::format(" {}{}", KWD_DAM, plev * 3 / 2);
        }

        return fmt::format(" {}d{}", KWD_DAM, plev * ((plev - 5) / 10 + 1));
    case 9:
        return fmt::format(" {}10+d{}", KWD_DURATION, plev * 3 / 2);
#ifdef JP
    case 10:
        return fmt::format(" 最大重量:{}.{}kg", lb_to_kg_integer(plev * 15), lb_to_kg_fraction(plev * 15));
#else
    case 10:
        return fmt::format(" max wgt {}", plev * 15);
#endif
    case 11:
        return fmt::format(" {}{}d6", KWD_DAM, plev / 2);
    case 12:
        return fmt::format(" {}d{}+{}", KWD_DAM, plev * 3, plev * 3);
    case 13:
        return fmt::format(_(" 行動:{}回", " {} acts."), (player_ptr->csp + 100 - player_ptr->energy_need - 50) / 100);
    default:
        return std::string();
    }
}

static std::string switch_mind_ki(PlayerType *player_ptr, const PLAYER_LEVEL plev, const int power)
{
    int boost = get_current_ki(player_ptr);
    if (heavy_armor(player_ptr)) {
        boost /= 2;
    }

    switch (power) {
    case 0:
        return fmt::format(" {}{}d4", KWD_DAM, 3 + ((plev - 1) / 5) + boost / 12);
    case 2:
        return fmt::format(" {}{}+d30", KWD_DURATION, 30 + boost / 5);
    case 3:
        return fmt::format(" {}{}d5", KWD_DAM, 5 + ((plev - 1) / 5) + boost / 10);
    case 4:
        return fmt::format(" {}{}+d20", KWD_DURATION, 20 + boost / 5);
    case 6:
        return fmt::format(" {}{}+d{}", KWD_DURATION, 15 + boost / 7, plev / 2);
    case 7:
        return fmt::format(" {}{}d8", KWD_DAM, 8 + ((plev - 5) / 4) + boost / 12);
    case 8:
        return fmt::format(" {}10d6+{}", KWD_DAM, plev * 3 / 2 + boost * 3 / 5);
    case 10:
        return fmt::format(_(" 最大{}体", " max {}"), 1 + boost / 100);
    case 11:
        return fmt::format(" {}{}", KWD_DAM, 100 + plev + boost);
    case 12:
        return fmt::format(" {}{}d15", KWD_DAM, 10 + plev / 2 + boost * 3 / 10);
    case 13:
        return fmt::format(_(" 行動:{}+d16回", " {}+d16 acts"), 16 + boost / 20);
    default:
        return std::string();
    }
}

static std::string switch_mind_mirror_master(const PLAYER_LEVEL plev, const int power)
{
    switch (power) {
    case 2:
        return fmt::format(" {}{}d4", KWD_DAM, 3 + ((plev - 1) / 5));
    case 3:
        return fmt::format(" {}10", KWD_SPHERE);
    case 5:
        return fmt::format(" {}{}", KWD_SPHERE, plev * 5);
    case 6:
        return fmt::format(" {}20+d20", KWD_DURATION);
    case 8:
        return fmt::format(" {}{}d8", KWD_DAM, 8 + ((plev - 5) / 4));
    case 10:
        return fmt::format(" {}{}d8", KWD_DAM, 11 + (plev - 5) / 4);
    case 12:
        return fmt::format(" {}20+d20", KWD_DURATION);
    case 13:
        return fmt::format(" {}150+d{}", KWD_DAM, plev * 2);
    case 16:
        return fmt::format(" {}{}", KWD_SPHERE, plev / 2 + 10);
    case 18:
        return fmt::format(" {}6+d6", KWD_DURATION);
    case 19:
        return fmt::format(" {}{}", KWD_DAM, plev * 11 + 5);
    case 20:
        return fmt::format(" {}4+d4", KWD_DURATION);
    default:
        return std::string();
    }
}

static std::string switch_mind_ninja(const PLAYER_LEVEL plev, const int power)
{
    switch (power) {
    case 2:
        return fmt::format(" {}10", KWD_SPHERE);
    case 4:
        return fmt::format(" {}{}", KWD_SPHERE, plev * 5);
    case 5:
        return fmt::format(" {}30", KWD_SPHERE);
    case 8:
        return fmt::format(" {}20+d20", KWD_DURATION);
    case 9:
        return fmt::format(" {}{}", KWD_DAM, (50 + plev) / 2);
    case 16:
        return fmt::format(" {}{}+d{}", KWD_DURATION, plev / 2, plev / 2);
    case 17:
        return fmt::format(" {}{}*3", KWD_DAM, (75 + plev * 2 / 3) / 2);
    case 18:
        return fmt::format(" {}{}d10", KWD_DAM, 6 + plev / 8);
    case 19:
        return fmt::format(" {}6+d6", KWD_DURATION);
    default:
        return std::string();
    }
}

/*!
 * @brief 特殊技能の効果情報をまとめたフォーマットを返す
 * @param use_mind 職業毎の特殊技能ID
 * @param power モンスター魔法のID
 * @return std::string 特殊技能の情報を表す文字列
 */
std::string mindcraft_info(PlayerType *player_ptr, MindKindType use_mind, int power)
{
    const PLAYER_LEVEL plev = player_ptr->lev;
    switch (use_mind) {
    case MindKindType::MINDCRAFTER:
        return switch_mind_mindcrafter(player_ptr, plev, power);
    case MindKindType::KI:
        return switch_mind_ki(player_ptr, plev, power);
    case MindKindType::MIRROR_MASTER:
        return switch_mind_mirror_master(plev, power);
    case MindKindType::NINJUTSU:
        return switch_mind_ninja(plev, power);
    default:
        return std::string();
    }
}
