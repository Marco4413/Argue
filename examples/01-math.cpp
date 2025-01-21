#define ARGUE_IMPLEMENTATION
#include "argue.hpp"

#include <iostream>

int main(int argc, const char** argv)
{
    Argue::ArgParser parser(argv[0], "Math ain't mathing.");
    Argue::IntOption     a(parser, "a", "a", "A", "The first operand.");
    Argue::IntOption     b(parser, "b", "b", "B", "The second operand.");
    Argue::ChoiceOption op(
        parser, "op", "op", "OPERATOR", "The operator to use. (default: +)",
        {"+", "-", "*", "/"}, 0);
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

    switch ((*op)[0]) {
    case '+': std::cout << (*a + *b) << std::endl; break;
    case '-': std::cout << (*a - *b) << std::endl; break;
    case '*': std::cout << (*a * *b) << std::endl; break;
    case '/': std::cout << (*a / *b) << std::endl; break;
    default: break; // Unreachable if parser succeeded
    }

    return 0;
}
