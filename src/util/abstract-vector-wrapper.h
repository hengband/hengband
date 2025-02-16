#pragma once

#include <cstddef>
#include <vector>

namespace util {

/*!
 * @brief std::vector をラップする抽象クラス
 *
 * @tparam T ラップする std::vector の要素の型
 */
template <typename T>
class AbstractVectorWrapper {
public:
    using Container = std::vector<T>;
    using Iterator = typename Container::iterator;
    using ConstIterator = typename Container::const_iterator;
    using ReverseIterator = typename Container::reverse_iterator;
    using ConstReverseIterator = typename Container::const_reverse_iterator;

    virtual ~AbstractVectorWrapper() = default;

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
    void resize(size_t new_size)
    {
        this->get_inner_container().resize(new_size);
    }
    void shrink_to_fit()
    {
        this->get_inner_container().shrink_to_fit();
    }

private:
    virtual Container &get_inner_container() = 0;

    const Container &get_inner_container() const
    {
        return static_cast<const Container &>(const_cast<AbstractVectorWrapper *>(this)->get_inner_container());
    }
};

}
