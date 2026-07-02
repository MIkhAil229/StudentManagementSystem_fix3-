cmake_minimum_required(VERSION 3.15)
project(StudentManagementSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Исходники этого решения сохранены в UTF-8 (в них есть кириллица - тексты
# меню, комментарии). Без /utf-8 MSVC по умолчанию интерпретирует .cpp/.h
# файлы без BOM в системной ANSI-кодировке, из-за чего строки типа
# "Загрузить DLL" превращаются в консоли в "кракозябры". Флаг /utf-8
# одновременно фиксирует и кодировку исходников (source charset), и
# кодировку строковых литералов в бинарнике (execution charset) как UTF-8.
if (MSVC)
    add_compile_options(/utf-8)
endif()

option(BUILD_TESTS "Собирать модульные тесты (StudentCoreTests)" ON)

# Часть 1: ядро (статическая библиотека StudentCore.lib)
add_subdirectory(StudentCore)

# Часть 3: DLL и консольный клиент.
# StudentDLL/StudentApp используют Windows API (LoadLibrary/GetProcAddress,
# .def экспорт), поэтому это "родная" Windows-часть решения. Под Linux/macOS
# также собираются (через dlopen/dlsym-обёртку в DynLib.h) - исключительно
# для самопроверки логики без Visual Studio.
add_subdirectory(StudentDLL)
add_subdirectory(StudentApp)

# Часть 2: тесты
if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(StudentCoreTests)
endif()
