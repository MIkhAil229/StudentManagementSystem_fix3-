// StudentApp.cpp
//
// Консольный клиент системы управления студенческими данными.
//
// Требования по ТЗ, выполненные здесь:
//   - никакой прямой статической линковки с StudentCore.lib;
//   - все вызовы ядра идут ТОЛЬКО через указатели на функции,
//     полученные из DLL через GetProcAddress (позднее связывание);
//   - при старте DLL не загружена;
//   - перед вызовом любого пункта меню (2-9) проверяется, загружена ли DLL;
//   - если DLL не найдена - сообщение об ошибке и предложение ввести путь;
//   - после выгрузки DLL (FreeLibrary) вызовы функций запрещены до
//     повторной загрузки.

#include "DynLib.h" // на _WIN32 транзитивно подключает <windows.h> (с NOMINMAX)
#include "Student.h" // только структура данных - не логика

#include <iostream>
#include <string>
#include <limits>

// ---------------------------------------------------------------------
// Вывод кириллицы в консоль Windows напрямую через WriteConsoleW.
//
// Простой SetConsoleOutputCP(CP_UTF8) не всегда надёжно работает на всех
// сборках/версиях conhost.exe (встречается "кракозябры" даже после его
// вызова). Поэтому вместо того, чтобы полагаться на текущую кодовую
// страницу консоли, отдельный streambuf сам конвертирует то, что писалось
// (в кодировке UTF-8 - см. флаг /utf-8 в CMakeLists.txt для MSVC) в UTF-16
// через MultiByteToWideChar и печатает Unicode-строку напрямую через
// WriteConsoleW - это работает всегда, независимо от текущей кодовой
// страницы консоли.
//
// На не-Windows платформах `out` - это просто ссылка на std::cout
// (терминал Linux/macOS и так ожидает UTF-8).
// ---------------------------------------------------------------------
#ifdef _WIN32
namespace
{
    class Utf8ConsoleStreambuf : public std::streambuf
    {
    public:
        Utf8ConsoleStreambuf() : hConsole_(GetStdHandle(STD_OUTPUT_HANDLE)) {}

    protected:
        int overflow(int c) override
        {
            if (c == EOF) return c;
            char ch = static_cast<char>(c);
            writeUtf8(&ch, 1);
            return c;
        }

        std::streamsize xsputn(const char* s, std::streamsize n) override
        {
            writeUtf8(s, static_cast<size_t>(n));
            return n;
        }

    private:
        void writeUtf8(const char* s, size_t n)
        {
            if (n == 0) return;
            int wlen = MultiByteToWideChar(CP_UTF8, 0, s, static_cast<int>(n), nullptr, 0);
            if (wlen <= 0) return;
            std::wstring wbuf(static_cast<size_t>(wlen), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, s, static_cast<int>(n), &wbuf[0], wlen);
            DWORD written = 0;
            WriteConsoleW(hConsole_, wbuf.c_str(), static_cast<DWORD>(wbuf.size()), &written, nullptr);
        }

        HANDLE hConsole_;
    };

    Utf8ConsoleStreambuf g_utf8Streambuf;
    std::ostream out(&g_utf8Streambuf);
}
#else
    static std::ostream& out = std::cout;
#endif

// ---------------------------------------------------------------------
// Указатели на функции ядра (сигнатуры зеркалируют StudentCore.h)
// ---------------------------------------------------------------------
using CreateGroupFunc          = Student* (*)(int);
using InitDemoDataFunc         = void     (*)(Student*, int);
using CalculateAllAveragesFunc = void     (*)(Student*, int);
using FindBestStudentFunc      = int      (*)(const Student*, int);
using CountDebtorsFunc         = int      (*)(const Student*, int);
using FilterByAverageFunc      = Student* (*)(const Student*, int, double, int*);
using SortByAverageFunc        = void     (*)(Student*, int);
using FreeGroupFunc            = void     (*)(Student*);

namespace
{
    // --- Состояние DLL ---
    LibHandle g_hDll = nullptr; // nullptr <=> DLL не загружена (тип зависит от платформы)
    bool      g_dllLoaded = false;

