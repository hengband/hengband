#pragma once

#include <cstddef>
#include <map>

namespace util {

/*!
 * @brief std::map をラップする抽象クラス
 *
 * @tparam K ラップする std::map のキーの型
 * @tparam V ラップする std::map の値の型
 */
template <typename K, typename V>
class AbstractMapWrapper {
public:
    using Container = std::map<K, V>;
    using Iterator = typename Container::iterator;
    using ConstIterator = typename Container::const_iterator;
    using ReverseIterator = typename Container::reverse_iterator;
    using ConstReverseIterator = typename Container::const_reverse_iterator;

    virtual ~AbstractMapWrapper() = default;

    Iterator begin() noexcept
    {
        return this->get_inner_container().begin();
    }
    ConstIterator begin() const noexcept
    {
        return this->get_inner_container().begin();
    }
    Iterator end() noexcept
    {
        return this->get_inner_container().end();
    }
    ConstIterator end() const noexcept
    {
        return this->get_inner_container().end();
    }
    ConstIterator cbegin() const noexcept
    {
        return this->get_inner_container().cbegin();
    }
    ConstIterator cend() const noexcept
    {
        return this->get_inner_container().cend();
    }
    ReverseIterator rbegin() noexcept
    {
        return this->get_inner_container().rbegin();
    }
    ConstReverseIterator rbegin() const noexcept
    {
        return this->get_inner_container().rbegin();
    }
    ReverseIterator rend() noexcept
    {
        return this->get_inner_container().rend();
    }
    ConstReverseIterator rend() const noexcept
    {
        return this->get_inner_container().rend();
    }
    ConstReverseIterator crbegin() const noexcept
    {
        return this->get_inner_container().crbegin();
    }
    ConstReverseIterator crend() const noexcept
    {
        return this->get_inner_container().crend();
    }

    bool empty() const noexcept
    {
        return this->get_inner_container().empty();
    }
    size_t size() const noexcept
    {
        return this->get_inner_container().size();
    }
    bool contains(const K &key) const
    {
        return this->get_inner_container().contains(key);
    }

private:
    virtual Container &get_inner_container() = 0;

    const Container &get_inner_container() const
    {
        return static_cast<const Container &>(const_cast<AbstractMapWrapper *>(this)->get_inner_container());
    }
};

}
