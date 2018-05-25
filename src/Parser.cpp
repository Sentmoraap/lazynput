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
        enum : uint8_t {START, INSIDE_BLOCK, LABELS_START, INHERITANCE, AFTER_INHERITANCE,
                LINE_START, LINE_2ND_TOKEN, LINE_COLOR, LINE_END}
                state = START;
        StrHash hash, labelsHash, lineHash;
        std::string token, labelsName, lineName, interfaceName;
        Labels newLabels;
        LabelInfosPrivate *labelInfos;
        Interface *interface;

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
                                interface = nullptr;
                            }
                            break;
                    }
                    break;
                case LABELS_START:
                    switch(hash)
                    {
                        case "{"_hash:
                            state = LINE_START;
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
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "{"_hash, token, LINE_START))
                        return false;
                    break;
                case LINE_START:
                    switch(hash)
                    {
                        case "}"_hash:
                            state = INSIDE_BLOCK;
                            newDevicesDb.labels[labelsHash] = std::move(newLabels);
                            break;
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
                            }
                            break;
                    }
                    break;
                case LINE_2ND_TOKEN:
                    switch(hash)
                    {
                        case ":"_hash:
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
                        default:
                            if(token[0] != '"')
                            {
                                errorsWriter.unexpectedTokenError(token);
                                return false;
                            }
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

    bool Parser::parseDevicesBlock()
    {
        return true;
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