    CreateGroupFunc          pCreateGroup          = nullptr;
    InitDemoDataFunc         pInitDemoData         = nullptr;
    CalculateAllAveragesFunc pCalculateAllAverages = nullptr;
    FindBestStudentFunc      pFindBestStudent      = nullptr;
    CountDebtorsFunc         pCountDebtors         = nullptr;
    FilterByAverageFunc      pFilterByAverage      = nullptr;
    SortByAverageFunc        pSortByAverage        = nullptr;
    FreeGroupFunc            pFreeGroup            = nullptr;

    // --- Состояние данных приложения ---
    Student* g_group = nullptr;
    int      g_groupSize = 0;

    Student* g_filtered = nullptr;
    int      g_filteredSize = 0;

    void resetFunctionPointers()
    {
        pCreateGroup = nullptr;
        pInitDemoData = nullptr;
        pCalculateAllAverages = nullptr;
        pFindBestStudent = nullptr;
        pCountDebtors = nullptr;
        pFilterByAverage = nullptr;
        pSortByAverage = nullptr;
        pFreeGroup = nullptr;
    }

    void printStudent(const Student& s)
    {
        out << "  [" << s.id << "] " << s.fullName
                   << " | Оценки: ";
        for (int i = 0; i < SUBJECTS_COUNT; ++i)
        {
            out << s.scores[i] << (i + 1 < SUBJECTS_COUNT ? ", " : "");
        }
        out << " | Средний балл: " << s.averageScore << "\n";
    }

    void printGroup(const Student* group, int size, const char* emptyMessage)
    {
        if (group == nullptr || size <= 0)
        {
            out << emptyMessage << "\n";
            return;
        }
        for (int i = 0; i < size; ++i)
        {
            printStudent(group[i]);
        }
    }

    // Освобождает всю память приложения через freeGroup() из DLL.
    // Вызывается перед выгрузкой DLL и при создании новой группы.
    void freeAppMemory()
    {
        if (pFreeGroup != nullptr)
        {
            if (g_group != nullptr)
            {
                pFreeGroup(g_group);
            }
            if (g_filtered != nullptr)
            {
                pFreeGroup(g_filtered);
            }
        }
        g_group = nullptr;
        g_groupSize = 0;
        g_filtered = nullptr;
        g_filteredSize = 0;
    }

    // Пытается получить все 8 адресов функций. Возвращает false, если хотя
    // бы одна функция не найдена (например, DLL собрана неверно / устарела).
    bool resolveAllSymbols()
    {
        pCreateGroup          = reinterpret_cast<CreateGroupFunc>(DynLib_GetSymbol(g_hDll, "createGroup"));
        pInitDemoData         = reinterpret_cast<InitDemoDataFunc>(DynLib_GetSymbol(g_hDll, "initDemoData"));
        pCalculateAllAverages = reinterpret_cast<CalculateAllAveragesFunc>(DynLib_GetSymbol(g_hDll, "calculateAllAverages"));
        pFindBestStudent      = reinterpret_cast<FindBestStudentFunc>(DynLib_GetSymbol(g_hDll, "findBestStudent"));
        pCountDebtors         = reinterpret_cast<CountDebtorsFunc>(DynLib_GetSymbol(g_hDll, "countDebtors"));
        pFilterByAverage      = reinterpret_cast<FilterByAverageFunc>(DynLib_GetSymbol(g_hDll, "filterByAverage"));
        pSortByAverage        = reinterpret_cast<SortByAverageFunc>(DynLib_GetSymbol(g_hDll, "sortByAverage"));
        pFreeGroup            = reinterpret_cast<FreeGroupFunc>(DynLib_GetSymbol(g_hDll, "freeGroup"));

        return pCreateGroup && pInitDemoData && pCalculateAllAverages &&
               pFindBestStudent && pCountDebtors && pFilterByAverage &&
               pSortByAverage && pFreeGroup;
    }

