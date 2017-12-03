#pragma once

#include <cstdint>
#include <iostream>
#include <vector>
#include "Device.hpp"

namespace Lazynput
{
    /// \class DevicesDb
    /// \brief Main database container and parser.
    ///
    /// This class contains all the database info.
    /// Use it to parse a config file and get the Devices info.
    class DevicesDb
    {
        public:
            /// \brief Set variables that apply to every device.
            ///
            /// Set variales that can be used to use device mapping overrides when they are provided.
            ///
            /// \param configTags : non device-specific configuration tags.
            void setGlobalConfigTags(const std::vector<std::string> configTags);

            /// \brief Check if the database contains a device.
            /// \param vid : device vendor id.
            /// \param pid : device product id.
            /// \return true if the device is present in the database, false otherwise.
            bool hasDevice(uint16_t vid, uint16_t pid) const;

            /// \brief Get a Device from it's vendor ID, product ID and optional configuration tags.
            ///
            /// Finds in the database the Device data of the corresponding vendor ID and product ID.
            /// The Device's mappings will be set according to the database info and the variables list.
            ///
            /// \param vid : device vendor id.
            /// \param pid : device product id.
            /// \param configTags : device specific variables.
            /// \return a Device object if found, or a dummy Device object otherwise.
            const Device getDevice(uint16_t vid, uint16_t pid, const std::vector<std::string> configTags) const;

            /// \overload getDevice
            const Device getDevice(uint16_t vid, uint16_t pid) const;

            /// \brief Parse data from an input stream.
            ///
            /// Parse config data from a text input stream. Existing data will be overrided.
            ///
            /// \param inStream : a istream providing the text input to be parsed.
            /// \param errors : a stream to write parsing errors, if any. Can be null.
            /// \return true if successfully parsed, false otherwise.
            bool parseFromIstream(std::istream &inStream, std::ostream *errors = nullptr);

            /// \brief Parse data from a file.
            ///
            /// Parse config data from a file. Existing data will be overrided.
            ///
            /// \param path : the path to the file.
            /// \param errors : a stream to write parsing errors, if any. Can be null.
            /// \return true if successfully parsed, false otherwise.
            bool parseFromFile(const char *path, std::ostream *errors = nullptr);

            /// \brief Parse data from the default file.
            ///
            /// Parse config data from .lazynputdb in the home directory. Existing data will be overrided.
            ///
            /// \param path : the path to the file.
            /// \param errors : a stream to write parsing errors, if any. Can be null.
            /// \return true if successfully parsed, false otherwise.
            bool parseFromDefaultFile(std::ostream *errors = nullptr);
    };
}
