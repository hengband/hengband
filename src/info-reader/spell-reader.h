#pragma once

#include "info-reader/parse-error-types.h"
#include <map>
#include <nlohmann/json_fwd.hpp>
#include <set>
#include <string>
#include <tl/expected.hpp>
#include <vector>

enum class RealmType;
class SpellInfo;
class SpellInfoList;

class SpellReader {
public:
    //! 1回の呪文定義の読み込み (init_json の呼び出し) ごとに所有し、登録済みのID・タグを領域ごとに保持する
    class Session {
        friend class SpellReader;

        struct RealmSpells {
            std::set<short> ids;
            std::set<std::string> tags;
        };
        std::map<RealmType, RealmSpells> realms;
    };

    SpellReader(const nlohmann::json &realm_data, SpellInfoList &spell_info_list, Session &session);
    SpellReader(nlohmann::json &&, SpellInfoList &, Session &) = delete;
    SpellReader(const SpellReader &) = delete;
    SpellReader(SpellReader &&) = delete;
    SpellReader &operator=(const SpellReader &) = delete;
    SpellReader &operator=(SpellReader &&) = delete;

    tl::expected<void, ParseErrorType> read() const;

private:
    tl::expected<RealmType, ParseErrorType> read_realm() const;
    tl::expected<void, ParseErrorType> set_spell_data(const nlohmann::json &spell_data, std::vector<SpellInfo> &spells) const;
    tl::expected<void, ParseErrorType> set_book_data(std::vector<SpellInfo> &spells) const;

    const nlohmann::json &realm_data;
    SpellInfoList &spell_info_list;
    Session &session;
};
