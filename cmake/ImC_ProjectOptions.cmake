include(CMakeDependentOption)
include(CheckCXXCompilerFlag)

macro(ImCommander_supports_sanitizers)
  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)
    set(SUPPORTS_UBSAN ON)
  else()
    set(SUPPORTS_UBSAN OFF)
  endif()

  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND WIN32)
    set(SUPPORTS_ASAN OFF)
  else()
    set(SUPPORTS_ASAN ON)
  endif()
endmacro()

macro(ImCommander_setup_options)
  option(ImCommander_ENABLE_HARDENING "Enable hardening" OFF)
  option(ImCommander_ENABLE_COVERAGE "Enable coverage reporting" OFF)
  cmake_dependent_option(
    ImCommander_ENABLE_GLOBAL_HARDENING
    "Attempt to push hardening options to built dependencies"
    OFF
    ImCommander_ENABLE_HARDENING
    OFF)

  ImCommander_supports_sanitizers()

  if(NOT PROJECT_IS_TOP_LEVEL OR ImCommander_PACKAGING_MAINTAINER_MODE)
    option(ImCommander_ENABLE_IPO "Enable IPO/LTO" OFF)
    option(ImCommander_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
    option(ImCommander_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(ImCommander_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(ImCommander_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(ImCommander_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(ImCommander_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(ImCommander_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(ImCommander_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(ImCommander_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
    option(ImCommander_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(ImCommander_ENABLE_PCH "Enable precompiled headers" OFF)
    option(ImCommander_ENABLE_CACHE "Enable ccache" OFF)
  else()
    option(ImCommander_ENABLE_IPO "Enable IPO/LTO" ON)
    option(ImCommander_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
    option(ImCommander_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(ImCommander_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ${SUPPORTS_ASAN})
    option(ImCommander_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(ImCommander_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${SUPPORTS_UBSAN})
    option(ImCommander_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(ImCommander_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(ImCommander_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(ImCommander_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
    option(ImCommander_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(ImCommander_ENABLE_PCH "Enable precompiled headers" OFF)
    option(ImCommander_ENABLE_CACHE "Enable ccache" ON)
  endif()

  if(NOT PROJECT_IS_TOP_LEVEL)
    mark_as_advanced(
      ImCommander_ENABLE_IPO
      ImCommander_WARNINGS_AS_ERRORS
      ImCommander_ENABLE_USER_LINKER
      ImCommander_ENABLE_SANITIZER_ADDRESS
      ImCommander_ENABLE_SANITIZER_LEAK
      ImCommander_ENABLE_SANITIZER_UNDEFINED
      ImCommander_ENABLE_SANITIZER_THREAD
      ImCommander_ENABLE_SANITIZER_MEMORY
      ImCommander_ENABLE_UNITY_BUILD
      ImCommander_ENABLE_CLANG_TIDY
      ImCommander_ENABLE_CPPCHECK
      ImCommander_ENABLE_COVERAGE
      ImCommander_ENABLE_PCH
      ImCommander_ENABLE_CACHE)
  endif()

  option(ImCommander_BUILD_FUZZ_TESTS "Enable fuzz testing executable" OFF)

endmacro()

macro(ImCommander_global_options)
  if(ImCommander_ENABLE_IPO)
    include(ImC_InterproceduralOptimization)
    ImCommander_enable_ipo()
  endif()

  ImCommander_supports_sanitizers()

  if(ImCommander_ENABLE_HARDENING AND ImCommander_ENABLE_GLOBAL_HARDENING)
    include(ImC_Hardening)
    if(NOT SUPPORTS_UBSAN
       OR ImCommander_ENABLE_SANITIZER_UNDEFINED
       OR ImCommander_ENABLE_SANITIZER_ADDRESS
       OR ImCommander_ENABLE_SANITIZER_THREAD
       OR ImCommander_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    message("${ImCommander_ENABLE_HARDENING} ${ENABLE_UBSAN_MINIMAL_RUNTIME} ${ImCommander_ENABLE_SANITIZER_UNDEFINED}")
    ImCommander_enable_hardening(ImCommander_options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()
endmacro()

macro(ImCommander_local_options)
  if(PROJECT_IS_TOP_LEVEL)
    include(ImC_StandardProjectSettings)
  endif()

  add_library(ImCommander_warnings INTERFACE)
  add_library(ImCommander_options INTERFACE)

  include(ImC_CompilerWarnings)
  ImCommander_set_project_warnings(
    ImCommander_warnings
    ${ImCommander_WARNINGS_AS_ERRORS}
    ""
    ""
    ""
    "")

  if(ImCommander_ENABLE_USER_LINKER)
    include(ImC_Linker)
    ImCommander_configure_linker(ImCommander_options)
  endif()

  include(ImC_Sanitizers)
  ImCommander_enable_sanitizers(
    ImCommander_options
    ${ImCommander_ENABLE_SANITIZER_ADDRESS}
    ${ImCommander_ENABLE_SANITIZER_LEAK}
    ${ImCommander_ENABLE_SANITIZER_UNDEFINED}
    ${ImCommander_ENABLE_SANITIZER_THREAD}
    ${ImCommander_ENABLE_SANITIZER_MEMORY})

  set_target_properties(ImCommander_options PROPERTIES UNITY_BUILD ${ImCommander_ENABLE_UNITY_BUILD})

  if(ImCommander_ENABLE_PCH)
    target_precompile_headers(
      ImCommander_options
      INTERFACE
      <vector>
      <string>
      <utility>)
  endif()

  if(ImCommander_ENABLE_CACHE)
    include(ImC_Cache)
    ImCommander_enable_cache()
  endif()

  include(ImC_StaticAnalyzers)
  if(ImCommander_ENABLE_CLANG_TIDY)
    ImCommander_enable_clang_tidy(ImCommander_options ${ImCommander_WARNINGS_AS_ERRORS})
  endif()

  if(ImCommander_ENABLE_CPPCHECK)
    ImCommander_enable_cppcheck(${ImCommander_WARNINGS_AS_ERRORS} "" # override cppcheck options
    )
  endif()

  if(ImCommander_ENABLE_COVERAGE)
    include(ImC_Tests)
    ImCommander_enable_coverage(ImCommander_options)
  endif()

  if(ImCommander_WARNINGS_AS_ERRORS)
    check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
    if(LINKER_FATAL_WARNINGS)
      # This is not working consistently, so disabling for now
      # target_link_options(ImCommander_options INTERFACE -Wl,--fatal-warnings)
    endif()
  endif()

  if(ImCommander_ENABLE_HARDENING AND NOT ImCommander_ENABLE_GLOBAL_HARDENING)
    include(ImC_Hardening)
    if(NOT SUPPORTS_UBSAN
       OR ImCommander_ENABLE_SANITIZER_UNDEFINED
       OR ImCommander_ENABLE_SANITIZER_ADDRESS
       OR ImCommander_ENABLE_SANITIZER_THREAD
       OR ImCommander_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    ImCommander_enable_hardening(ImCommander_options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()

endmacro()