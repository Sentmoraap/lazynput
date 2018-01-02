#pragma once

#include <cstdint>
#include <iostream>

namespace Lazynput
{
    class StrHash;
    class ErrorsWriter;

    /// \class TokenExtractor
    /// \brief Extracts tokens from an input stream.
    class TokenExtractor
    {
        private:
            /// The istream in which the tokens are extracted.
            std::istream &inStream;

            /// The errors writer, used for illegal characters errors.
            ErrorsWriter &errorsWriter;

        public:
            /// \brief Constructs and initializes the TokenExtractor
            /// \param inStream : the istream in which the token will be extracted.
            /// \param errorsWriter ; a errors writer to write parting errors.
            TokenExtractor(std::istream &inStream, ErrorsWriter &errorsWriter);

            /// \brief Extracts a token
            /// \param hash : will be set to the token's hash.
            /// \param token : a string to contain the extracted token. Can be null.
            /// \return value : true if a token is extracted without errors, false otherwise.
            bool getNextToken(StrHash &hash, std::string *token);
    };
}
