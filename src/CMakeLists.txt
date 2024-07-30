
if (APPLE)
    add_executable(imcommander MACOSX_BUNDLE "")
else()
    add_executable(imcommander "")
endif()

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
    backend/watch_dir.cpp
    types/errors.cpp
    types/op_file.cpp
)

target_link_libraries(imcommander PRIVATE imgui_bindings fmt::fmt date::date date::date-tz)
target_include_directories(imcommander PRIVATE ".")

if (APPLE)
    target_compile_definitions(imcommander PUBLIC _IMC_MAC)
elseif(UNIX)
    target_compile_definitions(imcommander PUBLIC _IMC_NIX)
elseif(WIN32)
    target_compile_definitions(imcommander PUBLIC _IMC_WINDOWS)
endif()

if (APPLE)
    SET(IMC_ICNS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/icon.icns)
    set_source_files_properties(${IMC_ICNS_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(imcommander PRIVATE ${IMC_ICNS_PATH})
    set_target_properties(imcommander PROPERTIES
        BUNDLE True
        MACOSX_BUNDLE_GUI_IDENTIFIER ca.maztheman.imcommander
        MACOSX_BUNDLE_BUNDLE_NAME imcommander
        MACOSX_BUNDLE_BUNDLE_VERSION ${IMC_FULL_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${IMC_FULL_VERSION}
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/cmake/customtemplate.plist.in
        MACOSX_BUNDLE_ICON_FILE icon.icns
    )
    install(TARGETS imcommander RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX} BUNDLE DESTINATION ${CMAKE_INSTALL_PREFIX})
else()
    install(TARGETS imcommander DESTINATION bin)
endif()
