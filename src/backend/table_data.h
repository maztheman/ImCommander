#pragma once

#include <memory>
#include <filesystem>
#include <string>
#include <vector>

namespace imc::backend {
    using file_time = std::filesystem::file_time_type;
    using file_perm = std::filesystem::perms;

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
        file_time       modified;
        std::string     modified_display;
        file_perm       permissions;
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

    using TableRowData = std::unique_ptr<table_row_data_t>;
    using TableRowDataVector = std::vector<TableRowData>;
    using TableRowDataVectorPtr = std::shared_ptr<TableRowDataVector>;
}