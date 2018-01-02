#include "Parser.hpp"
#include "StrHash.hpp"
#include "Types.hpp"
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
                            if(newInterfacesDb.count(hash))
                            {
                                errorsWriter.error("multiple definition of the interface " + token
                                    + " in the same stream");
                            }
                            else if(oldInterfacesDb.count(hash))
                            {
                                state = INTERFACE_START;
                                interfaceName = token;
                                interfaceHash = hash;
                                oldInterface = &oldInterfacesDb.at(hash);
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
                            else newInterfacesDb[interfaceHash] = std::move(newInterface);
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
        return true;
    }

    bool Parser::parseDevicesBlock()
    {
        return true;
    }

    Parser::Parser(std::istream &inStream, std::ostream *errors, InterfacesDb &interfacesDb)
        : errorsWriter(errors), extractor(inStream, errorsWriter), oldInterfacesDb(interfacesDb)
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
                    oldInterfacesDb.insert(newInterfacesDb.begin(), newInterfacesDb.end());
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
