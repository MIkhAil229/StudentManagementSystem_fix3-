// StudentCore.h
//
// Публичный интерфейс ядра системы управления студенческими данными.
// Проект StudentCore компилируется как статическая библиотека
// (StudentCore.lib на Windows / libStudentCore.a на *nix).
//
// Все функции объявлены как extern "C" по двум причинам:
//   1) отсутствие мангловки имён C++ упрощает повторное использование
//      этого же заголовка в StudentDLL (экспорт по имени в .def файле
//      и последующий поиск через GetProcAddress по строковому имени);
//   2) явно фиксируется стабильный ABI ядра.

#pragma once

#include "Student.h"

#ifdef __cplusplus
extern "C" {
#endif

// Создаёт массив из `size` студентов (динамическое выделение памяти).
// Поля структур обнуляются. При size <= 0 возвращает nullptr.
Student* createGroup(int size);

// Заполняет группу демонстрационными данными: ФИО из встроенного набора,
// случайные оценки (2..5) по каждому предмету. Средний балл НЕ считает -
// для этого нужно отдельно вызвать calculateAllAverages.
// Безопасно обрабатывает group == nullptr / size <= 0 (ничего не делает).
void initDemoData(Student* group, int size);

// Считает средний балл (по scores[SUBJECTS_COUNT]) для каждого студента
// группы и сохраняет результат в поле averageScore.
// Безопасно обрабатывает group == nullptr / size <= 0.
void calculateAllAverages(Student* group, int size);

// Возвращает id студента с максимальным averageScore.
// Если группа пуста / nullptr, возвращает -1.
int findBestStudent(const Student* group, int size);

// Возвращает количество студентов, у которых оценка < 3 хотя бы по одному
// предмету ("должники"). Если группа пуста / nullptr, возвращает 0.
int countDebtors(const Student* group, int size);

// Создаёт НОВЫЙ массив из студентов, у которых averageScore >= threshold.
// Количество найденных студентов записывается в *outSize.
// Если подходящих студентов нет (или group == nullptr), возвращает
// nullptr и *outSize = 0.
// Память, выделенную под результат, необходимо освободить через freeGroup().
Student* filterByAverage(const Student* group, int size, double threshold, int* outSize);

// Сортирует группу по averageScore по убыванию (сортировка на месте).
// Безопасно обрабатывает group == nullptr / size <= 1.
void sortByAverage(Student* group, int size);

// Освобождает память, выделенную createGroup()/filterByAverage().
// Безопасно обрабатывает group == nullptr.
void freeGroup(Student* group);

#ifdef __cplusplus
}
#endif
