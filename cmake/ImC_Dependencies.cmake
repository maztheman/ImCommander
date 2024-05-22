include(CPM)

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other
# targets
function(ImCommander_setup_dependencies)

  # For each dependency, see if it's
  # already been provided to us by a parent project
  find_package(Python3 COMPONENTS Interpreter REQUIRED)

  execute_process(COMMAND ${Python3_EXECUTABLE} -m venv "${CMAKE_CURRENT_BINARY_DIR}")
  set(ENV{VIRTUAL_ENV} "${CMAKE_CURRENT_BINARY_DIR}")
  set(Python3_FIND_VIRTUALENV FIRST)
  unset(Python3_EXECUTABLE)
  find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
  execute_process(COMMAND ${Python3_EXECUTABLE} -m pip install ${_pip_args} jinja2)

  if(NOT TARGET fmtlib::fmtlib)
    cpmaddpackage("gh:fmtlib/fmt#10.2.1")
  endif()

  if(NOT TARGET spdlog::spdlog)
    cpmaddpackage(
      NAME
      spdlog
      VERSION
      1.14.1
      GITHUB_REPOSITORY
      "gabime/spdlog"
      OPTIONS
      "SPDLOG_FMT_EXTERNAL ON")
  endif()

  if(NOT TARGET Catch2::Catch2WithMain)
    cpmaddpackage("gh:catchorg/Catch2@3.3.2")
  endif()

  if(NOT TARGET CLI11::CLI11)
    cpmaddpackage("gh:CLIUtils/CLI11@2.3.2")
  endif()

  if(NOT TARGET ftxui::screen)
    cpmaddpackage("gh:ArthurSonzogni/FTXUI@5.0.0")
  endif()

  if(NOT TARGET tools::tools)
    cpmaddpackage("gh:lefticus/tools#update_build_system")
  endif()

  if (NOT TARGET GLFW::glfw)
    cpmaddpackage("gh:glfw/glfw#3.4")
  endif()

  if (NOT TARGET date::date)
    cpmaddpackage(
      NAME date
      GITHUB_REPOSITORY "HowardHinnant/date"
      GIT_TAG master
      OPTIONS
      "BUILD_TZ_LIB ON")
  endif()

  cpmaddpackage(NAME freetype2
                GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
                GIT_TAG master)

  CPMAddPackage(
    NAME glad
    GITHUB_REPOSITORY Dav1dde/glad
    VERSION 2.0.6
    DOWNLOAD_ONLY
  )
  list(APPEND CMAKE_MODULE_PATH ${glad_SOURCE_DIR}/cmake)
  include(GladConfig)
  add_subdirectory("${glad_SOURCE_DIR}/cmake" glad_cmake)
  set(GLAD_LIBRARY glad_gl_core_33)
  # https://github.com/Dav1dde/glad/wiki/C#generating-during-build-process
  glad_add_library(${GLAD_LIBRARY} REPRODUCIBLE API gl:core=3.3)

  if (NOT TARGET imgui)
    cpmaddpackage("gh:ocornut/imgui#v1.90.6-docking")
    add_library(imgui_bindings STATIC)
    target_sources(imgui_bindings PRIVATE
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp
        ${imgui_SOURCE_DIR}/misc/fonts/binary_to_compressed_c.cpp
        ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    )
    target_include_directories(imgui_bindings PUBLIC ${imgui_SOURCE_DIR}/backends ${imgui_SOURCE_DIR} ${FT_INCLUDE_DIR}/freetype2)
    target_link_libraries(imgui_bindings PUBLIC glfw ${GLAD_LIBRARY} freetype)
  endif()
endfunction()
