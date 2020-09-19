#pragma once

#include <cstdint>
#include <iostream>
#include "Lazynput/Types.hpp"
#include "Lazynput/PrivateTypes.hpp"

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
            /// Fills bindings data from a given device data, for given config tags.
            /// Existing data is overwritten.
            ///
            /// \param bindings : a single device's bindings data.
            void fillBindings(const StrHashMap<FullBindingInfos> &bindings);

            /// \brief Fills bindings data from the definitions for a single device.
            ///
            /// Fills bindings data from a given device data, It does not use the definitions from it's parents.
            /// Is called recursively to get data from multiple/nested config tags.
            /// Existing data is overwritten.
            ///
            /// \param bindings : a single device's bindings data.
            /// \param configTags : config tags to use to extract data for this device.
            void fillBindings(const ConfigTagBindings &bindings, const std::vector<StrHash> &configTags);

            /// \brief Fills own data from a given device data.
            ///
            /// Fills labels and bindings data from a given device data. Existing data is overwritten.
            /// Is called recursively to get data from parent devices.
            ///
            /// \param deviceData : data for a specific device.
            /// \param devicesDb : devices database.
            /// \param configTags : config tags to use to extract data for this device.
            void fillData(const DeviceData &deviceData, const DevicesDb &devicesDb,
                    const std::vector<StrHash> &configTags);

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
            Device& operator=(Device &&) = default;

            /// \brief Check if the device provides an interface's input.
            /// \param hash : a hashed string of the interface's input name, in the form interfaceName.inputName.
            /// \return true if this input is present, false otherwise.
            bool hasInput(StrHash hash) const;

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
            const InputInfos getInputInfos(StrHash hash) const;

            /// \param name : the name of the interface's input name, in the form interfaceName.inputName.
            /// \overload getInputInfos
            const InputInfos getInputInfos(const char *name) const;

            /// \brief Get the begin and end constant iterators over the device's InputInfos.
            /// \return The iterators.
            std::pair<StrHashMap<InputInfos>::const_iterator, StrHashMap<InputInfos>::const_iterator>
                    getInputInfosIterators() const;

            /// \brief Get an english ASCII displayable label based on the label infos.
            ///
            /// A simple way to get an english string to display the input label. If it's a dollar name supposed to
            /// be replaced by an icon, it's replaced by an english string. If the input doesn't have a label, it
            /// returns 'A', 'B', 'H' or 'R' folowed by the input number starting at 1 and eventual modifiers.
            /// If it's a complex binding, it returns only the first input.
            /// If it's not bound, it returns "Nothing".
            ///
            /// \param hash : a hashed string of the interface's input name, in the form interfaceName.inputName.
            /// \return A displayable LabelInfos based on the input's InputInfos.
            LabelInfos getEnglishAsciiLabelInfos(StrHash hash) const;

            /// \param name : the name of the interface's input name, in the form interfaceName.inputName.
            /// \overload getEnglishAsciiLabelInfos
            LabelInfos getEnglishAsciiLabelInfos(const char *name) const;

            /// \brief Get device's name.
            /// \return The device's name.
            const std::string &getName() const;

            /// \brief Set the device's name
            ///
            /// When a device is not in the database, the input library may provide a name.
            //
            /// \param name : the new device's name.
            void setName(const char *name);

            /// \brief Set the devie's input infos
            ///
            /// Replaces the device's own input infos by the one one provided. This is used when a library wrapper
            /// provides fallback mappings.
            ///
            /// \param inputInfos : the new input infos.
            void setInputInfos(StrHashMap<InputInfos> &&inputInfos);

            /// \brief Check if the device corresponds to a real database entry or is a dummy one.
            /// \return true if it's a real device, false if it's a dummy one.
            operator bool() const;
    };
}
