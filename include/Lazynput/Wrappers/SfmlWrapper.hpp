#pragma once

#ifdef LAZYNPUT_USE_SFML_WRAPPER

#include "Lazynput/LibWrapper.hpp"

namespace Lazynput
{
    /// \class SfmlWrapper
    /// \brief Wraps SFML joystick functions to mappings-aware functions.
    class SfmlWrapper final : public LibWrapper
    {
        private:
            uint8_t remapAxis(uint8_t device, uint8_t axis) const;

        public:
            SfmlWrapper(const LazynputDb &lazynputDb);
            void update() override;

            uint8_t getNumAbs(uint8_t device) const override;
            float getAbsValue(uint8_t device, uint8_t axis) const override;
            uint8_t getNumBtn(uint8_t device) const override;
            bool getBtnPressed(uint8_t device, uint8_t btn) const override;
            uint8_t getNumHat(uint8_t device) const override;
            std::pair<float, float> getHatValues(uint8_t device, uint8_t hat) const override;
    };
}

#endif // LAZYNPUT_USE_SFML_WRAPPER
