#include "util/stack-trace.h"

#include "util/finalizer.h"
#include <iomanip>
#include <sstream>
#include <string_view>
#include <windows.h>

// DbgHelp.h は windows.h より後にインクルードする必要がある
#include <DbgHelp.h>

namespace util {

constexpr auto FRAMES_TO_CAPTURE_MAX = 100;
constexpr auto SYMBOL_NAME_LEN_MAX = 256;

namespace {
    std::string_view cut_out_relative_path(std::string_view path)
    {
        const std::string_view src_dir(R"(\src\)");
        auto pos = path.rfind(src_dir);
        if (pos == std::string_view::npos) {
            return path;
        }
        return path.substr(pos + src_dir.size());
    }

    std::pair<std::string, ULONG64> get_symbol_info(HANDLE process, ULONG64 address)
    {
        char symbol_info_buf[sizeof(SYMBOL_INFO) + SYMBOL_NAME_LEN_MAX]{};
        auto *symbol = reinterpret_cast<SYMBOL_INFO *>(symbol_info_buf);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = SYMBOL_NAME_LEN_MAX;

        DWORD64 displacement;
        const auto has_symbol = SymFromAddr(process, address, &displacement, symbol);
        return { has_symbol ? symbol->Name : "", symbol->Address };
    }

    std::pair<std::string, DWORD> get_line_info(HANDLE process, ULONG64 address)
    {
        IMAGEHLP_LINE64 line64{};
        line64.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD displacement;
        const auto has_line = SymGetLineFromAddr64(process, address, &displacement, &line64);
        const auto filename = has_line ? cut_out_relative_path(line64.FileName) : "";
        return { has_line ? std::string(filename) : "", line64.LineNumber };
    }
}

struct StackTrace::Frame {
    std::string file;
    DWORD line_num;
    std::string symbol_name;
    ULONG64 address;
};

StackTrace::StackTrace()
{
    const auto process = GetCurrentProcess();

    if (!SymInitialize(process, NULL, TRUE)) {
        return;
    }
    auto sym_cleanup_finalizer = util::make_finalizer([process] { SymCleanup(process); });

    SymSetOptions(SYMOPT_LOAD_LINES);

    void *back_trace[FRAMES_TO_CAPTURE_MAX];
    const auto num_frames = CaptureStackBackTrace(0, FRAMES_TO_CAPTURE_MAX, back_trace, NULL);

    for (auto i = 0; i < num_frames; i++) {
        const auto frame_address = reinterpret_cast<uintptr_t>(back_trace[i]);

        auto [symbol_name, symbol_address] = get_symbol_info(process, frame_address);
        auto [file, line_num] = get_line_info(process, frame_address);

        Frame frame{ std::move(file), line_num, std::move(symbol_name), symbol_address };
        this->frames.push_back(std::move(frame));
    }
}

StackTrace::~StackTrace() = default;

std::string StackTrace::dump() const
{
    std::ostringstream oss;
    for (auto frame_no = 0; const auto &frame : this->frames) {
        oss << '#' << frame_no++ << ' '
            << (frame.symbol_name.empty() ? "(No symbol)" : frame.symbol_name + "()");
        if (!frame.file.empty()) {
            oss << " at " << frame.file << ':' << frame.line_num;
        }
        oss << '\n';
    }
    return oss.str();
}

} // namespace util
