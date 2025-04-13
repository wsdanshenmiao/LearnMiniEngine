#pragma once
#ifndef __MACROS_H__
#define __MACROS_H__

#include "Utility.h"
#include <cstdio>


#define DSM_NONCOPYABLE(Class)  \
    Class(const Class&) = delete;   \
    Class& operator=(const Class&) = delete;

#define DSM_NONMOVABLE(Class)   \
    Class(Class&&) = delete;    \
    Class& operator=(Class&&) = delete

#define DSM_NONCOPYABLE_NONMOVABLE(Class)   \
    DSM_NONCOPYABLE(Class)   \
    DSM_NONMOVABLE(Class)

#define DSM_DEFAULT_COPYABLE(Class) \
    Class(const Class&) = default;   \
    Class& operator=(const Class&) = default;

#define DSM_DEFAULT_MOVABLE(Class)   \
    Class(Class&&) = default;    \
    Class& operator=(Class&&) = default

#define DSM_DEFAULT_COPYABLE_MOVABLE(Class) \
    DSM_DEFAULT_COPYABLE(Class) \
    DSM_DEFAULT_MOVABLE(Class)



#ifdef ERROR
#undef ERROR
#endif
#ifdef ASSERT
#undef ASSERT
#endif

#define STRINGIFY(x) #x
#define ASSERT( isFalse, ... ) \
    if (!(bool)(isFalse)) { \
        DSM::Utility::Print("\nAssertion failed in " STRINGIFY(__FILE__) " @ " STRINGIFY(__LINE__) "\n"); \
        DSM::Utility::PrintSubMessage("\'" #isFalse "\' is false"); \
        DSM::Utility::PrintSubMessage(__VA_ARGS__); \
        DSM::Utility::Print("\n"); \
        __debugbreak(); \
    }
#define ASSERT_SUCCEEDED( hr, ... ) \
    if (FAILED(hr)) { \
        DSM::Utility::Print("\nHRESULT failed in " STRINGIFY(__FILE__) " @ " STRINGIFY(__LINE__) "\n"); \
        DSM::Utility::PrintSubMessage("hr = 0x%08X", hr); \
        DSM::Utility::PrintSubMessage(__VA_ARGS__); \
        DSM::Utility::Print("\n"); \
        __debugbreak(); \
    }
#define ERROR( ... ) \
    DSM::Utility::Print("\nError reported in " STRINGIFY(__FILE__) " @ " STRINGIFY(__LINE__) "\n"); \
    DSM::Utility::PrintSubMessage(__VA_ARGS__); \
    DSM::Utility::Print("\n");

#define WARN_ONCE_IF( isTrue, ... ) \
    { \
        static bool s_TriggeredWarning = false; \
        if ((bool)(isTrue) && !s_TriggeredWarning) { \
            s_TriggeredWarning = true; \
            Utility::Print("\nWarning issued in " STRINGIFY(__FILE__) " @ " STRINGIFY(__LINE__) "\n"); \
            Utility::PrintSubMessage("\'" #isTrue "\' is true"); \
            Utility::PrintSubMessage(__VA_ARGS__); \
            Utility::Print("\n"); \
        } \
    }

#define WARN_ONCE_IF_NOT( isTrue, ... ) WARN_ONCE_IF(!(isTrue), __VA_ARGS__)



#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

#define DEFAULT_ALIGN 256
#define DEFAULT_RESOURCE_POOL_SIZE 0x20000

#define QUEUE_TYPE_MOVEBITS 56

#endif

