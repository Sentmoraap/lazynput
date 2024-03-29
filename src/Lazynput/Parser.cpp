#include <sstream>
#include <iomanip>
#include <algorithm>
#include <assert.h>
#include "Lazynput/Parser.hpp"
#include "Lazynput/StrHash.hpp"
#include "Lazynput/PrivateTypes.hpp"
#include "Lazynput/Utils.hpp"

using namespace Lazynput::Literals;

namespace Lazynput
{
    bool Parser::expectToken(uint8_t *state, StrHash hash, StrHash expectedHash, bool skipNewLines,
            std::string token, uint8_t nextState)
    {
        while(skipNewLines && hash == "\n"_hash) extractor.getNextToken(hash, &token);
        if(hash == expectedHash)
        {
            *state = nextState;
            return true;
        }
        else
        {
            errorsWriter.unexpectedTokenError(token);
            return false;
        }
    }

    Interface *Parser::getInterface(StrHash hash)
    {
        if(newDevicesDb.interfaces.count(hash))
        {
            return &newDevicesDb.interfaces.at(hash);
        }
        else if(oldDevicesDb.interfaces.count(hash))
        {
            return &oldDevicesDb.interfaces.at(hash);
        }
        else return nullptr;
    }

    std::pair<StrHash, Interface*>  Parser::getInputInterface(const std::vector<StrHash> &interfaces,
            StrHash inputHash, const std::string &inputStr)
    {
        std::pair<StrHash, Interface*> ret = std::make_pair(StrHash(), nullptr);
        for(StrHash hash : interfaces)
        {
            Interface *interface = getInterface(hash);
            if(interface->count(inputHash))
            {
                if(ret.first == StrHash()) ret = std::make_pair(hash, interface);
                else
                {
                    errorsWriter.error("input " + inputStr + " belongs to several"
                            " interfaces");
                    return std::make_pair(StrHash(), nullptr);
                }
            }
        }
        if(ret.first == StrHash())
        {
            errorsWriter.error("input " + inputStr + " does not belong to any interface");
        }
        return ret;
    }

