/*!
 * @brief 対邪悪結界の一時効果
 * @author Hourier
 * @date 2024/10/21
 */
#pragma once

class PlayerProtection {
public:
    PlayerProtection() = default;
    ~PlayerProtection() = default;
    PlayerProtection(const PlayerProtection &) = delete;
    PlayerProtection(PlayerProtection &&) = delete;
    PlayerProtection &operator=(const PlayerProtection &) = delete;
    PlayerProtection &operator=(PlayerProtection &&) = delete;

    short current() const;
    bool is_protected() const;
    bool is_larger_than(short value) const;
    void set(short value);
    void reset();

private:
    short protection = 0;
};
