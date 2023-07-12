#ifdef LAZYNPUT_USE_SDL_WRAPPER

#include "Lazynput/Wrappers/SdlWrapper.hpp"
#include "Lazynput/LazynputDb.hpp"
#include <assert.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_hints.h>

namespace Lazynput
{
    using namespace Literals;

    SdlWrapper::SdlWrapper(const LazynputDb &lazynputDb) : LibWrapper(lazynputDb)
    {
        configTags.push_back("sdl"_hash);
        #ifdef _WIN32 // Can be 64-bits
        configTags.push_back("xinput"_hash);
        #endif
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
            uint16_t deviceVersion = guidBytes[13] << 8 | guidBytes[12];
            char versionStr[20];
            snprintf(versionStr, 20, "device_version=%04x", deviceVersion);
            StrHash versionHash = StrHash::make(versionStr);
            configTags.push_back(versionHash);
            devicesData[slot].device = lazynputDb.getDevice(hidIds, configTags.data(), configTags.size());
            configTags.pop_back();
            if(devicesData[slot].device) devicesData[slot].status = DeviceStatus::SUPPORTED;
            else
            {
                // For now SDL doesn't expose it's SDL_ExtendedGameControllerBind structure, so the fallback
                // mappings are more limited than what SDL_GameController is actually capable to do.
                if(SDL_IsGameController(slot))
                {
                    devicesData[slot].device = Device();
                    StrHashMap<InputInfos> inputInfos;
                    devicesData[slot].status = DeviceStatus::FALLBACK;
                    SDL_GameController *controller = SDL_GameControllerOpen(slot);
                    auto setHalfBinding = [](SDL_GameControllerButtonBind bind, SingleBindingInfos &singleBinding)
                    {
                        singleBinding.options.half = false;
                        singleBinding.options.invert = false;
                        switch(bind.bindType)
                        {
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
                            default:
                                assert(false);
                                break;
                        }
                    };
                    auto setBinding = [&setHalfBinding](SDL_GameControllerButtonBind neg, SDL_GameControllerButtonBind pos,
                            InputInfos &inputInfo)
                    {
                        SingleBindingInfos *posBinding = nullptr, *negBinding = nullptr;
                        if(pos.bindType != SDL_CONTROLLER_BINDTYPE_NONE)
                        {
                            inputInfo.bindings.positive.emplace_back();
                            inputInfo.bindings.positive.back().emplace_back();
                            posBinding = &inputInfo.bindings.positive.back().back();
                            setHalfBinding(pos, *posBinding);
                        }
                        if(neg.bindType != SDL_CONTROLLER_BINDTYPE_NONE)
                        {
                            inputInfo.bindings.negative.emplace_back();
                            inputInfo.bindings.negative.back().emplace_back();
                            negBinding = &inputInfo.bindings.negative.back().back();
                            setHalfBinding(neg, *negBinding);
                        }
                        if(posBinding && negBinding && posBinding->type == negBinding->type
                                && posBinding->index == negBinding->index && !posBinding->options.half
                                && !negBinding->options.half)
                        {
                            posBinding->options.half = true;
                            negBinding->options.half = true;
                            negBinding->options.invert = !negBinding->options.invert;
                        }
                    };
                    auto bindButton = [&inputInfos, &setBinding, controller](SDL_GameControllerButton button,
                            StrHash hash)
                    {
                        SDL_GameControllerButtonBind neg;
                        neg.bindType = SDL_CONTROLLER_BINDTYPE_NONE;
                        setBinding(neg, SDL_GameControllerGetBindForButton(controller, button), inputInfos[hash]);
                    };
                    auto bindAxis = [&inputInfos, &setBinding, controller](SDL_GameControllerAxis axis, StrHash hash)
                    {
                        SDL_GameControllerButtonBind bind = SDL_GameControllerGetBindForAxis(controller, axis);
                        setBinding(bind, bind, inputInfos[hash]);
                    };
                    auto bindHat = [&inputInfos, &setBinding, controller](SDL_GameControllerButton neg,
                            SDL_GameControllerButton pos, StrHash hash)
                    {
                        setBinding(SDL_GameControllerGetBindForButton(controller, neg),
                                SDL_GameControllerGetBindForButton(controller, pos), inputInfos[hash]);
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
                    devicesData[slot].device.setInputInfos(std::move(inputInfos));
                }
                else
                {
                    // Provide default mappings so the wrapper can be used the same way for unsupported devices.
                    generateDefaultMappings(slot);
                }
            }
            if(devicesData[slot].device.getName().empty())
                devicesData[slot].device.setName(SDL_JoystickName(js));

            memset(relAxes[slot], 0, sizeof(float) * MAX_REL_AXES);
        }
    }

    SDL_Joystick *SdlWrapper::getJoystickFromSlot(uint8_t device) const
    {
        return joysticks[device];
    }
}

#endif // LAZYNPUT_USE_SDL_WRAPPER
