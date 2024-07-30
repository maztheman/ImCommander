#pragma once

#include <filesystem>
#include <functional>
#include <memory>

#include "table_data.h"
#include "error_message.h"

namespace imc::backend {

namespace fs = std::filesystem;

using FNUpdate = std::function<void(TableRowDataVectorPtr, size_t)>;
using FNError = std::function<void(const error_message_t&)>;

int watch_dir(const fs::path& cur, size_t oldHash, FNUpdate callback, FNError errorCallback);

}