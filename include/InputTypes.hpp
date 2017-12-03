#pragma once

#include <cstdint>

namespace Lazynput
{
    /// \brief A device's input type.
    enum class InputType : uint8_t
    {
        BUTTON,
        ABSOLUTE_AXIS,
        RELATIVE_AXIS
    };

    /// \brief sRGB color.
    struct Color
    {
        uint8_t r, g, b;
    };

    /// \brief Informations about an interface's input on a specific device with specific configTags.
    struct InputInfos
    {
        /// \brief true if this input has a distinctive color.
        bool hasColor : 1;

        /// \brief The type of the device's input as seen by the input library.
        InputType inputType;

        /// \brief Index as seen by the input library.
        uint8_t inputIndex;

        /// \brief Input's color if provided, dummy value otherwise.
        Color color;

        /// \brief Input's label if provided, can be null.
        const char *label;

        /// \brief Check if the InputInfos corresponds to a real database entry or is a dummy one.
        /// \return true if it's a real device, false if it's a dummy one.
        operator bool() const;
    };
}
