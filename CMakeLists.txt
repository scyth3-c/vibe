# CMakeList.txt

cmake_minimum_required(VERSION 3.14)
project(vermell)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

# Hardening flags for the framework itself: stack protector, _FORTIFY_SOURCE,
# PIE and full RELRO. (The readFileX toolchain applies its own set to the
# generated templates.)
set(VERMELL_HARDENING_FLAGS "-fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE")

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -pedantic -Wpedantic -Wnon-virtual-dtor -Wnull-dereference -Wstrict-overflow=5 -Wmissing-include-dirs -Wstrict-aliasing=2 -O3 -g -fsanitize=address ${VERMELL_HARDENING_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -Wextra -pedantic -Wpedantic -Wnon-virtual-dtor -Wnull-dereference -Wstrict-overflow=5 -Wmissing-include-dirs -Wstrict-aliasing=2 -O3 ${VERMELL_HARDENING_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-pie -Wl,-z,relro,-z,now")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-pie -Wl,-z,relro,-z,now")

file(GLOB_RECURSE SOURCES
        "${CMAKE_SOURCE_DIR}/sources/*.cpp"
)

file(GLOB_RECURSE HEADERS
        "${CMAKE_SOURCE_DIR}/include/vermell/*.h"
)
add_library(vermell STATIC ${SOURCES} ${HEADERS})
target_include_directories(vermell PUBLIC "${CMAKE_SOURCE_DIR}/include")

option(TESTING "Enable testing" OFF)
if (TESTING)

    enable_testing()
    add_executable(main tests/test_cases.cpp)
    target_link_libraries(
            main
            vermell
    )

    include(FetchContent)


    find_package(GTest  QUIET)

    if (NOT GTest_FOUND)

        message(STATUS "extracting https://github.com/google/googletest.git")
        FetchContent_Declare(
                googletest
                GIT_REPOSITORY https://github.com/google/googletest.git
                GIT_TAG v1.15.2
        )
        FetchContent_MakeAvailable(googletest)
        message(STATUS "using -> build/ [gtest]")
    else ()
        message(STATUS "#### using local GTest: In case of error verify your installation")
    endif ()

    target_link_libraries(main GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(main)


    find_package(CURL QUIET)
    if ( NOT CURL_FOUND )

        message( STATUS "extracting https://github.com/curl/curl.git")
        FetchContent_Declare(
                curl
                GIT_REPOSITORY https://github.com/curl/curl.git
                GIT_TAG 83bedbd
        )
        FetchContent_MakeAvailable(curl)
        message(STATUS "using -> build/ [curl]")
    else ()
        message(STATUS "#### using local curl: In case of error with .so.4 verify your installation curl --version")
    endif ()

    target_link_libraries(main CURL::libcurl)

else ()

    add_executable(vermellx tests/debug.cpp)
    target_link_libraries(vermellx vermell)

endif ()

install(TARGETS vermell
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/vermell" DESTINATION include)
install(FILES "${CMAKE_SOURCE_DIR}/README.md" DESTINATION share/doc/vermell)
