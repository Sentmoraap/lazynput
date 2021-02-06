#include "Lazynput/ErrorsWriter.hpp"
#include <string>

namespace Lazynput
{
    std::ostream &ErrorsWriter::beginError()
    {
        return *stream << "Line " << lineNumber << ": ";
    }

    ErrorsWriter::ErrorsWriter(std::ostream *stream) : stream(stream)
    {
    }

    void ErrorsWriter::increaseLineNumber()
    {
        lineNumber++;
    }

    void ErrorsWriter::illegalCharacterError(unsigned char chr)
    {
        if(stream)
        {
            beginError() << "illegal character " << static_cast<int>(chr) << "\n";
        }
    }

    void ErrorsWriter::unexpectedTokenError(const std::string &token)
    {
        if(stream)
        {
            if(token.empty())
            {
                beginError() << "unexpected end of file\n";
            }
            else
            {
                beginError() << "unexpected token " << token << "\n";
            }
        }
    }

    void ErrorsWriter::error(const std::string &errorString)
    {
        if(stream) beginError() << errorString << "\n";
    }
}
