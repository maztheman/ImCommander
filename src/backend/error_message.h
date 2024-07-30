#pragma once

#include <chrono>
#include <string>

namespace imc::backend {

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

}