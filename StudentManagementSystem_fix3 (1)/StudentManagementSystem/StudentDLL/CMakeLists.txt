# StudentDLL - динамическая библиотека, экспортирующая функции StudentCore
#
# StudentCore.cpp компилируется прямо как исходник этой цели (а не
# подключается через статическую линковку StudentCore.lib) - так символы
# createGroup/initDemoData/... гарантированно определены в самой DLL и
# однозначно резолвятся линковщиком по списку экспортов из .def файла
# (актуально что для MSVC, что для MinGW/GNU ld).
add_library(StudentDLL SHARED
    StudentDLL.cpp
    ${CMAKE_SOURCE_DIR}/StudentCore/StudentCore.cpp
)

target_include_directories(StudentDLL PRIVATE ${CMAKE_SOURCE_DIR}/StudentCore)

if (WIN32)
    # Экспорт функций через .def файл (вариант, выбранный по ТЗ) -
    # без __declspec(dllexport).
    target_sources(StudentDLL PRIVATE StudentDLL.def)
    set_target_properties(StudentDLL PROPERTIES
        LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/StudentDLL.def"
    )
endif()

set_target_properties(StudentDLL PROPERTIES OUTPUT_NAME "StudentDLL")
