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

            /// Dummy InputInfos whose reference is returned when an input is not present.
            static InputInfos dummyInputInfos;

            /// \brief Generate labels data from the device's label data and it's bindings.
            ///
            /// Copies the color from the database and generates it's string from the database string.
            /// When there is a dollar name and the icons database has an unicode string for it, the LabelInfo's
            /// Unicode string will be that one. Otherwise it will be the same as the ASCII string.
            /// The ASCII sctring replace the dollar name by an english string. If the input doesn't have a label, it
            /// returns 'A', 'B', 'H' or 'R' folowed by the input number starting at 1 and eventual modifiers.
            ///
            /// \param dbLabel : the database's label data.
            /// \params iconsDb : the icons database.
            /// \return the generated device's label data.
            LabelInfos genLabel(const DbLabelInfos &dbLabel, const IconsDb &iconsDb);

            /// \brief Generate labels data from it's binding
            ///
            /// Called when an input doesn't have a label. It generates a generic name 'A', 'B', 'H' or 'R' folowed by
            /// the input number starting at 1 and eventual modifiers.
            /// If it's a complex binding, it uses only the first input.
            ///
            /// \param inputinfos : the InputInfos structules whose label will be filled from it's bindings.
            void genGenericLabel(InputInfos &inputInfos);

            /// \brief Fills labels data.
            ///
            /// Fills labels data from labels data provided by the labels database or a single device's data.
            /// Existing data is overwritten.
            ///
            /// \param labels : labels data.
            /// \param iconsDb : icons database.
            void fillLabels(const StrHashMap<DbLabelInfos> &labels, const IconsDb &iconsDb);

            /// \brief Fills labels data from the labels database.
            ///
            /// Fills labels data from a labels preset defined in the labels database. Existing data is overwrittern.
            /// Is called recursively to get data from parent labels presets.
            ///
            /// \param labels : a labels preset.
            /// \param labelsDb : labels database.
            /// \param iconsDb : icons database.
            void fillLabels(const Labels &labels, const LabelsDb &labelsDb, const IconsDb &iconsDb);

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

            /// \overload
            /// \param name : the name of the interface's input name, in the form interfaceName.inputName.
            bool hasInput(const char *name) const;

            /// \brief Get the device's InputInfos for a given interface's input.
            ///
            /// Get the InputInfos for a given interface's input for this specific device with specific configTags.
            /// If the device don't have the given interface's input, it returns a dummy one.
            ///
            /// \param hash : a hashed string of the interface's input name, in the form interfaceName.inputName.
            /// \return the corresponding InputInfos if it exists or a dummy one.
            const InputInfos &getInputInfos(StrHash hash) const;

            /// \overload
            /// \param name : the name of the interface's input name, in the form interfaceName.inputName.
            const InputInfos &getInputInfos(const char *name) const;

            /// \brief Get the label of a given interface input.
            ///
            /// Returns a struct containing strings for displaying an input name. It has an ASCII string, an UTF-8
            /// string and a variable name so you have different ways to handle non-ASCII characters. If the input
            /// doesn't have a label, it's filled with generic names such as "B1".
            ///
            /// \param hash : a hashed string of the interface's input name, in the form interfaceName.inputName.
            /// \return A displayable LabelInfos based on the input's InputInfos.
            const LabelInfos &getLabel(StrHash hash) const;

            /// \overload
            /// \param name : the name of the interface's input name, in the form interfaceName.inputName.
            const LabelInfos &getLabel(const char *name) const;

            /// \brief Get the begin and end constant iterators over the device's InputInfos.
            /// \return The iterators.
            std::pair<StrHashMap<InputInfos>::const_iterator, StrHashMap<InputInfos>::const_iterator>
                    getInputInfosIterators() const;

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
