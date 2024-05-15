#pragma once

#include <filesystem>
#include <system_error>

namespace imc::backend {
    namespace fs = std::filesystem;
    // will use gui session to try to open the file
    // in linux it will try xdg-open, kde-open
    // in macos it will use open
    // in windows it will use ShellExecute
    int open(const fs::path& file);

    std::error_code copy(const fs::path& src, const fs::path& dst, bool can_override = true);
    // This function operates in 2 steps, copy, then remove.
    // return code, first is result of copy, second is result of remove.
    std::pair<std::error_code, std::error_code> move(const fs::path& src, const fs::path& dst, bool can_override = false);
    std::error_code delete_(const fs::path& src);
    std::error_code make_directory(const fs::path& dir);
}
