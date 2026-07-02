# StudentApp - консольный клиент (позднее связывание через LoadLibrary/GetProcAddress)
add_executable(StudentApp
    StudentApp.cpp
    DynLib.h
)

# ВАЖНО: здесь намеренно НЕТ target_link_libraries(StudentApp StudentCore) -
# по ТЗ клиент не должен линковаться со StudentCore напрямую. Единственная
# связь с ядром - через динамическую загрузку StudentDLL в рантайме.
target_include_directories(StudentApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/StudentCore   # нужен только Student.h (структура данных)
)

if (NOT WIN32)
    target_link_libraries(StudentApp PRIVATE ${CMAKE_DL_LIBS})
endif()

# Гарантируем, что StudentDLL собирается раньше и его можно найти рядом
# с StudentApp.exe при запуске из build-каталога.
add_dependencies(StudentApp StudentDLL)
add_custom_command(TARGET StudentApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:StudentDLL>
        $<TARGET_FILE_DIR:StudentApp>
)
