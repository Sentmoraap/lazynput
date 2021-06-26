#pragma once

#include <unordered_map>
#include <string>

/// \file StrHash.hpp
/// \brief Definition for StrHash use.

namespace Lazynput
{
    /// \class StrHash
    /// \brief This class computes a C string's hash.
    class StrHash
    {
        private:
            /// Empty string hash value.
            static constexpr uint32_t INITIAL_VALUE = 5381;

            /// Current string hash value.
            uint32_t hash = INITIAL_VALUE;

            /// \brief Computes a new hash bases on a previous string's hash and an extra character.
            /// \param chr : the character to append to the previous string.
            /// \param prevHash ; the previous string's hash value.
            /// \return ; the newhash value.
            static constexpr uint32_t hashCharacter(char chr, uint32_t prevHash)
            {
                return (prevHash << 5) + prevHash + chr;
            }

            /// \brief Recursive constexpr hash computation used by the litteral
            /// \param chr : the string left to hash
            /// \param prevHash ; the hashed part's hash value.
            /// \return ; the string's hash value.
            static constexpr uint32_t hashString(const char *str, uint32_t prevHash)
            {
                return *str ? hashString(str + 1, hashCharacter(*str, prevHash)) : prevHash;
            }

            constexpr StrHash(uint32_t value) : hash(value) {};

        public:
            /// \brief Empty string's hash.
            constexpr StrHash() {};

            /// \brief Computes the hash of a C string.
            /// \param str : the C string to hash.
            static constexpr StrHash make(const char *str)
            {
                return StrHash(hashString(str, 5381));
            }

            /// \brief Computes the hash of a C++ string.
            /// \param str : the C++ string to hash.
            static StrHash make(const std::string &str)
            {
                return StrHash::make(str.c_str());
            }

            /// \brief Add a character to the hashed string.
            /// Updates the hash. The new hash will be the hash of the previous string appended with the extra
            /// character.
            /// \param chr : the character to append.
            inline void hashCharacter(char chr)
            {
                hash = hashCharacter(chr, hash);
            }

            /// \brief Implicit conversion operator to uint32_t to be able to use StrHashes conviniently in switches.
            constexpr operator uint32_t() const {return hash;}
    };

    /// \class StrHashIdentity
    /// \brief Identity hash function to use pre-hashed strings in unordered_maps.
    class StrHashIdentity
    {
        public:
            constexpr StrHash operator()(StrHash hash) const
            {
                return hash;
            }
    };

    /// \brief unordered_map with StrHash as a prehashed key_type.
    template<typename T> using StrHashMap = std::unordered_map<StrHash, T, StrHashIdentity>;

    namespace Litterals
    {
        /// \brief String litteral to compute a string's hash.
        /// \param str : a C string.
        /// \return the string's hash.
        constexpr StrHash operator""_hash(const char *str, std::size_t size)
        {
            return StrHash::make(str);
        }
    }
}
