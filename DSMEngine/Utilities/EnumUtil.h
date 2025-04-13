#pragma once
#ifndef __ENUMUTIL_H__
#define __ENUMUTIL_H__

#include <concepts>

namespace DSM::Utility{
    
    template<typename Enum> requires std::is_enum_v<Enum>
    inline constexpr bool HasAllFlags(Enum value, Enum flags)
    {
        using T = std::underlying_type_t<Enum>;
        return (static_cast<T>(value) & static_cast<T>(flags)) == static_cast<T>(flags);
    }

    template<typename Enum> requires std::is_enum_v<Enum>
    inline constexpr bool HasAnyFlag(Enum value, Enum flags)
    {
        using T = std::underlying_type_t<Enum>;
        return (static_cast<T>(value) & static_cast<T>(flags)) != 0;
    }
}

#endif
