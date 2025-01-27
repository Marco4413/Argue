/*
Copyright (c) 2025 [Marco4413](https://github.com/Marco4413/Argue)

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#ifdef ARGUE_IMPLEMENTATION
  // Implementation-specific includes are put here
  //  so that they can be easily seen.
  #include <charconv> // int64_t std::from_chars
#endif // ARGUE_IMPLEMENTATION

#ifndef _ARGUE_HPP
#define _ARGUE_HPP

#include <cinttypes>
#include <stack>
#include <string>
#include <string_view>
#include <utility> // std::forward
#include <vector>

#define ARGUE_DELETE_MOVE_COPY(className)            \
    className(const className&) = delete;            \
    className(className&&) = delete;                 \
    className& operator=(const className&) = delete; \
    className& operator=(className&&) = delete;      \

#define ARGUE_UNUSED(var) \
    ((void)var)

namespace Argue
{
    template<typename ...Args>
    std::string s(Args&& ...args)
    {
        std::string result;
        // See https://en.cppreference.com/w/cpp/language/fold
        (result += ... += std::forward<Args>(args));
        return result;
    }

    constexpr std::string_view SPACE_CHARS = " \f\n\r\t\v";
    constexpr bool IsSpace(char ch)
    {
        return SPACE_CHARS.find(ch) != std::string_view::npos;
    }

    class ITextBuilder
    {
    public:
        virtual ~ITextBuilder() = default;

        virtual void PutText(std::string_view text) = 0;

        virtual void NewLine() = 0;
        virtual void Spacer() = 0;

        virtual void Indent() = 0;
        virtual void DeIndent() = 0;

        // Builds the final text resetting this builder instance
        virtual std::string Build() = 0;
    };

    class TextBuilder final :
        public ITextBuilder
    {
    public:
        TextBuilder(
                std::string_view indent="  ",
                bool indentOnWrap=true,
                size_t maxParagraphWidth=80) :
            m_Indent(indent),
            m_IndentOnWrap(indentOnWrap),
            m_MaxParagraphWidth(maxParagraphWidth)
        {}

        virtual ~TextBuilder() = default;

        void PutText(std::string_view text) override;

        void NewLine() override;
        void Spacer() override;

        void Indent() override;
        void DeIndent() override;

        /**
         * The built text will start with a non-space character
         * and end with a new line preceded by a non-space character.
         * Space characters are defined by the `::IsSpace()` function
         * and `SPACE_CHARS` variable.
         */
        std::string Build() override;
    
    protected:
        // Call this if text does not contain new lines
        void PutLine(std::string_view text);
        void PutLineIndent(bool hasWrapped);

    private:
        std::string m_Text = "";

        size_t m_IndentationLevel = 0;
        std::string m_CurrentLine = "";
        size_t m_CurrentLineIndentLength = 0;

        std::string m_Indent;
        bool m_IndentOnWrap;
        size_t m_MaxParagraphWidth;
    };

    class IArgParser; // Forward declaration

    // An option cannot be moved/copied and must live as long as its parser
    class IOption
    {
    public:
        IOption(IArgParser& parser,
                std::string_view name,
                std::string_view shortName,
                std::string_view metaVar,
                std::string_view description);

        virtual ~IOption() = default;

        ARGUE_DELETE_MOVE_COPY(IOption)

        const std::string& GetName() const { return m_Name; }
        const std::string& GetShortName() const { return m_ShortName; }

        bool HasMetaVar() const { return !m_MetaVar.empty(); }
        const std::string& GetMetaVar() const { return m_MetaVar; }

        bool HasDescription() const { return !m_Description.empty(); }
        const std::string& GetDescription() const { return m_Description; }

        const IArgParser& GetParser() const { return m_Parser; }

        bool HasShortName() const { return !m_ShortName.empty(); }

        operator bool() const { return HasValue(); }

        // Returns true if this option was specified by the user.
        // If this returns true, then ::HasValue() MUST return true.
        bool WasParsed() const { return m_WasParsed; }

        // Returns true if parsing was successful.
        // Returning false does not mean that ::SetError was called.
        // If the parser has no error, the option was not recognized.
        bool Parse(std::string_view arg, bool isShort)
        {
            if (ParseArg(arg, isShort)) {
                m_WasParsed = true;
                return true;
            }
            return false;
        }

    public:
        virtual bool HasDefaultValue() const { return false; }
        // Returns true if this option requires the MetaVar to have a value when parsing.
        virtual bool IsVarOptional() const { return false; }

        virtual void WriteHint(ITextBuilder& hint) const;
        virtual void WriteHelp(ITextBuilder& help) const;

        // true if GetValue() will return a valid value.
        // MUST return true if ::WasParsed() returns true.
        // i.e. Parse was successful or a default exists
        virtual bool HasValue() const { return WasParsed() || HasDefaultValue(); }

        // GetValue() method specific to the implementation
        // And a dereference alias to that method

    protected:
        // See ::Parse()
        virtual bool ParseArg(std::string_view arg, bool isShort);

        // Implementing this method will allow to use the default behaviour of ::ParseArg()
        // i.e. --longName=VALUE, -shortVALUE
        virtual bool ParseValue(std::string_view val)
        {
            ARGUE_UNUSED(val);
            return SetError(s("Value parsing was not implemented for '", GetName(), "'."));
        }

        // Always returns false, this allows `return SetError(...)` in ::Parse functions.
        bool SetError(std::string&& errorMessage);

        // Default name parsing behaviour, use this within ::Parse()
        bool ConsumeName(std::string_view& arg, bool isShort);

    private:
        bool m_WasParsed = false;
        IArgParser& m_Parser;
        std::string m_Name;
        std::string m_ShortName;
        std::string m_MetaVar;
        std::string m_Description;
    };

    class IPositionalArgument
    {
    public:
        // `metaVar` must have a length > 0 not counting spaces
        IPositionalArgument(
                IArgParser& parser,
                std::string_view metaVar,
                std::string_view description);
        virtual ~IPositionalArgument() = default;

        ARGUE_DELETE_MOVE_COPY(IPositionalArgument)

        const std::string& GetMetaVar() const { return m_MetaVar; }

        size_t HasDescription() const { return !m_Description.empty(); }
        const std::string& GetDescription() const { return m_Description; }

        const IArgParser& GetParser() const { return m_Parser; }

        operator bool() const { return HasValue(); }

        // Returns true if this option was specified by the user.
        // If this returns true, then ::HasValue() MUST return true.
        bool WasParsed() const { return m_WasParsed; }

        // Returns true if parsing was successful.
        // Returns false if ::SetError() was called.
        // A positional argument must be present at its position
        bool Parse(std::string_view arg)
        {
            if (ParseArg(arg)) {
                m_WasParsed = true;
                return true;
            }
            return false;
        }

    public:
        virtual bool HasDefaultValue() const { return IsVariadic(); }
        virtual bool IsVariadic() const = 0;

        virtual void WriteHint(ITextBuilder& hint) const;
        virtual void WriteHelp(ITextBuilder& help) const;

        // true if GetValue() will return a valid value
        // MUST return true if ::WasParsed() returns true.
        // i.e. Parse was successful or a default exists
        virtual bool HasValue() const { return WasParsed() || HasDefaultValue(); }

        // GetValue() method specific to the implementation
        // And a dereference alias to that method

    protected:
        virtual bool ParseArg(std::string_view arg) = 0;

        // Always returns false, this allows `return SetError(...)` in ::Parse functions.
        bool SetError(std::string&& errorMessage);

    private:
        bool m_WasParsed = false;
        IArgParser& m_Parser;
        std::string m_MetaVar;
        std::string m_Description;
    };

    // A parser cannot be moved/copied and must live as long as its options/subparsers
    class IArgParser
    {
    public:
        // `command` for a root parser is the program itself
        IArgParser(std::string_view command, std::string_view description) :
            m_Name(command),
            m_Description(description)
        {}

        virtual ~IArgParser() = default;

        ARGUE_DELETE_MOVE_COPY(IArgParser)

        const std::string& GetName() const { return m_Name; }

        size_t HasDescription() const { return !m_Description.empty(); }
        const std::string& GetDescription() const { return m_Description; }

        // The returned pointers are not null unless something terrible happened
        const std::vector<IOption*>& GetOptions() const { return m_Options; }
        const std::vector<IArgParser*>& GetSubCommands() const { return m_Commands; }
        const std::vector<IPositionalArgument*>& GetArguments() const { return m_Arguments; }

        // Returns true if this command was used and there was no error.
        // Moreover, all direct children options to this command have a value.
        operator bool() const { return !HasError() && m_WasUsed; }

        bool Parse(int argc, const char** argv)
        {
            std::stack<std::string_view> args;
            for (int i = argc-1; i >= 0; --i)
                args.emplace(argv[i]);
            return Parse(args);
        }

    public:
        virtual void WriteHint(ITextBuilder& hint) const;
        virtual void WriteHelp(ITextBuilder& help, bool briefOptions=false, bool briefSubcommands=true) const;

        // If this returns false, either there was an error or the command did not match.
        virtual bool Parse(std::stack<std::string_view> args);

        virtual const std::string& GetError() const = 0;
        // Always returns false, this allows `return SetError(...)` in ::Parse functions.
        virtual bool SetError(std::string&& errorMessage) = 0;
        virtual bool HasError() const = 0;

        virtual const std::string& GetPrefix() const = 0;
        virtual const std::string& GetShortPrefix() const = 0;
        virtual bool HasShortPrefix() const { return GetShortPrefix().length() > 0; }

    public: // The following methods are called by constructors
        void AddOption(IOption& opt)
        {
            m_Options.emplace_back(&opt);
        }

        void AddCommand(IArgParser& cmd)
        {
            m_Commands.emplace_back(&cmd);
        }

        void AddArgument(IPositionalArgument& arg)
        {
            m_Arguments.emplace_back(&arg);
        }

    private:
        bool m_WasUsed = false;

        std::string m_Name;
        std::string m_Description;

        std::vector<IOption*> m_Options;
        std::vector<IArgParser*> m_Commands;
        std::vector<IPositionalArgument*> m_Arguments;
    };

    class ArgParser final :
        public IArgParser
    {
    public:
        ArgParser(std::string_view program, std::string_view description) :
            ArgParser(program, description, "--", "-")
        {}

        ArgParser(
                std::string_view program,
                std::string_view description,
                std::string_view prefix,
                std::string_view shortPrefix) :
            IArgParser(program, description),
            m_Prefix(prefix),
            m_ShortPrefix(shortPrefix)
        {}

        ~ArgParser() = default;

        ARGUE_DELETE_MOVE_COPY(ArgParser)

    public:
        const std::string& GetError() const override { return m_ErrorMessage; }
        bool SetError(std::string&& errorMessage) override
        {
            m_ErrorMessage = std::forward<std::string>(errorMessage);
            return false;
        }
        bool HasError() const override { return !m_ErrorMessage.empty(); }

        const std::string& GetPrefix() const override { return m_Prefix; }
        const std::string& GetShortPrefix() const override { return m_ShortPrefix; }

    private:
        std::string m_Prefix;
        std::string m_ShortPrefix;
        std::string m_ErrorMessage;
    };

    class CommandParser final :
        public IArgParser
    {
    public:
        CommandParser(
                IArgParser& parent,
                std::string_view command,
                std::string_view description) :
            IArgParser(command, description),
            m_Parent(parent)
        {
            m_Parent.AddCommand(*this);
        }

        ~CommandParser() = default;

        ARGUE_DELETE_MOVE_COPY(CommandParser)

    public:
        const std::string& GetError() const override { return m_Parent.GetError(); }
        bool SetError(std::string&& errorMessage) override
        {
            m_Parent.SetError(std::forward<std::string>(errorMessage));
            return false;
        }
        bool HasError() const override { return m_Parent.HasError(); }

        const std::string& GetPrefix() const override { return m_Parent.GetPrefix(); }
        const std::string& GetShortPrefix() const override { return m_Parent.GetShortPrefix(); }

    private:
        IArgParser& m_Parent;
    };

    class FlagOption :
        public IOption
    {
    public:
        FlagOption(
                IArgParser& parser,
                std::string_view name,
                std::string_view shortName,
                std::string_view description,
                bool defaultValue=false) :
            IOption(parser, name, shortName, "", description),
            m_Default(defaultValue)
        {
            SetValue(m_Default);
        }

        virtual ~FlagOption() = default;

        ARGUE_DELETE_MOVE_COPY(FlagOption)
    
    public:
        bool HasDefaultValue() const override { return true; }
        bool IsVarOptional() const override { return true; }

        void WriteHint(ITextBuilder& hint) const override;
        void WriteHelp(ITextBuilder& help) const override;

        bool GetDefaultValue() const { return m_Default; }

        bool operator*() const { return GetValue(); }
        bool GetValue() const  { return m_Value; }

    public:
        virtual void SetValue(bool flag) { m_Value = flag; }

    protected:
        bool ParseArg(std::string_view arg, bool isShort) override;

    private:
        bool m_Value = false;
        bool m_Default = false;
    };

    class FlagGroupOption final :
        public FlagOption
    {
    public:
        template<typename ...FlagGroup>
        FlagGroupOption(
                IArgParser& parser,
                std::string_view name,
                std::string_view shortName,
                std::string_view description,
                bool defaultValue=false,
                FlagGroup& ...flagGroup) :
            FlagOption(parser, name, shortName, description, defaultValue)
        {
            (m_Group.push_back(&static_cast<FlagOption&>(flagGroup)), ...);
            SetValue(GetValue());
        }

        virtual ~FlagGroupOption() = default;

        ARGUE_DELETE_MOVE_COPY(FlagGroupOption)

        const std::vector<FlagOption*>& GetGroup() const { return m_Group; }

    public:
        void SetValue(bool flag) override
        {
            FlagOption::SetValue(flag);
            for (auto opt : m_Group)
                opt->SetValue(flag);
        }

    private:
        std::vector<FlagOption*> m_Group;
    };

    class IntOption final :
        public IOption
    {
    public:
        using IOption::IOption;

        IntOption(
                IArgParser& parser,
                std::string_view name,
                std::string_view shortName,
                std::string_view metaVar,
                std::string_view description,
                int64_t defaultValue) :
            IOption(parser, name, shortName, metaVar, description),
            m_HasDefault(true),
            m_Default(defaultValue)
        {}

        virtual ~IntOption() = default;

        ARGUE_DELETE_MOVE_COPY(IntOption)

    public:
        bool HasDefaultValue() const override { return m_HasDefault; }
        bool IsVarOptional() const override { return false; }

        int64_t GetDefaultValue() const { return m_Default; }

        int64_t operator*() const { return GetValue(); }
        int64_t GetValue() const
        {
            if (WasParsed()) return m_Value;
            return m_Default;
        }

    protected:
        bool ParseValue(std::string_view val) override;

    private:
        int64_t m_Value = 0;

        bool m_HasDefault = false;
        int64_t m_Default = 0;
    };

    class StrOption final :
        public IOption
    {
    public:
        using IOption::IOption;

        StrOption(
                IArgParser& parser,
                std::string_view name,
                std::string_view shortName,
                std::string_view metaVar,
                std::string_view description,
                std::string_view defaultValue) :
            IOption(parser, name, shortName, metaVar, description),
            m_HasDefault(true),
            m_Default(defaultValue)
        {}

        virtual ~StrOption() = default;

        ARGUE_DELETE_MOVE_COPY(StrOption)

    public:
        bool HasDefaultValue() const override { return m_HasDefault; }
        bool IsVarOptional() const override { return false; }

        const std::string& GetDefaultValue() const { return m_Default; }

        const std::string& operator*() const { return GetValue(); }
        const std::string& GetValue() const
        {
            if (WasParsed()) return m_Value;
            return m_Default;
        }

    protected:
        bool ParseValue(std::string_view val) override;

    private:
        std::string m_Value = "";

        bool m_HasDefault = false;
        std::string m_Default = "";
    };

    class ChoiceOption final :
        public IOption
    {
    public:
        ChoiceOption(
                IArgParser& parser,
                std::string_view name,
                std::string_view shortName,
                std::string_view metaVar,
                std::string_view description,
                std::initializer_list<std::string_view> choices) :
            IOption(parser, name, shortName, metaVar, description)
        {
            for (auto choice : choices)
                m_Choices.emplace_back(choice);
        }

        ChoiceOption(
                IArgParser& parser,
                std::string_view name,
                std::string_view shortName,
                std::string_view metaVar,
                std::string_view description,
                std::initializer_list<std::string_view> choices,
                size_t defaultChoiceIdx) :
            IOption(parser, name, shortName, metaVar, description),
            m_HasDefault(true)
        {
            for (auto choice : choices)
                m_Choices.emplace_back(choice);
            if (!m_Choices.empty()) {
                m_DefaultIdx = defaultChoiceIdx < m_Choices.size()
                    ? defaultChoiceIdx
                    : m_Choices.size()-1;
            }
        }

        virtual ~ChoiceOption() = default;

        ARGUE_DELETE_MOVE_COPY(ChoiceOption)

        std::string GetChoiceString() const;

    public:
        bool HasDefaultValue() const override { return m_HasDefault; }
        bool IsVarOptional() const override { return false; }

        void WriteHint(ITextBuilder& hint) const override;

        std::string_view GetDefaultValue() const
        {
            if (m_Choices.empty() || !m_HasDefault)
                return "";
            return m_Choices[m_DefaultIdx];
        }

        std::string_view operator*() const { return GetValue(); }
        std::string_view GetValue() const
        {
            if (m_Choices.empty()) return "";
            if (WasParsed()) return m_Choices[m_ValueIdx];
            return m_Choices[m_DefaultIdx];
        }

    protected:
        bool ParseValue(std::string_view val) override;

    private:
        size_t m_ValueIdx = 0;
        std::vector<std::string> m_Choices;

        bool m_HasDefault = false;
        size_t m_DefaultIdx = 0;
    };

    class CollectionOption final :
        public IOption
    {
    public:
        CollectionOption(
                IArgParser& parser,
                std::string_view name,
                std::string_view shortName,
                std::string_view metaVar,
                std::string_view description,
                bool acceptEmptyValues=false) :
            IOption(parser, name, shortName, metaVar, description),
            m_AcceptEmptyValues(acceptEmptyValues)
        {}

        virtual ~CollectionOption() = default;

        ARGUE_DELETE_MOVE_COPY(CollectionOption)

        bool AcceptsEmptyValues() const { return m_AcceptEmptyValues; }

    public:
        bool HasDefaultValue() const override { return true; }
        bool IsVarOptional() const override { return m_AcceptEmptyValues; }

        const std::vector<std::string>& operator*() const { return GetValue(); }
        const std::vector<std::string>& GetValue() const { return m_Value; }

    protected:
        bool ParseValue(std::string_view val) override;

    private:
        std::vector<std::string> m_Value;

        bool m_AcceptEmptyValues = false;
    };

    class StrArgument final :
        public IPositionalArgument
    {
    public:
        using IPositionalArgument::IPositionalArgument;

        StrArgument(
                IArgParser& parser,
                std::string_view metaVar,
                std::string_view description,
                std::string_view defaultValue) :
            IPositionalArgument(parser, metaVar, description),
            m_HasDefault(true),
            m_Default(defaultValue)
        {}

        virtual ~StrArgument() = default;

        ARGUE_DELETE_MOVE_COPY(StrArgument)

    public:
        bool HasDefaultValue() const override { return m_HasDefault; }
        bool IsVariadic() const override { return false; }

        const std::string& GetDefaultValue() const { return m_Default; }

        const std::string& operator*() const { return GetValue(); }
        const std::string& GetValue() const
        {
            if (WasParsed()) return m_Value;
            return m_Default;
        }

    protected:
        bool ParseArg(std::string_view arg) override;

    private:
        std::string m_Value;

        bool m_HasDefault = false;
        std::string m_Default = "";
    };

    class StrVarArgument final :
        public IPositionalArgument
    {
    public:
        using IPositionalArgument::IPositionalArgument;
        virtual ~StrVarArgument() = default;

        ARGUE_DELETE_MOVE_COPY(StrVarArgument)

    public:
        bool HasDefaultValue() const override { return true; }
        bool IsVariadic() const override { return true; }

        const std::vector<std::string>& operator*() const { return GetValue(); }
        const std::vector<std::string>& GetValue() const  { return m_Value; }

    protected:
        bool ParseArg(std::string_view arg) override;

    private:
        std::vector<std::string> m_Value;
    };

    class HelpCommand
    {
    public:
        HelpCommand(IArgParser& parser) :
            m_Parser(parser),
            m_Command(m_Parser, "help", "Prints this help message."),
            m_PrintType(m_Command, "print", "P", "TYPE", "Print all subcommands and their options. (default: brief)", {"brief", "full"}, 0),
            m_HelpFor(m_Command, "CMD", "The path to the command to print the help message for.")
        {}

        operator bool() const { return m_Command; }
        void operator()(ITextBuilder& help) const;

    private:
        IArgParser& m_Parser;
        CommandParser m_Command;
        ChoiceOption m_PrintType;
        StrVarArgument m_HelpFor;
    };
}

