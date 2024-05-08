#pragma once

#define IMC_ERR(z) (0x##z)

namespace imc::errors {

    inline namespace general {
        constexpr int success                   = IMC_ERR(0);
        constexpr int failed                    = IMC_ERR(1);
        constexpr int didnt_do_nothin           = IMC_ERR(10000);

    }

    //file errors 15000-15100
    inline namespace file_io_error {
        constexpr int shouldnt_try_open         = IMC_ERR(15000);
        constexpr int failed_to_copy            = IMC_ERR(15001);
        constexpr int failed_to_remove          = IMC_ERR(15002);
        constexpr int failed_to_mkdir           = IMC_ERR(15003);
    }
}
