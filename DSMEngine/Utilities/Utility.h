#pragma once
#ifndef __UTILITY_H__
#define __UTILITY_H__



#include <string>
#include <string_view>
#include <format>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>


namespace DSM::Utility {
    // 判断是否使用控制台程序
#if defined(_CONSOLE)
    inline void Print(const char* msg) { std::printf(msg);}
    inline void Print(const wchar_t* msg) { std::wprintf(msg);}
#else
    inline void Print( const char* msg ) { OutputDebugStringA(msg); }
    inline void Print( const wchar_t* msg ) { OutputDebugString(msg); }
#endif

    
    template<typename... Args>
    inline void Print(const std::string_view format, Args&&... args)
    {
        auto formatArgs{std::make_format_args(args...)};
        std::string outStr{std::vformat(format, formatArgs)};
        Print(outStr.c_str());
    }
    template<typename... Args>
    inline void Print(const std::wstring_view format, Args&&... args)
    {
        auto formatArgs{std::make_wformat_args(args...)};
        std::wstring outStr{std::vformat(format, formatArgs)};
        Print(outStr.c_str());
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


    template <typename T> 
    inline constexpr T AlignUp( T value, size_t alignment ) noexcept
    {
        if (alignment == 0 || alignment == 1) return value;
        else return (T)(((size_t)value + (alignment - 1)) & ~(alignment - 1));
    }

    inline std::wstring UTF8ToWString(const std::string& str) 
    {
        wchar_t wstr[MAX_PATH];
        if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str.c_str(), -1, wstr, MAX_PATH)) {
            wstr[0] = L'\0';
        }
        return wstr;
    }

    inline std::wstring WStringToUTF8(const std::wstring& wstr)
    {
        char str[MAX_PATH];
        if (!WideCharToMultiByte(CP_ACP, MB_PRECOMPOSED, wstr.c_str(), -1, str, MAX_PATH, nullptr, nullptr)) {
            str[0] = '\0';
        }
        return wstr;
    }



    inline constexpr std::uint64_t INVALID_ALLOC_OFFSET = (std::numeric_limits<std::uint64_t>::max)();
    
    
}

#endif