#endif // _ARGUE_HPP

#ifdef ARGUE_IMPLEMENTATION

void Argue::TextBuilder::PutText(std::string_view text)
{
    std::string_view::size_type newLineIdx;
    while ((newLineIdx = text.find('\n')) != std::string_view::npos) {
        PutLine(text.substr(0, newLineIdx));
        text.remove_prefix(newLineIdx+1);
        NewLine();
    }
    PutLine(text);
}

void Argue::TextBuilder::NewLine()
{
    if (m_CurrentLine.length() > 0) {
        if (!m_Text.empty()) m_Text += '\n';

        while (m_CurrentLine.length() > 0 && IsSpace(m_CurrentLine.back()))
            m_CurrentLine.pop_back();
        m_Text += m_CurrentLine;

        m_CurrentLineIndentLength = 0;
        m_CurrentLine.clear();
    }
}

void Argue::TextBuilder::Spacer()
{
    NewLine();
    if (!m_Text.ends_with('\n'))
        m_Text += '\n';
}

void Argue::TextBuilder::Indent()
{
    ++m_IndentationLevel;
}

void Argue::TextBuilder::DeIndent()
{
    if (m_IndentationLevel > 0)
        --m_IndentationLevel;
}

std::string Argue::TextBuilder::Build()
{
    std::string result;
    if (m_Text.empty()) {
        result += m_CurrentLine;
    } else {
        result += m_Text;
        result += '\n';
        result += m_CurrentLine;
    }

    while (!result.empty() && IsSpace(result.back()))
        result.pop_back();
    result += '\n';

    return result;
}

