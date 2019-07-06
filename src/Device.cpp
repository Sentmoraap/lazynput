#include "Device.hpp"
#include "PrivateTypes.hpp"

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

    void Device::fillData(const DeviceData &deviceData, const DevicesDb &devicesDb)
    {
        if(deviceData.parent != HidIds::invalid) fillData(devicesDb.devices.at(deviceData.parent), devicesDb);
        for(StrHash preset : deviceData.presetsLabels) fillLabels(devicesDb.labels.at(preset), devicesDb.labels);
        fillLabels(deviceData.ownLabels);
        // Use only the default bindings.
        // TODO: config tags support.
        fillBindings(deviceData.bindings.bindings);
    }

    void Device::removeNilBindings()
    {
        for(auto it = inputInfos.begin(); it != inputInfos.end();)
            if(it->second.binding.empty()) it = inputInfos.erase(it); else ++it;
    }

    Device::Device(const DeviceData &deviceData, const DevicesDb &devicesDb, const std::vector<StrHash> &configTags)
    {
        name = deviceData.name;
        fillData(deviceData, devicesDb);
        removeNilBindings();
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
