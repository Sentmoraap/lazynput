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
    using Interface = StrHashMap<InterfaceInputType>;

    /// \brief (name, interface) hash map to store all interfaces definitions.
    using InterfacesDb = StrHashMap<Interface>;

    /// \brief (name, string) hash map to replace dollar names with unicode strings.
    using IconsDb = StrHashMap<std::string>;

    /// \brief Informations about an input label and color.
    struct DbLabelInfos
    {
        /// True if this input has a distinctive color.
        bool hasColor : 1;

        /// Input's color if provided, dummy value otherwise.
        Color color;

        /// Input's label if provided, can be empty.
        std::string label;
    };

    /// \brief Labels preset.
    struct Labels
    {
        /// Inherited labels.
        StrHash parent;

        /// Own labels.
        StrHashMap<DbLabelInfos> map;
    };

    /// \brief (name, labeld) hash map to store all labels presets.
    using LabelsDb = StrHashMap<Labels>;

    struct ConfigTagPresent;

    /// \brief Bindings for a given config tag. Contains inner ConfigTagBindings for multiple config tags bindings.
    struct ConfigTagBindings
    {
        /// Bindings for nested config tags.
        StrHashMap<ConfigTagPresent> nestedConfigTags;

        /// Bindings for this config tag.
        StrHashMap<FullBindingInfos> bindings;
    };

    /// \brief Different bindings depending on if a given config tag is present or absent.
    struct ConfigTagPresent
    {
        /// Bindings if the config tag is present.
        std::unique_ptr<ConfigTagBindings> present;

        /// Bindings if the config tag is absent.
        std::unique_ptr<ConfigTagBindings> absent;
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
        StrHashMap<DbLabelInfos> ownLabels;

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

        /// Icon definitions.
        IconsDb icons;

        /// String from hash
        StrHashMap<std::string> stringFromHash;

        /// Labels presets.
        LabelsDb labels;

        /// Devices data.
        DevicesDataDb devices;
    };
}
