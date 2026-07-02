// StudentCore.cpp
//
// Реализация ядра системы управления студенческими данными.
// Никаких платформозависимых вызовов (Windows API, dllexport и т.п.) здесь
// нет - это чистая "бизнес-логика", которую переиспользуют и
// StudentDLL (динамическая линковка), и любое приложение, слинкованное
// с StudentCore.lib статически.

#include "StudentCore.h"

#include <cstring>
#include <cstdlib>
#include <ctime>
#include <algorithm>

namespace
{
    // Небольшой пул фамилий/имён для генерации демо-данных.
    const char* const kDemoNames[] = {
        "Ivanov Ivan",
        "Petrov Petr",
        "Sidorova Anna",
        "Kuznetsov Sergey",
        "Smirnova Olga",
        "Popov Dmitry",
        "Volkova Ekaterina",
        "Sokolov Mikhail",
        "Mikhailova Tatiana",
        "Fedorov Aleksey",
    };
    const int kDemoNamesCount = static_cast<int>(sizeof(kDemoNames) / sizeof(kDemoNames[0]));

    bool g_randSeeded = false;

    void ensureRandSeeded()
    {
        if (!g_randSeeded)
        {
            std::srand(static_cast<unsigned int>(std::time(nullptr)));
            g_randSeeded = true;
        }
    }

    // Безопасная копия строки в fullName с гарантированным завершающим '\0'.
    void safeCopyName(char* dst, size_t dstSize, const char* src)
    {
        if (dst == nullptr || dstSize == 0)
        {
            return;
        }
        std::strncpy(dst, src, dstSize - 1);
        dst[dstSize - 1] = '\0';
    }
}

Student* createGroup(int size)
{
    if (size <= 0)
    {
        return nullptr;
    }

    Student* group = new Student[static_cast<size_t>(size)];
    for (int i = 0; i < size; ++i)
    {
        group[i].id = 0;
        group[i].fullName[0] = '\0';
        for (int s = 0; s < SUBJECTS_COUNT; ++s)
        {
            group[i].scores[s] = 0;
        }
        group[i].averageScore = 0.0;
    }
    return group;
}

void initDemoData(Student* group, int size)
{
    if (group == nullptr || size <= 0)
    {
        return;
    }

    ensureRandSeeded();

    for (int i = 0; i < size; ++i)
    {
        group[i].id = i + 1;
        safeCopyName(group[i].fullName, FULLNAME_LEN, kDemoNames[i % kDemoNamesCount]);

        for (int s = 0; s < SUBJECTS_COUNT; ++s)
        {
            // Оценки в диапазоне [2, 5]
            group[i].scores[s] = 2 + (std::rand() % 4);
        }
        group[i].averageScore = 0.0; // считается отдельно, через calculateAllAverages
    }
}

void calculateAllAverages(Student* group, int size)
{
    if (group == nullptr || size <= 0)
    {
        return;
    }

    for (int i = 0; i < size; ++i)
    {
        int sum = 0;
        for (int s = 0; s < SUBJECTS_COUNT; ++s)
        {
            sum += group[i].scores[s];
        }
        group[i].averageScore = static_cast<double>(sum) / SUBJECTS_COUNT;
    }
}

int findBestStudent(const Student* group, int size)
{
    if (group == nullptr || size <= 0)
    {
        return -1;
    }

    int bestIndex = 0;
    for (int i = 1; i < size; ++i)
    {
        if (group[i].averageScore > group[bestIndex].averageScore)
        {
            bestIndex = i;
        }
    }
    return group[bestIndex].id;
}

int countDebtors(const Student* group, int size)
{
    if (group == nullptr || size <= 0)
    {
        return 0;
    }

    int debtors = 0;
    for (int i = 0; i < size; ++i)
    {
        bool hasDebt = false;
        for (int s = 0; s < SUBJECTS_COUNT; ++s)
        {
            if (group[i].scores[s] < 3)
            {
                hasDebt = true;
                break;
            }
        }
        if (hasDebt)
        {
            ++debtors;
        }
    }
    return debtors;
}

Student* filterByAverage(const Student* group, int size, double threshold, int* outSize)
{
    if (outSize != nullptr)
    {
        *outSize = 0;
    }

    if (group == nullptr || size <= 0)
    {
        return nullptr;
    }

    int matchCount = 0;
    for (int i = 0; i < size; ++i)
    {
        if (group[i].averageScore >= threshold)
        {
            ++matchCount;
        }
    }

    if (matchCount == 0)
    {
        return nullptr; // пустой результат
    }

    Student* result = new Student[static_cast<size_t>(matchCount)];
    int idx = 0;
    for (int i = 0; i < size; ++i)
    {
        if (group[i].averageScore >= threshold)
        {
            result[idx++] = group[i];
        }
    }

    if (outSize != nullptr)
    {
        *outSize = matchCount;
    }
    return result;
}

void sortByAverage(Student* group, int size)
{
    if (group == nullptr || size <= 1)
    {
        return;
    }

    std::sort(group, group + size, [](const Student& a, const Student& b)
    {
        return a.averageScore > b.averageScore; // по убыванию
    });
}

void freeGroup(Student* group)
{
    delete[] group; // delete[] на nullptr безопасен по стандарту C++
}
