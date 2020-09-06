#ifdef LAZYNPUT_USE_SDL_WRAPPER

#include "Lazynput/Wrappers/SdlWrapper.hpp"
#include "Lazynput/LazynputDb.hpp"
#include <SDL2/SDL_events.h>

namespace Lazynput
{
    using namespace Litterals;

    SdlWrapper::SdlWrapper(const LazynputDb &lazynputDb) : LibWrapper(lazynputDb)
    {
        configTags.push_back("sdl"_hash);
        devicesData.resize(MAX_JOYSTICKS);
    }

    uint8_t SdlWrapper::getNumAbs(uint8_t device) const
    {
        return SDL_JoystickNumAxes(joysticks[device]);
    }

    float SdlWrapper::getAbsValue(uint8_t device, uint8_t axis) const
    {
        int16_t val = SDL_JoystickGetAxis(joysticks[device], axis);
        if(val == -32768) return -1.f;
        return val / 32767.f;
    }

    uint8_t SdlWrapper::getNumBtn(uint8_t device) const
    {
        return SDL_JoystickNumButtons(joysticks[device]);
    }

    bool SdlWrapper::getBtnPressed(uint8_t device, uint8_t btn) const
    {
        return SDL_JoystickGetButton(joysticks[device], btn);
    }

    uint8_t SdlWrapper::getNumHat(uint8_t device) const
    {
        return SDL_JoystickNumHats(joysticks[device]);
    }

    std::pair<float, float> SdlWrapper::getHatValues(uint8_t device, uint8_t hat) const
    {
        Uint8 hatState = SDL_JoystickGetHat(joysticks[device], hat);
        float x = 0, y = 0;
        if(hatState & SDL_HAT_LEFT) x = -1;
        else if(hatState & SDL_HAT_RIGHT) x = 1;
        if(hatState & SDL_HAT_UP) y = -1;
        else if(hatState & SDL_HAT_DOWN) y = 1;
        return std::make_pair(x, y);
    }

    uint8_t SdlWrapper::getNumRel(uint8_t device) const
    {
        return SDL_JoystickNumBalls(joysticks[device]) * 2;
    }

    float SdlWrapper::getRelDelta(uint8_t device, uint8_t rel) const
    {
        return relAxes[device][rel];
    }

