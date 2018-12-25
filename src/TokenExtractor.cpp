#include "TokenExtractor.hpp"
#include "StrHash.hpp"
#include "ErrorsWriter.hpp"
#include "Utils.hpp"
#include <limits>

namespace Lazynput
{
    TokenExtractor::TokenExtractor(std::istream &inStream, ErrorsWriter &errorsWriter) : inStream(inStream),
        errorsWriter(errorsWriter)
    {
    }

    bool TokenExtractor::getNextToken(StrHash &hash, std::string *token)
    {
        enum : uint8_t {START, NAME, STRING}  state = START;
        hash = StrHash();
        if(token) token->clear();
        while(true)
        {
            unsigned char chr;
            switch(state)
            {
                case START:
                    chr = static_cast<unsigned char>(inStream.get());
                    if(inStream.eof())
                    {
                        return true;
                    }
                    if(chr == '#')
                    {
                        inStream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        hash.hashCharacter('\n');
                        if(token) *token += '\n';
                        errorsWriter.increaseLineNumber();
                        return true;
                    }
                    else if(Utils::isNameCharacter(chr))
                    {
                        state = NAME;
                        hash.hashCharacter(chr);
                        if(token) *token += chr;
                    }
                    else if(Utils::isWhiteSpace(chr)) continue;
                    else if(chr == '"')
                    {
                        state = STRING;
                        hash.hashCharacter(chr);
                        if(token) *token += chr;
                    }
                    else if(chr == '\r')
                    {
                        if(inStream.peek() == '\n') inStream.get();
                        hash.hashCharacter('\n');
                        if(token) *token += '\n';
                        errorsWriter.increaseLineNumber();
                        return true;
                    }
                    else if(chr == '\n')
                    {
                        hash.hashCharacter(chr);
                        if(token) *token += chr;
                        errorsWriter.increaseLineNumber();
                        return true;
                    }
                    else if(chr >= 32 && chr < 127)
                    {
                        hash.hashCharacter(chr);
                        if(token) *token += chr;
                        return true;
                    }
                    else
                    {
                        errorsWriter.illegalCharacterError(chr);
                        return false;
                    }
                    break;
                case NAME:
                    chr = static_cast<unsigned char>(inStream.peek());
                    if(inStream.eof())
                    {
                        return true;
                    }
                    if(Utils::isNameCharacter(chr))
                    {
                        inStream.get();
                        hash.hashCharacter(chr);
                        if(token) *token += chr;
                    }
                    else
                    {
                        return true;
                    }
                    break;
                case STRING:
                    chr = static_cast<unsigned char>(inStream.get());
                    if(inStream.eof())
                    {
                        return true;
                    }
                    if(chr < 32 || chr > 127)
                    {
                        errorsWriter.illegalCharacterError(chr);
                        return false;
                    }
                    else
                    {
                        hash.hashCharacter(chr);
                        if(token) *token += chr;
                        if(chr == '"') return true;
                    }
                    break;
            }
        }
    }

    bool TokenExtractor::isNextTokenStuck()
    {
        char next = inStream.peek();
        return next != ' ' && next != '\t' && next != '\r' && next != '\n';
    }
}
