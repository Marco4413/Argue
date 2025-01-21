#define ARGUE_IMPLEMENTATION
#include "argue.hpp"

#include <iostream>

int main(int argc, const char** argv)
{
    Argue::ArgParser parser(argv[0], "My first Argue.");
    Argue::StrArgument user(parser, "USER", "Greets USER.");
    parser.Parse(argc, argv);

    if (!parser) {
        // An error happened

        // Build help message and print it
        Argue::TextBuilder help;
        parser.WriteHelp(help);
        std::cout << help.Build() << std::endl;

        // Print error message
        std::cerr << "ERROR: " << parser.GetError() << std::endl;
        return 1;
    }

    // user can be dereferenced because there was no error.
    // Therefore user.HasValue() return true
    std::cout << "Hello, " << *user << "!" << std::endl;
    return 0;
}
