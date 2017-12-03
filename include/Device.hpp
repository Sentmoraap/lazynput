
#pragma once

#include <cstdint>
#include <iostream>
#include "InputTypes.hpp"

namespace Lazynput
{
    /// \class Device
    /// \brief A class describing a device's input mappings and labels.
    ///
    /// This class contains the data about a device input mappings and labels.
    /// It's mappings corresponds to the specific set of configTags defined by DevicesDb::setGlobalConfigTags and
    /// DevicesDb::getDevice.
    class Device
    {
        public:
            /// \brief Check if the device provides an interface's input.
            /// \param hash : a hashed string of the interface's input name, in the form interfaceName.inputName.
            /// \return true if this input is present, false otherwise.
            bool hasInput(uint32_t hash) const;

            /// \param name : the name of the interface's input name, in the form interfaceName.inputName.
            /// \overload hasInput
            bool hasInput(const char *name) const;

            /// \brief Get the device's InputInfos for a given interface's input.
            ///
            /// Get the InputInfos for a given interface's input for this specific device with specific configTags.
            /// If the device don't have the given interface's input, it returns a dummy one.
            ///
            /// \param hash : a hashed string of the interface's input name, in the form interfaceName.inputName.
            /// \return the corresponding InputInfos if it exists or a dummy one.
            const InputInfos getInputInfos(uint32_t hash) const;

            /// \param name : the name of the interface's input name, in the form interfaceName.inputName.
            /// \overload getInputInfos
            const InputInfos getInputInfos(const char *name) const;

            /// \brief Check if the device corresponds to a real database entry or is a dummy one.
            /// \return true if it's a real device, false if it's a dummy one.
            operator bool() const;
    };
}
