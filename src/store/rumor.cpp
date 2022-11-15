#include "store/rumor.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-town.h"
#include "io/files-util.h"
#include "io/tokenizer.h"
#include "monster-race/monster-race.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-kind-hook.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info-definition.h"
#include "system/dungeon-info.h"
#include "system/monster-race-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>
#include <string>
#include <utility>

/*
 * @brief 固定アーティファクト、モンスター、町 をランダムに1つ選び、ダンジョンを固定的に1つ選ぶ
 * @param zz 検索文字列
 * @param max_idx briefに挙げた各リストにおける最大数
 * @details rumor.txt (rumor_j.txt) の定義により、ダンジョンは鉄獄 (ダンジョンID1)が常に選ばれる
 * その他は常にランダム ("*")
 */
static short get_rumor_num(std::string_view zz, short max_idx)
{
    if (zz == "*") {
        return randint1(max_idx);
    }

    return static_cast<short>(atoi(zz.data()));
}

static std::string_view bind_rumor_name(std::string &base, concptr fullname)
{
    auto *s = strstr(base.data(), "{Name}");
    if (s) {
        s[0] = '\0';
        return format("%s%s%s", base.data(), fullname, (s + 6));
    }

    return base;
}

/*
 * @brief 噂の、町やモンスターを表すトークンを得る
 * @param rumor rumor.txt (rumor_j.txt)の1行
 * @return トークン読み込み成否 とトークン群の配列
 * @todo tmp_tokensを使わず単なるsplitにすればもっと簡略化できそう
 */
std::pair<bool, std::vector<std::string>> get_rumor_tokens(std::string rumor)
{
    constexpr auto num_tokens = 3;
    char *tmp_tokens[num_tokens];
    if (tokenize(rumor.data() + 2, num_tokens, tmp_tokens, TOKENIZE_CHECKQUOTE) != num_tokens) {
        msg_print(_("この情報は間違っている。", "This information is wrong."));
        return { false, {} };
    }

    std::vector<std::string> tokens(std::begin(tmp_tokens), std::end(tmp_tokens));
    return { true, tokens };
}

/*!
 * @brief 固定アーティファクト番号とその定義を、ランダムに抽選する
 * @param artifact_name rumor.txt (rumor_j.txt)の定義により、常に"*" (ランダム)
 * @details 固定アーティファクト番号は欠番があるので、もし欠番だったら再抽選する
 */
static std::pair<FixedArtifactId, const ArtifactType *> get_artifact_definition(std::string_view artifact_name)
{
    const auto max_idx = enum2i(artifacts_info.rbegin()->first);
    while (true) {
        const auto a_idx = i2enum<FixedArtifactId>(get_rumor_num(artifact_name.data(), max_idx));
        const auto *a_ptr = ArtifactsInfo::get_instance().get_artifact(a_idx);
        if (a_ptr == nullptr) {
            continue;
        }

        if (!a_ptr->name.empty()) {
            return { a_idx, a_ptr };
        }
    }
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

    const auto &[is_correct, tokens] = get_rumor_tokens(rumor);
    if (!is_correct) {
        return;
    }

    concptr rumor_eff_format = nullptr;
    char fullname[1024] = "";
    const auto &category = tokens[0];
    if (category == "ARTIFACT") {
        const auto &artifact_name = tokens[1];
        const auto &[a_idx, a_ptr] = get_artifact_definition(artifact_name);
        const auto k_idx = lookup_baseitem_id(a_ptr->bi_key);
        ObjectType item;
        item.prep(k_idx);
        item.fixed_artifact_idx = a_idx;
        item.ident = IDENT_STORE;
        describe_flavor(player_ptr, fullname, &item, OD_NAME_ONLY);
    } else if (category == "MONSTER") {
        monster_race *r_ptr;
        const auto &monster_name = tokens[1];

        // @details プレイヤーもダミーで入っているので、1つ引いておかないと数が合わなくなる.
        const auto monraces_size = static_cast<short>(monraces_info.size() - 1);
        while (true) {
            auto r_idx = i2enum<MonsterRaceId>(get_rumor_num(monster_name, monraces_size));
            r_ptr = &monraces_info[r_idx];
            if (!r_ptr->name.empty()) {
                break;
            }
        }

        strcpy(fullname, r_ptr->name.data());

        if (!r_ptr->r_sights) {
            r_ptr->r_sights++;
        }
    } else if (category == "DUNGEON") {
        DUNGEON_IDX d_idx;
        dungeon_type *d_ptr;
        const auto dungeons_size = static_cast<short>(dungeons_info.size());
        const auto &d_idx_str = tokens[1];
        while (true) {
            d_idx = get_rumor_num(d_idx_str, dungeons_size);
            d_ptr = &dungeons_info[d_idx];
            if (!d_ptr->name.empty()) {
                break;
            }
        }

        strcpy(fullname, d_ptr->name.data());
        if (!max_dlv[d_idx]) {
            max_dlv[d_idx] = d_ptr->mindepth;
            rumor_eff_format = _("%sに帰還できるようになった。", "You can recall to %s.");
        }
    } else if (category == "TOWN") {
        IDX t_idx;
        const auto &town_name = tokens[1];
        while (true) {
            t_idx = get_rumor_num(town_name, VALID_TOWNS);
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
    } else {
        throw std::runtime_error("Unknown token exists in rumor.txt");
    }

    auto base = std::string(tokens[2]);
    const auto rumor_msg = bind_rumor_name(base, fullname);
    msg_print(rumor_msg);
    if (rumor_eff_format) {
        msg_print(nullptr);
        msg_format(rumor_eff_format, fullname);
    }
}
