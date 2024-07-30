cmake_minimum_required(VERSION 3.21)

if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(IMC_FULL_VERSION "0.0.4")

project(ImCommander
    VERSION ${IMC_FULL_VERSION}
    DESCRIPTION "ImGui Total Commander clone"
    LANGUAGES C CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(ImC_PreventInSourcebuilds)
include(ImC_ProjectOptions)
include(CPM)
include(ImC_Dependencies)

ImCommander_setup_options()
ImCommander_global_options()
ImCommander_setup_dependencies()
ImCommander_local_options()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(GIT_SHA
    "Unknown"
    CACHE STRING "SHA this build was generated from")
string(
  SUBSTRING "${GIT_SHA}"
            0
            8
            GIT_SHORT_SHA)

target_compile_features(ImCommander_options INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

add_library(ImCommander::ImCommander_options ALIAS ImCommander_options)
add_library(ImCommander::ImCommander_warnings ALIAS ImCommander_warnings)

add_subdirectory(src)

# do not look at tests if we are not top level project
if(NOT PROJECT_IS_TOP_LEVEL)
  return()
endif()

include(CTest)

if(BUILD_TESTING)
  message(AUTHOR_WARNING "Testing is disabled for now")
  #add_subdirectory(test)
endif()

if(ImCommander_BUILD_FUZZ_TESTS)
  message(AUTHOR_WARNING "[Disabled For now] Building Fuzz Tests, using fuzzing sanitizer https://www.llvm.org/docs/LibFuzzer.html")
  if (NOT myproject_ENABLE_ADDRESS_SANITIZER AND NOT myproject_ENABLE_THREAD_SANITIZER)
    message(WARNING "You need asan or tsan enabled for meaningful fuzz testing")
  endif()
  #add_subdirectory(fuzz_test)
endif()

if(MSVC)
  get_all_installable_targets(all_targets)
  message("all_targets=${all_targets}")
  set_target_properties(${all_targets} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=$(VC_ExecutablePath_x64);%PATH%")
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT imcommander)

if(CMAKE_SKIP_INSTALL_RULES)
  return()
endif()

include(ImC_PackageProject)

if(NOT APPLE)
  ImCommander_package_project(
    TARGETS
    imcommander
    ImCommander_options
    ImCommander_warnings
    # FIXME: this does not work! CK
    # PRIVATE_DEPENDENCIES_CONFIGURED project_options project_warnings
  )
endif()

set(CPACK_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${GIT_SHORT_SHA}-${CMAKE_SYSTEM_NAME}-${CMAKE_BUILD_TYPE}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
)

include(CPack)
