#include "LazynputDb.hpp"
#include "Utils.hpp"
#include "TokenExtractor.hpp"
#include "StrHash.hpp"
#include "ErrorsWriter.hpp"
#include "Parser.hpp"
#include <fstream>
#include <string.h>

using namespace Lazynput::Litterals;

namespace Lazynput
{
    Device LazynputDb::getDevice(HidIds ids, const std::vector<StrHash> &configTags) const
    {
        return devicesDb.devices.count(ids) ? Device(devicesDb.devices.at(ids), devicesDb, configTags)
                : Device();
    }

    void LazynputDb::setGlobalConfigTags(const StrHash *hashs, int size)
    {
        globalConfigTags.resize(size);
        memcpy(globalConfigTags.data(), hashs, size * sizeof(StrHash));
    }

    Device LazynputDb::getDevice(HidIds ids, const StrHash *hashs, int size)
    const
    {
        // TODO: handle duplicate tags
        std::vector<StrHash> configTags = globalConfigTags;
        for(int i = 0; i < size; i++) configTags.push_back(hashs[i]);
        return getDevice(ids, configTags);
    }

    Device LazynputDb::getDevice(HidIds ids) const
    {
        return getDevice(ids, globalConfigTags);
    }

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
