#pragma once

#include <utility>
#include <vector>
#include <cstdint>
#include <memory>
#include "Lazynput/Types.hpp"
#include "Lazynput/StrHash.hpp"

/// \file PrivateTypes.hpp
/// \brief Types used internally.

namespace Lazynput
{
    /// \brief (name, type) hash map to store an interface definition.
    using Interface = StrHashMap<InputType>;

    /// \brief (name, interface) hash map to store all interfaces definitions.
    using InterfacesDb = StrHashMap<Interface>;

    /// \brief Labels preset.
    struct Labels
    {
        /// Inherited labels.
        StrHash parent;

        /// Own labels.
        StrHashMap<LabelInfos> map;
    };

    /// \brief (name, labeld) hash map to store all labels presets.
    using LabelsDb = StrHashMap<Labels>;

    /// \brief Store one input mapping for one config tag (or none).
    struct InputData
    {
        /// The type of the device's input as seen by the input library.
        InputType inputType;

        /// Index as seen by the input library.
        uint8_t inputIndex;
    };

    /// \brief Bindings for a given config tag. Contains inner ConfigTagBindings for multiple config tags bindings.
    struct ConfigTagBindings
    {
        /// Bindings for nested config tags.
        StrHashMap<std::unique_ptr<ConfigTagBindings>> nestedConfigTags;

        /// Bindings for this config tag.
        StrHashMap<FullBindingInfos> bindings;
    };

    /// \brief Internal struct to store device data
    struct DeviceData
    {
        /// Inherited data.
        HidIds parent;

        /// Device name.
        std::string name;

        /// Implemented interfaces, in increasing hash order
        std::vector<StrHash> interfaces;

        /// Used labels presets. Last overrides first.
        std::vector<StrHash> presetsLabels;

        /// Own labels.
        StrHashMap<LabelInfos> ownLabels;

        /// All bindings for every config tag combinations. Outer struct is for no config tag.
        ConfigTagBindings bindings;
    };

    /// \brief (name, labeld) hash map to store all devices data.
    using DevicesDataDb = std::unordered_map<HidIds, DeviceData, HidIdsIdentity>;

    /// \brief Complete devices database.
    /// This struct contains all the data needed to get a Device with given config tags.
    struct DevicesDb
    {
        /// Interface definitions.
        InterfacesDb interfaces;

        /// Labels presets.
        LabelsDb labels;

        /// Devices data.
        DevicesDataDb devices;
    };
}
