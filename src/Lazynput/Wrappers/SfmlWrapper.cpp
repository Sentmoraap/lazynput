#ifdef LAZYNPUT_USE_SFML_WRAPPER

#include "Lazynput/Wrappers/SfmlWrapper.hpp"
#include "Lazynput/LazynputDb.hpp"
#include <SFML/Window/Joystick.hpp>

namespace Lazynput
{
    using namespace Literals;

    SfmlWrapper::SfmlWrapper(const LazynputDb &lazynputDb) : LibWrapper(lazynputDb)
    {
        configTags.push_back("sfml"_hash);
    }

    uint8_t SfmlWrapper::getNumAbs(uint8_t device) const
    {
        uint8_t num = 0;
        while(num < sf::Joystick::PovX && sf::Joystick::hasAxis(device, static_cast<sf::Joystick::Axis>(num))) num++;
        return num;
    }

    uint8_t SfmlWrapper::remapAxis(uint8_t device, uint8_t axis) const
    {
        // Remove holes
        uint8_t axisCount = 0;
        for(uint8_t i = 0; i < sf::Joystick::PovX; i++)
        {
            uint8_t reorderedAxis = i;
            // SFML maps RX RY and RZ in an unexpected order
            if(i == 3) reorderedAxis = 4;
            else if(i == 4) reorderedAxis = 5;
            else if(i == 5) reorderedAxis = 3;
            if(sf::Joystick::hasAxis(device, static_cast<sf::Joystick::Axis>(reorderedAxis)))
            {
                if(axisCount == axis) return reorderedAxis;
                axisCount++;
            }
        }

        // Axis not found
        return 255;
    }

    float SfmlWrapper::getAbsValue(uint8_t device, uint8_t axis) const
    {
        uint8_t sfmlAxis = remapAxis(device, axis);
        return sfmlAxis < 255
                ? sf::Joystick::getAxisPosition(device, static_cast<sf::Joystick::Axis>(sfmlAxis)) * 0.01
                : 0.f;
    }

    uint8_t SfmlWrapper::getNumBtn(uint8_t device) const
    {
        return sf::Joystick::getButtonCount(device);
    }

    bool SfmlWrapper::getBtnPressed(uint8_t device, uint8_t btn) const
    {
        return sf::Joystick::isButtonPressed(device, btn);
    }

    uint8_t SfmlWrapper::getNumHat(uint8_t device) const
    {
        return sf::Joystick::hasAxis(device, sf::Joystick::PovX) ? 1 : 0;
    }

    std::pair<float, float> SfmlWrapper::getHatValues(uint8_t device, uint8_t hat) const
    {
        return hat == 0 ? std::make_pair(sf::Joystick::getAxisPosition(device, sf::Joystick::PovX) * 0.01f,
#ifdef _WIN32
                -sf::Joystick::getAxisPosition(device, sf::Joystick::PovY) * 0.01f)
#else
                sf::Joystick::getAxisPosition(device, sf::Joystick::PovY) * 0.01f)
#endif
                : std::make_pair(0.f, 0.f);
    }

    void SfmlWrapper::update()
    {
        sf::Joystick::update();
        for(uint8_t i = 0; i < sf::Joystick::Count; i++)
        {
            if(sf::Joystick::isConnected(i))
            {
                if(devicesData.size() <= i) devicesData.resize(i + 1);
                if(devicesData[i].status == DeviceStatus::DISCONNECTED)
                {
                    sf::Joystick::Identification joystickId = sf::Joystick::getIdentification(i);
                    devicesData[i].device = std::move(lazynputDb.getDevice(Lazynput::HidIds{
                            static_cast<uint16_t>(joystickId.vendorId), static_cast<uint16_t>(joystickId.productId)},
                            configTags.data(), configTags.size()));
                    if(devicesData[i].device) devicesData[i].status = DeviceStatus::SUPPORTED;
                    else generateDefaultMappings(i);
                    if(devicesData[i].device.getName().empty())
                        devicesData[i].device.setName(joystickId.name.toAnsiString().c_str());
                }
            }
            else if(devicesData.size() > i) devicesData[i].status = DeviceStatus::DISCONNECTED;
        }
    }
}

#endif // LAZYNPUT_USE_SFML_WRAPPER
