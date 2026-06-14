#include "system/baseitem/baseitem-configs.h"
#include "system/angband-exceptions.h"
#include "system/baseitem/baseitem-config.h"
#include "term/z-rand.h"
#include <fmt/format.h>
#include <span>

BaseitemConfigs BaseitemConfigs::instance{};

BaseitemConfigs::~BaseitemConfigs() = default;

BaseitemConfigs &BaseitemConfigs::get_instance()
{
    return instance;
}

void BaseitemConfigs::emplace_back(const DisplaySymbol &ds)
{
    this->configs.emplace_back(ds);
}

const BaseitemConfig &BaseitemConfigs::get_config(short bi_id) const
{
    this->validate(bi_id);
    return this->configs.at(bi_id);
}

BaseitemConfig &BaseitemConfigs::get_config(short bi_id)
{
    this->validate(bi_id);
    return this->configs[bi_id];
}

/*!
 * @brief BaseitemConfigをランダムに1つ選ぶ
 *
 * 0番はダミーアイテム.
 * このクラスではベースアイテムの有効無効は分からないので呼び出し元で判定すること.
 */
const BaseitemConfig &BaseitemConfigs::pick_one_at_random() const
{
    const auto candidates = std::span(this->configs).subspan(1);
    return rand_choice(candidates);
}

char BaseitemConfigs::get_character(short bi_id) const
{
    if (bi_id == 0) {
        return '\0';
    }

    this->validate(bi_id);
    return this->configs.at(bi_id).get_character();
}

uint8_t BaseitemConfigs::get_color(short bi_id) const
{
    if (bi_id == 0) {
        return 0;
    }

    this->validate(bi_id);
    return this->configs.at(bi_id).get_color();
}

void BaseitemConfigs::set_config(short bi_id, const DisplaySymbol &ds)
{
    if (bi_id == 0) {
        return;
    }

    this->validate(bi_id);
    this->configs[bi_id].set_symbol(ds);
}

void BaseitemConfigs::validate(short bi_id) const
{
    if ((bi_id < 0) || (bi_id >= static_cast<short>(this->size()))) {
        THROW_EXCEPTION(std::runtime_error, fmt::format("Invalid Baseitem ID: {}", bi_id));
    }
}
