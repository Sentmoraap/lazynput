#include "Lazynput/LibWrapper.hpp"
#include "Lazynput/LazynputDb.hpp"
#include <assert.h>
#include <algorithm>

namespace Lazynput
{
    using namespace Literals;

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

    float LibWrapper::getHalfInputValue(uint8_t device, const HalfBindingInfos &bindings) const
    {
        float value = 0.f;
        for(const auto &andBinding : bindings)
        {
            float andValue = 1.f;
            for(const SingleBindingInfos &singleBinding : andBinding)
            {
                float singleValue;
                switch(singleBinding.type)
                {
                    case Lazynput::DeviceInputType::NIL:
                        assert(false);
                        break;
                    case Lazynput::DeviceInputType::BUTTON:
                        singleValue = getBtnPressed(device, singleBinding.index) ? 1.f : -1;
                        break;
                    case Lazynput::DeviceInputType::HAT:
                    {
                        std::pair<float, float> values = getHatValues(device, singleBinding.index / 2);
                        singleValue = singleBinding.index % 2 ? values.second : values.first;
                        if(singleBinding.options.half) singleValue = (singleValue + 1.f) * 0.5f;
                        break;
                    }
                    case Lazynput::DeviceInputType::ABSOLUTE_AXIS:
                        singleValue = getAbsValue(device, singleBinding.index);
                        if(singleBinding.options.half) singleValue = (singleValue + 1.f) * 0.5f;
                        break;
                    case Lazynput::DeviceInputType::RELATIVE_AXIS:
                        singleValue = getRelDelta(device, singleBinding.index);
                        break;
                }
                if(singleBinding.options.invert)
                {
                    if(singleBinding.options.half) singleValue = 1.f - singleValue;
                    else singleValue = -singleValue;
                }
                singleValue = std::max(0.f, singleValue);
                andValue = std::min(andValue, singleValue);
            }
            value = std::max(value, andValue);
        }
        return value;
    }

    float LibWrapper::getInputValue(uint8_t device, StrHash hash) const
    {
        if(getDeviceStatus(device) == DeviceStatus::DISCONNECTED) return 0.f;
        const FullBindingInfos &bindings = getDevice(device).getInputInfos(hash).bindings;
        if(bindings.positive.empty() && bindings.negative.empty()) return 0.f;
        return getHalfInputValue(device, bindings.positive) - getHalfInputValue(device, bindings.negative);
    }

    float LibWrapper::getInputValue(uint8_t device, const char *name) const
    {
        return getInputValue(device, StrHash::make(name));
    }

    void LibWrapper::generateDefaultMappings(uint8_t device)
    {
        devicesData[device].status = DeviceStatus::UNSUPPORTED;
        devicesData[device].device = Device();
        StrHashMap<InputInfos> inputInfos;
        auto bindInput = [&inputInfos](uint8_t input, StrHash hash, DeviceInputType type)
        {
            InputInfos &inputInfo = inputInfos[hash];
            inputInfo.bindings.positive.emplace_back();
            inputInfo.bindings.positive.back().emplace_back();
            SingleBindingInfos &positiveBinding = inputInfo.bindings.positive.back().back();
            positiveBinding.options.invert = false;
            positiveBinding.type = type;
            positiveBinding.index = input;
            if(type == DeviceInputType::ABSOLUTE_AXIS || type == DeviceInputType::HAT)
            {
                positiveBinding.options.half = true;
                inputInfo.bindings.negative.emplace_back();
                inputInfo.bindings.negative.back().emplace_back();
                SingleBindingInfos &negativeBinding = inputInfo.bindings.negative.back().back();
                negativeBinding = positiveBinding;
                negativeBinding.options.invert = true;
            }
            else positiveBinding.options.half = false;
        };
        auto bindButton = [&bindInput](uint8_t button, StrHash hash)
        {
            bindInput(button, hash, DeviceInputType::BUTTON);
        };
        auto bindAxis = [&bindInput](uint8_t axis, StrHash hash)
        {
            bindInput(axis, hash, DeviceInputType::ABSOLUTE_AXIS);
        };
        switch(getNumBtn(device))
        {
            default:
            case 32: bindButton(31, "extra.btn11"_hash);
            case 31: bindButton(30, "extra.btn10"_hash);
            case 30: bindButton(29, "extra.btn9"_hash);
            case 29: bindButton(28, "extra.btn8"_hash);
            case 28: bindButton(27, "extra.btn7"_hash);
            case 27: bindButton(26, "extra.btn6"_hash);
            case 26: bindButton(25, "extra.btn5"_hash);
            case 25: bindButton(24, "extra.btn4"_hash);
            case 24: bindButton(23, "extra.btn3"_hash);
            case 23: bindButton(22, "extra.btn2"_hash);
            case 22: bindButton(21, "extra.btn1"_hash);
            case 21: bindButton(20, "extra.btn0"_hash);
            case 20: bindButton(19, "extended_gamepad.capture"_hash);
            case 19: bindButton(18, "extended_gamepad.home"_hash);
            case 18: bindButton(17, "extended_gamepad.r4"_hash);
            case 17: bindButton(16, "extended_gamepad.l4"_hash);
            case 16: bindButton(15, "extended_gamepad.r3"_hash);
            case 15: bindButton(14, "extended_gamepad.l3"_hash);
            case 14: bindButton(13, "extended_gamepad.z"_hash);
            case 13: bindButton(12, "extended_gamepad.c"_hash);
            case 12: bindButton(11, "basic_gamepad.rs"_hash);
            case 11: bindButton(10, "basic_gamepad.ls"_hash);
            case 10: bindButton( 9, "basic_gamepad.start"_hash);
            case  9: bindButton( 8, "basic_gamepad.select"_hash);
            case  8: bindButton( 7, "basic_gamepad.r2"_hash);
            case  7: bindButton( 6, "basic_gamepad.l2"_hash);
            case  6: bindButton( 5, "basic_gamepad.r1"_hash);
            case  5: bindButton( 4, "basic_gamepad.l1"_hash);
            case  4: bindButton( 3, "basic_gamepad.y"_hash);
            case  3: bindButton( 2, "basic_gamepad.x"_hash);
            case  2: bindButton( 1, "basic_gamepad.b"_hash);
            case  1: bindButton( 0, "basic_gamepad.a"_hash);
            case  0:;
        }
        switch(getNumAbs(device))
        {
            default:
            case  8: bindAxis( 7, "extra.abs3"_hash);
            case  7: bindAxis( 6, "extra.abs2"_hash);
            case  6: bindAxis( 5, "extra.abs1"_hash);
            case  5: bindAxis( 4, "extra.abs0"_hash);
            case  4: bindAxis( 3, "basic_gamepad.rsy"_hash);
            case  3: bindAxis( 2, "basic_gamepad.rsx"_hash);
            case  2: bindAxis( 1, "basic_gamepad.lsy"_hash);
            case  1: bindAxis( 0, "basic_gamepad.lsx"_hash);
            case  0:;
        }
        if(getNumHat(device))
        {
            bindInput(0, "basic_gamepad.dpx"_hash, DeviceInputType::HAT);
            bindInput(1, "basic_gamepad.dpy"_hash, DeviceInputType::HAT);
        }
        devicesData[device].device.setInputInfos(std::move(inputInfos));
    }
}
