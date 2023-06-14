#pragma once

#include <stdint.h>
#include <iostream>

namespace Lazynput
{
    /// \class ErrorsWriter
    /// \brief Writes formatted errors to an output stream.
    class ErrorsWriter
    {
        private:
            /// The stream in which the errors will be written.
            std::ostream *stream;

            /// The line currently parsed.
            uint16_t      lineNumber = 1;

            /// \brief Write the begining of a formatted error
            /// \return the errors stream
            std::ostream &beginError();

        public:
            /// \brief Constructor
            /// \param stream : the stream in which the errors will be written.
            ErrorsWriter(std::ostream *stream);

            /// \brief Increase the line number shown in errors
            void increaseLineNumber();

            /// \brief Writes an illegal character error to the errors stream.
            /// \param chr : the illegal character
            void illegalCharacterError(unsigned char chr);

            /// \brief Writes an unexpected token error to the errors stream.
            /// \param token : the unexpected token.
            void unexpectedTokenError(const std::string &token);

            /// \brief Writes an error to the errors stream.
            /// \param token : the error string
            void error(const std::string &errorString);
    };
}
