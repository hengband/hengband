#include "system/h-basic.h"
#include "util/stack-trace.h"
#ifdef HAVE_BACKTRACE
#include <cstdlib>
#include <execinfo.h>
#include <sstream>
#include <utility>
#endif // HAVE_BACKTRACE

namespace util {

#ifdef HAVE_BACKTRACE

constexpr auto FRAMES_TO_CAPTURE_MAX = 100;

struct StackTrace::Frame {
    void *address;
    std::string symbol_name;
};

StackTrace::StackTrace()
{
    void *addrs[FRAMES_TO_CAPTURE_MAX];
    auto count = backtrace(addrs, FRAMES_TO_CAPTURE_MAX);
    auto names = backtrace_symbols(addrs, count);

    if (names) {
        try {
            for (auto i = 0; i < count; i++) {
                Frame frame{ addrs[i], names[i] };
                this->frames.push_back(std::move(frame));
            }
        } catch (...) {
        }
        std::free(names);
    }
}

StackTrace::~StackTrace() = default;

std::string StackTrace::dump() const
{
    std::ostringstream oss;
    for (auto frame_no = 0; const auto &frame : this->frames) {
        oss << '#' << frame_no++ << ' '
            << (frame.symbol_name.empty() ? "(No symbol)" : frame.symbol_name + "()");
        oss << '\n';
    }
    return oss.str();
}

#else

struct StackTrace::Frame {
};

StackTrace::StackTrace()
{
}

StackTrace::~StackTrace() = default;

std::string StackTrace::dump() const
{
    return "Not implemented\n";
}

#endif // else HAVE_BACKTRACE

} // namespace util
