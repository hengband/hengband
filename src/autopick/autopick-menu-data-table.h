#pragma once

#include "autopick/autopick-commands-table.h"
#include "util/abstract-vector-wrapper.h"
#include <string_view>
#include <tl/optional.hpp>
#include <vector>

struct CommandMenuDatum {
public:
    CommandMenuDatum(std::string_view name, tl::optional<size_t> level, tl::optional<char> key, tl::optional<EditorCommandId> com_id)
        : name(name)
        , depth(level)
        , key(key)
        , com_id(com_id)
    {
    }

    std::string_view name;
    tl::optional<size_t> depth; //!< メニュー階層.
    tl::optional<char> key;
    tl::optional<EditorCommandId> com_id;
};

class CommandMenuData : public util::AbstractVectorWrapper<CommandMenuDatum> {
public:
    CommandMenuData(CommandMenuData &&) = delete;
    CommandMenuData(const CommandMenuData &) = delete;
    CommandMenuData &operator=(const CommandMenuData &) = delete;
    CommandMenuData &operator=(CommandMenuData &&) = delete;

    static CommandMenuData &get_instance();

    void initialize();
    const CommandMenuDatum &get_datum(size_t num) const;
    EditorCommandId get_com_id(char key) const;

private:
    CommandMenuData() = default;

    static CommandMenuData instance;

    std::vector<CommandMenuDatum> menu_data;

    std::vector<CommandMenuDatum> &get_inner_container() override
    {
        return this->menu_data;
    }
};
