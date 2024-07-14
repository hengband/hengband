#include "util/stack-trace.h"

namespace util {

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

} // namespace util
