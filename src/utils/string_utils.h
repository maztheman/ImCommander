#pragma once

#include <string>

namespace imc::string_utils {
    int icompare(const std::string& lhs, const std::string& rhs);
    std::string size_to_display(size_t sz);
    std::string size_to_display_no_padding(size_t sz);
}
