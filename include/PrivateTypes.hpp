#pragma once

#include <utility>
#include <vector>
#include <cstdint>
#include "Types.hpp"
#include "StrHash.hpp"

/// \file PrivateTypes.hpp
/// \brief Types used internally.

namespace Lazynput
{
    /// \brief (name, type) hash map to store an interface definition.
    using Interface = StrHashMap<InputType>;

    /// \brief (name, interface) hash map to store all interfaces definitions.
    using InterfacesDb = StrHashMap<Interface>;

    /// \brief Internal storage of a label's name and color.
    struct LabelInfosPrivate
    {
        /// True if this input has a distinctive color.
        bool hasColor : 1;

        /// Input's color if provided, dummy value otherwise.
        Color color;

        /// Input's label if provided, can be null.
        std::string label;
    };

    /// \brief Labels preset.
    struct Labels
    {
        /// Inherited labels.
        StrHash parent;

        /// Own labels.
        StrHashMap<LabelInfosPrivate> map;
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
        StrHashMap<LabelInfosPrivate> ownLabels;

        /// Bindings, outer map is config tag, inner map is interface.input.
        StrHashMap<StrHashMap<FullBindingInfos>> bindings;
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
