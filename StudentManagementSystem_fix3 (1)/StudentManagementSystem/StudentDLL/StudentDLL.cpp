// StudentDLL.cpp
//
// Точка входа динамической библиотеки StudentDLL.dll.
//
// Реальную логику модуль не дублирует: файл StudentCore.cpp (Часть 1)
// добавлен как исходник ЭТОГО же проекта (см. CMakeLists.txt: цель
// StudentDLL собирается из StudentDLL.cpp + ../StudentCore/StudentCore.cpp;
// в Visual Studio это делается через "Add Existing Item" -> StudentCore.cpp
// в проект StudentDLL). Так функции createGroup/initDemoData/... становятся
// реально ОПРЕДЕЛЁННЫМИ символами в самой DLL, и линковщику не нужно
// "угадывать", какие объектные файлы вытащить из стороннего .lib только
// по списку экспортов - это надёжно работает одинаково и в MSVC (link.exe),
// и в MinGW/GNU ld.
//
// Экспорт функций наружу выполняется через StudentDLL.def (вариант
// "явное перечисление экспортов в .def файле" из ТЗ) - поэтому здесь НЕ
// нужны __declspec(dllexport) аннотации. Так как функции StudentCore
// объявлены как extern "C", их имена в объектном коде не манглятся и
// совпадают один в один с именами в .def.
//
// Единственное, что реально нужно именно в этом файле - штатная точка
// входа DllMain (обязательна для любой Win32 DLL).

#include "StudentCore.h" // объявления функций/структуры Student

#ifdef _WIN32
    // NOMINMAX/WIN32_LEAN_AND_MEAN - чтобы windows.h не тянул макросы
    // min/max и лишние заголовки (см. подробный комментарий в DynLib.h).
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reasonForCall, LPVOID reserved)
{
    (void)hModule;
    (void)reserved;

    switch (reasonForCall)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
        default:
            break;
    }
    return TRUE;
}
#endif // _WIN32
