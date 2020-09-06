#pragma once

#ifdef LAZYNPUT_USE_SDL_WRAPPER

#include "Lazynput/LibWrapper.hpp"
#include <SDL2/SDL_joystick.h>

namespace Lazynput
{
    /// \class SdlWrapper
    /// \brief Wraps SDL joysticks functions to mappings-aware functions.
    ///
    /// This class uses the SDL_Joystick api to get unmapped inputs. If the device is not present in the Lazynput
    /// database, it uses SDL_Gamecontroller as a fallback database.
    class SdlWrapper final : public LibWrapper
    {
        public:
            static constexpr uint8_t MAX_JOYSTICKS = 16;

        private:
            static constexpr uint8_t MAX_REL_AXES = 16;

            SDL_Joystick *joysticks[MAX_JOYSTICKS] = {0};
            float relAxes[MAX_JOYSTICKS][MAX_REL_AXES];

        public:

            SdlWrapper(const LazynputDb &lazynputDb);
            void update() override;

            uint8_t getNumAbs(uint8_t device) const override;
            float getAbsValue(uint8_t device, uint8_t axis) const override;
            uint8_t getNumBtn(uint8_t device) const override;
            bool getBtnPressed(uint8_t device, uint8_t btn) const override;
            uint8_t getNumHat(uint8_t device) const override;
            std::pair<float, float> getHatValues(uint8_t device, uint8_t hat) const override;
            uint8_t getNumRel(uint8_t device) const override;
            float getRelDelta(uint8_t device, uint8_t rel) const override;
    };
}

#endif // LAZYNPUT_USE_SDL_WRAPPER
