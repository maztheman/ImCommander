#pragma once

namespace imc::types {
    struct op_file_t;
}

namespace imc::gui {
    int ask_make_directory(types::op_file_t& directory);
}