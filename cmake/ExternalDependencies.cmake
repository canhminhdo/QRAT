# Declare all external dependencies and make sure that they are available.

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE AND NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/mqt-core/CMakeLists.txt")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

# check whether `modulename` is correctly cloned in the `extern` directory.
function(CHECK_SUBMODULE_PRESENT modulename)
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/${modulename}/CMakeLists.txt")
        message(
            FATAL_ERROR
            "${modulename} submodule not cloned properly. \
        Please run `git submodule update --init --recursive` \
        from the main project directory")
    endif()
endfunction()

check_submodule_present(mqt-core)
check_submodule_present(googletest)
check_submodule_present(exact-dd)

set(MQT_CORE_VERSION 2.7.0 CACHE STRING "MQT Core version")

# A different way to get GoogleTest
#include(FetchContent)
#FetchContent_Declare(
#    googletest
#    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
#)
## For Windows: Prevent overriding the parent project's compiler/linker settings
#set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
#FetchContent_MakeAvailable(googletest)