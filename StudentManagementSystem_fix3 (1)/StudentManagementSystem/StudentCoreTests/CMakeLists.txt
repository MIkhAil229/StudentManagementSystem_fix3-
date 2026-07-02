# StudentCoreTests - модульные тесты на Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
# Для Windows: используем ту же настройку рантайма MSVC, что и у остальных целей
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(StudentCoreTests
    StudentCoreTests.cpp
)

target_link_libraries(StudentCoreTests PRIVATE StudentCore gtest gtest_main)

include(GoogleTest)
gtest_discover_tests(StudentCoreTests)
