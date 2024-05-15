#include "file_operations.h"

#ifdef _IMC_WINDOWS
#include <shellapi.h>
#endif

#include <filesystem>
#include <cstdlib>

#include <fmt/format.h>

#include "types/errors.h"
#include "utils/string_utils.h"

using namespace imc::string_utils;
using namespace imc::errors;

namespace fs = std::filesystem;

namespace {
#ifdef _IMC_WINDOWS
    struct com_initter
    {
        com_initter()
        {
            CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        }

        void init() { /*nop*/ }
    };

    com_initter& com() {
        static com_initter instance;
        return instance;
    }
#endif


    bool can_execute(fs::perms p)
    {
        return ((p & fs::perms::others_exec) != fs::perms::none) ||
               ((p & fs::perms::group_exec) != fs::perms::none) ||
               ((p & fs::perms::owner_exec) != fs::perms::none);
    }

    int do_open(const fs::path& file)
    {
#ifdef _IMC_NIX
        int ret = std::system(fmt::format("xdg-open \"{}\"", file.generic_string()).c_str());
        if (ret != 0) {
            ret = std::system(fmt::format("kde-open \"{}\"", file.generic_string()).c_str());
        }
        return ret;
#endif
#ifdef _IMC_MAC
        return system(fmt::format("open \"{}\"", file.generic_string()).c_str());
#endif
#ifdef _IMC_WINDOWS
    com().init();
    INT_PTR val = reinterpret_cast<INT_PTR>(ShellExecuteA(nullptr, "open", file.generic_string().c_str(), nullptr, nullptr, SW_SHOWNORMAL));
    return val >= 32 ? 0 : 1;
#endif
    }

    int do_execute(const fs::path& file)
    {
#if defined(_IMC_NIX) || defined(_IMC_MAC)
        return std::system(fmt::format("\"{}\"", file.generic_string()).c_str());
#endif
#ifdef _IMC_WINDOWS
        //call createprocess or what ?
        std::string writable_string;
        writable_string.reserve(1024);
        writable_string = file.generic_string();
        STARTUPINFOA st={0};
        st.cb = sizeof(STARTUPINFOA);
        st.dwFlags = STARTF_USESHOWWINDOW;
        st.wShowWindow = SW_SHOWDEFAULT;
        PROCESS_INFORMATION pi = {0};
        int ret = CreateProcessA(nullptr, writable_string.data(), nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS | DETACHED_PROCESS, nullptr, file.parent_path().generic_string().c_str(), &st, &pi);
        if (ret == 0) {
            return open_error::failed;
        }
        return open_error::success;
#endif
    }
}

int imc::backend::open(const std::filesystem::path& file)
{
#ifdef _IMC_NIX
    if (file.has_stem() && file.stem() != file.filename()) {
        return do_open(file);
    } else {
        auto filestatus = fs::status(file);
        if (filestatus.type() == fs::file_type::regular) {
            if (can_execute(filestatus.permissions()))
                return do_execute(file);
            return do_open(file);
        }
        return file_io_error::shouldnt_try_open;
    }
#endif
#ifdef _IMC_MAC
    return do_open(file);
#endif
#ifdef _IMC_WINDOWS
    if (file.has_stem()) {
        bool is_exe = icompare(".exe", file.stem()) == 0;
        if (is_exe)
            return do_execute(file);
        return do_open(file);
    }
    return do_open(file);
#endif
}

std::error_code imc::backend::copy(const fs::path& src, const fs::path& dst, bool can_override)
{
    //might use native if I have to
    std::error_code ec;
    fs::copy_file(src, dst, can_override ? fs::copy_options::overwrite_existing : fs::copy_options::none, ec);
    return ec;
}

std::error_code imc::backend::delete_(const fs::path& src)
{
    std::error_code ec;
    fs::remove(src, ec);
    return ec;
}

std::error_code imc::backend::make_directory(const fs::path& dir)
{
    std::error_code ec;
    fs::create_directory(dir, ec);
    return ec;
}

std::pair<std::error_code, std::error_code> imc::backend::move(const fs::path& src, const fs::path& dst, bool can_override)
{
    std::pair<std::error_code, std::error_code> ec;
    fs::copy_file(src, dst, ec.first);
    if (ec.first) {
        if (ec.first == std::errc::file_exists && can_override) {
            fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec.first);
        }
        if (ec.first)
            return ec;
    }
    fs::remove(src, ec.second);
    return ec;
}



