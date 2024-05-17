#include "mainframe.h"

#ifndef _IMC_MAC
#include <date/tz.h>
#endif

#include "imgui.h"
#include "imgui_internal.h"

#include "utils/string_utils.h"
#include "backend/file_operations.h"
#include "types/op_file.h"
#include "types/errors.h"
#include <fmt/format.h>
#include <fmt/chrono.h>

#include "viewer.h"
#include "copy_file.h"
#include "move_file.h"
#include "delete_file.h"
#include "make_directory.h"

#include <filesystem>
#include <array>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <format>
#include <iostream>
#include <string>
#include <atomic>
#include <set>
#include <unordered_set>
#include <unordered_map>


namespace fs = std::filesystem;

using namespace imc::string_utils;
using namespace imc::types;

namespace {

    using ftt = fs::file_time_type;

    namespace sortable_columns {
        constexpr int Name = 0;
        constexpr int Ext = 1;
        constexpr int Size = 2;
        constexpr int Modified = 3;
        constexpr int Permissions = 4;
    }

    struct table_row_data_t
    {
        size_t          id;
        std::string     name;
        std::string     ext;
        size_t          size{0U};
        std::string     size_display;
        ftt             modified;
        std::string     modified_display;
        fs::perms       permissions;
        std::string     permissions_display;
        std::string     absolute_path;
        bool            is_directory{false};
        bool            is_block_file{false};
        bool            is_character_file{false};
        bool            is_fifo{false};
        bool            is_other{false};
        bool            is_socket{false};
        bool            is_symlink{false};
        bool            is_regular_file{false};
        bool            is_imaginary{false};
        std::string get_column(int col_id) const
        {
            switch(col_id)
            {
                using namespace sortable_columns;
                case Name:
                    return name;
                case Ext:
                    return ext;
                case Size:
                    return size_display;
                case Modified:
                    return modified_display;
                case Permissions:
                    return permissions_display;
                default:
                    return "";
            }
        }
    };

    struct error_message_t
    {
        error_message_t()
        {

        }

        explicit error_message_t(
            std::string message,
            std::chrono::milliseconds how_long = std::chrono::milliseconds(2000)
        )
            : last_error(message)
            , show_until(std::chrono::high_resolution_clock::now() + how_long)
        {

        }
        std::string last_error;
        std::chrono::time_point<std::chrono::high_resolution_clock> show_until;
    };

    struct selected_file_t
    {
        selected_file_t()
        : id(~0U)
        {

        }

        uint64_t id;
        fs::path old_file;
        std::array<char, 1025> file = {0};
    };

    struct pane_data_t
    {
        explicit pane_data_t(int the_id)
        : id(the_id)
        {
            table_data.reserve(1024);
            move_to(fs::current_path());
        }

        int move_to(const auto& to_path)
        {
            std::error_code ec;
            auto beg = fs::directory_iterator(to_path, ec);
            if (ec) {
                last_error = error_message_t(ec.message(), std::chrono::milliseconds(5000));
                return 1;
            }
            //if we are here, we have access, proceed normally.
            current_path = to_path;
            dir.fill('\0');
            auto curDir = current_path.generic_string();
            std::copy(curDir.begin(), curDir.end(), dir.begin());
            table_data.clear();
            if (current_path.has_parent_path())
                table_data.push_back(create_imaginary_up_dir());

            std::transform(beg, fs::directory_iterator(), std::back_inserter(table_data), [this](const auto& entry) {
                return entry_to_table_row(entry);
            });

            return 0;
            //we could iterate here and assign an int for the id, eg. for(auto& row : table_data) { row.id = row_num++; }
        }

        static table_row_data_t create_imaginary_up_dir()
        {
            table_row_data_t row_data;
            row_data.absolute_path = "..";
            row_data.name = "..";
            row_data.is_imaginary = true;
            row_data.is_directory = true;
            row_data.ext = "";
            row_data.size_display = fmt::format("{:>10}", "<DIR>");
            row_data.id = std::hash<std::string>{}(row_data.absolute_path);
            return row_data;
        }

        table_row_data_t entry_to_table_row(const fs::directory_entry& entry)
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
            row_data.id = std::hash<std::string>{}(fmt::format("[{}] {}", id, row_data.absolute_path));
            return row_data;
        }

        static std::string permissions_to_string(fs::perms p)
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


