#include "string_utils.h"

#include <cctype>
#include <iostream>
#include <string>
#include <string_view>
#include <array>
#include <format>
#include <algorithm>
#include <fmt/format.h>

using namespace std::string_view_literals;

namespace {
struct ci_char_traits : public std::char_traits<char>
{
    static char to_upper(char ch)
    {
        return std::toupper((unsigned char) ch);
    }

    static bool eq(char c1, char c2)
    {
        return to_upper(c1) == to_upper(c2);
    }

    static bool lt(char c1, char c2)
    {
         return to_upper(c1) < to_upper(c2);
    }

    static int compare(const char* s1, const char* s2, std::size_t n)
    {
        while (n-- != 0)
        {
            if (to_upper(*s1) < to_upper(*s2))
                return -1;
            if (to_upper(*s1) > to_upper(*s2))
                return 1;
            ++s1;
            ++s2;
        }
        return 0;
    }

    static const char* find(const char* s, std::size_t n, char a)
    {
        const auto ua{to_upper(a)};
        while (n-- != 0)
        {
            if (to_upper(*s) == ua)
                return s;
            s++;
        }
        return nullptr;
    }
};

template<class DstTraits, class CharT, class SrcTraits>
constexpr std::basic_string_view<CharT, DstTraits>
    traits_cast(const std::basic_string_view<CharT, SrcTraits> src) noexcept
{
    return {src.data(), src.size()};
}

template<class DstTraits, class CharT, class SrcTraits>
constexpr std::basic_string<CharT, DstTraits>
    traits_cast(const std::basic_string<CharT, SrcTraits>& src) noexcept
{
    return {src.data(), src.size()};
}

    std::string size_to_string(size_t sz)
    {
        if (sz < (1024ULL << 1)) {
            return fmt::format("{} B", sz);
        } else if (sz < (1024ULL << 10)) {
            return fmt::format("{:.1f} KiB", sz / static_cast<float>(1ULL << 10));
        } else if (sz < (1024ULL << 20)) {
            return fmt::format("{:.1f} MiB", sz / static_cast<float>(1ULL << 20));
        } else if (sz < (1024ULL << 30)) {
            return fmt::format("{:.1f} GiB", sz / static_cast<float>(1ULL << 30));
        } else {
            return fmt::format("{:.1f} TiB", sz / static_cast<float>(1ULL << 40));
        }
    }
}


int imc::string_utils::icompare(const std::string& lhs, const std::string& rhs)
{
    return traits_cast<ci_char_traits>(lhs).compare(traits_cast<ci_char_traits>(rhs));
}

std::string imc::string_utils::size_to_display(size_t sz)
{
    return fmt::format("{:>10}", size_to_string(sz));
}

std::string imc::string_utils::size_to_display_no_padding(size_t sz)
{
    return size_to_string(sz);
}