    bool Parser::parseInterfacesBlock()
    {
        enum : uint8_t {START, INSIDE_BLOCK, INTERFACE_START, INSIDE_INTERFACE, INPUT_TYPE_COLON} state = START;
        InterfaceInputType inputType = InterfaceInputType::BUTTON;
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
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "{"_hash, true, token, INSIDE_BLOCK))
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
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "{"_hash, true, token, INSIDE_INTERFACE))
                        return false;
                    break;
                case INSIDE_INTERFACE:
                    switch(hash)
                    {
                        case "btn"_hash:
                            inputType = InterfaceInputType::BUTTON;
                            inputTypeDefined = true;
                            state = INPUT_TYPE_COLON;
                            break;
                        case "abs"_hash:
                            inputType = InterfaceInputType::ABSOLUTE_AXIS;
                            inputTypeDefined = true;
                            state = INPUT_TYPE_COLON;
                            break;
                        case "rel"_hash:
                            inputType = InterfaceInputType::RELATIVE_AXIS;
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
                                        else
                                        {
                                            newInterface[hash] = inputType;
                                            std::string completeName = interfaceName + "." + token;
                                            newDevicesDb.stringFromHash[StrHash::make(completeName)] =
                                                    std::move(completeName);
                                        }
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
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, ":"_hash, false, token, INSIDE_INTERFACE))
                        return false;
                    break;
            }
        }
        return false;
    }

    bool Parser::parseIconsBlock()
    {
        enum : uint8_t {START, INSIDE_BLOCK, EXPECT_STRING, LINE_END} state = START;
        StrHash hash, icon;
        std::string token;

        while(extractor.getNextToken(hash, &token))
        {
            switch(state)
            {
                case START:
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "{"_hash, true, token, INSIDE_BLOCK))
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
                            else if(newDevicesDb.icons.count(hash))
                            {
                                errorsWriter.error("multiple definition of the icon " + token
                                    + " in the same stream");
                            }
                            else
                            {
                                icon = hash;
                                state = EXPECT_STRING;
                            }
                    }
                    break;
                case EXPECT_STRING:
                    if(token[0] != '"')
                    {
                        errorsWriter.unexpectedTokenError(token);
                        return false;
                    }
                    token.pop_back();
                    token.erase(0,1);
                    newDevicesDb.icons[icon] = token;
                    state = LINE_END;
                    break;
                case LINE_END:
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "\n"_hash, false, token, INSIDE_BLOCK))
                        return false;
                    break;
            }
        }
        return false;
    }

    bool Parser::parseLabelsSubBlock(const std::vector<StrHash> *interfaces, StrHashMap<DbLabelInfos> &labels)
    {
        enum : uint8_t {LINE_START, LINE_NAME, LINE_2ND_TOKEN, LINE_COLOR, LINE_END} state = LINE_START;
        StrHash hash, lineHash, interfaceHash;
        std::string token, labelsName, lineName, interfaceName;
        DbLabelInfos *labelInfos;
        Interface *interface = nullptr;

        auto getLabelInput = [this, interfaces,
                &interface, &lineHash, &interfaceHash, &lineName, &interfaceName, &labels, &labelInfos]()
        {
            if(interfaces)
            {
                if(!interface)
                {
                    interfaceHash = getInputInterface(*interfaces, lineHash, lineName).first;
                    if(interfaceHash == StrHash()) return false;
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
            if(labels.count(lineHash))
            {
                errorsWriter.error("label " + lineName + " defined multiple times");
                return false;
            }
            labelInfos = &labels[lineHash];
            return true;
        };

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
                            interface = getInterface(lineHash);
                            if(!interface)
                            {
                                errorsWriter.error("unknwon interface " + interfaceName);
                                return false;
                            }
                            interfaceName = lineName;
                            interfaceHash = lineHash;
                            state = LINE_END;
                            break;
                        case "."_hash:
                            if(!interfaces || interface)
                            {
                                errorsWriter.unexpectedTokenError(token);
                                return false;
                            }
                            interface = getInterface(lineHash);
                            if(!interface)
                            {
                                errorsWriter.error("unknwon interface " + interfaceName);
                                return false;
                            }
                            interfaceHash = lineHash;
                            interfaceName = lineName;
                            state = LINE_NAME;
                            break;
                        case "nil"_hash:
                            if(!getLabelInput()) return false;
                            state = LINE_COLOR;
                        break;
                        default:
                            if(token[0] != '"')
                            {
                                errorsWriter.unexpectedTokenError(token);
                                return false;
                            }
                            if(!getLabelInput()) return false;
                            token.pop_back();
                            token.erase(0,1);
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
                        case ","_hash:
                            if(interfaces)
                            {
                                labelInfos->hasColor = false;
                                state = LINE_START;
                            }
                            else
                            {
                                errorsWriter.unexpectedTokenError(token);
                                return false;
                            }
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
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "\n"_hash, false, token, LINE_START))
                        return false;
                    break;
            }
        }
        return false;
    }

    bool Parser::parseLabelsBlock()
    {
        enum : uint8_t {START, INSIDE_BLOCK, LABELS_START, INHERITANCE, AFTER_INHERITANCE} state = START;
        StrHash hash, labelsHash;
        std::string token, labelsName, lineName, interfaceName;
        Labels newLabels;

        while(extractor.getNextToken(hash, &token))
        {
            switch(state)
            {
                case START:
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "{"_hash, true, token, INSIDE_BLOCK))
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
                            if(!parseLabelsSubBlock(nullptr, newLabels.map)) return false;
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
                            if(!parseLabelsSubBlock(nullptr, newLabels.map)) return false;
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

    bool Parser::parseSingleBindingInput(SingleBindingInfos &positive, SingleBindingInfos *negative,
            bool &unparsedToken, StrHash &hash, std::string &token)
    {
        enum : uint8_t {MAY_INVERT, INPUT, AXIS_HALF} state = MAY_INVERT;
        positive.options.half = false;
        positive.options.invert = false;
        auto mirrorBinding = [this, &positive, negative]()
        {
            if(!negative) return true;
            if(positive.options.half)
            {
                errorsWriter.error("can’t bind a button to a full axis");
                return false;
            }
            positive.options.half = true;
            *negative = positive;
            negative->options.invert = !negative->options.invert;
            return true;
        };
        while(extractor.isNextTokenStuck() && extractor.getNextToken(hash, &token))
        {
            switch(state)
            {
                case MAY_INVERT:
                    if(hash == "~"_hash)
                    {
                        positive.options.invert = true;
                        state = INPUT;
                        break;
                    }
                // Fallthrough
                case INPUT:
                {
                    auto parseInputIndex = [this, &token, &positive]()
                    {
                        char *endChar;
                        const char *beginChar = token.c_str();
                        static char errorChar = 1;
                        int inputIndex = strtol(beginChar + 1, &endChar, 10);
                        if(endChar == beginChar + 1)
                        {
                            errorsWriter.error("index is missing in " + token);
                            return &errorChar;
                        }
                        if(inputIndex < 0 || inputIndex > 255)
                        {
                            errorsWriter.error("index of " + token + " outside range [0-255]");
                            return &errorChar;
                        }
                        positive.index = static_cast<uint8_t>(inputIndex);
                        return endChar;
                    };
                    switch(token[0])
                    {
                        case 'a':
                            positive.type = DeviceInputType::ABSOLUTE_AXIS;
                            if(*(parseInputIndex())) return false;
                            if(positive.options.invert) return mirrorBinding();
                            else state = AXIS_HALF;
                            break;
                        case 'b':
                            positive.type = DeviceInputType::BUTTON;
                            if(*(parseInputIndex())) return false;
                            if(negative)
                            {
                                errorsWriter.error("can’t bind a button to a full axis");
                                return false;
                            }
                            return true;
                        case 'h':
                        {
                            positive.type = DeviceInputType::HAT;
                            const char *endChar = parseInputIndex();
                            positive.index *= 2;
                            switch(*endChar)
                            {
                                case 'y':
                                    positive.index++;
                                // Fallthrough
                                case 'x':
                                    if(!*(endChar + 1)) break;
                                // Fallthrough
                                default:
                                    if(endChar > token.c_str() + 1) errorsWriter.unexpectedTokenError(token);
                                    return false;
                            }
                            if(positive.options.invert) return mirrorBinding();
                            else state = AXIS_HALF;
                            break;
                        }
                        case 'r':
                            positive.type = DeviceInputType::RELATIVE_AXIS;
                            if(*(parseInputIndex()))
                            {
                                errorsWriter.unexpectedTokenError(token);
                                return false;
                            }
                            if(positive.options.invert) return mirrorBinding();
                            else state = AXIS_HALF;
                            break;
                        default:
                            if(hash == "nil"_hash)
                            {
                                positive.type = DeviceInputType::NIL;
                                if(negative) negative->type = DeviceInputType::NIL;
                                return true;
                            }
                            errorsWriter.error("unknown input " + token);
                            return false;
                    }
                    break;
                }
                case AXIS_HALF:
                    switch(hash)
                    {
                        case "+"_hash:
                            positive.options.half = true;
                            break;
                        case "-"_hash:
                            positive.options.invert = true;
                            positive.options.half = true;
                            break;
                        default:
                            unparsedToken = true;
                            break;
                    }
                    return mirrorBinding();
            }
        }
        if(state < AXIS_HALF)
        {
            errorsWriter.error("binding not complete");
            return false;
        }
        else return mirrorBinding();
    }

    bool Parser::parseHalvesBindingInput(HalfBindingInfos &positive, HalfBindingInfos *negative)
    {
        assert(positive.empty());
        assert(!negative || negative->empty());
        positive.emplace_back();
        if(negative) negative->emplace_back();
        bool hasToken = false;
        StrHash hash;
        std::string token;
        enum : uint8_t {BINDING, OPERATOR} state = BINDING;
        while(hasToken || extractor.isNextTokenStuck())
        {
            switch(state)
            {
                case BINDING:
                    positive.back().emplace_back();
                    if(negative) negative->back().emplace_back();
                    if(!parseSingleBindingInput(positive.back().back(), negative ? &negative->back().back() : nullptr,
                            hasToken, hash, token)) return false;
                    if(positive.back().back().type == DeviceInputType::NIL
                            && (positive.size() > 1 || positive[0].size() > 1))
                    {
                        errorsWriter.error("nil input in complex binding");
                        return false;
                    }
                    state = OPERATOR;
                    break;
                case OPERATOR:
                    if(!hasToken) extractor.getNextToken(hash, &token);
                    switch(hash)
                    {
                        case "|"_hash:
                            state = BINDING;
                            positive.emplace_back();
                            if(negative) negative->emplace_back();
                            break;
                        case "&"_hash:
                            state = BINDING;
                            break;
                        default:
                            errorsWriter.unexpectedTokenError(token);
                            return false;
                    }
                    hasToken = false;
                    break;
            }
        }
        if(state == BINDING)
        {
            errorsWriter.error("binding expected");
            return false;
        }
        else
        {
            if(positive.back().back().type == DeviceInputType::NIL)
            {
                positive.clear();
                if(negative) negative->clear();
            }
            return true;
        }
    }

    bool Parser::parseDecomposeFullBindingInput(FullBindingInfos &fullBinding, InterfaceInputType inputType,
            AxisHalves axisHalves)
    {
        if(((axisHalves == POSITIVE_ONLY || axisHalves == FULL) && fullBinding.positive.size())
                || ((axisHalves == NEGATIVE_ONLY || axisHalves == FULL) && fullBinding.negative.size()))
                return errorsWriter.error("input defined multiple times for the same config tag"), false;
        switch(inputType)
        {
            case InterfaceInputType::NIL:
                assert(false);
                break;
            case InterfaceInputType::BUTTON:
                if(axisHalves != FULL) return errorsWriter.error("Cannot bind to half a button"), false;
                return parseHalvesBindingInput(fullBinding.positive, nullptr);
                break;
            case InterfaceInputType::ABSOLUTE_AXIS:
            case InterfaceInputType::RELATIVE_AXIS:
                switch(axisHalves)
                {
                    case NEGATIVE_ONLY:
                        return parseHalvesBindingInput(fullBinding.negative, nullptr);
                    case POSITIVE_ONLY:
                        return parseHalvesBindingInput(fullBinding.positive, nullptr);
                    case FULL:
                        return parseHalvesBindingInput(fullBinding.positive, &fullBinding.negative);
                }
                break;
        }
        assert(false);
    }

    bool Parser::parseDevice(DeviceData &device, std::vector<StrHash> deviceInterfaces)
    {
        enum : uint8_t {INSIDE_DEVICE, EXPECT_NAME, EXPECT_INTERFACE, EXPECT_LABELS,
                EXPECT_LABELS_BLOCK, TAG_OR_INPUT, TAG_ABSENT, END_TAG_OR_INPUT, EXPECT_INTERFACE_INPUT,
                EQUALS_DEVICE_INPUT, EXPECT_EQUALS, END_OF_LINE}
                state = INSIDE_DEVICE, nextState;
        AxisHalves axisHalves = FULL;
        StrHash hash, prevHash, inputHash, interfaceHash;
        std::string token, prevToken;
        bool nameDefined = false, interfacesDefined = false, labelsDefined = false;
        std::vector<ConfigTagBindings*> tagsStack;
        tagsStack.reserve(4);
        uint8_t stackPos = 0;
        Interface *interface;

        auto newTag = [&tagsStack, &state, &stackPos, this](StrHash hash, bool isPresent)
        {
            tagsStack.erase(tagsStack.begin() + stackPos + 1, tagsStack.end());
            ConfigTagPresent &configTagPresent = tagsStack[stackPos]->nestedConfigTags[hash];
            if((configTagPresent.present && isPresent) || (configTagPresent.absent && !isPresent))
            {
                errorsWriter.error("config tag already defined");
                return false;
            }
            tagsStack.push_back(new ConfigTagBindings());
            (isPresent ? configTagPresent.present : configTagPresent.absent).reset(tagsStack.back());
            state = TAG_OR_INPUT;
            return true;
        };
        while(extractor.getNextToken(hash, &token))
        {
            switch(state)
            {
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
                        case "default"_hash:
                            extractor.getNextToken(hash, &token);
                            tagsStack.clear();
                            tagsStack.push_back(&device.bindings);

                            if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, ":"_hash, false, token,
                                    TAG_OR_INPUT)) return false;
                            break;
                        case "}"_hash:
                            return true;
                        case "!"_hash:
                            tagsStack.clear();
                            tagsStack.push_back(&device.bindings);
                            state = TAG_ABSENT;
                            break;
                        default:
                            if(!Utils::isNameCharacter(token[0]))
                            {
                                errorsWriter.unexpectedTokenError(token);
                                return false;
                            }
                            tagsStack.clear();
                            tagsStack.push_back(&device.bindings);
                            newTag(hash, true);
                            extractor.getNextToken(hash, &token);
                            if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, ":"_hash, false, token,
                                    TAG_OR_INPUT)) return false;
                            break;
                    }
                    break;
                case EXPECT_NAME:
                    if(hash == "\n"_hash || hash == ""_hash)
                    {
                        errorsWriter.error("no device name at the end of line");
                        return false;
                    }
                    if(token[0] != '"')
                    {
                        errorsWriter.unexpectedTokenError(token);
                        return false;
                    }
                    token.pop_back();
                    token.erase(0,1);
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
                            if(!getInterface(hash))
                            {
                                errorsWriter.error("unknwon interface " + token);
                                return false;
                            }
                            std::vector<StrHash>::iterator it = std::lower_bound(device.interfaces.begin(),
                                    device.interfaces.end(), hash);
                            if(it == device.interfaces.end()) device.interfaces.push_back(hash);
                            else if(*it == hash)
                            {
                                errorsWriter.error("multiple definition of the interface " + token
                                    + " in the same stream");
                                return false;
                            }
                            else device.interfaces.insert(it, hash);
                            it = std::lower_bound(deviceInterfaces.begin(),
                                    deviceInterfaces.end(), hash);
                            if(it == deviceInterfaces.end() || *it != hash) deviceInterfaces.insert(it, hash);
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
                                state = EXPECT_LABELS_BLOCK;
                            }
                            else state = INSIDE_DEVICE;
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
                            if(!parseLabelsSubBlock(&deviceInterfaces, device.ownLabels)) return false;
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
                            if(!newDevicesDb.labels.count(hash) && !oldDevicesDb.labels.count(hash))
                            {
                                errorsWriter.error("unknown labels " + token);
                                return false;
                            }
                            device.presetsLabels.push_back(hash);
                            break;
                    }
                    break;
                case EXPECT_LABELS_BLOCK:
                    switch(hash)
                    {
                        case "{"_hash:
                            if(!parseLabelsSubBlock(&deviceInterfaces, device.ownLabels)) return false;
                            state = END_OF_LINE;
                            nextState = INSIDE_DEVICE;
                            break;
                        default:
                            errorsWriter.unexpectedTokenError(token);
                            return false;
                    }
                    break;
                case TAG_OR_INPUT:
                    switch(hash)
                    {
                        case "\n"_hash:
                            break;
                        case "{"_hash:
                            stackPos++;
                            if(stackPos > tagsStack.size() - 1)
                            {
                                errorsWriter.error("no config tag to nest");
                            }
                            break;
                        case "}"_hash:
                            if(stackPos) stackPos--;
                            else return true;
                            break;
                        case "!"_hash:
                            state = TAG_ABSENT;
                            break;
                        default:
                            if(!Utils::isNameCharacter(token[0]))
                            {
                                errorsWriter.unexpectedTokenError(token);
                                return false;
                            }
                            prevHash = hash;
                            prevToken = token;
                            state = END_TAG_OR_INPUT;
                            break;
                    }
                    break;
                case TAG_ABSENT:
                {
                    if(!Utils::isNameCharacter(token[0]))
                    {
                        errorsWriter.unexpectedTokenError(token);
                        return false;
                    }
                    StrHash nextHash;
                    if(!extractor.getNextToken(nextHash, &token)) return false;
                    if(nextHash != ":"_hash) return errorsWriter.unexpectedTokenError(token), false;
                    if(!newTag(hash, false)) return false;
                    break;
                }
                case END_TAG_OR_INPUT:
                    switch(hash)
                    {
                        case ":"_hash: // The previous token should be a config tag.
                            newTag(prevHash, true);
                            break;
                        case "."_hash: // The previous token should be an interface.
                            if(std::find(device.interfaces.begin(), device.interfaces.end(), prevHash)
                                    == device.interfaces.end())
                            {
                                errorsWriter.error("device does not implement interface " + prevToken);
                                return false;
                            }
                            interface = getInterface(prevHash);
                            state = EXPECT_INTERFACE_INPUT;
                            break;
                        case "-"_hash:
                            axisHalves = NEGATIVE_ONLY;
                            if(false)
                        // Fallthrough but skip to the "=" case
                        case "+"_hash:
                            axisHalves = POSITIVE_ONLY;
                        // Fallthrough
                        case "="_hash: // The previous token should be an input.
                        {
                            std::tie(inputHash, interface) = getInputInterface(deviceInterfaces, prevHash, prevToken);
                            if(inputHash == StrHash()) return false;
                            inputHash.hashCharacter('.');
                            for(const char *c = prevToken.c_str(); *c; c++) inputHash.hashCharacter(*c);
                            interfaceHash = prevHash;
                            if(hash == "="_hash) goto parseDeviceInput;
                            state = EQUALS_DEVICE_INPUT;
                            break;
                        }
                        default:
                            errorsWriter.unexpectedTokenError(token);
                            return false;
                    }
                    break;
                case EXPECT_INTERFACE_INPUT:
                    if(!Utils::isNameCharacter(token[0]))
                    {
                        errorsWriter.unexpectedTokenError(token);
                        return false;
                    }
                    if(!interface->count(hash))
                    {
                        errorsWriter.error("unknown input " + token + " in interface " + prevToken);
                        return false;
                    }
                    prevHash.hashCharacter('.');
                    for(const char *c = prevToken.c_str(); *c; c++) prevHash.hashCharacter(*c);
                    state = EXPECT_EQUALS;
                    nextState = EQUALS_DEVICE_INPUT;
                    inputHash = prevHash;
                    interfaceHash = hash;
                    break;
                case EQUALS_DEVICE_INPUT:
                    if(hash != "="_hash) return errorsWriter.unexpectedTokenError(token), false;
                parseDeviceInput:
                    if(!parseDecomposeFullBindingInput(tagsStack.back()->bindings[inputHash],
                            interface->at(interfaceHash), axisHalves)) return false;
                    state = TAG_OR_INPUT;
                    axisHalves = FULL;
                    break;
                case EXPECT_EQUALS:
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "="_hash, false, token, nextState))
                        return false;
                    break;
                case END_OF_LINE:
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "\n"_hash, false, token, nextState))
                        return false;
                    break;
            }
        }
        return false;
    }

    bool Parser::parseDevicesBlock()
    {
        enum : uint8_t {START, INSIDE_BLOCK, AFTER_VID, EXPECT_PID, AFTER_PID, EXPECT_PARENT_VID, AFTER_PARENT_VID,
                EXPECT_PARENT_PID, AFTER_INHERITANCE}
                state = START;
        StrHash hash;
        std::string token;
        DeviceData device;
        std::vector<StrHash> deviceInterfaces;
        HidIds ids, parentIds;
        while(extractor.getNextToken(hash, &token))
        {
            switch(state)
            {
                case START:
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "{"_hash, true, token, INSIDE_BLOCK))
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
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "."_hash, false, token, EXPECT_PID))
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
                            device.parent = HidIds::invalid;
                            if(parseDevice(device, deviceInterfaces))
                            {
                                newDevicesDb.devices[ids] = std::move(device);
                                state = INSIDE_BLOCK;
                            }
                            else return false;
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
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "."_hash, false, token,
                            EXPECT_PARENT_PID)) return false;
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
                                    << '.' << std::setfill('0') << std::setw(4) << std::hex << parentIds.pid;
                            errorsWriter.error(oss.str());
                            return false;
                        }
                        device.parent = parentIds;
                        HidIds parentIds = device.parent;
                        // Construct list of all implemented interfaces, own ones and inherited ones.
                        while(parentIds != HidIds::invalid)
                        {
                            DeviceData &parent =
                                    (newDevicesDb.devices.count(parentIds) ? newDevicesDb : oldDevicesDb)
                                    .devices[parentIds];
                            parentIds = parent.parent;
                            std::vector<StrHash>::iterator it = deviceInterfaces.begin(),
                                    parentIt = parent.interfaces.begin();
                            while(parentIt != parent.interfaces.end())
                            {
                                while(it != deviceInterfaces.end() && *it < *parentIt) it++;
                                if(it == deviceInterfaces.end() || *it != *parentIt)
                                {
                                    deviceInterfaces.insert(it, *parentIt);
                                    it = deviceInterfaces.begin();
                                }
                                parentIt++;
                            }
                        }
                        state = AFTER_INHERITANCE;
                    }
                    break;
                }
                case AFTER_INHERITANCE:
                    if(!expectToken(reinterpret_cast<uint8_t*>(&state), hash, "{"_hash, true, token, INSIDE_BLOCK))
                        return false;
                    if(parseDevice(device, deviceInterfaces))
                    {
                        newDevicesDb.devices[ids] = std::move(device);
                        state = INSIDE_BLOCK;
                    }
                    else return false;
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
                    oldDevicesDb.icons.insert(newDevicesDb.icons.begin(), newDevicesDb.icons.end());
                    oldDevicesDb.stringFromHash.insert(newDevicesDb.stringFromHash.begin(),
                            newDevicesDb.stringFromHash.end());
                    oldDevicesDb.labels.insert(newDevicesDb.labels.begin(), newDevicesDb.labels.end());
                    for(auto it = newDevicesDb.devices.begin(); it != newDevicesDb.devices.end(); ++it)
                            oldDevicesDb.devices[it->first] = std::move(it->second);
                    return true;
                case "interfaces"_hash:
                    if(!parseInterfacesBlock()) return false;
                    break;
                case "icons"_hash:
                    if(!parseIconsBlock()) return false;
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
