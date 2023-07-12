#include "Lazynput/Device.hpp"
#include "Lazynput/PrivateTypes.hpp"
#include <algorithm>
#include <string>
#include <assert.h>

namespace Lazynput
{
    InputInfos Device::dummyInputInfos;

    Device::Device() {}

    LabelInfos Device::genLabel(const DbLabelInfos &dbLabel, const IconsDb &iconsDb)
    {
        LabelInfos ret;
        ret.hasColor = dbLabel.hasColor;
        ret.color = dbLabel.color;
        if(dbLabel.label[0])
        {
            ret.hasLabel = true;
            if(dbLabel.label[0] == '$')
            {
                size_t spacePos = dbLabel.label.find_first_of(' ', 1);
                ret.variableName = dbLabel.label.substr(1, spacePos - 1);
                if(spacePos == std::string::npos)
                {
                    ret.ascii.reserve(ret.variableName.length());
                    for(uint8_t pos = 0; ret.variableName[pos]; pos++)
                    {
                        if(ret.variableName[pos] == '_') ret.ascii.push_back(' ');
                        else ret.ascii.push_back(ret.variableName[pos] | (pos == 0 ? 0 : 32));
                    }
                }
                else ret.ascii = dbLabel.label.substr(spacePos + 1);
                StrHash hash = StrHash::make(ret.variableName);
                ret.utf8 = iconsDb.count(hash) ? iconsDb.at(hash) : ret.ascii;
            }
            else
            {
                ret.ascii = dbLabel.label;
                ret.utf8 = dbLabel.label;
            }
        }
        else ret.hasLabel = false;
        return ret;
    }

    void Device::genGenericLabel(InputInfos &inputInfos)
    {
        inputInfos.label.ascii.clear();
        inputInfos.label.variableName.clear();
        const SingleBindingInfos &singlePositive = inputInfos.bindings.positive[0][0];
        if(singlePositive.options.invert && !singlePositive.options.half)
                inputInfos.label.ascii.push_back('~');
        switch(singlePositive.type)
        {
            case Lazynput::DeviceInputType::NIL:
                assert(false);
                break;
            case Lazynput::DeviceInputType::BUTTON:
                inputInfos.label.ascii.push_back('B');
                inputInfos.label.ascii += std::to_string(singlePositive.index + 1);
                break;
            case Lazynput::DeviceInputType::HAT:
                inputInfos.label.ascii.push_back('H');
                inputInfos.label.ascii += std::to_string(singlePositive.index / 2 + 1);
                break;
            case Lazynput::DeviceInputType::ABSOLUTE_AXIS:
                inputInfos.label.ascii.push_back('A');
                inputInfos.label.ascii += std::to_string(singlePositive.index + 1);
                break;
            case Lazynput::DeviceInputType::RELATIVE_AXIS:
                inputInfos.label.ascii.push_back('R');
                inputInfos.label.ascii += std::to_string(singlePositive.index + 1);
                break;
        }
        if(singlePositive.options.half)
        {
            const SingleBindingInfos &singleNegative = inputInfos.bindings.negative[0][0];
            if(singleNegative.type != singlePositive.type || singleNegative.index != singlePositive.index)
                    inputInfos.label.ascii.push_back(singlePositive.options.invert ? '-': '+');
        }
        inputInfos.label.utf8 = inputInfos.label.ascii;
    }

    void Device::fillLabels(const StrHashMap<DbLabelInfos> &labels, const IconsDb &iconsDb)
    {
        for(auto it = labels.begin(); it != labels.end(); ++it)
            inputInfos[it->first].label = genLabel(it->second, iconsDb);

        for(auto it = inputInfos.begin(); it != inputInfos.end(); ++it)
            if(!it->second.label.hasLabel) genGenericLabel(it->second);
    }

    void Device::fillLabels(const Labels &labels, const LabelsDb &labelsDb, const IconsDb &iconsDb)
    {
        if(labels.parent != StrHash()) fillLabels(labelsDb.at(labels.parent), labelsDb, iconsDb);
        fillLabels(labels.map, iconsDb);
    }

    void Device::fillBindings(const StrHashMap<FullBindingInfos> &bindings)
    {
        for(auto it = bindings.begin(); it != bindings.end(); ++it)
            inputInfos[it->first].bindings = it->second;
    }

    void Device::fillBindings(const ConfigTagBindings &bindings, const std::vector<StrHash> &configTags)
    {
        fillBindings(bindings.bindings);
        for(auto it = bindings.nestedConfigTags.begin(); it != bindings.nestedConfigTags.end(); it++)
                if(std::find(configTags.begin(), configTags.end(), it->first) != configTags.end())
                        fillBindings(*it->second.get(), configTags);
    }

    void Device::fillData(const DeviceData &deviceData, const DevicesDb &devicesDb,
            const std::vector<StrHash> &configTags)
    {
        if(deviceData.parent != HidIds::invalid)
                fillData(devicesDb.devices.at(deviceData.parent), devicesDb, configTags);
        if(!deviceData.name.empty()) name = deviceData.name;
        for(StrHash preset : deviceData.presetsLabels) fillLabels(devicesDb.labels.at(preset), devicesDb.labels,
                devicesDb.icons);
        fillBindings(deviceData.bindings, configTags);
        fillLabels(deviceData.ownLabels, devicesDb.icons);
    }

    void Device::removeNilBindings()
    {
        for(auto it = inputInfos.begin(); it != inputInfos.end();)
                if(it->second.bindings.positive.empty() && it->second.bindings.negative.empty())
                it = inputInfos.erase(it); else ++it;
    }

    Device::Device(const DeviceData &deviceData, const DevicesDb &devicesDb, const std::vector<StrHash> &configTags)
    {
        fillData(deviceData, devicesDb, configTags);
        removeNilBindings();
    }

    bool Device::hasInput(StrHash hash) const
    {
        if(inputInfos.count(hash))
        {
            const FullBindingInfos &bindings = inputInfos.at(hash).bindings;
            return bindings.positive.size() > 0 || bindings.negative.size() > 0;
        }
        else return false;
    }

    bool Device::hasInput(const char *name) const
    {
        return hasInput(StrHash::make(name));
    }

    const InputInfos &Device::getInputInfos(StrHash hash) const
    {
        return hasInput(hash) ? inputInfos.at(hash) : dummyInputInfos;
    }

    const InputInfos &Device::getInputInfos(const char *name) const
    {
        return getInputInfos(StrHash::make(name));
    }

    const LabelInfos &Device::getLabel(StrHash hash) const
    {
        return getInputInfos(hash).label;
    }

    const LabelInfos &Device::getLabel(const char *name) const
    {
        return getLabel(StrHash::make(name));
    }

    std::pair<StrHashMap<InputInfos>::const_iterator, StrHashMap<InputInfos>::const_iterator>
            Device::getInputInfosIterators() const
    {
        return std::make_pair(inputInfos.cbegin(), inputInfos.cend());
    }

    const std::string &Device::getName() const
    {
        return name;
    }

    void Device::setName(const char *name)
    {
        this->name = std::string(name);
    }

    void Device::setInputInfos(StrHashMap<InputInfos> &&inputInfos)
    {
        this->inputInfos = std::move(inputInfos);

        removeNilBindings();

        for(auto it = this->inputInfos.begin(); it != this->inputInfos.end(); ++it)
            if(!it->second.label.hasLabel) genGenericLabel(it->second);
    }

    Device::operator bool() const
    {
        return !inputInfos.empty();
    }
}
