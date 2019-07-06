#pragma once

#include <cstdint>
#include <iostream>
#include "Types.hpp"
#include "PrivateTypes.hpp"

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
        private:
            /// Device's name.
            std::string name;

            /// Labels and bindings.
            StrHashMap<InputInfos> inputInfos;

            /// \brief Fills labels data.
            ///
            /// Fills labels data from labels data provided by the labels database or a single device's data.
            /// Existing data is overwritten.
            ///
            /// \param labels : labels data.
            void fillLabels(const StrHashMap<LabelInfos> &labels);

            /// \brief Fills labels data from the labels database.
            ///
            /// Fills labels data from a labels preset defined in the labels database. Existing data is overwrittern.
            /// Is called recursively to get data from parent labels presets.
            ///
            /// \param labels : a labels preset.
            /// \param labelsDb : labels database.
            void fillLabels(const Labels &labels, const LabelsDb &labelsDb);

            /// \brief Fills bindings data from the definitions for a single device.
            ///
            /// Fills bindings data from a given device data. It does not use the definitions from it's parents.
            /// Existing data is overwritten.
            ///
            /// \param bindings : a single device's bindings data.
            void fillBindings(const StrHashMap<FullBindingInfos> &bindings);

            /// \brief Fills own data from a given device data.
            ///
            /// Fills labels and bindings data from a given device data. Existing data is overwritten.
            /// Is called recursively to get data from parent devices.
            ///
            /// \param deviceData : data for a specific device.
            /// \param devicesDb : devices database.
            void fillData(const DeviceData &deviceData, const DevicesDb &devicesDb);

            /// \brief Remove input infos containing nil device inputs bindings.
            ///
            /// Called after filling the inputInfos from all the provided data.
            /// Removes the InputInfos, with or without a label defined, which is not bound to any device input.
            /// Those nil bindings may come from label presets and unused inputs from them, or are defined in a parent
            /// device and is overriden in it's child.
            void removeNilBindings();

        public:
            /// \brief Constructor for dummy device.
            Device();

            /// \brief Constructs the Device from a DeviceData and condig tags.
            /// \param deviceData : data for every config tags combinations.
            /// \param devicesDb : devices database.
            /// \param configTags : config tags to use to extract data for this device.
            Device(const DeviceData &deviceData, const DevicesDb &devicesDb, const std::vector<StrHash> &configTags);

            Device(Device &&) = default;

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

            /// \brief Get device's name.
            /// \return The device's name.
            const std::string &getName() const;

            /// \brief Check if the device corresponds to a real database entry or is a dummy one.
            /// \return true if it's a real device, false if it's a dummy one.
            operator bool() const;
    };
}
