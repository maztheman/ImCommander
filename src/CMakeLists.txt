add_executable(imcommander "")

target_sources(imcommander PRIVATE
    main.cpp
    utils/string_utils.cpp
    gui/app.cpp
    gui/mainframe.cpp
    gui/viewer.cpp
    gui/copy_file.cpp
    gui/move_file.cpp
    gui/delete_file.cpp
    gui/make_directory.cpp
    backend/file_operations.cpp
    types/errors.cpp
    types/op_file.cpp
)

target_link_libraries(imcommander PRIVATE imgui_bindings fmt::fmt)
target_include_directories(imcommander PRIVATE ".")

if (APPLE)
    target_compile_definitions(imcommander PUBLIC _IMC_MAC)
elseif(UNIX)
    target_compile_definitions(imcommander PUBLIC _IMC_NIX)
elseif(WIN32)
    target_compile_definitions(imcommander PUBLIC _IMC_WINDOWS)
endif()