void Argue::TextBuilder::PutLine(std::string_view text)
{
    while (true) {
        size_t currentLineWidth = m_CurrentLine.length() - m_CurrentLineIndentLength;
        bool hasWrapped = !m_CurrentLine.empty()    &&
            currentLineWidth >= m_MaxParagraphWidth &&
            (
                IsSpace(m_CurrentLine.back()) ||
                (!text.empty() && IsSpace(text[0]))
            );

        if (hasWrapped) NewLine();
        PutLineIndent(hasWrapped);

        std::string_view::size_type nextWordIdx = text.find_first_of(SPACE_CHARS);
        if (nextWordIdx == std::string_view::npos) {
            m_CurrentLine += text;
            break;
        }

        if (nextWordIdx > 0) {
            m_CurrentLine += text.substr(0, nextWordIdx);
            m_CurrentLine += ' ';
        } else if (!hasWrapped) {
            m_CurrentLine += ' ';
        }
        text.remove_prefix(nextWordIdx+1);
    }
}

void Argue::TextBuilder::PutLineIndent(bool hasWrapped)
{
    if (m_CurrentLine.empty()) {
        m_CurrentLineIndentLength = m_IndentationLevel * m_Indent.length();
        if (hasWrapped && m_IndentOnWrap) {
            m_CurrentLineIndentLength += m_Indent.length();
            m_CurrentLine += m_Indent;
        }

        for (size_t i = 0; i < m_IndentationLevel; ++i)
            m_CurrentLine += m_Indent;
    }
}

