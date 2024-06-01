#pragma once

enum class PlayerRaceType : int;
class InnerGameData {
public:
    InnerGameData(InnerGameData &&) = delete;
    InnerGameData(const InnerGameData &) = delete;
    InnerGameData &operator=(const InnerGameData &) = delete;
    InnerGameData &operator=(InnerGameData &&) = delete;
    ~InnerGameData() = default;
    static InnerGameData &get_instance();

    PlayerRaceType get_start_race() const;
    int get_game_turn_limit() const;
    int get_real_turns(int turns) const;
    void set_start_race(PlayerRaceType race);
    void init_turn_limit();

private:
    InnerGameData();
    static InnerGameData instance;

    PlayerRaceType start_race; //!< ゲーム開始時の種族.
    int game_turn_limit = 0; //!< 最大ゲームターン
};
