#include "store/rumor.h"
#include "dungeon/dungeon.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-town.h"
#include "floor/wild.h"
#include "io/files-util.h"
#include "io/tokenizer.h"
#include "monster-race/monster-race.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-kind-hook.h"
#include "system/artifact-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"

/*
 * Display a rumor and apply its effects
 */

IDX rumor_num(char *zz, IDX max_idx)
{
    if (strcmp(zz, "*") == 0) {
        return randint1(max_idx - 1);
    }
    return (IDX)atoi(zz);
}

concptr rumor_bind_name(char *base, concptr fullname)
{
    char *s, *v;
    s = strstr(base, "{Name}");
    if (s) {
        s[0] = '\0';
        v = format("%s%s%s", base, fullname, (s + 6));
        return v;
    }

    v = base;
    return v;
}

void display_rumor(PlayerType *player_ptr, bool ex)
{
    char rumor[1024];
    int section = (ex && (randint0(3) == 0)) ? 1 : 0;
    errr err = _(get_rnd_line_jonly("rumors_j.txt", section, rumor, 10), get_rnd_line("rumors.txt", section, rumor));
    if (err) {
        strcpy(rumor, _("嘘の噂もある。", "Some rumors are wrong."));
    }

    if (strncmp(rumor, "R:", 2) != 0) {
        msg_format("%s", rumor);
        return;
    }

    char *zz[4];
    if (tokenize(rumor + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) {
        msg_print(_("この情報は間違っている。", "This information is wrong."));
        return;
    }

    concptr rumor_eff_format = nullptr;
    char fullname[1024] = "";
    if (strcmp(zz[0], "ARTIFACT") == 0) {
        FixedArtifactId a_idx;
        ArtifactType *a_ptr;
        while (true) {
            a_idx = i2enum<FixedArtifactId>(rumor_num(zz[1], enum2i(a_info.rbegin()->first)));
            a_ptr = &a_info.at(a_idx);
            if (!a_ptr->name.empty()) {
                break;
            }
        }

        KIND_OBJECT_IDX k_idx = lookup_kind(a_ptr->tval, a_ptr->sval);
        ObjectType forge;
        auto *q_ptr = &forge;
        q_ptr->prep(k_idx);
        q_ptr->fixed_artifact_idx = a_idx;
        q_ptr->ident = IDENT_STORE;
        describe_flavor(player_ptr, fullname, q_ptr, OD_NAME_ONLY);
    } else if (strcmp(zz[0], "MONSTER") == 0) {
        monster_race *r_ptr;
        while (true) {
            auto r_idx = i2enum<MonsterRaceId>(rumor_num(zz[1], static_cast<IDX>(r_info.size())));
            r_ptr = &r_info[r_idx];
            if (!r_ptr->name.empty()) {
                break;
            }
        }

        strcpy(fullname, r_ptr->name.c_str());

        if (!r_ptr->r_sights) {
            r_ptr->r_sights++;
        }
    } else if (strcmp(zz[0], "DUNGEON") == 0) {
        DUNGEON_IDX d_idx;
        dungeon_type *d_ptr;
        while (true) {
            d_idx = rumor_num(zz[1], static_cast<IDX>(d_info.size()));
            d_ptr = &d_info[d_idx];
            if (!d_ptr->name.empty()) {
                break;
            }
        }

        strcpy(fullname, d_ptr->name.c_str());

        if (!max_dlv[d_idx]) {
            max_dlv[d_idx] = d_ptr->mindepth;
            rumor_eff_format = _("%sに帰還できるようになった。", "You can recall to %s.");
        }
    } else if (strcmp(zz[0], "TOWN") == 0) {
        IDX t_idx;
        while (true) {
            t_idx = rumor_num(zz[1], NO_TOWN);
            if (town_info[t_idx].name[0] != '\0') {
                break;
            }
        }

        strcpy(fullname, town_info[t_idx].name);

        int32_t visit = (1UL << (t_idx - 1));
        if ((t_idx != SECRET_TOWN) && !(player_ptr->visit & visit)) {
            player_ptr->visit |= visit;
            rumor_eff_format = _("%sに行ったことがある気がする。", "You feel you have been to %s.");
        }
    }

    concptr rumor_msg = rumor_bind_name(zz[2], fullname);
    msg_print(rumor_msg);
    if (rumor_eff_format) {
        msg_print(nullptr);
        msg_format(rumor_eff_format, fullname);
    }
}