Argue::IOption::IOption(
        IArgParser& parser,
        std::string_view name,
        std::string_view shortName,
        std::string_view metaVar,
        std::string_view description) :
    m_Parser(parser),
    m_Name(name),
    m_ShortName(shortName),
    m_MetaVar(metaVar),
    m_Description(description)
{
    m_Parser.AddOption(*this);
}

void Argue::IOption::WriteHint(ITextBuilder& hint) const
{
    if (HasMetaVar()) {
        const char VAR_OPEN  = IsVarOptional() ? '[' : '<';
        const char VAR_CLOSE = IsVarOptional() ? ']' : '>';

        if (m_Parser.HasShortPrefix() && HasShortName()) {
            hint.PutText(s(
                m_Parser.GetPrefix(), GetName(), '=', VAR_OPEN, GetMetaVar(), VAR_CLOSE, ", ",
                m_Parser.GetShortPrefix(), GetShortName(), VAR_OPEN, GetMetaVar(), VAR_CLOSE
            ));
        } else {
            hint.PutText(s(
                m_Parser.GetPrefix(), GetName(), '=', VAR_OPEN, GetMetaVar(), VAR_CLOSE
            ));
        }
    } else {
        if (m_Parser.HasShortPrefix() && HasShortName()) {
            hint.PutText(s(
                m_Parser.GetPrefix(), GetName(), ", ",
                m_Parser.GetShortPrefix(), GetShortName()
            ));
        } else {
            hint.PutText(s(
                m_Parser.GetPrefix(), GetName()
            ));
        }
    }
}

