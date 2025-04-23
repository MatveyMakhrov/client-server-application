cmake_minimum_required(VERSION 3.10)
project(client_server_app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS)
    set(EXTRA_LIBS ws2_32)
endif()

include_directories(include)

add_executable(app
    src/main.cpp
    src/client.cpp
    src/processing_server.cpp
    src/display_server.cpp
)

target_link_libraries(app ${EXTRA_LIBS})

option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    enable_testing()
    
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
    )
    FetchContent_MakeAvailable(googletest)
    
    add_executable(tests
        test/tests.cpp
        src/client.cpp
        src/processing_server.cpp
        src/display_server.cpp
    )

    target_link_libraries(tests
        GTest::gtest_main
        ${EXTRA_LIBS}
    )
    
    add_test(NAME client_server_tests COMMAND tests)
endif()