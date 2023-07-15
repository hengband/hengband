#include "wizard/fixed-artifacts-spoiler.h"
#include "io/files-util.h"
#include "object/object-kind-hook.h"
#include "system/angband-version.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "wizard/artifact-analyzer.h"
#include "wizard/spoiler-util.h"
#include <sstream>

/*!
 * @brief フラグ名称を出力する汎用関数
 * @param header ヘッダに出力するフラグ群の名前
 * @param descriptions フラグ名リスト
 * @param separator フラグ表示の区切り記号
 */
void spoiler_outlist(std::string_view header, const std::vector<std::string> &descriptions, char separator)
{
    if (descriptions.empty()) {
        return;
    }

    std::string line = spoiler_indent;
    if (!header.empty()) {
        line.append(header).append(" ");
    }

    for (size_t i = 0; i < descriptions.size(); i++) {
        auto elem = descriptions[i];
        if (i < descriptions.size() - 1) {
            elem.push_back(separator);
            elem.push_back(' ');
        }

        if (line.length() + elem.length() <= MAX_LINE_LEN) {
            line.append(elem);
            continue;
        }

        if (line.length() > 1 && line[line.length() - 1] == ' ' && line[line.length() - 2] == list_separator) {
            line[line.length() - 2] = '\0';
            fprintf(spoiler_file, "%s\n", line.data());
            line = spoiler_indent;
            line.append(elem);
        } else {
            fprintf(spoiler_file, "%s\n", line.data());
            line = "      ";
            line.append(elem);
        }
    }

    fprintf(spoiler_file, "%s\n", line.data());
}

/*!
 * @brief アーティファクト情報を出力するためにダミー生成を行う
 * @param fixed_artifact_idx 生成するアーティファクトID
 * @return 生成したアーティファクト (連番で埋まっているので不存在例外は吐かない)
 */
static ItemEntity make_fake_artifact(FixedArtifactId fixed_artifact_idx)
{
    const auto &artifact = ArtifactsInfo::get_instance().get_artifact(fixed_artifact_idx);
    const auto bi_id = lookup_baseitem_id(artifact.bi_key);
    ItemEntity item;
    item.prep(bi_id);
    item.fixed_artifact_idx = fixed_artifact_idx;
    item.pval = artifact.pval;
    item.ac = artifact.ac;
    item.dd = artifact.dd;
    item.ds = artifact.ds;
    item.to_a = artifact.to_a;
    item.to_h = artifact.to_h;
    item.to_d = artifact.to_d;
    item.weight = artifact.weight;
    return item;
}

/*!
 * @brief アーティファクト一件をスポイラー出力する /
 * Create a spoiler file entry for an artifact
 * @param art_ptr アーティファクト情報をまとめた構造体の参照ポインタ
 */
static void spoiler_print_art(obj_desc_list *art_ptr)
{
    const auto *pval_ptr = &art_ptr->pval_info;
    fprintf(spoiler_file, "%s\n", art_ptr->description.data());
    if (!pval_ptr->pval_desc.empty()) {
        std::stringstream ss;
        ss << pval_ptr->pval_desc << _("の修正:", " to");
        spoiler_outlist(ss.str(), pval_ptr->pval_affects, item_separator);
    }

    spoiler_outlist(_("対:", "Slay"), art_ptr->slays, item_separator);
    spoiler_outlist(_("武器属性:", ""), art_ptr->brands, list_separator);
    spoiler_outlist(_("免疫:", "Immunity to"), art_ptr->immunities, item_separator);
    spoiler_outlist(_("耐性:", "Resist"), art_ptr->resistances, item_separator);
    spoiler_outlist(_("弱点:", "Vulnerable"), art_ptr->vulnerabilities, item_separator);
    spoiler_outlist(_("維持:", "Sustain"), art_ptr->sustenances, item_separator);
    spoiler_outlist("", art_ptr->misc_magic, list_separator);

    if (!art_ptr->addition.empty()) {
        fprintf(spoiler_file, _("%s追加: %s\n", "%sAdditional %s\n"), spoiler_indent.data(), art_ptr->addition.data());
    }

    if (!art_ptr->activation.empty()) {
        fprintf(spoiler_file, _("%s発動: %s\n", "%sActivates for %s\n"), spoiler_indent.data(), art_ptr->activation.data());
    }

    fprintf(spoiler_file, "%s%s\n\n", spoiler_indent.data(), art_ptr->misc_desc.data());
}

/*!
 * @brief アーティファクト情報のスポイラー出力を行うメインルーチン
 */
SpoilerOutputResultType spoil_fixed_artifact()
{
    const auto &path = path_build(ANGBAND_DIR_USER, "artifact.txt");
    spoiler_file = angband_fopen(path, FileOpenMode::WRITE);
    if (!spoiler_file) {
        return SpoilerOutputResultType::FILE_OPEN_FAILED;
    }

    std::stringstream ss;
    ss << "Artifact Spoilers for Hengband Version " << get_version();
    spoiler_underline(ss.str());
    for (const auto &[tval_list, name] : group_artifact_list) {
        spoiler_blanklines(2);
        spoiler_underline(name);
        spoiler_blanklines(1);

        for (auto tval : tval_list) {
            for (const auto &[a_idx, artifact] : artifacts_info) {
                if (artifact.bi_key.tval() != tval) {
                    continue;
                }

                const auto item = make_fake_artifact(a_idx);
                PlayerType dummy;
                obj_desc_list artifact_descriptions;
                object_analyze(&dummy, &item, &artifact_descriptions);
                spoiler_print_art(&artifact_descriptions);
            }
        }
    }

    return ferror(spoiler_file) || angband_fclose(spoiler_file) ? SpoilerOutputResultType::FILE_CLOSE_FAILED
                                                                : SpoilerOutputResultType::SUCCESSFUL;
}