void Argue::IOption::WriteHelp(ITextBuilder& help) const
{
    WriteHint(help);
    if (HasDescription()) {
        help.NewLine();
        help.Indent();
        help.PutText(GetDescription());
        help.DeIndent();
    }
}

bool Argue::IOption::ParseArg(std::string_view arg, bool isShort)
{
    if (!ConsumeName(arg, isShort))
        return false;

    if (!isShort && HasMetaVar()) {
        if (!arg.starts_with('='))
            return false;
        arg.remove_prefix(1);
    }

    return ParseValue(arg);
}

bool Argue::IOption::SetError(std::string&& errorMessage)
{
    return m_Parser.SetError(std::forward<std::string>(errorMessage));
}

bool Argue::IOption::ConsumeName(std::string_view& arg, bool isShort)
{
    if (isShort) {
        if (!HasShortName() || !arg.starts_with(GetShortName()))
            return false;
        arg.remove_prefix(GetShortName().length());
        return true;
    }

    if (!arg.starts_with(GetName()))
        return false;
    arg.remove_prefix(GetName().length());
    return true;
}

Argue::IPositionalArgument::IPositionalArgument(IArgParser& parser, std::string_view metaVar, std::string_view description) :
    m_Parser(parser),
    m_MetaVar(metaVar),
    m_Description(description)
{
    m_Parser.AddArgument(*this);
}

