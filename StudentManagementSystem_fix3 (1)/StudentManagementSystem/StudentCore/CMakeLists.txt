# StudentCore - статическая библиотека (ядро системы)
add_library(StudentCore STATIC
    StudentCore.cpp
    StudentCore.h
    Student.h
)

target_include_directories(StudentCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# На Windows статическая библиотека будет называться StudentCore.lib -
# как и требуется в ТЗ (имя проекта совпадает с именем выходного файла).
set_target_properties(StudentCore PROPERTIES OUTPUT_NAME "StudentCore")
