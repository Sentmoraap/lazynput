#include <sstream>
#include <iomanip>
#include <algorithm>
#include "Parser.hpp"
#include "StrHash.hpp"
#include "PrivateTypes.hpp"
#include "Utils.hpp"

using namespace Lazynput::Litterals;

namespace Lazynput
{
    bool Parser::expectToken(uint8_t *state, StrHash hash, StrHash expectedHash, const std::string &token,
        uint8_t nextState)
    {
        if(hash == expectedHash)
        {
            *state = nextState;
            return true;
        }
        else if(hash == "\n"_hash) return true;
        else
        {
            errorsWriter.unexpectedTokenError(token);
            return false;
        }
    }

    bool Parser::parseInterfacesBlock()
    {
        enum : uint8_t {START, INSIDE_BLOCK, INTERFACE_START, INSIDE_INTERFACE, INPUT_TYPE_COLON} state = START;
        InputType inputType = InputType::BUTTON;
        bool inputTypeDefined = true;
        StrHash hash, interfaceHash;
        std::string token, interfaceName;
        Interface newInterface;
        Interface *oldInterface;

        while(extractor.getNextToken(hash, &token))
        {
            switch(state)
            {
                case START:
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "{"_hash, token, INSIDE_BLOCK))
                        return false;
                    break;
                case INSIDE_BLOCK:
                    switch(hash)
                    {
                        case "}"_hash:
                            return true;
                        case "\n"_hash:
                            break;
                        default:
                            if(!Utils::isNameCharacter(token[0]))
                            {
                                errorsWriter.unexpectedTokenError(token);
                                return false;
                            }
                            else if(newDevicesDb.interfaces.count(hash))
                            {
                                errorsWriter.error("multiple definition of the interface " + token
                                    + " in the same stream");
                            }
                            else if(oldDevicesDb.interfaces.count(hash))
                            {
                                state = INTERFACE_START;
                                interfaceName = token;
                                interfaceHash = hash;
                                oldInterface = &oldDevicesDb.interfaces.at(hash);
                                newInterface = *oldInterface;
                                inputTypeDefined = false;
                            }
                            else
                            {
                                state = INTERFACE_START;
                                interfaceName = token;
                                interfaceHash = hash;
                                oldInterface = nullptr;
                                newInterface.clear();
                                inputTypeDefined = false;
                            }
                            break;
                    }
                    break;
                case INTERFACE_START:
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "{"_hash, token, INSIDE_INTERFACE))
                        return false;
                    break;
                case INSIDE_INTERFACE:
                    switch(hash)
                    {
                        case "btn"_hash:
                            inputType = InputType::BUTTON;
                            inputTypeDefined = true;
                            state = INPUT_TYPE_COLON;
                            break;
                        case "abs"_hash:
                            inputType = InputType::ABSOLUTE_AXIS;
                            inputTypeDefined = true;
                            state = INPUT_TYPE_COLON;
                            break;
                        case "rel"_hash:
                            inputType = InputType::RELATIVE_AXIS;
                            inputTypeDefined = true;
                            state = INPUT_TYPE_COLON;
                            break;
                        case "\n"_hash:
                            break;
                        case "}"_hash:
                            state = INSIDE_BLOCK;
                            if(oldInterface)
                            {
                                if(!newInterface.empty())
                                {
                                    errorsWriter.error("the current interface definition does not match the"
                                                        "previous definition");
                                    return false;
                                }
                            }
                            else newDevicesDb.interfaces[interfaceHash] = std::move(newInterface);
                            break;
                        default:
                            if(Utils::isNameCharacter(token[0]))
                            {
                                if(inputTypeDefined)
                                {
                                    if(oldInterface)
                                    {
                                        if(newInterface.count(hash))
                                        {
                                            if(newInterface[hash] != inputType)
                                            {
                                                errorsWriter.error("the current interface definition does not match the"
                                                                    "previous definition");
                                                return false;
                                            }
                                            newInterface.erase(hash);
                                        }
                                        else
                                        {
                                            if(oldInterface->count(hash))
                                            {
                                                errorsWriter.error("input " + token + " defined multiple times");
                                            }
                                            else
                                            {
                                                errorsWriter.error("the current interface definition does not match the"
                                                                    "previous definition");
                                            }
                                            return false;
                                        }
                                    }
                                    else
                                    {
                                        if(newInterface.count(hash))
                                        {
                                            errorsWriter.error("input " + token + " defined multiple times");
                                            return false;
                                        }
                                        else newInterface[hash] = inputType;
                                    }
                                }
                                else
                                {
                                    errorsWriter.error("undefined input type");
                                    return false;
                                }
                            }
                            else
                            {
                                errorsWriter.unexpectedTokenError(token);
                                return false;
                            }
                            break;
                    }
                    break;
                case INPUT_TYPE_COLON:
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, ":"_hash, token, INSIDE_INTERFACE))
                        return false;
                    break;
            }
        }
        return false;
    }

    bool Parser::parseLabelsBlock()
    {
        enum : uint8_t {START, INSIDE_BLOCK, LABELS_START, INHERITANCE, AFTER_INHERITANCE} state = START;
        StrHash hash, labelsHash, lineHash, interfaceHash;
        std::string token, labelsName, lineName, interfaceName;
        Labels newLabels;

        while(extractor.getNextToken(hash, &token))
        {
            switch(state)
            {
                case START:
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "{"_hash, token, INSIDE_BLOCK))
                        return false;
                    break;
                case INSIDE_BLOCK:
                    switch(hash)
                    {
                        case "}"_hash:
                            return true;
                        case "\n"_hash:
                            break;
                        default:
                            if(!Utils::isNameCharacter(token[0]))
                            {
                                errorsWriter.unexpectedTokenError(token);
                                return false;
                            }
                            else if(newDevicesDb.labels.count(hash))
                            {
                                errorsWriter.error("multiple definition of the labels " + token
                                    + " in the same stream");
                            }
                            else
                            {
                                state = LABELS_START;
                                labelsName = token;
                                labelsHash = hash;
                                newLabels.map.clear();
                            }
                            break;
                    }
                    break;
                case LABELS_START:
                    switch(hash)
                    {
                        case "{"_hash:
                            if(!parseLabelsBlock(nullptr, newLabels.map)) return false;
                            newDevicesDb.labels[labelsHash] = std::move(newLabels);
                            state = INSIDE_BLOCK;
                            break;
                        case ":"_hash:
                            state = INHERITANCE;
                            break;
                        case "\n"_hash:
                            break;
                        default:
                            errorsWriter.unexpectedTokenError(token);
                            return false;
                    }
                    break;
                case INHERITANCE:
                    if(Utils::isNameCharacter(token[0]))
                    {
                        if(newDevicesDb.labels.count(hash) || oldDevicesDb.labels.count(hash)) newLabels.parent = hash;
                        else
                        {
                            errorsWriter.error("labels " + labelsName + " extends unknown labels " + token);
                            return false;
                        }
                        state = AFTER_INHERITANCE;
                    }
                    else
                    {
                        errorsWriter.unexpectedTokenError(token);
                        return false;
                    }
                    break;
                case AFTER_INHERITANCE:
                    switch(hash)
                    {
                        case "\n"_hash:
                            break;
                        case "{"_hash:
                            if(!parseLabelsBlock(nullptr, newLabels.map)) return false;
                            newDevicesDb.labels[labelsHash] = std::move(newLabels);
                            state = INSIDE_BLOCK;
                            break;
                        default:
                            errorsWriter.unexpectedTokenError(token);
                            return false;
                    }
                    break;
            }
        }
        return false;
    }

    bool Parser::parseDevicesBlock()
    {
        enum : uint8_t {START, INSIDE_BLOCK, AFTER_VID, EXPECT_PID, AFTER_PID, EXPECT_PARENT_VID, AFTER_PARENT_VID,
                EXPECT_PARENT_PID, AFTER_INHERITANCE, INSIDE_DEVICE, EXPECT_NAME, EXPECT_INTERFACE, EXPECT_LABELS,
                EXPECT_EQUALS,  END_OF_LINE}
                state = START, nextState;
        StrHash hash;
        std::string token;
        DeviceData device;
        HidIds ids, parentIds;
        bool nameDefined, interfacesDefined, labelsDefined;
        while(extractor.getNextToken(hash, &token))
        {
            switch(state)
            {
                case START:
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "{"_hash, token, INSIDE_BLOCK))
                        return false;
                    break;
                case INSIDE_BLOCK:
                    switch(hash)
                    {
                        case "}"_hash:
                            return true;
                        case "\n"_hash:
                            break;
                        default:
                        {
                            char *end;
                            uint32_t val = strtoul(token.c_str(), &end, 16);
                            if(*end || val > 0xFFFF)
                            {
                                errorsWriter.error("invalid device id " + token);
                                return false;
                            }
                            else
                            {
                                ids.vid = static_cast<uint16_t>(val);
                                state = AFTER_VID;
                            }
                            break;
                        }
                    }
                    break;
                case AFTER_VID:
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "."_hash, token, EXPECT_PID))
                        return false;
                    break;
                case EXPECT_PID:
                {
                    char *end;
                    uint32_t val = strtoul(token.c_str(), &end, 16);
                    if(*end || val > 0xFFFF)
                    {
                        errorsWriter.error("invalid product id " + token);
                        return false;
                    }
                    else
                    {
                        ids.pid = static_cast<uint16_t>(val);
                        if(newDevicesDb.devices.count(ids))
                        {
                            errorsWriter.error("multiple definition of the device " + token
                                + " in the same stream");
                            return false;
                        }
                        device = DeviceData();
                        nameDefined = false;
                        interfacesDefined = false;
                        labelsDefined = false;
                        state = AFTER_PID;
                    }
                    break;
                }
                case AFTER_PID:
                    switch(hash)
                    {
                        case "\n"_hash:
                            break;
                        case ":"_hash:
                            state = EXPECT_PARENT_VID;
                            break;
                        case "{"_hash:
                            state = INSIDE_DEVICE;
                            device.parent = HidIds::invalid;
                            break;
                        default:
                            errorsWriter.unexpectedTokenError(token);
                            return false;
                    }
                    break;
                case EXPECT_PARENT_VID:
                {
                    char *end;
                    uint32_t val = strtoul(token.c_str(), &end, 16);
                    if(*end || val > 0xFFFF)
                    {
                        errorsWriter.error("invalid parent vendor id " + token);
                        return false;
                    }
                    else
                    {
                        parentIds.vid = static_cast<uint16_t>(val);
                        state = AFTER_PARENT_VID;
                    }
                    break;
                }
                case AFTER_PARENT_VID:
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "."_hash, token, EXPECT_PARENT_PID))
                        return false;
                    break;
                case EXPECT_PARENT_PID:
                {
                    char *end;
                    uint32_t val = strtoul(token.c_str(), &end, 16);
                    if(*end || val > 0xFFFF)
                    {
                        errorsWriter.error("invalid parent vendor id " + token);
                        return false;
                    }
                    else
                    {
                        parentIds.pid = static_cast<uint16_t>(val);
                        if(!newDevicesDb.devices.count(parentIds) && !oldDevicesDb.devices.count(parentIds))
                        {
                            std::ostringstream oss;
                            oss << "unknown parent " << std::setfill('0') << std::setw(4) << std::hex << parentIds.vid
                                    << '.' << std::setfill('0') << std::setw(4) << std::hex << parentIds.pid << '\n';
                            errorsWriter.error(oss.str());
                            return false;
                        }
                        device.parent = parentIds;
                        state = AFTER_INHERITANCE;
                    }
                    break;
                }
                case AFTER_INHERITANCE:
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "{"_hash, token, INSIDE_DEVICE))
                        return false;
                    break;
                case INSIDE_DEVICE:
                    switch(hash)
                    {
                        case "\n"_hash:
                            break;
                        case "name"_hash:
                            if(nameDefined)
                            {
                                errorsWriter.error("multiple name definition");
                                return false;
                            }
                            nameDefined = true;
                            device.name = token;
                            state = EXPECT_EQUALS;
                            nextState = EXPECT_NAME;
                            break;
                        case "interfaces"_hash:
                            if(interfacesDefined)
                            {
                                errorsWriter.error("multiple interfaces definition");
                                return false;
                            }
                            interfacesDefined = true;
                            state = EXPECT_EQUALS;
                            nextState = EXPECT_INTERFACE;
                            break;
                        case "labels"_hash:
                            if(labelsDefined)
                            {
                                errorsWriter.error("multiple labels definition");
                                return false;
                            }
                            labelsDefined = true;
                            state = EXPECT_EQUALS;
                            nextState = EXPECT_LABELS;
                            break;
                        case "}"_hash:
                            newDevicesDb.devices[ids] = std::move(device);
                            state = INSIDE_BLOCK;
                            break;
                    }
                    break;
                case EXPECT_NAME:
                    if(hash == "\n"_hash || hash == ""_hash)
                    {
                        errorsWriter.error("no device name at the end of line");
                        return false;
                    }
                    device.name = token;
                    state = END_OF_LINE;
                    nextState = INSIDE_DEVICE;
                    break;
                case EXPECT_INTERFACE:
                    switch(hash)
                    {
                        case "\n"_hash:
                            if(!device.interfaces.size())
                            {
                                errorsWriter.error("no interfaces at the end of line");
                                return false;
                            }
                            state = INSIDE_DEVICE;
                            break;
                        case "+"_hash:
                            if(!device.interfaces.size())
                            {
                                errorsWriter.error("no interfaces at the end of line");
                                return false;
                            }
                            state = END_OF_LINE;
                            nextState = EXPECT_INTERFACE;
                            break;
                        default:
                        {
                            if(!Utils::isNameCharacter(token[0]))
                            {
                                errorsWriter.unexpectedTokenError(token);
                                return false;
                            }
                            std::vector<StrHash>::iterator it = std::lower_bound(device.interfaces.begin(),
                                    device.interfaces.end(), hash);
                            if(!newDevicesDb.interfaces.count(hash) && !oldDevicesDb.interfaces.count(hash))
                            {
                                errorsWriter.error("unknwon interface " + token);
                                return false;
                            }
                            else if(it == device.interfaces.end()) device.interfaces.push_back(hash);
                            else if(*it == hash)
                            {
                                errorsWriter.error("multiple definition of the interface " + token
                                    + " in the same stream");
                                return false;
                            }
                            else device.interfaces.insert(it, hash);
                        }
                        break;
                    }
                    break;
                case EXPECT_LABELS:
                    switch(hash)
                    {
                        case "\n"_hash:
                            if(!device.presetsLabels.size() && !device.ownLabels.size())
                            {
                                errorsWriter.error("no labels at the end of line");
                                return false;
                            }
                            state = INSIDE_DEVICE;
                            break;
                        case "+"_hash:
                            if(!device.presetsLabels.size() && !device.ownLabels.size())
                            {
                                errorsWriter.error("no labels at the end of line");
                                return false;
                            }
                            state = END_OF_LINE;
                            nextState = EXPECT_LABELS;
                            break;
                        case "{"_hash:
                        {
                            DeviceData *d = &device;
                            while(d->interfaces.empty() && d->parent != HidIds::invalid)
                            {
                                if(newDevicesDb.devices.count(d->parent)) d = &newDevicesDb.devices[d->parent];
                                else d = &oldDevicesDb.devices[d->parent];
                            }
                            if(!parseLabelsBlock(&d->interfaces, device.ownLabels)) return false;
                            state = END_OF_LINE;
                            nextState = INSIDE_DEVICE;
                            break;
                        }
                        default:
                            if(!Utils::isNameCharacter(token[0]))
                            {
                                errorsWriter.unexpectedTokenError(token);
                                return false;
                            }
                            if(std::find(device.presetsLabels.begin(), device.presetsLabels.end(), hash)
                                    != device.presetsLabels.end())
                            {
                                errorsWriter.error("labels preset " + token + " used multiple times");
                                return false;
                            }
                            device.presetsLabels.push_back(hash);
                            break;
                    }
                    break;
                case EXPECT_EQUALS:
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "="_hash, token, nextState))
                        return false;
                    break;
                case END_OF_LINE:
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "\n"_hash, token, nextState))
                        return false;
                    break;
                default:
                    return false;
            }
        }
        return false;
    }

    bool Parser::parseLabelsBlock(const std::vector<StrHash> *interfaces, StrHashMap<LabelInfosPrivate> &labels)
    {
        enum : uint8_t {LINE_START, LINE_NAME, LINE_2ND_TOKEN, LINE_COLOR, LINE_END} state = LINE_START;
        StrHash hash, labelsHash, lineHash, interfaceHash;
        std::string token, labelsName, lineName, interfaceName;
        Labels newLabels;
        LabelInfosPrivate *labelInfos;
        Interface *interface = nullptr;

        while(extractor.getNextToken(hash, &token))
        {
            switch(state)
            {
                case LINE_START:
                    switch(hash)
                    {
                        case "}"_hash:
                            return true;
                        case "\n"_hash:
                            break;
                        default:
                            if(!Utils::isNameCharacter(token[0]))
                            {
                                errorsWriter.unexpectedTokenError(token);
                                return false;
                            }
                            else
                            {
                                lineHash = hash;
                                lineName = token;
                                state = LINE_2ND_TOKEN;
                                if(interfaces) interface = nullptr;
                            }
                            break;
                    }
                    break;
                case LINE_NAME:
                    if(!interface->count(hash))
                    {
                        errorsWriter.error("unknown input " + token + " in interface " + interfaceName);
                        return false;
                    }
                    lineHash = hash;
                    lineName = token;
                    state = LINE_2ND_TOKEN;
                    break;
                case LINE_2ND_TOKEN:
                    switch(hash)
                    {
                        case ":"_hash:
                            if(interfaces)
                            {
                                errorsWriter.unexpectedTokenError(token);
                                return false;
                            }
                            if(newDevicesDb.interfaces.count(lineHash))
                            {
                                interface = &newDevicesDb.interfaces.at(lineHash);
                            }
                            else if(oldDevicesDb.interfaces.count(lineHash))
                            {
                                interface = &oldDevicesDb.interfaces.at(lineHash);
                            }
                            else
                            {
                                errorsWriter.error("unknwon interface " + token);
                                return false;
                            }
                            interfaceName = token;
                            state = LINE_END;
                            break;
                        case "."_hash:
                            if(!interfaces || interface)
                            {
                                errorsWriter.unexpectedTokenError(token);
                                return false;
                            }
                            if(newDevicesDb.interfaces.count(lineHash))
                            {
                                interface = &newDevicesDb.interfaces.at(lineHash);
                            }
                            else if(oldDevicesDb.interfaces.count(lineHash))
                            {
                                interface = &oldDevicesDb.interfaces.at(lineHash);
                            }
                            else
                            {
                                errorsWriter.error("unknwon interface " + lineName);
                                return false;
                            }
                            interfaceHash = lineHash;
                            interfaceName = lineName;
                            state = LINE_NAME;
                            break;
                        default:
                            if(token[0] != '"')
                            {
                                errorsWriter.unexpectedTokenError(token);
                                return false;
                            }
                            if(interfaces)
                            {
                                if(interface)
                                {

                                }
                                else
                                {
                                    interfaceHash = StrHash();
                                    for(StrHash hash : *interfaces)
                                    {
                                        if(newDevicesDb.interfaces.count(hash))
                                        {
                                            interface = &newDevicesDb.interfaces.at(hash);
                                        }
                                        else if(oldDevicesDb.interfaces.count(hash))
                                        {
                                            interface = &oldDevicesDb.interfaces.at(hash);
                                        }
                                        if(interface->count(lineHash))
                                        {
                                            if(interfaceHash == StrHash()) interfaceHash = hash;
                                            else
                                            {
                                                errorsWriter.error("label " + lineName + " belongs to several"
                                                        " interfaces");
                                                return false;
                                            }
                                        }
                                    }
                                    if(interfaceHash == StrHash())
                                    {
                                        errorsWriter.error("label " + lineName + " does not belong to any interface");
                                        return false;
                                    }
                                }

                            }
                            else
                            {
                                if(!interface)
                                {
                                    errorsWriter.error("label " + lineName + " does not belong to any interface");
                                    return false;
                                }
                                if(!interface->count(lineHash))
                                {
                                    errorsWriter.error("unknown input " + lineName + " in interface " + interfaceName);
                                    return false;
                                }
                            }
                            lineHash = interfaceHash;
                            lineHash.hashCharacter('.');
                            for(const char *c = lineName.c_str(); *c; c++) lineHash.hashCharacter(*c);
                            if(newLabels.map.count(lineHash))
                            {
                                errorsWriter.error("label " + lineName + " defined multiple times");
                                return false;
                            }
                            token.pop_back();
                            token.erase(0,1);
                            labelInfos = &newLabels.map[lineHash];
                            labelInfos->label = token;
                            state = LINE_COLOR;
                            break;
                    }
                    break;
                case LINE_COLOR:
                    switch(hash)
                    {
                        case "\n"_hash:
                            labelInfos->hasColor = false;
                            state = LINE_START;
                            break;
                        default:
                        {
                            uint8_t i;
                            for(i = 0; i < 6; i++) if(!isxdigit(token[i])) break;
                            if(i != 6 || token[6] != 0)
                            {
                                errorsWriter.error(token + " is not an RRGGBB sRGB hex color");
                                return false;
                            }
                            uint32_t val = strtoul(token.c_str(), nullptr, 16);
                            labelInfos->color.r = val >> 16;
                            labelInfos->color.g = (val >> 8) & 255;
                            labelInfos->color.b = val & 255;
                            labelInfos->hasColor = true;
                            state = LINE_START;
                            break;
                        }
                    }
                    break;
                case LINE_END:
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "\n"_hash, token, LINE_START))
                        return false;
                    break;
            }
        }
        return false;
    }

    Parser::Parser(std::istream &inStream, std::ostream *errors, DevicesDb &devicesDb)
        : errorsWriter(errors), extractor(inStream, errorsWriter), oldDevicesDb(devicesDb)
    {
    }

    bool Parser::parse()
    {
        StrHash hash;
        std::string token;
        while(extractor.getNextToken(hash, &token))
        {
            switch(hash)
            {
                case StrHash():
                    oldDevicesDb.interfaces.insert(newDevicesDb.interfaces.begin(), newDevicesDb.interfaces.end());
                    oldDevicesDb.labels.insert(newDevicesDb.labels.begin(), newDevicesDb.labels.end());
                    oldDevicesDb.devices.insert(newDevicesDb.devices.begin(), newDevicesDb.devices.end());
                    return true;
                case "interfaces"_hash:
                    if(!parseInterfacesBlock()) return false;
                    break;
                case "labels"_hash:
                    if(!parseLabelsBlock()) return false;
                    break;
                case "devices"_hash:
                    if(!parseDevicesBlock()) return false;
                    break;
                case "\n"_hash:
                    break;
                default:
                    errorsWriter.unexpectedTokenError(token);
                    return false;
            }
        }
        return false;
    }
}
