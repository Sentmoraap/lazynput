#include "Lazynput/LazynputDb.hpp"
#include "Lazynput/Utils.hpp"
#include "Lazynput/TokenExtractor.hpp"
#include "Lazynput/StrHash.hpp"
#include "Lazynput/ErrorsWriter.hpp"
#include "Lazynput/Parser.hpp"
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
            if(errors) *errors << "Error: can't open file " << path << "\n";
            return false;
        }
    }

    bool LazynputDb::parseFromDefault(std::ostream *errors)
    {
        const char *path = nullptr;
        char str[256];
        #ifdef __linux__
            path = std::getenv("XDG_DATA_HOME");
            if(!path) path = std::getenv("HOME");
            snprintf(str, 256, "%s/.local/share", path);
            path = str;
        #endif
        #ifdef TARGET_OS_MAC
            snprintf(str, 256, "~/Library");
        #endif
        #ifdef _WIN32
            path = std::getenv("LOCALAPPDATA");
        #endif
        if(path)
        {
            if(path == str) strcat(str, "/lazynput/lazynputdb.txt");
            else snprintf(str, 256, "%s/lazynput/lazynputdb.txt", path);
            std::fstream file;
            file.open(str, std::fstream::in);
            if(file.is_open()) return parseFromIstream(file, errors);
        }
        std::fstream file;
        file.open("lazynputdb.txt");
        if(file.is_open()) return parseFromIstream(file, errors);
        if(errors) *errors << "Error: no file found\n";
        return false;
    }
}
