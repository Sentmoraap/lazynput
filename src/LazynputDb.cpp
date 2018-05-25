#include "LazynputDb.hpp"
#include "Utils.hpp"
#include "TokenExtractor.hpp"
#include "StrHash.hpp"
#include "ErrorsWriter.hpp"
#include "Parser.hpp"
#include <fstream>

using namespace Lazynput::Litterals;

namespace Lazynput
{
    bool LazynputDb::parseFromIstream(std::istream &inStream, std::ostream *errors)
    {
        Parser parser(inStream, errors, devicesDb);
        return parser.parse();
    }

    bool LazynputDb::parseFromFile(const char *path, std::ostream *errors)
    {
        std::fstream file;
        file.open(path, std::fstream::in);
        if(file.is_open())
        {
            return parseFromIstream(file, errors);
        }
        else
        {
            if(errors) *errors << "Error : can't open file " << path << "\n";
            return false;
        }
    }

    bool LazynputDb::parseFromDefaultFile(std::ostream *errors)
    {
        return parseFromFile(""/*Utils::getHomeDirectory()*/, errors);
    }
}
