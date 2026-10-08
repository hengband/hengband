#pragma once

#include <memory>
#include <utility>

namespace test {

// move代入できない要素も含めて、AbstractVectorWrapperの状態を退避・復元する。
template <typename Wrapper>
class ScopedVectorWrapper {
public:
    explicit ScopedVectorWrapper(Wrapper &wrapper)
        : wrapper(wrapper)
    {
        this->saved.reserve(wrapper.size());
        for (auto &element : wrapper) {
            this->saved.push_back(std::move(element));
        }
        wrapper.resize(0);
    }

    ~ScopedVectorWrapper()
    {
        this->wrapper.resize(this->saved.size());
        auto target = this->wrapper.begin();
        for (auto &element : this->saved) {
            std::destroy_at(std::addressof(*target));
            std::construct_at(std::addressof(*target), std::move(element));
            ++target;
        }
    }

    ScopedVectorWrapper(const ScopedVectorWrapper &) = delete;
    ScopedVectorWrapper &operator=(const ScopedVectorWrapper &) = delete;

private:
    Wrapper &wrapper;
    typename Wrapper::Container saved;
};

}