        int id;
        fs::path current_path;
        std::array<char, 1024> dir = {0};
        std::vector<table_row_data_t> table_data;
        std::set<size_t> selection;
        //quick access to selection data, could have literally all files in directory here..is that good?
        //maybe we just pull dynamically ?
        //or maintain a index for id sorted list.
        std::unordered_map<size_t, table_row_data_t> selection_data;
        //rename state
        selected_file_t rename;
        //view state
        selected_file_t view;
        //copy state
        selected_file_t copy;
        op_file_t copy_file;
        //move state
        selected_file_t move;
        op_file_t move_file;
        //delete state
        selected_file_t delete_;
        op_file_t delete_file;
        //make directoy state
        selected_file_t make_directory;
        op_file_t make_directory_file;
        //moving dir state
        bool im_moving{false};
        fs::path move_to_path;
        bool dir_dirty{false};
        error_message_t last_error;
    };

    //global mainframe state
    pane_data_t ldata(0), rdata(1);
    bool force_dir_always_before = true;
    int selected_panel = 0;
    std::string hover_text = "";
    bool rename_mode = false;
    bool view_mode = false;
    bool should_close = false;

    int sort_by_name(const table_row_data_t& lhs, const table_row_data_t& rhs)
    {
        const bool lhs_is_directory = lhs.is_directory;
        const bool rhs_is_directory = rhs.is_directory;
        if (lhs_is_directory) {
            if (rhs_is_directory) {
                if (lhs.is_imaginary)
                    return -1;
                if (rhs.is_imaginary)
                    return +1;
                return icompare(lhs.name, rhs.name);
            }
            return -1;
        } else {
            if (rhs_is_directory)
                return +1;
            return icompare(lhs.name, rhs.name);
        }
    }

    int sort_by_ext(const table_row_data_t& lhs, const table_row_data_t& rhs)
    {
        const bool lhs_is_directory = lhs.is_directory;
        const bool rhs_is_directory = rhs.is_directory;
        if (lhs_is_directory) {
            if (rhs_is_directory) {
                if (lhs.is_imaginary)
                    return true;
                if (rhs.is_imaginary)
                    return false;
                return icompare(lhs.name, rhs.name);
            }
            return +1;
        } else {
            if (rhs_is_directory)
                return -1;
            return icompare(lhs.ext, rhs.ext);
        }
    }

    int sort_by_size(const table_row_data_t& lhs, const table_row_data_t& rhs)
    {
        const bool lhs_is_directory = lhs.is_directory;
        const bool rhs_is_directory = rhs.is_directory;
        if (lhs_is_directory) {
            if (rhs_is_directory) {
                if (lhs.is_imaginary)
                    return true;
                if (rhs.is_imaginary)
                    return false;
                return icompare(lhs.name, rhs.name);
            }
            return +1;
        } else {
            if (rhs_is_directory)
                return -1;
            if (lhs.size == rhs.size)
                return 0;
            else if (lhs.size < rhs.size)
                return -1;
            else
                return +1;
        }
    }

    int sort_by_modified(const table_row_data_t& lhs, const table_row_data_t& rhs)
    {
        const bool lhs_is_directory = lhs.is_directory;
        const bool rhs_is_directory = rhs.is_directory;
        if (lhs_is_directory) {
            if (rhs_is_directory) {
                if (lhs.is_imaginary)
                    return true;
                if (rhs.is_imaginary)
                    return false;
                return (lhs.modified - rhs.modified).count();
            }
            return +1;
        } else {
            if (rhs_is_directory)
                return -1;
            return (lhs.modified - rhs.modified).count();
        }
    }

    void sort_data_by(ImGuiTableSortSpecs* sort_specs, std::vector<table_row_data_t>& table_data)
    {
        std::sort(table_data.begin(), table_data.end(), [&](const table_row_data_t& lhs, const table_row_data_t& rhs) -> bool {
            for(int n = 0; n < sort_specs->SpecsCount; n++) {
                int delta = 0;
                const ImGuiTableColumnSortSpecs* sort_spec = &sort_specs->Specs[n];
                switch(sort_spec->ColumnUserID) {
                    using namespace sortable_columns;
                    case Name: delta = sort_by_name(lhs, rhs); break;
                    case Ext:  delta = sort_by_ext(lhs, rhs);  break;
                    case Size: delta = sort_by_size(lhs, rhs); break;
                    case Modified: delta = sort_by_modified(lhs, rhs); break;
                    case Permissions: delta = icompare(lhs.permissions_display, rhs.permissions_display);
                    default: assert(false); break;
                }

                if (force_dir_always_before && (lhs.is_directory || rhs.is_directory) && !(lhs.is_directory && rhs.is_directory)) {
                    if (lhs.is_directory)
                        return true;
                    return false;
                }
                if (force_dir_always_before && lhs.is_directory && rhs.is_directory) {
                    if (lhs.is_imaginary)
                        return true;
                    if (rhs.is_imaginary)
                        return false;
                }
                if (delta > 0)
                    return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? false : true;
                if (delta < 0)
                    return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? true: false;
            }
            return sort_by_name(lhs, rhs) < 0;
        });
    }

    void pre_draw_pane(pane_data_t& data)
    {
        if (data.im_moving) {
            data.dir.fill('\0');
            auto move_to = data.move_to_path.generic_string();
            std::copy(move_to.begin(), move_to.end(), data.dir.begin());
        }
    }

    void process_change_dir(pane_data_t& data, bool& dir_dirty)
    {
        fs::path changeTo = data.dir.data();
        if (fs::exists(changeTo)) {
            if (0 != data.move_to(changeTo)) {
                auto undo = data.current_path.generic_string();
                data.dir.fill('\0');
                std::copy(undo.begin(), undo.end(), data.dir.begin());
                //if you were trying to move, now you are not.
                data.im_moving = false;
                return;
            }
            hover_text += fmt::format("=[{}, {}, {}]", dir_dirty, data.dir_dirty, data.im_moving);
            data.selection.clear();
            data.selection_data.clear();
            dir_dirty = true;
            data.im_moving = false;
            data.dir_dirty = false;
        }
    }

    void process_navigate(pane_data_t& data, const table_row_data_t& row)
    {
        if (row.is_directory) {
            data.im_moving = true;
            data.move_to_path = row.is_imaginary ?
                data.current_path.parent_path() :
                data.current_path / row.name;
        } else if (row.is_regular_file) {
            imc::backend::open(fs::path(row.absolute_path));
        }
    }

    void process_selection(pane_data_t& data, table_row_data_t& row, const bool is_selected)
    {
        selected_panel = data.id;

        if (data.im_moving)
            return;

        if (ImGui::GetIO().KeyCtrl)
        {
            if (is_selected) {
                data.selection.erase(row.id);
            } else {
                data.selection.insert(row.id);
                data.selection_data[row.id] = row;
            }
        } else {
            data.selection.clear();
            data.selection.insert(row.id);
            data.selection_data[row.id] = row;
        }
    }

    void process_rename_file(pane_data_t& data, table_row_data_t& row)
    {
        std::error_code ec;
        fs::path rename_to = data.rename.old_file;
        rename_to.replace_filename(fs::path(data.rename.file.data()));
        if (fs::rename(data.rename.old_file, rename_to, ec); !ec) {
            row.absolute_path = rename_to.generic_string();
            if (rename_to.has_extension())
                row.ext = rename_to.extension();
            if (rename_to.has_stem())
                row.name = rename_to.stem();
            rename_mode = false;
        }
    }

    std::string get_hover_text_from_row(const table_row_data_t& row)
    {
        if (row.is_imaginary)
            return "";
        if (row.is_directory)
            return row.name;
        if (row.is_regular_file)
            return fmt::format("{}{} ({}) ({})", row.name, row.ext, size_to_display_no_padding(row.size), row.id);
        return row.name;
    }

    void draw_pane(pane_data_t& data)
    {
        bool dir_dirty = false;
        pre_draw_pane(data);
        ImGui::PushItemWidth(-1.0f);
        if (ImGui::InputText("##[D]", data.dir.data(), data.dir.size()) || data.im_moving || data.dir_dirty) {
            process_change_dir(data, dir_dirty);
        }
        ImGui::PopItemWidth();
        if (!data.last_error.last_error.empty() && data.last_error.show_until >= std::chrono::high_resolution_clock::now())
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", data.last_error.last_error.c_str());
        if (ImGui::BeginTable("#file_list", 5, ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_PreferSortAscending, 0.0f, sortable_columns::Name);
            ImGui::TableSetupColumn("Ext", ImGuiTableColumnFlags_WidthFixed, 40.0f, sortable_columns::Ext);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80.0f, sortable_columns::Size);
            ImGui::TableSetupColumn("Modified", ImGuiTableColumnFlags_WidthFixed, 120.0f, sortable_columns::Modified);
            ImGui::TableSetupColumn("rwx", ImGuiTableColumnFlags_WidthFixed, 80.0f, sortable_columns::Permissions);
            ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
            ImGui::TableHeadersRow();

            if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs(); sort_specs->SpecsDirty || dir_dirty) {
                sort_data_by(sort_specs, data.table_data);
                sort_specs->SpecsDirty = false;
            }
            const int ciMaxCol = 5;
            for(auto& row : data.table_data) {
                ImGui::PushID(row.absolute_path.c_str());
                ImGui::TableNextRow();
                for(int col = 0; col < ciMaxCol; col++) {
                    ImGui::TableSetColumnIndex(col);
                    if (col == 0) {
                        const bool is_selected = data.selection.contains(row.id);
                        if (data.id == selected_panel && rename_mode && data.rename.id == row.id) {
                            if (ImGui::InputText("##edit", data.rename.file.data(), data.rename.file.size(),
                                ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
                                process_rename_file(data, row);
                            }
                        } else {
                            if (ImGui::Selectable(row.get_column(col).c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns |
                                    ImGuiSelectableFlags_AllowDoubleClick)) {
                                if (ImGui::IsMouseDoubleClicked(ImGuiPopupFlags_MouseButtonLeft)) {
                                    process_navigate(data, row);
                                }
                                process_selection(data, row, is_selected);
                            }
                            if (ImGui::IsItemHovered()) {//"Sample Hover Text.pdf 500B PDF Document";
                                hover_text = get_hover_text_from_row(row);
                            }
                        }
                    }
                    else
                        ImGui::TextUnformatted(row.get_column(col).c_str());
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }

    void get_selected_file(pane_data_t& data, selected_file_t& sel, bool& enable_mode)
    {
        if (!data.selection.empty()) {
            size_t selected_id;
            if (data.selection.size() == 1) {
                selected_id = *data.selection.begin();
            } else {
                //maybe use vector hmm
                selected_id = *std::max_element(data.selection.begin(), data.selection.end());
            }
            auto& selected_data = data.selection_data[selected_id];
            if (!selected_data.is_imaginary) {
                auto selected_path = fs::path(selected_data.absolute_path);
                auto selected_file = selected_path.filename().generic_string();
                sel.file.fill('\0');
                std::copy(selected_file.begin(), selected_file.end(), sel.file.begin());
                enable_mode = true;
                sel.old_file = selected_path;
                sel.id = selected_id;
            }
        }
    }

    void get_rename_file(pane_data_t& data)
    {
        get_selected_file(data, data.rename, rename_mode);
    }

    void get_view_file(int panel_selected)
    {
        bool enable_view = false;
        if (panel_selected == 0)
            get_selected_file(ldata, ldata.view, enable_view);
        else if (panel_selected == 1)
            get_selected_file(rdata, rdata.view, enable_view);
    }

    void get_copy_file(int panel_selected)
    {
        bool enable_view = false;
        if (panel_selected == 0) {
            get_selected_file(ldata, ldata.copy, enable_view);
            if (enable_view) {
                ldata.copy_file.old_file = ldata.copy.old_file;
                auto dst = (rdata.current_path / ldata.copy.old_file.filename()).generic_string();
                memset(ldata.copy_file.file.data(), 0, ldata.copy_file.file.size());
                std::copy(dst.begin(), dst.end(), ldata.copy_file.file.begin());
            }
        }
        else if (panel_selected == 1) {
            get_selected_file(rdata, rdata.copy, enable_view);
            if (enable_view) {
                rdata.copy_file.old_file = rdata.copy.old_file;
                auto dst = (ldata.current_path / rdata.copy.old_file.filename()).generic_string();
                memset(rdata.copy_file.file.data(), 0, rdata.copy_file.file.size());
                std::copy(dst.begin(), dst.end(), rdata.copy_file.file.begin());
            }
        }
    }

    void get_move_file(int panel_selected)
    {
        bool enable_view = false;
        if (panel_selected == 0) {
            get_selected_file(ldata, ldata.move, enable_view);
            if (enable_view) {
                ldata.move_file.old_file = ldata.move.old_file;
                auto dst = (rdata.current_path / ldata.move.old_file.filename()).generic_string();
                memset(ldata.move_file.file.data(), 0, ldata.move_file.file.size());
                std::copy(dst.begin(), dst.end(), ldata.move_file.file.begin());
            }
        }
        else if (panel_selected == 1) {
            get_selected_file(rdata, rdata.move, enable_view);
            if (enable_view) {
                rdata.move_file.old_file = rdata.move.old_file;
                auto dst = (ldata.current_path / rdata.move.old_file.filename()).generic_string();
                memset(rdata.move_file.file.data(), 0, rdata.move_file.file.size());
                std::copy(dst.begin(), dst.end(), rdata.move_file.file.begin());
            }
        }
    }

    void get_delete_file(int panel_selected)
    {
        bool enable_view = false;
        if (panel_selected == 0) {
            get_selected_file(ldata, ldata.delete_, enable_view);
            if (enable_view) {
                ldata.delete_file.old_file = ldata.delete_.old_file;
                auto dst = ldata.delete_.old_file.generic_string();
                memset(ldata.delete_file.file.data(), 0, ldata.delete_file.file.size());
                std::copy(dst.begin(), dst.end(), ldata.delete_file.file.begin());
            }
        }
        else if (panel_selected == 1) {
            get_selected_file(rdata, rdata.delete_, enable_view);
            if (enable_view) {
                rdata.delete_file.old_file = rdata.delete_.old_file;
                auto dst = rdata.delete_.old_file.generic_string();
                memset(rdata.delete_file.file.data(), 0, rdata.delete_file.file.size());
                std::copy(dst.begin(), dst.end(), rdata.delete_file.file.begin());
            }
        }
    }

    void get_make_directory_file(int panel_selected)
    {
        bool enable_view = false;
        if (panel_selected == 0) {
            get_selected_file(ldata, ldata.make_directory, enable_view);
            if (enable_view) {
                ldata.make_directory_file.old_file = ldata.make_directory.old_file;
                auto dst = ldata.make_directory.old_file.generic_string();
                memset(ldata.make_directory_file.file.data(), 0, ldata.make_directory_file.file.size());
                std::copy(dst.begin(), dst.end(), ldata.make_directory_file.file.begin());
            }
        }
        else if (panel_selected == 1) {
            get_selected_file(rdata, rdata.make_directory, enable_view);
            if (enable_view) {
                rdata.make_directory_file.old_file = rdata.make_directory.old_file;
                auto dst = rdata.make_directory.old_file.generic_string();
                memset(rdata.make_directory_file.file.data(), 0, rdata.make_directory_file.file.size());
                std::copy(dst.begin(), dst.end(), rdata.make_directory_file.file.begin());
            }
        }
    }

    void do_rename_file(int pane_selected)
    {
        if (pane_selected == 0) {
            get_rename_file(ldata);
        } else if (pane_selected == 1) {
            get_rename_file(rdata);
        }
    }

    void do_viewfile(int pane_selected)
    {
        get_view_file(pane_selected);
        ImGui::OpenPopup("View File");
    }

    void do_copy_file(int pane_selected)
    {
        get_copy_file(pane_selected);
        ImGui::OpenPopup("Copy File");
    }

    void do_move_file(int pane_selected)
    {
        get_move_file(pane_selected);
        ImGui::OpenPopup("Move File");
    }

    void do_make_directory(int pane_selected)
    {
        get_make_directory_file(pane_selected);
        ImGui::OpenPopup("MkDir");
    }

    void do_delete_file(int pane_selected)
    {
        get_delete_file(pane_selected);
        ImGui::OpenPopup("Delete File");
    }

    void draw_bottom_menu(int panel_selected)
    {
        float item_width = (ImGui::GetWindowWidth() / 9.0f) - 1.0f;
        ImGui::SetNextItemShortcut(ImGuiKey_F2);
        if (ImGui::Button("F2 Rename", ImVec2(item_width, 0.0f))) {
            do_rename_file(panel_selected);
        }
        ImGui::SameLine(0.0f, 1.0f);
        ImGui::SetNextItemShortcut(ImGuiKey_F3);
        if (ImGui::Button("F3 View", ImVec2(item_width, 0.0f))) {
            do_viewfile(panel_selected);
        }
        ImGui::SameLine(0.0f, 1.0f);
        ImGui::SetNextItemShortcut(ImGuiKey_F4);
        if (ImGui::Button("F4 Edit", ImVec2(item_width, 0.0f))) {
        }
        ImGui::SameLine(0.0f, 1.0f);
        ImGui::SetNextItemShortcut(ImGuiKey_F5);
        if (ImGui::Button("F5 Copy", ImVec2(item_width, 0.0f))) {
            do_copy_file(panel_selected);
        }
        ImGui::SameLine(0.0f, 1.0f);
        ImGui::SetNextItemShortcut(ImGuiKey_F6);
        if (ImGui::Button("F6 Move", ImVec2(item_width, 0.0f))) {
            do_move_file(panel_selected);
        }
        ImGui::SameLine(0.0f, 1.0f);
        ImGui::SetNextItemShortcut(ImGuiKey_F7);
        if (ImGui::Button("F7 Mkdir", ImVec2(item_width, 0.0f))) {
            do_make_directory(panel_selected);
        }
        ImGui::SameLine(0.0f, 1.0f);
        ImGui::SetNextItemShortcut(ImGuiKey_F8);
        if (ImGui::Button("F8 Delete", ImVec2(item_width, 0.0f))) {
            do_delete_file(panel_selected);
        }
        ImGui::SameLine(0.0f, 1.0f);
        ImGui::SetNextItemShortcut(ImGuiKey_F9);
        if (ImGui::Button("F9 Term", ImVec2(item_width, 0.0f))) {
#ifdef _IMC_NIX
            int rc = imc::backend::open("/usr/bin/konsole"); //try konsole, then kitty, then xterm ? TODO: make this configurable
            if (rc != 0)
                imc::backend::open("/usr/bin/kitty");
#endif
        }
        ImGui::SameLine(0.0f, 1.0f);
        ImGui::SetNextItemShortcut(ImGuiKey_F10);
        if (ImGui::Button("F10 Quit", ImVec2(item_width, 0.0f))) {
            should_close = true;
        }
        ImGui::TextUnformatted(hover_text.c_str());
    }

    using namespace imc::errors;
    using namespace imc::gui;

    void draw_popups(int pane_selected)
    {
        view_file(pane_selected == 0 ? ldata.view.old_file : rdata.view.old_file);
        int ret = ask_copy(pane_selected == 0 ? ldata.copy_file : rdata.copy_file);
        if (ret == success) {
            if (pane_selected == 0)
                rdata.dir_dirty = true;
            else
                ldata.dir_dirty = true;
        }
        ret = ask_move(pane_selected == 0 ? ldata.move_file : rdata.move_file);
        if (ret == success) {
            rdata.dir_dirty = ldata.dir_dirty = true;
        }
        ret = ask_delete(pane_selected == 0 ? ldata.delete_file : rdata.delete_file);
        if (ret == success) {
            if (ldata.current_path == rdata.current_path) {
                ldata.dir_dirty = rdata.dir_dirty = true;
            }
            if (pane_selected == 0)
                ldata.dir_dirty = true;
            else
                rdata.dir_dirty = true;
        }
        ret = ask_make_directory(pane_selected == 0 ? ldata.make_directory_file : rdata.make_directory_file);
        if (ret == success) {
            if (ldata.current_path == rdata.current_path) {
                ldata.dir_dirty = rdata.dir_dirty = true;
            }
            if (pane_selected == 0)
                ldata.dir_dirty = true;
            else
                rdata.dir_dirty = true;
        }
    }
}

bool imc::gui::draw_mainframe(int width, int height)
{
    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    if (ImGui::Begin("ImCommander", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar)) {
        int panel_selected = selected_panel;
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Close", "Ctrl+W")) { }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Actions")) {
                if (ImGui::MenuItem("Rename", "F2")) {
                    do_rename_file(panel_selected);
                }
                if (ImGui::MenuItem("View File", "F3")) {
                    do_viewfile(panel_selected);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        int modified_selected = -1;
        if (panel_selected == 0)
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.16f, 0.36f, 0.36f, 1.00f));
        if (ImGui::BeginChild("left pane", ImVec2(width / 2.0f, height - 75.0f), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX, ImGuiWindowFlags_NoSavedSettings)) {
            draw_pane(ldata);
            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
                modified_selected = 0;
            }
        }
        ImGui::EndChild();
        if (panel_selected == 0)
            ImGui::PopStyleColor();

        ImGui::SameLine();

        if (panel_selected == 1)
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.16f, 0.36f, 0.36f, 1.00f));
        if (ImGui::BeginChild("right pane", ImVec2(0, height - 75.0f), ImGuiChildFlags_Border, ImGuiWindowFlags_NoSavedSettings)) {
            draw_pane(rdata);
            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
                modified_selected = 1;
            }
        }
        ImGui::EndChild();
        if (panel_selected == 1)
            ImGui::PopStyleColor();

        if (ImGui::BeginChild("bottom menu", ImVec2(width, 0))) {
            draw_bottom_menu(panel_selected);
            draw_popups(panel_selected);
        }
        ImGui::EndChild();
        if (modified_selected != -1 && modified_selected != selected_panel) {
            selected_panel = modified_selected;
        }
    }
    ImGui::End();

    return should_close;
}
