// StudentCoreTests.cpp
//
// Модульные тесты ядра StudentCore на Google Test.
// Проект StudentCoreTests линкуется статически со StudentCore.lib.
//
// Всего 8 тестов (минимум по ТЗ был 4):
//   1. CreateAndFreeGroup    - выделение/освобождение памяти без утечек
//   2. AverageCalculation    - расчёт среднего балла
//   3. FindBestStudent       - поиск студента с максимальным баллом
//   4. DebtorsCount          - подсчёт должников (оценка < 3)
//   5. FilterByAverage       - фильтрация студентов с баллом >= 4.0
//   6. FilterEmptyResult     - фильтрация, когда никто не подходит
//   7. SortByAverage         - сортировка по убыванию среднего балла
//   8. NullPointerHandling   - безопасная обработка nullptr

#include <gtest/gtest.h>
#include <cstring>
#include "StudentCore.h"

TEST(StudentCoreTest, CreateAndFreeGroup)
{
    const int size = 5;
    Student* group = createGroup(size);

    ASSERT_NE(group, nullptr);
    for (int i = 0; i < size; ++i)
    {
        EXPECT_EQ(group[i].id, 0);
        EXPECT_EQ(group[i].averageScore, 0.0);
    }

    // При запуске под ASan/valgrind отсутствие двойного free / утечки
    // как раз и проверяется этим вызовом.
    freeGroup(group);

    // createGroup с некорректным размером должен вернуть nullptr,
    // а freeGroup(nullptr) не должен падать.
    EXPECT_EQ(createGroup(0), nullptr);
    EXPECT_EQ(createGroup(-3), nullptr);
    freeGroup(nullptr);
}

TEST(StudentCoreTest, AverageCalculation)
{
    const int size = 1;
    Student* group = createGroup(size);
    ASSERT_NE(group, nullptr);

    group[0].id = 1;
    std::strcpy(group[0].fullName, "Ivanov Ivan");
    int scores[SUBJECTS_COUNT] = {5, 4, 5, 3, 4}; // сумма = 21
    for (int s = 0; s < SUBJECTS_COUNT; ++s)
    {
        group[0].scores[s] = scores[s];
    }

    calculateAllAverages(group, size);

    EXPECT_DOUBLE_EQ(group[0].averageScore, 21.0 / 5.0);

    freeGroup(group);
}

TEST(StudentCoreTest, FindBestStudent)
{
    const int size = 3;
    Student* group = createGroup(size);
    ASSERT_NE(group, nullptr);

    group[0].id = 1; std::strcpy(group[0].fullName, "Ivanov"); group[0].averageScore = 3.5;
    group[1].id = 2; std::strcpy(group[1].fullName, "Petrov"); group[1].averageScore = 4.8;
    group[2].id = 3; std::strcpy(group[2].fullName, "Sidorov"); group[2].averageScore = 4.2;

    int bestId = findBestStudent(group, size);
    ASSERT_EQ(bestId, 2);

    freeGroup(group);
}

TEST(StudentCoreTest, DebtorsCount)
{
    const int size = 4;
    Student* group = createGroup(size);
    ASSERT_NE(group, nullptr);

    // Студент 0: без долгов
    int s0[SUBJECTS_COUNT] = {5, 4, 3, 4, 5};
    // Студент 1: долг по одному предмету (2)
    int s1[SUBJECTS_COUNT] = {2, 4, 3, 4, 5};
    // Студент 2: долги по двум предметам
    int s2[SUBJECTS_COUNT] = {2, 2, 3, 4, 5};
    // Студент 3: без долгов
    int s3[SUBJECTS_COUNT] = {4, 4, 4, 4, 4};

    for (int i = 0; i < SUBJECTS_COUNT; ++i)
    {
        group[0].scores[i] = s0[i];
        group[1].scores[i] = s1[i];
        group[2].scores[i] = s2[i];
        group[3].scores[i] = s3[i];
    }

    int debtors = countDebtors(group, size);
    EXPECT_EQ(debtors, 2); // студенты 1 и 2

    freeGroup(group);
}

TEST(StudentCoreTest, FilterByAverage)
{
    const int size = 4;
    Student* group = createGroup(size);
    ASSERT_NE(group, nullptr);

    group[0].id = 1; group[0].averageScore = 4.5;
    group[1].id = 2; group[1].averageScore = 3.9;
    group[2].id = 3; group[2].averageScore = 4.0;
    group[3].id = 4; group[3].averageScore = 2.1;

    int outSize = 0;
    Student* filtered = filterByAverage(group, size, 4.0, &outSize);

    ASSERT_NE(filtered, nullptr);
    ASSERT_EQ(outSize, 2); // id=1 (4.5) и id=3 (4.0, т.к. фильтр >= threshold)

    bool foundId1 = false, foundId3 = false;
    for (int i = 0; i < outSize; ++i)
    {
        if (filtered[i].id == 1) foundId1 = true;
        if (filtered[i].id == 3) foundId3 = true;
    }
    EXPECT_TRUE(foundId1);
    EXPECT_TRUE(foundId3);

    freeGroup(filtered);
    freeGroup(group);
}

TEST(StudentCoreTest, FilterEmptyResult)
{
    const int size = 3;
    Student* group = createGroup(size);
    ASSERT_NE(group, nullptr);

    group[0].id = 1; group[0].averageScore = 2.5;
    group[1].id = 2; group[1].averageScore = 3.0;
    group[2].id = 3; group[2].averageScore = 3.4;

    int outSize = -1;
    Student* filtered = filterByAverage(group, size, 4.0, &outSize);

    EXPECT_EQ(filtered, nullptr);
    EXPECT_EQ(outSize, 0);

    freeGroup(filtered); // freeGroup(nullptr) не должен падать
    freeGroup(group);
}

TEST(StudentCoreTest, SortByAverage)
{
    const int size = 5;
    Student* group = createGroup(size);
    ASSERT_NE(group, nullptr);

    double values[size] = {3.2, 4.8, 2.5, 4.0, 3.9};
    for (int i = 0; i < size; ++i)
    {
        group[i].id = i + 1;
        group[i].averageScore = values[i];
    }

    sortByAverage(group, size);

    for (int i = 0; i < size - 1; ++i)
    {
        EXPECT_GE(group[i].averageScore, group[i + 1].averageScore);
    }
    // Максимум должен оказаться первым
    EXPECT_DOUBLE_EQ(group[0].averageScore, 4.8);

    freeGroup(group);
}

TEST(StudentCoreTest, NullPointerHandling)
{
    // Ни одна из функций не должна падать (segfault) при передаче nullptr.
    EXPECT_NO_THROW(initDemoData(nullptr, 5));
    EXPECT_NO_THROW(calculateAllAverages(nullptr, 5));
    EXPECT_NO_THROW(sortByAverage(nullptr, 5));
    EXPECT_NO_THROW(freeGroup(nullptr));

    EXPECT_EQ(findBestStudent(nullptr, 5), -1);
    EXPECT_EQ(countDebtors(nullptr, 5), 0);

    int outSize = -1;
    Student* filtered = filterByAverage(nullptr, 5, 4.0, &outSize);
    EXPECT_EQ(filtered, nullptr);
    EXPECT_EQ(outSize, 0);

    // outSize == nullptr тоже не должен приводить к падению
    EXPECT_NO_THROW(filterByAverage(nullptr, 5, 4.0, nullptr));

    EXPECT_EQ(createGroup(0), nullptr);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
