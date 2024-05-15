#pragma once

#include <filesystem>

namespace imc::gui {
    namespace fs = std::filesystem;
    int view_file(const fs::path& file);
}