void Argue::IPositionalArgument::WriteHint(ITextBuilder& hint) const
{
    if (HasDefaultValue()) {
        if (IsVariadic()) {
            hint.PutText(s("[...", GetMetaVar(), ']'));
        } else{
            hint.PutText(s('[', GetMetaVar(), ']'));
        }
    } else {
        hint.PutText(s('<', GetMetaVar(), '>'));
    }
}

void Argue::IPositionalArgument::WriteHelp(ITextBuilder& help) const
{
    if (IsVariadic()) {
        help.PutText(s("...", GetMetaVar(), ':'));
    } else {
        help.PutText(s(GetMetaVar(), ':'));
    }
    if (HasDescription()) {
        help.NewLine();
        help.Indent();
        help.PutText(GetDescription());
        help.DeIndent();
    }
}

bool Argue::IPositionalArgument::SetError(std::string&& errorMessage)
{
    return m_Parser.SetError(std::forward<std::string>(errorMessage));
}

void Argue::IArgParser::WriteHint(ITextBuilder& hint) const
{
    if (m_Commands.size() > 0) {
        std::string subcommands = m_Commands[0]->GetName();
        for (size_t i = 1; i < m_Commands.size(); ++i) {
            subcommands += '|';
            subcommands += m_Commands[i]->GetName();
        }

        hint.PutText(GetName());
        if (m_Options.size() > 0) {
            hint.PutText(" [...OPTIONS]");
        }
        hint.PutText(s(" [", subcommands, " ...]"));
    } else {
        hint.PutText(GetName());
        if (m_Options.size() > 0) {
            hint.PutText(" [...OPTIONS]");
        }
    }
    if (m_Arguments.size() > 0) {
        hint.PutText(" [--]");
        for (const IPositionalArgument* arg : m_Arguments) {
            hint.PutText(" ");
            arg->WriteHint(hint);
        }
    }
}

