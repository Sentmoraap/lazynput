#pragma once

#ifdef LAZYNPUT_USE_GLFW_WRAPPER

#include "Lazynput/LibWrapper.hpp"
#include <GLFW/glfw3.h>

namespace Lazynput
{
    /// \class GlfwWrapper
    /// \brief Wraps GLFW joystick functions to mappings-aware functions.
    class GlfwWrapper final : public LibWrapper
    {
        private:
            /// If a joystick uses XInput, so the wrapper must change the axes order
            bool joystickUsesXInput[GLFW_JOYSTICK_LAST];

        public:
            GlfwWrapper(const LazynputDb &lazynputDb);
            void update() override;

            uint8_t getNumAbs(uint8_t device) const override;
            float getAbsValue(uint8_t device, uint8_t axis) const override;
            uint8_t getNumBtn(uint8_t device) const override;
            bool getBtnPressed(uint8_t device, uint8_t btn) const override;
            uint8_t getNumHat(uint8_t device) const override;
            std::pair<float, float> getHatValues(uint8_t device, uint8_t hat) const override;
    };
}

#endif // LAZYNPUT_USE_GLFW_WRAPPER
