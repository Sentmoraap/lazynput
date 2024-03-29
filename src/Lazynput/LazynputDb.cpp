#include "Lazynput/LazynputDb.hpp"
#include "Lazynput/Utils.hpp"
#include "Lazynput/TokenExtractor.hpp"
#include "Lazynput/StrHash.hpp"
#include "Lazynput/ErrorsWriter.hpp"
#include "Lazynput/Parser.hpp"
#include <fstream>
#include <string.h>

using namespace Lazynput::Literals;

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

    std::string LazynputDb::getStringFromHash(StrHash hash) const
    {
        try
        {
            return devicesDb.stringFromHash.at(hash);
        }
        catch(std::exception&)
        {
            return "";
        }
    }

    InterfaceInputType LazynputDb::getInterfaceInputType(StrHash hash) const
    {
        std::string str = getStringFromHash(hash);
        if(str.empty()) return InterfaceInputType::NIL;
        else
        {
            uint8_t length = str.length();
            uint8_t pos = 0;
            while(str[pos] != '.' && pos < length) pos++;
            str[pos] = 0;
            if(pos >= length) return InterfaceInputType::NIL;
            try
            {
                return devicesDb.interfaces.at(StrHash::make(str)).at(StrHash::make(str.c_str() + pos + 1));
            }
            catch(std::exception&)
            {
                return InterfaceInputType::NIL;
            }
        }
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
            #ifdef __GNUC__
            #pragma GCC diagnostic ignored "-Wrestrict" // Dumb GCC still think that the else branch can be taken
            #pragma GCC diagnostic ignored "-Wformat-truncation" // when path == str
            #endif
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