void Argue::IArgParser::WriteHelp(ITextBuilder& help, bool briefOptions, bool briefSubcommands) const
{
    WriteHint(help);
    help.Spacer();

    if (HasDescription()) {
        help.Indent();
        help.PutText(GetDescription());
        help.DeIndent();
        help.Spacer();
    }

    for (const IPositionalArgument* arg : m_Arguments) {
        if (arg->HasDescription()) {
            arg->WriteHelp(help);
            help.Spacer();
        }
    }

    if (m_Options.size() > 0) {
        help.PutText("OPTIONS:");
        help.NewLine();
        if (briefOptions) {
            for (const IOption* opt : m_Options) {
                help.Indent();
                opt->WriteHint(help);
                help.DeIndent();
                help.NewLine();
            }
            help.Spacer();
        } else {
            for (const IOption* opt : m_Options) {
                help.Indent();
                opt->WriteHelp(help);
                help.DeIndent();
                help.Spacer();
            }
        }
    }

    if (m_Commands.size() > 0) {
        help.PutText("SUBCOMMANDS:");
        help.NewLine();
        if (briefSubcommands) {
            for (const IArgParser* cmd : m_Commands) {
                help.Indent();
                cmd->WriteHint(help);
                help.DeIndent();
                help.NewLine();
            }
            help.Spacer();
        } else {
            for (const IArgParser* cmd : m_Commands) {
                help.Indent();
                cmd->WriteHelp(help, briefOptions, briefSubcommands);
                help.DeIndent();
                help.Spacer();
            }
        }
    }
}

bool Argue::IArgParser::Parse(std::stack<std::string_view> args)
{
    if (args.top() != GetName())
        return false;
    args.pop();

    const bool arePrefixesTheSame = GetShortPrefix() == GetPrefix();

    m_WasUsed = true;
    bool isParsingPositionals = false;
    size_t positionalIdx = 0;
    for (; !args.empty(); args.pop()) {
        std::string_view fullArg = args.top();
        std::string_view arg = fullArg;

        bool isShortPrefix = false;
        if (isParsingPositionals) {
            if (positionalIdx >= m_Arguments.size())
                return SetError(s("Unexpected positional argument '", fullArg, "'."));
            IPositionalArgument* positional = m_Arguments[positionalIdx];
            if (!positional->Parse(arg))
                return false;
            if (!positional->IsVariadic())
                ++positionalIdx;
            continue;
        } else if (arg == "--") {
            isParsingPositionals = true;
            continue;
        } else if (arg.starts_with(GetPrefix())) {
            arg.remove_prefix(GetPrefix().length());
            isShortPrefix = false;
        } else if (HasShortPrefix() && arg.starts_with(GetShortPrefix())) {
            arg.remove_prefix(GetShortPrefix().length());
            isShortPrefix = true;
        } else {
            // Try Parse Commands
            for (IArgParser* cmd : m_Commands) {
                if (cmd->Parse(args))
                    return true;
                if (cmd->HasError())
                    return false;
            }

            isParsingPositionals = true;
            if (positionalIdx >= m_Arguments.size())
                return SetError(s("Unexpected positional argument '", fullArg, "'."));
            IPositionalArgument* positional = m_Arguments[positionalIdx];
            if (!positional->Parse(arg))
                return false;
            if (!positional->IsVariadic())
                ++positionalIdx;
            continue;
        }

        bool hasParsedOption = false;
        for (IOption* opt : m_Options) {
            if (opt->Parse(arg, isShortPrefix)) {
                hasParsedOption = true;
                break;
            }

            if (HasError())
                return false;

            if (arePrefixesTheSame) {
                if (opt->Parse(arg, !isShortPrefix)) {
                    hasParsedOption = true;
                    break;
                }

                if (HasError())
                    return false;
            }
        }

        if (!hasParsedOption) {
            return SetError(s("Unknown option '", fullArg, "'."));
        }
    }

    for (const IOption* opt : m_Options) {
        if (!opt->HasValue())
            return SetError(s("Missing option '", GetPrefix(), opt->GetName(), "'."));
    }

    for (const IPositionalArgument* arg : m_Arguments) {
        if (!arg->HasValue())
            return SetError(s("Missing argument '", arg->GetMetaVar(), "'."));
    }

    return !HasError();
}

