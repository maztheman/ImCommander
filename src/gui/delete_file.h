#pragma once

namespace imc::types {
    struct op_file_t;
}

namespace imc::gui {
    int ask_delete(types::op_file_t& delete_file);
}
