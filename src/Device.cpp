#include "Device.hpp"
#include "PrivateTypes.hpp"
#include <algorithm>

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
        name = deviceData.name;
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

    const std::string &Device::getName() const
    {
        return name;
    }

    Device::operator bool() const
    {
        return !inputInfos.empty();
    }
}
