#include "Lazynput/Utils.hpp"

namespace Lazynput
{
    bool Utils::isWhiteSpace(char chr)
    {
        return chr == ' ' || chr == '\t';
    }

    bool Utils::isNameCharacter(char chr)
    {
        return (chr >= '0' && chr <= '9')
                || (chr >= 'A' && chr <= 'Z')
                || (chr >= 'a' && chr <= 'z')
                || chr == '_';
    }
}
