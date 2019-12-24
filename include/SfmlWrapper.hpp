#pragma once

#include "LibWrapper.hpp"

namespace Lazynput
{
    /// \class SfmlWrapper
    /// \brief Wraps SFML joystick functions to mappings-aware functions.
    class SfmlWrapper final : public LibWrapper
    {
        private:
            float getAbsValue(uint8_t device, uint8_t axis) const override;
            bool getBtnPressed(uint8_t device, uint8_t btn) const override;
            std::pair<float, float> getHatValues(uint8_t device, uint8_t hat) const override;

        public:
            SfmlWrapper(const LazynputDb &lazynputDb);
            void update() override;
    };
}
