#define ARGUE_IMPLEMENTATION
#include "argue.hpp"

#include <iostream>

class DoubleArgument final :
    public Argue::IPositionalArgument
{
public:
    // Inherit constructors
    using Argue::IPositionalArgument::IPositionalArgument;
    virtual ~DoubleArgument() = default;

    ARGUE_DELETE_MOVE_COPY(DoubleArgument)

public:
    bool IsVariadic() const override { return false; }

    double operator*() const { return GetValue(); }
    double GetValue() const { return m_Value; }

protected:
    bool ParseArg(std::string_view arg) override
    {
        double value;
        auto result = std::from_chars(&arg.front(), &arg.back()+1, value);
        if (result.ptr != &arg.back()+1) {
            return SetError(Argue::s(
                "Expected number for '", GetMetaVar(), "', got '", arg, "'."
            ));
        }

        m_Value = value;
        return true;
    }

private:
    double m_Value = 0;
};

int main(int argc, const char** argv)
{
    // This is an improved version of the previous example: '01-math.cpp'
    Argue::ArgParser parser(argv[0], "Math ain't mathing. 2.0");
    // Argue provides a default help command which will
    //  traverse parsers automatically to filter commands.
    Argue::HelpCommand help(parser);

    DoubleArgument       a(parser, "A", "The first operand.");
    DoubleArgument       b(parser, "B", "The second operand.");
    Argue::ChoiceOption op(
        parser, "op", "op", "OPERATOR", "The operator to use. (default: +)",
        {"+", "-", "*", "/"}, 0);
    parser.Parse(argc, argv);

    if (!parser) {
        // An error happened

        // Build help message and print it
        Argue::TextBuilder helpMsg;
        parser.WriteHelp(helpMsg);
        std::cout << helpMsg.Build() << std::endl;

        // Print error message
        std::cerr << "ERROR: " << parser.GetError() << std::endl;
        return 1;
    } else if (help) {
        // Help command invoked!

        // The help command DOES NOT print stuff automatically.
        // So we must build the help message using a TextBuilder.
        Argue::TextBuilder helpMsg;
        help(helpMsg);
        std::cout << helpMsg.Build() << std::endl;
        return 0;
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
