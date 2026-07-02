# Модульная система управления студенческими данными

Учебный проект: ядро (статическая библиотека), модульные тесты (Google Test),
динамическая библиотека (DLL) и консольный клиент с поздним связыванием
(`LoadLibrary` / `GetProcAddress`).

## Структура решения

```
StudentManagementSystem/
├── StudentCore/            # Часть 1: ядро (StudentCore.lib)
│   ├── Student.h            - структура Student
│   ├── StudentCore.h        - объявления функций ядра (extern "C")
│   └── StudentCore.cpp      - реализация
├── StudentCoreTests/        # Часть 2: тесты Google Test
│   ├── StudentCoreTests.cpp - 8 тестов (минимум по ТЗ был 4)
│   └── CMakeLists.txt
├── StudentDLL/               # Часть 3.1: StudentDLL.dll
│   ├── StudentDLL.def        - явный список экспортов (выбранный вариант)
│   ├── StudentDLL.cpp        - DllMain
│   └── CMakeLists.txt
├── StudentApp/               # Часть 3.2-3.3: консольный клиент
│   ├── DynLib.h              - обёртка LoadLibrary/GetProcAddress
│   ├── StudentApp.cpp        - меню, позднее связывание
│   └── CMakeLists.txt
└── CMakeLists.txt            # корневой файл сборки
```

## Сборка в Visual Studio (основной сценарий)

Самый простой способ получить полноценное решение (.sln) со всеми 4
проектами сразу - сгенерировать его через CMake (Visual Studio его
откроет как обычный `.sln`, все 4 цели появятся как отдельные проекты):

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --open build     # либо открыть build\StudentManagementSystem.sln вручную
```

После генерации в решении будет 4 проекта:
- **StudentCore** - статическая библиотека (Configuration Type: Static Library),
  выходной файл `StudentCore.lib`.
- **StudentDLL** - динамическая библиотека (Configuration Type: Dynamic Library),
  собирает `StudentCore.cpp` как свой собственный исходник (добавлен через
  `target_sources`/можно вручную сделать "Add Existing Item" в VS) и
  экспортирует функции через `StudentDLL.def`
  (Linker → Input → Module Definition File).
- **StudentApp** - консольное приложение (Configuration Type: Application).
  **Важно**: в Project Properties → Linker → Input → Additional Dependencies
  НЕ должно быть `StudentCore.lib` - клиент работает только через
  `LoadLibrary`/`GetProcAddress`.
- **StudentCoreTests** - тесты на Google Test (подтягивается автоматически
  через `FetchContent` при наличии интернета; см. альтернативу ниже).

Если создаёте проекты вручную (без CMake) - последовательность та же:
1. Новый проект `StudentCore` → Static Library (.lib) → добавить `Student.h`,
   `StudentCore.h`, `StudentCore.cpp`.
2. Новый проект `StudentDLL` → Dynamic-Link Library (.dll) → добавить
   `StudentDLL.cpp` и **этот же файл** `StudentCore.cpp` (Add Existing Item,
   не копируя физически) → в Linker → Input → Module Definition File указать
   путь к `StudentDLL.def`.
3. Новый проект `StudentApp` → Console Application (.exe) → добавить
   `StudentApp.cpp`, `DynLib.h` и `Student.h` (только структура, БЕЗ
   `StudentCore.h` и БЕЗ линковки `StudentCore.lib`).
4. Новый проект `StudentCoreTests` → Console Application → подключить
   Google Test (через vcpkg: `vcpkg install gtest`, либо NuGet-пакет
   `Microsoft.googletest.v140.windesktop.msvcstda` для старых VS) → добавить
   `StudentCoreTests.cpp` и слинковать со `StudentCore.lib`.
5. `StudentApp.exe` и `StudentDLL.dll` должны оказаться в одной папке
   (рядом), чтобы путь по умолчанию `StudentDLL.dll` находился без
   указания полного пути.

## Сборка через CMake (кроссплатформенно, для самопроверки)

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build     # прогон тестов
```

`StudentApp`/`StudentDLL` используют Win32 API (`LoadLibraryA`,
`GetProcAddress`, `FreeLibrary`, `.def`-экспорт) как того требует ТЗ.
Чтобы решение можно было собрать и прогнать также и вне Windows (в этой
среде разработки нет Visual Studio/MSVC), `DynLib.h` под `#else` использует
`dlopen`/`dlsym`/`dlclose` - это чисто вспомогательная ветка для
самопроверки логики, на итоговую Windows-сборку она не влияет.

## Как это было проверено

Сеть в среде разработки не даёт доступа к GitHub (нельзя скачать реальный
Google Test), поэтому проверка выполнена в два шага:

1. **StudentCore + StudentDLL + StudentApp** собраны напрямую через `g++`
   (Linux-аналог: `StudentDLL.so` вместо `.dll`, `dlopen` вместо
   `LoadLibrary`) и прогнаны под `-fsanitize=address,undefined` через
   весь сценарий меню (загрузка DLL, создание группы, демо-данные, расчёт
   среднего, поиск лучшего, должники, фильтрация, сортировка, повторное
   создание группы, выгрузка DLL, попытка вызова после выгрузки, выход) -
   утечек памяти и неопределённого поведения не обнаружено.
2. Логика всех 8 тестовых сценариев из `StudentCoreTests.cpp`
   (CreateAndFreeGroup, AverageCalculation, FindBestStudent, DebtorsCount,
   FilterByAverage, FilterEmptyResult, SortByAverage, NullPointerHandling)
   продублирована в виде отдельного smoke-теста на `assert()` и также
   прогнана под ASan/UBSan - все проверки прошли.

При сборке в реальной Visual Studio с настоящим Google Test
(`StudentCoreTests.cpp` идентичен) все 8 тестов должны пройти без
изменений - логика уже проверена.

## Особенности реализации

- Все функции ядра объявлены `extern "C"` - имена не манглятся, поэтому
  строки для `GetProcAddress("createGroup")` и т.п. в `StudentApp.cpp`
  совпадают буквально с именами в `StudentCore.h`/`StudentDLL.def`.
- `filterByAverage` по контракту отбирает студентов с `averageScore >=
  threshold` (это и есть требование "балл >= threshold" из сигнатуры функции
  в ТЗ); в меню `StudentApp` пункт 8 использует `threshold = 4.0`.
- При отсутствии подходящих студентов `filterByAverage` возвращает
  `nullptr` и `*outSize = 0` (а не пустой ненулевой массив) - `freeGroup`
  и `printGroup` в клиенте безопасно обрабатывают этот случай.
- `StudentApp` хранит два указателя (основная группа и результат
  последнего фильтра) и освобождает оба перед созданием новой группы и
  перед выгрузкой DLL, поэтому после `FreeLibrary` в памяти не остаётся
  указателей на функции, которые стали недействительны.
- Если входной поток исчерпан (EOF, например при перенаправлении ввода из
  файла), приложение корректно освобождает память/DLL и завершает работу
  вместо бесконечного цикла.
