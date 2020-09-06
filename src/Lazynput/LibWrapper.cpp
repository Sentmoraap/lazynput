#include "Lazynput/LibWrapper.hpp"

namespace Lazynput
{
    using namespace Litterals;

    LibWrapper::LibWrapper(const LazynputDb &lazynputDb) : lazynputDb(lazynputDb)
    {
        #ifdef __ANDROID__
            configTags.push_back("android"_hash);
        #endif
        #ifdef __APPLE__
            configTags.push_back("apple"_hash);
        #endif
        #ifdef __linux__ // Linux kernel, can be GNU/Linux or Android
            configTags.push_back("linux"_hash);
        #endif
        #ifdef TARGET_OS_MAC
            configTags.push_back("macos"_hash);
        #endif
        #ifdef TARGET_OS_IPHONE
            configTags.push_back("ios"_hash);
        #endif
        #ifdef _WIN32 // Can be 64-bits
            configTags.push_back("windows"_hash);
        #endif
    }

    LibWrapper::DeviceStatus LibWrapper::getDeviceStatus(uint8_t index) const
    {
        return devicesData.size() > index ? devicesData[index].status : DeviceStatus::DISCONNECTED;
    }

    const Device& LibWrapper::getDevice(uint8_t index) const
    {
        return devicesData[index].device;
    }

    float LibWrapper::getInputValue(uint8_t device, StrHash hash) const
    {
        if(getDeviceStatus(device) == DeviceStatus::DISCONNECTED) return 0.f;
        float value = -1.f;
        const FullBindingInfos &binding = getDevice(device).getInputInfos(hash).binding;
        if(binding.empty()) return 0.f;
        for(const auto &andBinding : binding)
        {
            float andValue = 1.f;
            for(const SingleBindingInfos &singleBinding : andBinding)
            {
                float singleValue;
                switch(singleBinding.type)
                {
                    case Lazynput::DeviceInputType::NIL:
                        // Should not happen
                        singleValue = 0;
                        break;
                    case Lazynput::DeviceInputType::BUTTON:
                        singleValue = getBtnPressed(device, singleBinding.index) ? 1.f : 0.f;
                        break;
                    case Lazynput::DeviceInputType::HAT:
                    {
                        std::pair<float, float> values = getHatValues(device, singleBinding.index / 2);
                        singleValue = singleBinding.index % 2 ? values.second : values.first;
                        break;
                    }
                    case Lazynput::DeviceInputType::ABSOLUTE_AXIS:
                        singleValue = getAbsValue(device, singleBinding.index);
                        break;
                    case Lazynput::DeviceInputType::RELATIVE_AXIS:
                        singleValue = getRelDelta(device, singleBinding.index);
                        break;
                }
                if(singleBinding.options.invert) singleValue = -singleValue;
                if(singleBinding.options.half)
                {
                    singleValue = 2 * singleValue - 1;
                    if(singleValue < 0) singleValue = 0;
                }
                andValue = std::min(andValue, singleValue);
            }
            value = std::max(value, andValue);
        }
        return value;
    }

    float LibWrapper::getInputValue(uint8_t device, const char *name) const
    {
        return getInputValue(device, StrHash::make(name));
    }
}
