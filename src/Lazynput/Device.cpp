#include "Lazynput/Device.hpp"
#include "Lazynput/PrivateTypes.hpp"
#include <algorithm>
#include <string>

namespace Lazynput
{
    Device::Device() {}

    void Device::fillLabels(const StrHashMap<LabelInfos> &labels)
    {
        for(auto it = labels.begin(); it != labels.end(); ++it)
            inputInfos[it->first].labelInfos = it->second;
    }

    void Device::fillLabels(const Labels &labels, const LabelsDb &labelsDb)
    {
        if(labels.parent != StrHash()) fillLabels(labelsDb.at(labels.parent), labelsDb);
        fillLabels(labels.map);
    }

    void Device::fillBindings(const StrHashMap<FullBindingInfos> &bindings)
    {
        for(auto it = bindings.begin(); it != bindings.end(); ++it)
            inputInfos[it->first].binding = it->second;
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
        for(StrHash preset : deviceData.presetsLabels) fillLabels(devicesDb.labels.at(preset), devicesDb.labels);
        fillLabels(deviceData.ownLabels);
        fillBindings(deviceData.bindings, configTags);
    }

    void Device::removeNilBindings()
    {
        for(auto it = inputInfos.begin(); it != inputInfos.end();)
            if(it->second.binding.empty()) it = inputInfos.erase(it); else ++it;
    }

    Device::Device(const DeviceData &deviceData, const DevicesDb &devicesDb, const std::vector<StrHash> &configTags)
    {
        fillData(deviceData, devicesDb, configTags);
        removeNilBindings();
    }

    bool Device::hasInput(StrHash hash) const
    {
        return inputInfos.count(hash);
    }

    bool Device::hasInput(const char *name) const
    {
        return hasInput(StrHash::make(name));
    }

    const InputInfos Device::getInputInfos(StrHash hash) const
    {
        return hasInput(hash) ? inputInfos.at(hash) : InputInfos();
    }

    const InputInfos Device::getInputInfos(const char *name) const
    {
        return getInputInfos(StrHash::make(name));
    }

    std::pair<StrHashMap<InputInfos>::const_iterator, StrHashMap<InputInfos>::const_iterator>
            Device::getInputInfosIterators() const
    {
        return std::make_pair(inputInfos.cbegin(), inputInfos.cend());
    }

    LabelInfos Device::getEnglishAsciiLabelInfos(StrHash hash) const
    {
        const InputInfos inputInfos = getInputInfos(hash);
        LabelInfos ret = inputInfos.labelInfos;
        if(ret.label[0])
        {
            if(ret.label[0] == '$')
            {
                uint8_t pos = 0;
                while(ret.label[++pos]) if(ret.label[pos] == ' ')
                {
                    uint8_t offset = pos + 1;
                    while(ret.label[++pos]) ret.label[pos - offset] = ret.label[pos];
                    ret.label.resize(pos - offset);
                    return ret;
                }
                pos = 0;
                while(true)
                {
                    switch(ret.label[++pos])
                    {
                        case '_':
                            ret.label[pos - 1] = ' ';
                            break;
                        case 0:
                            ret.label[pos - 1] = 0;
                            ret.label.pop_back();
                            return ret;
                        default:
                            ret.label[pos - 1] = ret.label[pos] | (pos == 1 ? 0 : 32);
                            break;
                    }
                }
            }
            return ret;
        }
        if(inputInfos.binding.empty()) return LabelInfos();
        const Lazynput::SingleBindingInfos &binding = inputInfos.binding[0][0];
        ret.label.clear();
        if(binding.options.invert && !binding.options.half) ret.label.push_back('~');
        switch(binding.type)
        {
            case Lazynput::DeviceInputType::NIL:
                // Should not happen
                break;
            case Lazynput::DeviceInputType::BUTTON:
                ret.label.push_back('B');
                ret.label += std::to_string(binding.index + 1);
                break;
            case Lazynput::DeviceInputType::HAT:
                ret.label.push_back('H');
                ret.label += std::to_string(binding.index / 2 + 1);
                ret.label.push_back(binding.index % 2 ? 'Y' : 'X');
                break;
            case Lazynput::DeviceInputType::ABSOLUTE_AXIS:
                ret.label.push_back('A');
                ret.label += std::to_string(binding.index + 1);
                break;
            case Lazynput::DeviceInputType::RELATIVE_AXIS:
                ret.label.push_back('R');
                ret.label += std::to_string(binding.index + 1);
                break;
        }
        if(binding.options.half) std::cout << (binding.options.invert ? "-" : "+");
        return ret;
    }

    LabelInfos Device::getEnglishAsciiLabelInfos(const char *name) const
    {
        return getEnglishAsciiLabelInfos(StrHash::make(name));
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
    }

    Device::operator bool() const
    {
        return !inputInfos.empty();
    }
}
