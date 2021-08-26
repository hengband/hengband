#pragma once

#include "object/tval-types.h"

#include <functional>

struct object_type;
struct player_type;

class ItemTester {
public:
    ItemTester() = default;
    ItemTester(std::function<bool(const object_type *)> pred);
    ItemTester(std::function<bool(player_type *, const object_type *)> pred);

    bool okay(player_type *player_ptr, const object_type *o_ptr, tval_type tval) const;

    void set_tester(std::function<bool(const object_type *)> pred);
    void set_tester(std::function<bool(player_type *, const object_type *)> pred);

private:
    std::function<bool(player_type *, const object_type *)> tester;
};
