#ifdef LAZYNPUT_USE_GLFW_WRAPPER

#include "Lazynput/Wrappers/GlfwWrapper.hpp"
#include "Lazynput/LazynputDb.hpp"

namespace Lazynput
{
    using namespace Literals;
    
    GlfwWrapper::GlfwWrapper(const LazynputDb &lazynputDb) : LibWrapper(lazynputDb)
    {
        configTags.push_back("glfw"_hash);
        #ifdef _WIN32 // Can be 64-bits
        configTags.push_back("xinput"_hash);
        #endif
        devicesData.resize(GLFW_JOYSTICK_LAST);
    }

    void GlfwWrapper::update()
    {
        glfwPollEvents();
        for(uint8_t i = GLFW_JOYSTICK_1; i < GLFW_JOYSTICK_LAST; i++)
        {
            if(glfwJoystickPresent(i))
            {
                if(devicesData[i].status == DeviceStatus::DISCONNECTED)
                {
                    auto a2i = [](const char *c)
                    {
                        auto c2i = [](char c)
                        {
                            if(c >= 'a') return c - 'a' + 10;
                            if(c >= 'A') return c - 'A' + 10;
                            return c - '0';
                        };
                        return c2i(c[1]) | c2i(c[0]) << 4 | c2i(c[3]) << 8 | c2i(c[2]) << 12;
                    };
                    const char *guid = glfwGetJoystickGUID(i);
                    std::cout << guid << std::endl;
                    Lazynput::HidIds hidIds;
                    // Method based on an incorrect behaviour.
                    // It needs a better way to distinguish an XInput controller.
                    joystickUsesXInput[i] = guid[12] != '0' || guid[13] != '0';
                    if(joystickUsesXInput[i]) // Wrong GUID on windows. Temporary fix until it's fixed in GLFW. 
                    {
                        // An XInput controller, but doesn't know which one.
                        // Pretend it's an Xbox 360 controller
                        hidIds.vid = 0x045e;
                        hidIds.pid = 0x028e;
                    }
                    else
                    {
                        hidIds.vid = a2i(guid + 8);
                        hidIds.pid = a2i(guid + 16);
                        StrHash driverHash = "device_version="_hash;
                        driverHash.hashCharacter(guid[26]);
                        driverHash.hashCharacter(guid[27]);
                        driverHash.hashCharacter(guid[24]);
                        driverHash.hashCharacter(guid[25]);
                        configTags.push_back(driverHash);
                    }
                    devicesData[i].device = lazynputDb.getDevice(hidIds, configTags.data(), configTags.size());
                    if(!joystickUsesXInput[i]) configTags.pop_back();
                    if(devicesData[i].device) devicesData[i].status = DeviceStatus::SUPPORTED;
                    else
                    {
                        devicesData[i].status = DeviceStatus::UNSUPPORTED;
                        generateDefaultMappings(i);
                        if(devicesData[i].device.getName().empty())
                            devicesData[i].device.setName(glfwGetJoystickName(i));
                    }
                }
                /*else
                {
                    int count;
                    const float *axes = glfwGetJoystickAxes(0, &count);
                    for (int i = 0; i < count; i++) std::cout << "a" << i << "=" << axes[i] << " ";
                    std::cout << std::endl;
                }*/
            }
            else devicesData[i].status = DeviceStatus::DISCONNECTED;
        }
    }

    uint8_t GlfwWrapper::getNumAbs(uint8_t device) const
    {
        int count;
        glfwGetJoystickAxes(device, &count);
        return static_cast<uint8_t>(count);
    }

    float GlfwWrapper::getAbsValue(uint8_t device, uint8_t axis) const
    {
        int count;
        const float *axes = glfwGetJoystickAxes(device, &count);
        if (joystickUsesXInput[device])
        {
            // Reorder the axis of XInput controllers to be consistent with other libraries
            switch (axis)
            {
                case 2: axis = 4; break;
                case 3: axis = 2; break;
                case 4: axis = 3; break;
                default: break;
            }
        }
        return count > axis ? axes[axis] : 0.f;
    }

    uint8_t GlfwWrapper::getNumBtn(uint8_t device) const
    {
        int count;
        glfwGetJoystickButtons(device, &count);
        return static_cast<uint8_t>(count);
    }

    bool GlfwWrapper::getBtnPressed(uint8_t device, uint8_t btn) const
    {
        int count;
        const uint8_t *buttons = glfwGetJoystickButtons(device, &count);
        return count > btn && buttons[btn] == GLFW_PRESS;
    }

    uint8_t GlfwWrapper::getNumHat(uint8_t device) const
    {
        int count;
        glfwGetJoystickHats(device, &count);
        return count;
    }

    std::pair<float, float> GlfwWrapper::getHatValues(uint8_t device, uint8_t hat) const
    {
        int count;
        const uint8_t *hats = glfwGetJoystickHats(device, &count);
        if(hat >= count) return std::make_pair(0.f, 0.f);
        uint8_t hatState = hats[hat];
        float x = 0.f, y = 0.f;
        if(hatState & GLFW_HAT_LEFT) x = -1.f;
        else if(hatState & GLFW_HAT_RIGHT) x = 1.f;
        if(hatState & GLFW_HAT_UP) y = -1.f;
        else if(hatState & GLFW_HAT_DOWN) y = 1.f;
        return std::make_pair(x, y);
    }

}
#endif // LAZYNPUT_USE_GLFW_WRAPPER

