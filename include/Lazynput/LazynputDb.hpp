#pragma once

#include <cstdint>
#include <iostream>
#include <vector>
#include "Lazynput/Device.hpp"
#include "Lazynput/Types.hpp"
#include "Lazynput/PrivateTypes.hpp"
#include "Lazynput/StrHash.hpp"

namespace Lazynput
{
    /// \class LazynputDb
    ///
    /// \brief Main database container and parser.
    ///
    /// This class contains all the database info.
    /// Use it to parse a config file and get the Devices info.
    class LazynputDb
    {
        private:
            /// Devices database.
            DevicesDb devicesDb;

            /// Global config tags.
            std::vector<StrHash> globalConfigTags;

            /// \brief Get a Device from it's vendor ID, product ID and optional configuration tags.
            ///
            /// Get device data from a vector of configTags. Is used by the public getDevice() functions.

            /// \param ids : device HID ids.
            /// \param configTags : all the config tags to use: global and extra tags given to the caller functions.
            /// \return a Device object if found, or a dummy Device object otherwise.
            Device getDevice(HidIds ids, const std::vector<StrHash> &configTags) const;

        public:
            /// \brief Set variables that apply to every device.
            ///
            /// Set variales that can be used to use device mapping overrides when they are provided.
            ///
            /// \param configTags : a pointer to an array of hashed strings.
            /// \param size : the array size.
            void setGlobalConfigTags(const StrHash *configTags, int size);

            /// \brief Get a Device from it's vendor ID, product ID and optional configuration tags.
            ///
            /// Finds in the database the Device data of the corresponding HID IDs.
            /// The Device's mappings will be set according to the database info and the configuration tags list.
            ///
            /// \param ids : device HID ids.
            /// \param configTags : a pointer to an array of hashed strings.
            /// \param size : the array size.
            /// \return a Device object if found, or a dummy Device object otherwise.
            Device getDevice(HidIds ids, const StrHash *configTags, int size) const;

            /// \overload
            Device getDevice(HidIds ids) const;

            /// \brief Gets the interface input string corresponding to the hash.
            ///
            /// \param hash : the hashed "interfaceName.inputName" string.
            /// \return : the unhashed string.
            std::string getStringFromHash(StrHash hash) const;

            /// \brief Gets an interface input type from it's hash.
            ///
            /// \param hash : the hashed "interfaceName.inputName" string.
            /// \return : the interface input type.
            InterfaceInputType getInterfaceInputType(StrHash hash) const;

            /// \brief Parse data from an input stream.
            ///
            /// Parse config data from a text input stream. Existing devices data will be overrided.
            ///
            /// \param inStream : a istream providing the text input to be parsed.
            /// \param errors : a stream to write parsing errors, if any. Can be null.
            /// \return true if successfully parsed, false otherwise.
            bool parseFromIstream(std::istream &inStream, std::ostream *errors = nullptr);

            /// \brief Parse data from a file.
            ///
            /// Parse config data from a file. Existing devices data will be overrided.
            ///
            /// \param path : the path to the file.
            /// \param errors : a stream to write parsing errors, if any. Can be null.
            /// \return true if successfully parsed, false otherwise.
            bool parseFromFile(const char *path, std::ostream *errors = nullptr);

            /// \brief Parse data from the default file locations.
            ///
            /// Looks for a file in the system-specific user data folder and in the working directory.
            /// Existing devices data will be overrided.
            ///
            /// \param errors : a stream to write parsing errors, if any. Can be null.
            /// \return true if successfully parsed, false otherwise.
            bool parseFromDefault(std::ostream *errors = nullptr);
    };
}