    void SdlWrapper::update()
    {
        SDL_PumpEvents();
        uint8_t nbConnectedJoysticks = 0;
        for(uint8_t js = 0; js < MAX_JOYSTICKS; js++)
        {
            if(devicesData[js].status != DeviceStatus::DISCONNECTED)
            {
                if(SDL_JoystickGetAttached(joysticks[js]) == SDL_TRUE)
                {
                    nbConnectedJoysticks++;
                    uint8_t nbBalls = SDL_JoystickNumBalls(joysticks[js]);
                    for(uint8_t i = 0; i < nbBalls; i++)
                    {
                        int x, y;
                        SDL_JoystickGetBall(joysticks[js], i, &x, &y);
                        relAxes[js][i * 2] = x;
                        relAxes[js][i * 2 + 1] = y;
                    }
                }
                else
                {
                    devicesData[js].status = DeviceStatus::DISCONNECTED;
                    SDL_JoystickClose(joysticks[js]);
                    joysticks[js] = nullptr;
                }
            }
        }
        uint8_t nbJoysticks = SDL_NumJoysticks();
        uint8_t slot = 0;
        if(nbConnectedJoysticks < nbJoysticks && nbConnectedJoysticks < MAX_JOYSTICKS)
                for(uint8_t testJs = 0; testJs < nbJoysticks; testJs++)
        {
            SDL_Joystick *js = SDL_JoystickOpen(testJs);
            if(!js) continue;
            uint8_t comp = 0;
            for(comp = 0; comp < MAX_JOYSTICKS; comp++) if(js == joysticks[comp]) break;
            if(comp != MAX_JOYSTICKS) continue;
            while(joysticks[slot]) slot++;
            joysticks[slot] = js;
            SDL_JoystickGUID guid = SDL_JoystickGetGUID(js);
            uint8_t *guidBytes = reinterpret_cast<uint8_t*>(&guid);
            Lazynput::HidIds hidIds;
            hidIds.vid = guidBytes[5] << 8 | guidBytes[4];
            hidIds.pid = guidBytes[9] << 8 | guidBytes[8];
            uint16_t driverVersion = guidBytes[13] << 8 | guidBytes[12];
            char driverStr[20];
            snprintf(driverStr, 20, "driver_version=%04x", driverVersion);
            StrHash driverHash = StrHash::make(driverStr);
            devicesData[slot].device = std::move(lazynputDb.getDevice(hidIds, &driverHash, 1));
            if(devicesData[slot].device) devicesData[slot].status = DeviceStatus::SUPPORTED;
            else
            {
                devicesData[slot].device = Device();
                StrHashMap<InputInfos> inputInfos;
                // For now SDL doesn't expose it's SDL_ExtendedGameControllerBind structure, so the fallback
                // mappings are more limited than what SDL_GameController is actually capable to do.
                if(SDL_IsGameController(slot))
                {
                    devicesData[slot].status = DeviceStatus::FALLBACK;
                    SDL_GameController *controller = SDL_GameControllerOpen(slot);
                    auto setBinding = [](SDL_GameControllerButtonBind bind, SingleBindingInfos &singleBinding)
                    {
                        singleBinding.options.half = false;
                        singleBinding.options.invert = false;
                        switch(bind.bindType)
                        {
                            case SDL_CONTROLLER_BINDTYPE_NONE:
                                // To supress the warning, should not happen.
                                break;
                            case SDL_CONTROLLER_BINDTYPE_BUTTON:
                                singleBinding.type = DeviceInputType::BUTTON;
                                singleBinding.index = bind.value.button;
                                break;
                            case SDL_CONTROLLER_BINDTYPE_AXIS:
                                singleBinding.type = DeviceInputType::ABSOLUTE_AXIS;
                                singleBinding.index = bind.value.axis;
                                break;
                            case SDL_CONTROLLER_BINDTYPE_HAT:
                                singleBinding.type = DeviceInputType::HAT;
                                singleBinding.index = bind.value.hat.hat * 2;
                                singleBinding.options.half = true;
                                singleBinding.options.invert =
                                        bind.value.hat.hat_mask & (SDL_HAT_UP | SDL_HAT_LEFT);
                                if(bind.value.hat.hat_mask & (SDL_HAT_UP | SDL_HAT_DOWN)) singleBinding.index++;
                                break;
                        }
                    };
                    auto bindControl = [&inputInfos, &setBinding](SDL_GameControllerButtonBind bind,
                            StrHash hash)
                    {
                        if(bind.bindType != SDL_CONTROLLER_BINDTYPE_NONE)
                        {
                            InputInfos &inputInfo = inputInfos[hash];
                            inputInfo.binding.emplace_back();
                            inputInfo.binding.back().emplace_back();
                            setBinding(bind, inputInfo.binding.back().back());
                        }
                    };
                    auto bindButton = [&bindControl, controller](SDL_GameControllerButton button, StrHash hash)
                    {
                        bindControl(SDL_GameControllerGetBindForButton(controller, button), hash);
                    };
                    auto bindAxis = [&bindControl, controller](SDL_GameControllerAxis axis, StrHash hash)
                    {
                        bindControl(SDL_GameControllerGetBindForAxis(controller, axis), hash);
                    };
                    auto bindHat = [&inputInfos, &setBinding, controller](SDL_GameControllerButton neg,
                            SDL_GameControllerButton pos, StrHash hash)
                    {
                        SingleBindingInfos negBinding, posBinding;
                        setBinding(SDL_GameControllerGetBindForButton(controller, neg), negBinding);
                        setBinding(SDL_GameControllerGetBindForButton(controller, pos), posBinding);
                        if(negBinding.type == posBinding.type && negBinding.index == posBinding.index
                                // Uncomment thesetwo lines when we can get SDL_ExtendedGameControllerBind
                                // infos.
                                //&& negBinding.options.invert == !posBinding.options.invert
                                //&& negBinding.options.half == true && posBinding.options.half == true
                                && (posBinding.type == DeviceInputType::HAT
                                || posBinding.type == DeviceInputType::ABSOLUTE_AXIS))
                        {
                            posBinding.options.half = false;
                            InputInfos &inputInfo = inputInfos[hash];
                            inputInfo.binding.emplace_back();
                            inputInfo.binding.back();
                            inputInfo.binding.back().push_back(posBinding);
                        }
                    };
                    bindButton(SDL_CONTROLLER_BUTTON_A, "basic_gamepad.a"_hash);
                    bindButton(SDL_CONTROLLER_BUTTON_B, "basic_gamepad.b"_hash);
                    bindButton(SDL_CONTROLLER_BUTTON_X, "basic_gamepad.x"_hash);
                    bindButton(SDL_CONTROLLER_BUTTON_Y, "basic_gamepad.y"_hash);
                    bindButton(SDL_CONTROLLER_BUTTON_BACK, "basic_gamepad.select"_hash);
                    bindButton(SDL_CONTROLLER_BUTTON_GUIDE, "extended_gamepad.home"_hash);
                    bindButton(SDL_CONTROLLER_BUTTON_START, "basic_gamepad.start"_hash);
                    bindButton(SDL_CONTROLLER_BUTTON_LEFTSTICK, "basic_gamepad.ls"_hash);
                    bindButton(SDL_CONTROLLER_BUTTON_RIGHTSTICK, "basic_gamepad.rs"_hash);
                    bindButton(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, "basic_gamepad.l1"_hash);
                    bindButton(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, "basic_gamepad.r1"_hash);
                    bindAxis(SDL_CONTROLLER_AXIS_LEFTX, "basic_gamepad.lsx"_hash);
                    bindAxis(SDL_CONTROLLER_AXIS_LEFTY, "basic_gamepad.lsy"_hash);
                    bindAxis(SDL_CONTROLLER_AXIS_RIGHTX, "basic_gamepad.rsx"_hash);
                    bindAxis(SDL_CONTROLLER_AXIS_RIGHTY, "basic_gamepad.rsy"_hash);
                    bindAxis(SDL_CONTROLLER_AXIS_TRIGGERLEFT, "basic_gamepad.l2"_hash);
                    bindAxis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, "basic_gamepad.r2"_hash);
                    bindHat(SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
                            "basic_gamepad.dpx"_hash);
                    bindHat(SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
                            "basic_gamepad.dpy"_hash);

                    SDL_GameControllerClose(controller);
                }
                else
                {
                    // Provide default mappings so the wrapper can be used the same way for unsupported devices.
                    devicesData[slot].status = DeviceStatus::UNSUPPORTED;
                    auto bindInput = [&inputInfos](uint8_t input, StrHash hash, DeviceInputType type)
                    {
                        InputInfos &inputInfo = inputInfos[hash];
                        inputInfo.binding.emplace_back();
                        inputInfo.binding.back().emplace_back();
                        SingleBindingInfos &singleBinding = inputInfo.binding.back().back();
                        singleBinding.options.half = false;
                        singleBinding.options.invert = false;
                        singleBinding.type = type;
                        singleBinding.index = input;
                    };
                    auto bindButton = [&bindInput](uint8_t button, StrHash hash)
                    {
                        bindInput(button, hash, DeviceInputType::BUTTON);
                    };
                    auto bindAxis = [&bindInput](uint8_t axis, StrHash hash)
                    {
                        bindInput(axis, hash, DeviceInputType::ABSOLUTE_AXIS);
                    };
                    switch(SDL_JoystickNumButtons(js))
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
                    switch(SDL_JoystickNumAxes(js))
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
                    if(SDL_JoystickNumHats(js))
                    {
                        bindInput(0, "basic_gamepad.dpx"_hash, DeviceInputType::HAT);
                        bindInput(1, "basic_gamepad.dpy"_hash, DeviceInputType::HAT);
                    }
                }
                devicesData[slot].device.setInputInfos(std::move(inputInfos));
            }
            if(devicesData[slot].device.getName().empty())
                devicesData[slot].device.setName(SDL_JoystickName(js));

            memset(relAxes[slot], 0, sizeof(float) * MAX_REL_AXES);
        }
    }
}

#endif // LAZYNPUT_USE_SDL_WRAPPER
