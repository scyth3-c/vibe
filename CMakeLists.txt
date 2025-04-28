# CMakeList.txt

cmake_minimum_required(VERSION 3.14)
project(vibe)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -pedantic -Wpedantic -Wnon-virtual-dtor -Wnull-dereference -Wstrict-overflow=5 -Wmissing-include-dirs -Wstrict-aliasing=2 -O0 -g -fsanitize=address  -static-libasan")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -Wextra -pedantic -Wpedantic -Wnon-virtual-dtor -Wnull-dereference -Wstrict-overflow=5 -Wmissing-include-dirs -Wstrict-aliasing=2 -O3")

file(GLOB_RECURSE SOURCES
        "${CMAKE_SOURCE_DIR}/sources/*.cpp"
)

file(GLOB_RECURSE HEADERS
        "${CMAKE_SOURCE_DIR}/include/vibe/*.h"
)
add_library(vibe STATIC ${SOURCES} ${HEADERS})

option(TESTING "Enable testing" OFF)
if (TESTING)

    enable_testing()
    add_executable(main tests/main_test.cpp)
    target_link_libraries(
            main
            vibe
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

    add_executable(vibex tests/debug.cpp)
    target_link_libraries(vibex vibe)

endif ()

install(TARGETS vibe
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/vibe" DESTINATION include)
install(FILES "${CMAKE_SOURCE_DIR}/README.md" DESTINATION share/doc/vibe)