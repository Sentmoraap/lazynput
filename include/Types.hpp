#pragma once

#include <utility>
#include <cstdint>
#include "StrHash.hpp"

/// \file Types.hpp
/// \brief Types definitions for devices database.

namespace Lazynput
{
    /// \brief IDs used in the HID specification.
    struct HidIds
    {
        /// \param pid : device vendor id.
        uint16_t vid;
        /// \param vid : device product id.
        uint16_t pid;
    };

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

    /// \brief Informations about an input label and color.
    struct LabelInfos
    {
        /// True if this input has a distinctive color.
        bool hasColor : 1;

        /// Input's color if provided, dummy value otherwise.
        Color color;

        /// Input's label if provided, can be null.
        const char *label;
    };

    /// \brief Informations about an interface's input on a specific device with specific configTags.
    struct InputInfos
    {
        /// The type of the device's input as seen by the input library.
        InputType inputType;

        /// Index as seen by the input library.
        uint8_t inputIndex;

        /// Label and color.
        LabelInfos labelInfos;

        /// \brief Check if the InputInfos corresponds to a real database entry or is a dummy one.
        /// \return true if it's a real device, false if it's a dummy one.
        operator bool() const;
    };
}

namespace std
{
    /// \brief HidIds hasher for the devices database.
    template<>
    class hash<Lazynput::HidIds>
    {
        public:
            size_t operator()(const Lazynput::HidIds &ids) const
            {
                return std::hash<uint32_t>()(ids.vid << 16 | ids.pid);
            }
    };
}
