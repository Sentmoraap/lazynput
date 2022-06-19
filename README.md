# Lazẏnput
Lazẏnput is a game input device abstraction layer. It allows to read controller inputs in a consistent way, regardless
of how the inputs are actually numbered. It can also provide button names and colors, so games can display the actual
controller button instead of a number or the Xbox equivalent.

A device can implement one or several interfaces. For now the interfaces are `basic_gamepad` and `extended_gamepad`. In
the future it could work with other devices such as steering wheels and flight sticks.

By default the mappings are supposed to be the same everywhere unless specified otherwise. A device can provide mappings
for specific environments with the use of config tags.

It gets it's data from a human-readable file. It's designed to be easily modifiable and avoid repetition. Devices can
inherit other devices, the labels needs to be set only once even if a device has different mappings and labels presets
can be made for the most common cases like labels from a popular game console.

The labels are UTF-8 strings, plus ASCII strings for compatibility. For symbols, it's close looking character(s) or a
text description in english. Variable names are provided for custom handling such as translating texts or using images.

It can be used like an input library with the use of a wrapper class. Wrappers are provided for SDL and SFML but it can
be used with other input libraries as long as they provide joystick identification data. It can use less fully featured
databases like SDL_GameController as a fallback. It also provides default, probably wrong mappings when a device is not
present in any database so you can still read inputs the same way.

## Non-goals
This library does not provide dead zone, saturation nor any other calibration setting.

Lazẏnput does not map inputs to actions. You have to bring your own input manager on the top of that. Having consistent
mappings does not remove the usefulness of an input manager.

## Install
Clone this repo and copy the files in `src/Lazynput` and `include/Lazynput` somewhere in your project directory. Or
make it a submodule. The main.cpp example program is outside those folders on purpose. Define a
`LAZYNPUT_USE_XXX_WRAPPER` macro (replace `XXX`) to compile one of the included wrappers.

## Usage
The initialization looks like this:

    using namespace Lazynput::Literals; // To use the _hash literal.
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
    std::cout << "Press " << device.getLabel("basic_gamepad.a"_hash).utf8 << " to jump.";

If it does not have a label, it will display something generic like "B1".

main.cpp is an example game with SFML.

You may need to write a wrapper for the input library you are using. You can look at `SdlWrapper` and `SfmlWrapper` to
know what you need to do.

The library has Doxygen documentation. Run `doxygen` in the root folder then open `doc/html/index.html`.

You can read the file `lazynputdb.txt` and it's comments to learn it's format.
