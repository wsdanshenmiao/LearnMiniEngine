#pragma once
#ifndef __UTILITY_H__
#define __UTILITY_H__

#define NOMINMAX

#include <string>
#include <string_view>
#include <format>
#include <Windows.h>


namespace DSM::Utility {
    template<typename... Args>
    inline void Print(const std::string_view format, Args&&... args)
    {
        auto formatArgs{std::make_format_args(args...)};
        std::string outStr{std::vformat(format, formatArgs)};
        fputs(outStr.c_str(), stdout);
    }
    template<typename... Args>
    inline void Print(const std::wstring_view format, Args&&... args)
    {
        auto formatArgs{std::make_format_args(args...)};
        std::wstring outStr{std::vformat(format, formatArgs)};
        fputws(outStr.c_str(), stdout);
    }

    template<typename... Args>
    inline void PrintSubMessage(const std::string_view format, Args&&... args)
    {
        Print("--> ");
        Print(format, std::forward<Args>(args)...);
        Print("\n");
    }
    template<typename... Args>
    inline void PrintSubMessage(const std::wstring_view format, Args&&... args)
    {
        Print("--> ");
        Print(format, std::forward<Args>(args)...);
        Print("\n");
    }
 
    inline void PrintSubMessage(){}
    
    // 判断是否使用控制台程序
#if defined(_CONSOLE)
    inline void Print(const char* msg) { Print(msg);}
    inline void Print(const wchar_t* msg) { Print(msg);}
#else
    inline void Print( const char* msg ) { OutputDebugStringA(msg); }
    inline void Print( const wchar_t* msg ) { OutputDebugString(msg); }
#endif


    
}

#endif