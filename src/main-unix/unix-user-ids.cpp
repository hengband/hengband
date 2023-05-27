/*!
 * @brief UNIX用ユーザID定義
 * @author Hourier
 * @date 2023/05/27
 */

#include "main-unix/unix-user-ids.h"

UnixUserIds UnixUserIds::instance{};

UnixUserIds &UnixUserIds::get_instance()
{
    return instance;
}

int UnixUserIds::get_user_id() const
{
    return this->user_id;
}

void UnixUserIds::set_user_id(const int id)
{
    this->user_id = id;
}

void UnixUserIds::mod_user_id(const int increment)
{
    this->user_id += increment;
}

int UnixUserIds::get_effective_user_id() const
{
    return this->effective_user_id;
}

void UnixUserIds::set_effective_user_id(const int id)
{
    this->effective_user_id = id;
}

int UnixUserIds::get_effective_group_id() const
{
    return this->effective_group_id;
}

void UnixUserIds::set_effective_group_id(const int id)
{
    this->effective_group_id = id;
}