void Argue::FlagOption::WriteHint(ITextBuilder& hint) const
{
    const IArgParser& parser = GetParser();
    if (parser.HasShortPrefix() && HasShortName()) {
        hint.PutText(s(parser.GetPrefix(), GetName(), ", ", parser.GetShortPrefix(), GetShortName()));
    } else {
        hint.PutText(s(parser.GetPrefix(), GetName()));
    }
}

void Argue::FlagOption::WriteHelp(ITextBuilder& help) const
{
    const IArgParser& parser = GetParser();

    WriteHint(help);
    help.PutText(", ");
    if (parser.HasShortPrefix() && HasShortName())
        help.NewLine();
    help.PutText(s(GetParser().GetPrefix(), "no-", GetName()));

    if (HasDescription()) {
        help.NewLine();
        help.Indent();
        help.PutText(GetDescription());
        help.DeIndent();
    }
}

bool Argue::FlagOption::ParseArg(std::string_view arg, bool isShort)
{
    if (ConsumeName(arg, isShort)) {
        if (!arg.empty())
            return false;
        SetValue(true);
        return true;
    }
    
    if (isShort)
        return false;

    if (arg.starts_with("no-")) {
        arg.remove_prefix(3);
        if (arg != GetName())
            return false;
        SetValue(false);
        return true;
    }

    return false;
}

bool Argue::IntOption::ParseValue(std::string_view val)
{
    int64_t value;
    auto result = std::from_chars(&val.front(), &val.back()+1, value, 10);
    if (result.ptr != &val.back()+1)
        return SetError(s("Expected integer for '", GetParser().GetPrefix(), GetName(), "', got '", val, "'."));

    m_Value = value;
    return true;
}

bool Argue::StrOption::ParseValue(std::string_view val)
{
    m_Value = val;
    return true;
}

void Argue::ChoiceOption::WriteHint(ITextBuilder& hint) const
{
    const IArgParser& parser = GetParser();
    if (parser.HasShortPrefix() && HasShortName()) {
        hint.PutText(s(
            parser.GetPrefix(), GetName(), '=', GetChoiceString(), ", ",
            parser.GetShortPrefix(), GetShortName(), GetChoiceString()
        ));
    } else {
        hint.PutText(s(
            parser.GetPrefix(), GetName(), '=', GetChoiceString()
        ));
    }
}

bool Argue::ChoiceOption::ParseValue(std::string_view val)
{
    for (size_t i = 0; i < m_Choices.size(); ++i) {
        if (m_Choices[i] == val) {
            m_ValueIdx = i;
            return true;
        }
    }

    return SetError(s("Expected one of ", GetChoiceString(), " for '", GetParser().GetPrefix(), GetName(), "', got '", val, "'."));
}

std::string Argue::ChoiceOption::GetChoiceString() const
{
    std::string result = "{";
    for (const auto& choice : m_Choices) {
        result += choice;
        result += ',';
    }
    if (result.back() == ',')
        result.back() = '}';
    else result += '}';
    return result;
}

bool Argue::CollectionOption::ParseValue(std::string_view val)
{
    if (!m_AcceptEmptyValues && val.empty()) {
        return SetError(s(
            "Empty values are not allowed for '", GetParser().GetPrefix(), GetName(), "'."
        ));
    }
    m_Value.emplace_back(val);
    return true;
}

bool Argue::StrArgument::ParseArg(std::string_view arg)
{
    m_Value = arg;
    return true;
}

bool Argue::StrVarArgument::ParseArg(std::string_view arg)
{
    m_Value.emplace_back(arg);
    return true;
}

void Argue::HelpCommand::operator()(ITextBuilder& help) const
{
    std::string pathUntilLast;
    std::stack<std::string_view> helpPath;
    {
        const auto& rawPath = *m_HelpFor;
        for (size_t i = rawPath.size(); i > 0; --i)
            helpPath.emplace(rawPath[i-1]);
    }

    const IArgParser* currentSubcommand = &m_Parser;
    while (!helpPath.empty()) {
        std::string_view cmdToMatch = helpPath.top();
        bool hasFoundCommand = false;
        for (const IArgParser* sub : currentSubcommand->GetSubCommands()) {
            if (sub->GetName() == cmdToMatch) {
                hasFoundCommand = true;
                currentSubcommand = sub;
                break;
            }
        }

        if (!hasFoundCommand) {
            help.PutText(s("Could not find help for '", pathUntilLast, cmdToMatch, "'."));
            help.NewLine();
            return;
        }

        helpPath.pop();
        if (!helpPath.empty()) {
            pathUntilLast += cmdToMatch;
            pathUntilLast += " ";
        }
    }

    bool briefOptions = false;
    bool briefSubcommands = *m_PrintType == "brief";

    help.PutText(pathUntilLast);
    currentSubcommand->WriteHelp(help, briefOptions, briefSubcommands);
}

#endif // ARGUE_IMPLEMENTATION
