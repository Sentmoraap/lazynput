#include <iostream>
#include "LazynputDb.hpp"

using namespace Lazynput::Litterals;

int main(int argc, char** argv)
{
    Lazynput::LazynputDb lazynputDb;
    lazynputDb.parseFromFile("../lazynputdb.txt", &std::cerr);
    Lazynput::Device device = lazynputDb.getDevice(Lazynput::HidIds{0x054C, 0x0268});
    std::cout << "Device name: " << device.getName() << "\n";
    const std::array<std::pair<std::string, Lazynput::StrHash>, 9> inputNames =
    {
        std::make_pair("A", "basic_gamepad.a"_hash),
        std::make_pair("B", "basic_gamepad.b"_hash),
        std::make_pair("X", "basic_gamepad.x"_hash),
        std::make_pair("Y", "basic_gamepad.y"_hash),
        std::make_pair("L1", "basic_gamepad.l1"_hash),
        std::make_pair("L2", "basic_gamepad.l2"_hash),
        std::make_pair("R1", "basic_gamepad.r1"_hash),
        std::make_pair("R2", "basic_gamepad.r2"_hash),
        std::make_pair("HATX", "basic_gamepad.dpx"_hash)
    };
    for(const auto &input : inputNames)
    {
        const Lazynput::InputInfos inputInfos = device.getInputInfos(input.second);
        std::cout << "Button " << input.first << " is ";
        if(inputInfos.binding.empty()) std::cout << "not bound.\n";
        else
        {
            if(inputInfos.binding.size() > 1 || inputInfos.binding[0].size() > 1) std::cout << "complex binding";
            else
            {
                const Lazynput::SingleBindingInfos &binding = inputInfos.binding[0][0];
                if(binding.options.invert && !binding.options.half) std::cout << "~";
                switch(binding.type)
                {
                    case Lazynput::InputType::NIL:
                        // Should not happen
                        break;
                    case Lazynput::InputType::BUTTON:
                        std::cout << "b" << static_cast<int>(binding.index);
                        break;
                    case Lazynput::InputType::HAT:
                        std::cout << "h" << static_cast<int>(binding.index / 2) << (binding.index % 2 ? "y" : "x");
                        break;
                    case Lazynput::InputType::ABSOLUTE_AXIS:
                        std::cout << "a" << static_cast<int>(binding.index);
                        break;
                    case Lazynput::InputType::RELATIVE_AXIS:
                        std::cout << "r" << static_cast<int>(binding.index);
                        break;
                }
                if(binding.options.half) std::cout << (binding.options.invert ? "-" : "+");
            }
            if(!inputInfos.labelInfos.label[0]) std::cout << " and is not labeled.\n";
            else
            {
                std::cout << " and is labeled ";
                if(inputInfos.labelInfos.hasColor)
                {
                    const Lazynput::Color &color = inputInfos.labelInfos.color;
                    std::cout << "\033[38;5;"
                        << (16 + 36 * ((color.r + 26) / 51) + 6 * ((color.g + 26) / 51) + ((color.b + 26) / 51)) << "m";
                }
                std::cout << inputInfos.labelInfos.label << "\033[0m.\n";
            }
        }
    }
    return 0;
}
