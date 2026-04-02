# Test configuration for idotc

# Option to download Catch2 if not found
option(IDOTC_FETCH_CATCH2 "Download Catch2 if not found" ON)

# Try to find Catch2, or fetch it
find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND AND IDOTC_FETCH_CATCH2)
    message(STATUS "Catch2 not found, fetching from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.2
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif()

if(NOT TARGET Catch2::Catch2WithMain)
    message(WARNING "Catch2 not available - tests will not be built")
    return()
endif()

include(CTest)
include(Catch)

# Common include directories for tests
set(TEST_INCLUDE_DIRS 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Lexer tests
add_executable(lexer_tests 
    lexer_tests.cpp
    ${CMAKE_SOURCE_DIR}/src/lexer/token.cpp
    ${CMAKE_SOURCE_DIR}/src/lexer/lexer.cpp
    ${CMAKE_SOURCE_DIR}/src/util/error.cpp
    ${CMAKE_SOURCE_DIR}/src/util/source_location.cpp
)
target_include_directories(lexer_tests PRIVATE ${TEST_INCLUDE_DIRS})
target_link_libraries(lexer_tests PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(lexer_tests)

# Parser tests
add_executable(parser_tests 
    parser_tests.cpp
    ${CMAKE_SOURCE_DIR}/src/lexer/token.cpp
    ${CMAKE_SOURCE_DIR}/src/lexer/lexer.cpp
    ${CMAKE_SOURCE_DIR}/src/parser/ast.cpp
    ${CMAKE_SOURCE_DIR}/src/parser/parser.cpp
    ${CMAKE_SOURCE_DIR}/src/util/error.cpp
    ${CMAKE_SOURCE_DIR}/src/util/source_location.cpp
)
target_include_directories(parser_tests PRIVATE ${TEST_INCLUDE_DIRS})
target_link_libraries(parser_tests PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(parser_tests)