    // Пункт меню 1: Загрузить DLL.
    void menuLoadDll()
    {
        if (g_dllLoaded)
        {
            out << "DLL уже загружена. Сначала выгрузите её (пункт 10), "
                          "если хотите загрузить заново.\n";
            return;
        }

        std::string path = DynLib_DefaultName();
        out << "Путь к библиотеке по умолчанию: " << path << "\n";
        out << "Нажмите Enter, чтобы использовать путь по умолчанию, "
                      "или введите свой путь: ";
        std::string userPath;
        if (!std::getline(std::cin, userPath))
        {
            out << "Ввод завершён (EOF). Загрузка DLL отменена.\n";
            return;
        }
        if (!userPath.empty())
        {
            path = userPath;
        }

        for (;;)
        {
            g_hDll = DynLib_Load(path.c_str());
            if (g_hDll != nullptr)
            {
                break;
            }

            out << "Ошибка: не удалось найти/загрузить библиотеку \""
                       << path << "\".\n";
            out << "Введите путь к библиотеке заново (или оставьте "
                          "пустым и нажмите Enter для отмены): ";
            std::string retryPath;
            if (!std::getline(std::cin, retryPath) || retryPath.empty())
            {
                out << "Загрузка DLL отменена.\n";
                return;
            }
            path = retryPath;
        }

        if (!resolveAllSymbols())
        {
            out << "Ошибка: библиотека загружена, но не все функции "
                          "найдены (GetProcAddress вернул nullptr). "
                          "Проверьте, что DLL собрана из актуального "
                          "StudentDLL.def.\n";
            DynLib_Free(g_hDll);
            g_hDll = nullptr;
            resetFunctionPointers();
            return;
        }

        g_dllLoaded = true;
        out << "DLL успешно загружена: " << path << "\n";
    }

    // Общая проверка для пунктов 2-9.
    bool requireDllLoaded()
    {
        if (!g_dllLoaded)
        {
            out << "Ошибка: DLL не загружена. Сначала выполните пункт 1 "
                          "(\"Загрузить DLL\").\n";
            return false;
        }
        return true;
    }

