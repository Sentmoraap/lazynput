#pragma once

#include <cstdint>

namespace Lazynput
{
    /// \file StringHash.hpp

    /// \brief Compute a string's hash.
    /// \param str : a C string
    /// \return the string's hash.
    constexpr uint32_t strHash(const char *str);

    /// \brief String litteral to compute a string's hash.
    /// \param str : a C string
    /// \return the string's hash.
    constexpr uint32_t operator""_hash(const char *str, std::size_t size);
}
