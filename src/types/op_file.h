#pragma once

#include <filesystem>
#include <vector>

namespace imc::types
{
    namespace fs = std::filesystem;

    struct op_file_t
    {
        op_file_t();
        std::vector<char> file;
        fs::path old_file;
    };
}