    void menuCreateGroup()
    {
        if (!requireDllLoaded()) return;

        out << "Введите размер группы (кол-во студентов): ";
        int size = 0;
        if (!(std::cin >> size) || size <= 0)
        {
            out << "Ошибка: размер группы должен быть положительным целым числом.\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        // Освобождаем предыдущую группу, если она была.
        if (g_group != nullptr)
        {
            pFreeGroup(g_group);
            g_group = nullptr;
        }
        if (g_filtered != nullptr)
        {
            pFreeGroup(g_filtered);
            g_filtered = nullptr;
            g_filteredSize = 0;
        }

        g_group = pCreateGroup(size);
        if (g_group == nullptr)
        {
            out << "Ошибка: не удалось создать группу.\n";
            g_groupSize = 0;
            return;
        }
        g_groupSize = size;
        out << "Группа из " << size << " студентов создана.\n";
    }

    void menuInitDemoData()
    {
        if (!requireDllLoaded()) return;
        if (g_group == nullptr)
        {
            out << "Сначала создайте группу (пункт 2).\n";
            return;
        }
        pInitDemoData(g_group, g_groupSize);
        out << "Группа заполнена демо-данными (не забудьте пункт 5, "
                      "чтобы рассчитать средний балл).\n";
    }

    void menuShowAll()
    {
        if (!requireDllLoaded()) return;
        out << "--- Список студентов (" << g_groupSize << ") ---\n";
        printGroup(g_group, g_groupSize, "Группа пуста. Сначала создайте группу (пункт 2).");
    }

    void menuCalculateAverages()
    {
        if (!requireDllLoaded()) return;
        if (g_group == nullptr)
        {
            out << "Сначала создайте группу (пункт 2).\n";
            return;
        }
        pCalculateAllAverages(g_group, g_groupSize);
        out << "Средние баллы рассчитаны.\n";
    }

    void menuFindBest()
    {
        if (!requireDllLoaded()) return;
        if (g_group == nullptr)
        {
            out << "Сначала создайте группу (пункт 2).\n";
            return;
        }
        int bestId = pFindBestStudent(g_group, g_groupSize);
        if (bestId == -1)
        {
            out << "Не удалось определить лучшего студента.\n";
            return;
        }
        out << "Лучший студент (id=" << bestId << "):\n";
        for (int i = 0; i < g_groupSize; ++i)
        {
            if (g_group[i].id == bestId)
            {
                printStudent(g_group[i]);
                break;
            }
        }
    }

    void menuDebtors()
    {
        if (!requireDllLoaded()) return;
        if (g_group == nullptr)
        {
            out << "Сначала создайте группу (пункт 2).\n";
            return;
        }
        int debtors = pCountDebtors(g_group, g_groupSize);
        out << "Количество должников: " << debtors << "\n";
        if (debtors > 0)
        {
            out << "Список должников (есть хотя бы одна оценка < 3):\n";
            for (int i = 0; i < g_groupSize; ++i)
            {
                bool hasDebt = false;
                for (int s = 0; s < SUBJECTS_COUNT; ++s)
                {
                    if (g_group[i].scores[s] < 3) { hasDebt = true; break; }
                }
                if (hasDebt) printStudent(g_group[i]);
            }
        }
    }

    void menuFilter()
    {
        if (!requireDllLoaded()) return;
        if (g_group == nullptr)
        {
            out << "Сначала создайте группу (пункт 2).\n";
            return;
        }

        const double threshold = 4.0;
        if (g_filtered != nullptr)
        {
            pFreeGroup(g_filtered);
            g_filtered = nullptr;
            g_filteredSize = 0;
        }

        g_filtered = pFilterByAverage(g_group, g_groupSize, threshold, &g_filteredSize);
        out << "--- Студенты с баллом >= " << threshold << " ---\n";
        printGroup(g_filtered, g_filteredSize, "Нет студентов, удовлетворяющих условию.");
    }

    void menuSort()
    {
        if (!requireDllLoaded()) return;
        if (g_group == nullptr)
        {
            out << "Сначала создайте группу (пункт 2).\n";
            return;
        }
        pSortByAverage(g_group, g_groupSize);
        out << "Группа отсортирована по среднему баллу (по убыванию):\n";
        printGroup(g_group, g_groupSize, "Группа пуста.");
    }

    void menuUnloadDll()
    {
        if (!g_dllLoaded)
        {
            out << "DLL и так не загружена.\n";
            return;
        }

        // Освобождаем память, выделенную функциями DLL, ПОКА DLL ещё
        // загружена (после FreeLibrary вызывать её функции нельзя).
        freeAppMemory();

        DynLib_Free(g_hDll);
        g_hDll = nullptr;
        g_dllLoaded = false;
        resetFunctionPointers();

        out << "DLL выгружена. Вызовы функций (пункты 2-9) заблокированы "
                      "до повторной загрузки (пункт 1).\n";
    }

    void printMenu()
    {
        out << "\n=== СИСТЕМА УПРАВЛЕНИЯ СТУДЕНТАМИ ===\n";
        out << "Статус DLL: " << (g_dllLoaded ? "ЗАГРУЖЕНА" : "не загружена") << "\n";
        out << "1. Загрузить DLL\n";
        out << "2. Создать группу студентов (ввод размера)\n";
        out << "3. Заполнить демо-данными\n";
        out << "4. Показать всех студентов\n";
        out << "5. Рассчитать средние баллы\n";
        out << "6. Найти лучшего студента\n";
        out << "7. Показать должников\n";
        out << "8. Отфильтровать по баллу (>= 4.0)\n";
        out << "9. Отсортировать по среднему баллу\n";
        out << "10. Выгрузить DLL\n";
        out << "0. Выход\n";
        out << "Выберите пункт: ";
    }

}

int main()
{
    out << "Приложение запущено. DLL пока не загружена.\n";

    bool running = true;
    while (running)
    {
        printMenu();

        int choice = 0;
        if (!(std::cin >> choice))
        {
            if (std::cin.eof())
            {
                // Входной поток закончился (например, ввод перенаправлен из
                // файла/канала) - корректно завершаем работу вместо того,
                // чтобы бесконечно печатать меню.
                out << "\nВвод завершён (EOF). Завершение работы.\n";
                if (g_dllLoaded)
                {
                    menuUnloadDll();
                }
                break;
            }
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            out << "Некорректный ввод. Попробуйте снова.\n";
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice)
        {
            case 1:  menuLoadDll(); break;
            case 2:  menuCreateGroup(); break;
            case 3:  menuInitDemoData(); break;
            case 4:  menuShowAll(); break;
            case 5:  menuCalculateAverages(); break;
            case 6:  menuFindBest(); break;
            case 7:  menuDebtors(); break;
            case 8:  menuFilter(); break;
            case 9:  menuSort(); break;
            case 10: menuUnloadDll(); break;
            case 0:
                if (g_dllLoaded)
                {
                    menuUnloadDll();
                }
                out << "Выход из программы.\n";
                running = false;
                break;
            default:
                out << "Неизвестный пункт меню.\n";
                break;
        }
    }

    return 0;
}
