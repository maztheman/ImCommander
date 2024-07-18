#include "watch_dir.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <chrono>

#include <fmt/format.h>
#include <fmt/chrono.h>

#ifndef _IMC_MAC
#include <date/tz.h>
#endif

#include "utils/string_utils.h"

using namespace std::chrono_literals;

namespace {

using namespace imc::backend;
using namespace imc::string_utils;

std::string permissions_to_string(fs::perms p)
{
    return fmt::format("{}{}{}{}{}{}{}{}{}",
        ((p & fs::perms::owner_read) != fs::perms::none ? "r" : "-"),
        ((p & fs::perms::owner_write) != fs::perms::none ? "w" : "-"),
        ((p & fs::perms::owner_exec) != fs::perms::none ? "x" : "-"),
        ((p & fs::perms::group_read) != fs::perms::none ? "r" : "-"),
        ((p & fs::perms::group_write) != fs::perms::none ? "w" : "-"),
        ((p & fs::perms::group_exec) != fs::perms::none ? "x" : "-"),
        ((p & fs::perms::others_read) != fs::perms::none ? "r" : "-"),
        ((p & fs::perms::others_write) != fs::perms::none ? "w" : "-"),
        ((p & fs::perms::others_exec) != fs::perms::none ? "x" : "-")
    );
}

TableRowData create_imaginary_up_dir()
{
    table_row_data_t row_data;
    row_data.absolute_path = "..";
    row_data.name = "..";
    row_data.is_imaginary = true;
    row_data.is_directory = true;
    row_data.ext = "";
    row_data.size_display = fmt::format("{:>10}", "<DIR>");
    row_data.id = std::hash<std::string>{}(row_data.absolute_path);
    return std::make_unique<table_row_data_t>(row_data);
}

std::tuple<size_t, std::vector<fs::directory_entry>> hash_dir(const fs::path& cur)
{
    std::vector<fs::directory_entry> entries;
    entries.reserve(2048);
    size_t newHash = 0ULL;
    std::error_code ec;
    for(const auto& entry : fs::directory_iterator(cur)) {
        //any of these items below shall cause a reload.
        auto rowid = fs::hash_value(entry.path());
        newHash ^= (rowid << 1);
        if (size_t sz = entry.file_size(ec); !ec)
            newHash ^= (sz << 1);
        if (auto file_time = entry.last_write_time(ec); !ec)
            newHash ^= (file_time.time_since_epoch().count() << 1);
        entries.push_back(entry);
    }
    return std::make_tuple(newHash, entries);
}

TableRowData entry_to_table_row(const fs::directory_entry& entry)
{
    table_row_data_t row_data;
    std::error_code ec;
    row_data.name = entry.path().stem().generic_string();
    row_data.is_regular_file = entry.is_regular_file();
    row_data.is_directory = entry.is_directory();
    row_data.is_block_file = entry.is_block_file();
    row_data.is_character_file = entry.is_character_file();
    row_data.is_fifo = entry.is_fifo();
    row_data.is_other = entry.is_other();
    row_data.is_socket = entry.is_socket();
    row_data.is_symlink = entry.is_symlink();
    row_data.size = 0U;
    if (entry.is_directory())
    {
        row_data.ext = "";
        if (entry.path().has_extension())
            row_data.name += entry.path().extension();

        row_data.size_display = fmt::format("{:>10}", "<DIR>");
    } else {
        row_data.ext = entry.path().extension().generic_string();
        if (size_t try_size = entry.file_size(ec); !ec) {
            row_data.size = try_size;
        }
        row_data.size_display = size_to_display(row_data.size);
    }
    if (entry.is_symlink()) {
        if (auto try_last_write_time = fs::directory_entry(fs::read_symlink(entry.path())).last_write_time(ec); !ec) {
            row_data.modified = try_last_write_time;
        }
    } else {
        if (auto try_last_write_time = entry.last_write_time(ec); !ec) {
            row_data.modified = try_last_write_time;
        }
    }

#ifdef _IMC_MAC
    //for now utc only on macs..
    row_data.modified_display = fmt::format("{:%m-%d-%y %I:%M %p}", std::chrono::file_clock::to_sys(row_data.modified));
#else
    auto t = make_zoned(date::current_zone(), std::chrono::file_clock::to_sys(row_data.modified));
    row_data.modified_display = date::format("%m-%d-%y %I:%M %p", t);
#endif
    if (entry.is_symlink()) {
        row_data.permissions = fs::symlink_status(entry.path()).permissions();
    } else {
        row_data.permissions = fs::status(entry).permissions();
    }
    row_data.permissions_display = permissions_to_string(row_data.permissions);
    row_data.absolute_path = entry.path().generic_string();
    row_data.id = fs::hash_value(entry.path());
    return std::make_unique<table_row_data_t>(std::move(row_data));
}

}

int imc::backend::watch_dir(const fs::path& cur, size_t oldHash, FNUpdate callback, FNError errorCallback)
{
    std::error_code ec;
    auto beg = fs::directory_iterator(cur, ec);
    if (ec) {
        errorCallback(error_message_t(ec.message(), 5000ms));
        return 1;
    }

    auto [newHash, entries] = hash_dir(cur);
    if (newHash == oldHash) {
        return 0;
    }

    size_t all_data_size = entries.size() + (cur.has_parent_path() ? 1 : 0);

    TableRowDataVector data;
    data.reserve(all_data_size);
    if (cur.has_parent_path())
        data.push_back(create_imaginary_up_dir());

    std::transform(entries.begin(), entries.end(), std::back_inserter(data), [](const auto& entry) {
        return entry_to_table_row(entry);
    });

    callback(std::make_shared<TableRowDataVector>(std::move(data)), newHash);

    return 0;
}