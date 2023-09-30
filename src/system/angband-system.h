#pragma once

class AngbandSystem {
public:
    AngbandSystem(const AngbandSystem &) = delete;
    AngbandSystem(AngbandSystem &&) = delete;
    AngbandSystem &operator=(const AngbandSystem &) = delete;
    AngbandSystem &operator=(AngbandSystem &&) = delete;

    static AngbandSystem &get_instance();

private:
    AngbandSystem() = default;

    static AngbandSystem instance;
};
