#pragma once

#include <cstdint>

namespace Lazynput
{
    /// \class Utils
    /// \brief Various utility functions
    class Utils
    {
        public:
            /// \brief Get the user's home directory in this operating system.
            /// \return The path of the home directory.
            static const char* getHomeDirectory();

            /// \brief Check if a character is a space or a tab.
            /// \param chr ; the character to test.
            /// \return true if it's a whitespace character, false otherwise.
            static bool isWhiteSpace(char chr);

            /// \brief Check if a character can be used in a name.
            /// Valid characters are 0-9, A-Z, a-z and _.
            /// \param chr ; the character to test.
            /// \return true if it can be used in a name, false otherwise.
            static bool isNameCharacter(char chr);
    };
}
