#pragma once

class PlayerType;

class PlayerSpellStatus {
public:
    PlayerSpellStatus(PlayerType *player_ptr);

    class Realm {
    public:
        Realm(PlayerType *player_ptr, bool is_realm1);

        void initialize();
        bool is_nothing_learned() const;
        bool is_learned(int spell_id) const;
        bool is_worked(int spell_id) const;
        bool is_forgotten(int spell_id) const;
        void set_learned(int spell_id, bool value = true);
        void set_worked(int spell_id, bool value = true);
        void set_forgotten(int spell_id, bool value = true);

    private:
        PlayerType *player_ptr;
        bool is_realm1;
    };

    Realm realm1() const;
    Realm realm2() const;

private:
    PlayerType *player_ptr;
};
