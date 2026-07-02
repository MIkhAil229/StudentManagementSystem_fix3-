// DynLib.h
//
// Тонкая обёртка над платформенным API позднего связывания.
//
// ОСНОВНАЯ (проверяемая по ТЗ) реализация - ветка _WIN32:
//   LoadLibraryA / GetProcAddress / FreeLibrary.
//
// Ветка #else (dlopen/dlsym/dlclose, POSIX) добавлена ТОЛЬКО для того,
// чтобы StudentApp можно было собрать и прогнать на не-Windows системе
// в рамках самопроверки (в этой среде разработки нет Visual Studio /
// MSVC). На итоговую Windows-сборку эта ветка не влияет - там всегда
// используется LoadLibrary/GetProcAddress, как и требуется в задании.

#pragma once

#ifdef _WIN32
    // NOMINMAX обязателен: без него windows.h объявляет макросы min/max,
    // которые ломают std::numeric_limits<...>::max() (и любой другой код,
    // где встречается идентификатор max/min) - именно это вызывало ошибки
    // "недопустимая лексема справа от ::" / "требуется идентификатор" при
    // сборке в Visual Studio.
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    using LibHandle = HMODULE;
    inline LibHandle DynLib_Load(const char* path)
    {
        return ::LoadLibraryA(path);
    }
    inline void* DynLib_GetSymbol(LibHandle lib, const char* name)
    {
        return reinterpret_cast<void*>(::GetProcAddress(lib, name));
    }
    inline void DynLib_Free(LibHandle lib)
    {
        ::FreeLibrary(lib);
    }
    inline const char* DynLib_DefaultName()
    {
        return "StudentDLL.dll";
    }
#else
    #include <dlfcn.h>
    using LibHandle = void*;
    inline LibHandle DynLib_Load(const char* path)
    {
        return ::dlopen(path, RTLD_NOW);
    }
    inline void* DynLib_GetSymbol(LibHandle lib, const char* name)
    {
        return ::dlsym(lib, name);
    }
    inline void DynLib_Free(LibHandle lib)
    {
        ::dlclose(lib);
    }
    inline const char* DynLib_DefaultName()
    {
        return "./libStudentDLL.so";
    }
#endif
