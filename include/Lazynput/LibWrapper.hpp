#pragma once

#include <stdint.h>
#include <vector>
#include "Lazynput/Device.hpp"

namespace Lazynput
{
    /// \class LibWrapper
    /// \brief Base class to wrap input library to mappings-aware functions.
    ///
    /// This class wraps an input library and uses the devices mappings to give mappings-aware input functions.
    /// It's public interface contains the methods to call to get an input device state in place of using directly an
    /// input library. If a device is not in the Lazynput database this class can get mappings from another source or
    /// expose unmapped devices.
    /// It contains methode to read unmapped device inputs with the underlying library. It calls those methods to
    /// provide the interface inputs, but they can also be called directly when a device doesn't have mappings.
    /// Child classes should override those methods to make a functioning wrapper.
    /// getNumXxx() methods are used only for the latter purpose, you don't need to override them when you write a
    /// wrapper only for the former purpose.

    class LazynputDb;
    class Device;

    class LibWrapper
    {
        public:
            /// \brief If a device is connected, and it's support level.
            enum class DeviceStatus : uint8_t
            {
                /// No device is connected at this slot.
                DISCONNECTED,

                /// A device is connected and no mappings are provided.
                UNSUPPORTED,

                /// A device is connected and mappings are provided through a fallback method.
                FALLBACK,

                /// A device is connected and present in the database.
                SUPPORTED
            };

        protected:
            /// \brief Data about connected devices.
            struct DeviceData
            {
                /// If the input library detects a connected joystick for that index.
                DeviceStatus status = DeviceStatus::DISCONNECTED;

                /// Device mappings and labels.
                Device device;
            };

            /// Devices data for each slot.
            std::vector<DeviceData> devicesData;

            /// Config tags to be used.
            std::vector<StrHash> configTags;

            /// Database, to get devices data.
            const LazynputDb &lazynputDb;

        public:
            /// \brief Constructor. The wrappers need to use a database.
            /// This constructor also adds some OS config tags using preprocessor macros.
            /// Child classes can add global config tags. They must call setGlobalConfigTags, even when no config tags
            /// are added.
            /// \param lazynputDb : the Lazynput database to use.
            LibWrapper(const LazynputDb &lazynputDb);

            /// \brief Update the devices states.
            ///
            /// In some cases, you need to manually call a function to update the devices states. The conditions for it
            /// to be actually needed depends on the used input library.
            /// Child classes can also update their internal state.
            virtual void update() {};

            /// \brief Get if there is a device connected at a given slot and it's support status.
            /// \param index : the slot index.
            /// \return the status of the device's at this index.
            DeviceStatus getDeviceStatus(uint8_t index) const;

            /// \brief Get the device at a given slot.
            ///
            /// Do not call this function if no device is connected at this slot.
            ///
            /// \param index : the slot index.
            /// \return The device data.
            const Device& getDevice(uint8_t index) const;

            /// \brief Get an interface's input value from a connected device.
            ///
            /// Get an interface's input value from the device's inputs and mappings.
            /// Buttons are in [0; 1], absolute axes in [-1, 1], and relative axes can have limitless displacements.
            ///
            /// \param device : the device's slot index.
            /// \param hash : a hashed string of the interface's input name, in the form interfaceName.inputName.
            float getInputValue(uint8_t device, StrHash hash) const;

            /// \param name : the name of the interface's input name, in the form interfaceName.inputName.
            /// \overload getInputValue
            float getInputValue(uint8_t device, const char *name) const;

            /// \brief Get a device's number of absolute axes.
            /// \param device : the device slot.
            /// \return the number of absolute axes.
            virtual uint8_t getNumAbs(uint8_t device) const {return 0;}

            /// \brief Gets the position of a device's absolute axis.
            /// \param device : the device slot.
            /// \param axis : the device's absolute axis number.
            /// \return the axis position in [-1; 1].
            virtual float getAbsValue(uint8_t device, uint8_t axis) const {return 0.f;}

            /// \brief Get a device's number of buttons.
            /// \param device : the device slot.
            /// \return the number of buttons.
            virtual uint8_t getNumBtn(uint8_t device) const {return 0;}

            /// \brief Gets the state of a device's button.
            /// \param device : the device slot.
            /// \param btn : the device's button number.
            /// \return true if pressed, false if released.
            virtual bool getBtnPressed(uint8_t device, uint8_t btn) const {return false;}

            /// \brief Get a device's number of hat switches.
            /// \param device : the device slot.
            /// \return the number of hat switches.
            virtual uint8_t getNumHat(uint8_t device) const {return 0;}

            /// \brief Gets the position of a device's hat switch/POV.
            /// \param device : the device slot.
            /// \param hat : the device's hat number.
            /// \return a (X, Y) pair, each axis in [-1; 1].
            virtual std::pair<float, float> getHatValues(uint8_t device, uint8_t hat) const
                    {return std::make_pair(0.f, 0.f);}

            /// \brief Get a device's number of relative axes.
            /// \param device : the device slot.
            /// \return the number of relative axes.
            virtual uint8_t getNumRel(uint8_t device) const {return 0;}

            /// \brief Gets the difference of a device's relative axis since it's last update.
            /// \param device : the device slot.
            /// \param hat : the device's relative axis number.
            /// \return the difference since the last update.
            virtual float getRelDelta(uint8_t device, uint8_t rel) const {return 0.f;}


    };
}
