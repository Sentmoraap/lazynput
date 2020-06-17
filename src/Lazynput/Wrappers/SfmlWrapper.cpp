#include "Lazynput/Wrappers/SfmlWrapper.hpp"
#include "Lazynput/LazynputDb.hpp"
#include <SFML/Window/Joystick.hpp>

namespace Lazynput
{
    using namespace Litterals;

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

    float SfmlWrapper::getAbsValue(uint8_t device, uint8_t axis) const
    {
        return axis < sf::Joystick::PovX
                ? sf::Joystick::getAxisPosition(device, static_cast<sf::Joystick::Axis>(axis)) * 0.01
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
        return hat == 0 ? std::make_pair(sf::Joystick::getAxisPosition(device, sf::Joystick::PovX),
                sf::Joystick::getAxisPosition(device, sf::Joystick::PovY))
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
                            static_cast<uint16_t>(joystickId.vendorId), static_cast<uint16_t>(joystickId.productId)}));
                    devicesData[i].status = devicesData[i].device ? DeviceStatus::SUPPORTED : DeviceStatus::UNSUPPORTED;
                    if(devicesData[i].device.getName().empty())
                        devicesData[i].device.setName(joystickId.name.toAnsiString().c_str());
                }
            }
            else if(devicesData.size() > i) devicesData[i].status = DeviceStatus::DISCONNECTED;
        }
    }
}
