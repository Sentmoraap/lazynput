#include <iostream>
#include "LazynputDb.hpp"

int main(int argc, char** argv)
{
    Lazynput::LazynputDb lazynputDb;
    lazynputDb.parseFromFile("../lazynputdb.txt", &std::cerr);
    Lazynput::Device device = lazynputDb.getDevice(Lazynput::HidIds{0x044f, 0xb323});
    std::cout << "Device name: " << device.getName() << "\n";
    return 0;
}
