# Lazẏnput
Lazẏnput is a game input device abstraction layer. It allows to read controller inputs in a consistent way, regardless
of how the inputs are actually numbered. It can also provide button names and colors, so games can display the actual
controller button instead of a number or the Xbox equivalent.

A device can implement one or several interfaces. For now the interfaces are `basic_gamepad` and `extended_gamepad`. In
the future it could work with other device such as steering wheels and flight sticks.

By default the mappings are supposed to be the same everywhere unless specified otherwise. A device can provide mappings
for specific environments with the use of config tags.

It gets it's data from a human-readable file. It's designed to be easily modifiable and avoid repetition. Devices can
inherit other devices, the labels needs to be set only once even if a device has different mappings and labels presets
can be made for the most common cases like labels from a popular game console.

It can be used like an input library with the use of a wrapper class. Wrappers are provided for SDL and SFML but it can
be used with other input libraries as long as they provide joystick identification data. It can use less fully featured
databases like SDL_GameController as a fallback.

## Non-goals
This library does not provide dead zone, saturation nor any other calibration setting.

Lazẏnput does not map inputs to actions. You have to bring your own input manager on the top of that. Having consistent
mappings does not remove the usefulness of an input manager.

## Install
Clone this repo and copy the files in `src/Lazynput` and `include/Lazynput` somewhere in your project directory. Copy
the wrappers you need.

## Usage
The initialization looks like this:

    using namespace Lazynput::Litterals; // To use the _hash litteral.
    Lazynput::LazynputDb lazynputDb;
    lazynputDb.parseFromDefault(&std::cerr); // Look for lazynputdb.txt in default paths.

To use it like an input library, you must instantiate a wrapper.

    Lazynput::SdlWrapper wrapper(lazynputDb);

You can read the inputs for the controller 0 like this:

    wrapper.update(); // Call this every frame before reading the inputs.
    if(wrapper.getInputValue(0, "basic_gamepad.a"_hash) > 0) jump(); // Buttons have values between 0.f and 1.f
    pos.x += wrapper.getInputValue(0, "basic_gamepad.lsx"_hash) * speed; // [-1.f; 1.f]

You can display the device's labels and colors like this:

    const Lazynput::Device &device = libWrapper.getDevice(0);
    std::cout << "Press " << device.getEnglishAsciiLabelInfos("basic_gamepad.a"_hash).label << " to jump.";

If it does not have a label, it will display something generic like "B1".

main.cpp is an example game with SFML.

You may need to write a wrapper for the input library you are using. You can look at `SdlWrapper` and `SfmlWrapper` to
know what you need to do.

The library has Doxygen documentation.

You can read the file `lazynputdb.txt` and it's comments to learn it's format.
