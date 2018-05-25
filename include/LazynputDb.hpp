#pragma once

#include <cstdint>
#include <iostream>
#include <vector>
#include "Device.hpp"
#include "Types.hpp"
#include "PrivateTypes.hpp"
#include "StrHash.hpp"

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

        public:
            /// \brief Set variables that apply to every device.
            ///
            /// Set variales that can be used to use device mapping overrides when they are provided.
            /// It uses an iterator to a container of C strings or std::strings.
            /// Teplate instanciations are provided for C arrays and some STL containers of const C strings and const
            /// std::strings.
            /// Include DevicesDb.tpp to instanciate it with other iterators.
            ///
            /// \param begin : an iterator to a non device-specific configuration tags container.
            /// \param end : the end iterator.
            template<class ForwardIteratorType>
            void setGlobalConfigTags(ForwardIteratorType begin, ForwardIteratorType end);

            /// \brief Check if the database contains a device.
            /// \param ids : device HID ids.
            /// \return true if the device is present in the database, false otherwise.
            bool hasDevice(HidIds ids) const;

            /// \brief Get a Device from it's vendor ID, product ID and optional configuration tags.
            ///
            /// Finds in the database the Device data of the corresponding HID IDs.
            /// The Device's mappings will be set according to the database info and the configuration tags list.
            /// It uses an iterator to a container of C strings or std::strings.
            /// Teplate instanciations are provided for C arrays and some STL containers of const C strings and const
            /// std::strings.
            /// Include DevicesDb.tpp to instanciate it with other iterators.
            ///
            /// \param vid : device vendor id.
            /// \param configTagsBegin : an iterator to a device-specific configuration tags container.
            /// \param configTagsEnd : the end iterator.
            /// \return a Device object if found, or a dummy Device object otherwise.
            template<class ForwardIteratorType>
            Device getDevice(HidIds ids, ForwardIteratorType configTagsBegin, ForwardIteratorType configTagsEnd) const;

            /// \overload getDevice
            Device getDevice(HidIds ids) const;

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

            /// \brief Parse data from the default file.
            ///
            /// Parse config data from .lazynputdb in the home directory. Existing devices data will be overrided.
            ///
            /// \param path : the path to the file.
            /// \param errors : a stream to write parsing errors, if any. Can be null.
            /// \return true if successfully parsed, false otherwise.
            bool parseFromDefaultFile(std::ostream *errors = nullptr);
    };
}
