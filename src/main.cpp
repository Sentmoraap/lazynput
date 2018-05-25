#include <iostream>
#include "LazynputDb.hpp"

int main(int argc, char** argv)
{
    Lazynput::LazynputDb lazunputDb;
    lazunputDb.parseFromFile("../lazynputdb.txt", &std::cerr);
    return 0;
}
