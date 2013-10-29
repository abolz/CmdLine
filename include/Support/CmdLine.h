// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#include "StringRef.h"
#include "Utility.h"

#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4512) // assignment operator could not be generated
#endif

namespace support
{
namespace cl
{

//--------------------------------------------------------------------------------------------------
// Option flags
//

// Flags for the number of occurrences allowed
enum NumOccurrences : unsigned char {
    Optional,               // Zero or one occurrence allowed
    ZeroOrMore,             // Zero or more occurrences allowed
    Required,               // Exactly one occurrence required
    OneOrMore,              // One or more occurrences required
};

// Is a value required for the option?
enum NumArgs : unsigned char {
    ArgOptional,            // A value can appear... or not
    ArgRequired,            // A value is required to appear!
    ArgDisallowed,          // A value may not be specified (for flags)
};

// This controls special features that the option might have that cause it to be
// parsed differently...
enum Formatting : unsigned char {
    DefaultFormatting,      // Nothing special
    Prefix,                 // Can this option directly prefix its value?
    StrictPrefix,           // Must this option directly prefix its value?
    Grouping,               // Can this option group with other options?
    Positional,             // Is a positional argument, no '-' required
};

enum MiscFlags : unsigned char {
    None = 0,
    CommaSeparated = 0x01,  // Should this list split between commas?
    Hidden = 0x02,          // Do not show this option in the usage
};

//--------------------------------------------------------------------------------------------------
// CmdLine
//

class OptionBase;

class CmdLine
{
    friend class OptionBase;

public:
    using OptionMap     = std::map<StringRef, OptionBase*>;
    using OptionVector  = std::vector<OptionBase*>;
    using StringVector  = std::vector<std::string>;

private:
    // The name of the program
    std::string program;
    // Additional text displayed in the help menu
    std::string overview;
    // List of options
    OptionMap options;
    // List of positional options
    OptionVector positionals;
    // List of error message
    StringVector errors;

public:
    explicit CmdLine(std::string programName, std::string overview = "");

    // Adds the given option to the command line
    bool add(OptionBase* opt);

    // Parse the given command line arguments
    bool parse(StringVector argv, bool ignoreUnknowns = false);

    // Prints the help message
    void help() const;

    // Returns the list of errors
    StringVector const& getErrors() const {
        return errors;
    }

private:
    OptionVector getOptions() const;

    OptionBase* findOption(StringRef name) const;

    bool isPossibleOption(StringRef name) const;

    bool expandResponseFile(StringVector& argv, size_t i);
    bool expandResponseFiles(StringVector& argv);

    bool handlePositional(bool& success, StringRef name, size_t i, OptionVector::iterator& pos);
    bool handleOption(bool& success, StringRef name, size_t& i, StringVector& argv);
    bool handlePrefix(bool& success, StringRef name, size_t i);
    bool handleGroup(bool& success, StringRef name, size_t i);

    bool addOccurrence(OptionBase* opt, StringRef name, StringRef value, size_t i);

    bool check();
    bool check(OptionBase* opt);

    // Adds an error message. Returns false.
    bool error(std::string str);
};

//--------------------------------------------------------------------------------------------------
// Name
//

struct Name
{
    std::string value;

    explicit Name(std::string value)
        : value(std::move(value))
    {
    }
};

//--------------------------------------------------------------------------------------------------
// ArgName
//

struct ArgName
{
    std::string value;

    explicit ArgName(std::string value)
        : value(std::move(value))
    {
    }
};

//--------------------------------------------------------------------------------------------------
// Desc
//

struct Desc
{
    std::string value;

    explicit Desc(std::string x)
        : value(std::move(x))
    {
    }
};

//--------------------------------------------------------------------------------------------------
// Initializer
//

template <class T>
struct Initializer
{
    T value;

    explicit Initializer(T x)
        : value(std::forward<T>(x))
    {
    }

    operator T() { // extract
        return std::forward<T>(value);
    }
};

template <class T>
inline auto init(T&& value) -> Initializer<T&&>
{
    return Initializer<T&&>(std::forward<T>(value));
}

//--------------------------------------------------------------------------------------------------
// Parser
//

template <class T>
struct Parser
{
    bool operator ()(StringRef value, size_t /*i*/, T& result) const
    {
        std::stringstream stream;
        return (stream << value.str()) && (stream >> std::setbase(0) >> result) && stream.eof();
    }
};

template <>
struct Parser<bool>
{
    bool operator ()(StringRef value, size_t /*i*/, bool& result) const
    {
        if (value == "" || value == "1" || value == "true")
            result = true;
        else if (value == "0" || value == "false")
            result = false;
        else
            return false;

        return true;
    }
};

template <>
struct Parser<std::string>
{
    bool operator ()(StringRef value, size_t /*i*/, std::string& result) const
    {
        result.assign(value.data(), value.size());
        return true;
    }
};

template <class BinaryOp>
struct BinaryOpParser
{
    template <class T>
    bool operator ()(StringRef value, size_t i, T& result) const
    {
        T t;

        if (Parser<T>()(value, i, t))
        {
            result = BinaryOp()(result, t);
            return true;
        }

        return false;
    }
};

template <class T, class MapType = std::map<StringRef, T>>
struct MapParser
{
    MapType map;

    explicit MapParser()
    {
    }

    explicit MapParser(std::initializer_list<typename MapType::value_type> ilist)
        : map(ilist)
    {
    }

    bool operator ()(StringRef value, size_t /*i*/, T& result) const
    {
        auto I = map.find(value);

        if (I != map.end())
        {
            result = I->second;
            return true;
        }

        return false;
    }

    auto begin() const -> decltype(mapFirstIterator(map.begin())) {
        return mapFirstIterator(map.begin());
    }

    auto end() const -> decltype(mapFirstIterator(map.end())) {
        return mapFirstIterator(map.end());
    }
};

//--------------------------------------------------------------------------------------------------
// Traits
//

namespace details
{
    struct Inserter
    {
        template <class C, class V>
        void operator ()(C& c, V&& v) const {
            c.insert(c.end(), std::forward<V>(v));
        }
    };

    struct HasInsertImpl
    {
        template <class U>
        static auto test(U&& u)
            -> decltype(u.insert(u.end(), std::declval<typename U::value_type>()), std::true_type());
        static auto test(...) -> std::false_type;
    };

    template <class T>
    using HasInsert = decltype(HasInsertImpl::test(std::declval<T>()));

    template <class T, class = HasInsert<T>>
    struct DefaultValueType {
        using type = T;
    };

    template <class T>
    struct DefaultValueType<T, std::true_type> {
        using type = RemoveCVRec<typename T::value_type>;
    };

    template <class T, class = HasInsert<T>>
    struct DefaultInserter {
        using type = void;
    };

    template <class T>
    struct DefaultInserter<T, std::true_type> {
        using type = Inserter;
    };
}

template <class ValueT, class InserterT = void>
struct TraitsBase
{
    using value_type = ValueT;
    using inserter_type = InserterT;
};

template <class T>
struct Traits
    : TraitsBase<
        typename details::DefaultValueType<T>::type,
        typename details::DefaultInserter<T>::type
      >
{
};

template <class T>
struct Traits<T&> : Traits<T>
{
};

template <>
struct Traits<std::string> : TraitsBase<std::string, void>
{
};

//--------------------------------------------------------------------------------------------------
// OptionBase
//

class OptionBase
{
    friend class CmdLine;

    // The name of this option
    std::string name;
    // The name of the value of this option
    std::string argName;
    // The help message for this option
    std::string desc;
    // Controls how often the option must/may be specified on the command line
    NumOccurrences numOccurrences;
    // Controls whether the option expects a value
    NumArgs numArgs;
    // Controls how the option might be specified
    Formatting formatting;
    // Other flags
    MiscFlags miscFlags;
    // The number of times this option was specified on the command line
    unsigned count;

protected:
    explicit OptionBase();

public:
    // Returns the name of this option
    std::string const& getName() const {
        return name;
    }

    // Return name of the value
    std::string const& getArgName() const {
        return argName;
    }

    // Resturns the description of this option
    std::string const& getDesc() const {
        return desc;
    }

    // Returns the number of times this option has been specified on the command line
    unsigned getCount() const {
        return count;
    }

protected:
    void apply(std::string x)       { name = std::move(x); }
    void apply(Name x)              { name = std::move(x.value); }
    void apply(ArgName x)           { argName = std::move(x.value); }
    void apply(Desc x)              { desc = std::move(x.value); }
    void apply(NumOccurrences x)    { numOccurrences = x; }
    void apply(NumArgs x)           { numArgs = x; }
    void apply(Formatting x)        { formatting = x; }
    void apply(MiscFlags x)         { miscFlags = static_cast<MiscFlags>(miscFlags | x); }

    template <class U>
    void apply(Initializer<U>) {
        // NOTE: this is handled in the ctors of BasicOption...
    }

    void done();

private:
    std::string usage() const;

    void help() const;

    bool isOccurrenceAllowed() const;
    bool isOccurrenceRequired() const;
    bool isUnbounded() const;
    bool isOptional() const;
    bool isPrefix() const;

    // Parses the given value and stores the result.
    virtual bool parse(StringRef value, size_t i) = 0;

    // Returns a list of the values for this option.
    virtual std::vector<StringRef> getValueNames() const = 0;
};

//--------------------------------------------------------------------------------------------------
// BasicOption<T>
//

template <class T>
class BasicOption : public OptionBase
{
    using BaseType = OptionBase;

    T value;

private:
    struct Tag {};

    explicit BasicOption(Tag)
        : BaseType()
        , value{} // NOTE: error here if T is a reference type and not initialized using init(T)
    {
    }

    template <class... An, class X = T, class = EnableIf<std::is_reference<X>>>
    explicit BasicOption(Tag, Initializer<T> x, An&&...)
        : BaseType()
        , value(x)
    {
    }

    template <class U, class... An, class X = T, class = DisableIf<std::is_reference<X>>>
    explicit BasicOption(Tag, Initializer<U> x, An&&...)
        : BaseType()
        , value{ x }
    {
    }

    template <class A, class... An>
    explicit BasicOption(Tag tag, A&&, An&&... an)
        : BasicOption(tag, std::forward<An>(an)...)
    {
    }

protected:
    template <class... An>
    explicit BasicOption(An&&... an)
        : BasicOption(Tag(), std::forward<An>(an)...)
    {
    }

public:
    using stored_type = RemoveReference<T>;

public:
    // Returns the value
    stored_type& get() { return value; }

    // Returns the value
    stored_type const& get() const { return value; }

    // Returns a pointer to the value
    stored_type* operator ->() { return std::addressof(value); }

    // Returns a pointer to the value
    stored_type const* operator ->() const { return std::addressof(value); }

    // Explicitly convert to the stored type
    explicit operator stored_type&() { return get(); }

    // Explicitly convert to the stored type
    explicit operator stored_type const&() const { return get(); }
};

//--------------------------------------------------------------------------------------------------
// Option
//

template <class T, class ParserT = Parser<typename Traits<T>::value_type>>
class Option : public BasicOption<T>
{
    using BaseType = BasicOption<T>;

    ParserT parser;

public:
    using traits_type = Traits<T>;

    using value_type        = typename traits_type::value_type;
    using inserter_type     = typename traits_type::inserter_type;
    using is_scalar         = typename std::is_void<inserter_type>::type;

public:
    struct WithParser {};

    template <class P, class... An>
    explicit Option(WithParser, P&& p, An&&... an)
        : BaseType(std::forward<An>(an)...)
        , parser(std::forward<P>(p))
    {
        applyRec(is_scalar::value ? Optional : ZeroOrMore, std::forward<An>(an)...);
    }

    template <class... An>
    explicit Option(An&&... an)
        : BaseType(std::forward<An>(an)...)
        , parser()
    {
        applyRec(is_scalar::value ? Optional : ZeroOrMore, std::forward<An>(an)...);
    }

    // Returns the parser
    ParserT& getParser() {
        return parser;
    }

    // Returns the parser
    ParserT const& getParser() const {
        return parser;
    }

private:
    // End recursion - check for valid flags
    void applyRec() {
        this->done();
    }

    template <class A, class... An>
    void applyRec(A&& a, An&&... an)
    {
        this->apply(std::forward<A>(a));
        this->applyRec(std::forward<An>(an)...);
    }

    template <class... An>
    void applyRec(CmdLine& cmd, An&&... an)
    {
        this->applyRec(std::forward<An>(an)...);

        if (!cmd.add(this))
            throw std::runtime_error("failed to register option");
    }

    // Used when T is a container type
    bool parse(StringRef value, size_t i, std::false_type)
    {
        value_type t;

        if (parser(value, i, t))
        {
            inserter_type()(this->get(), std::move(t));
            return true;
        }

        return false;
    }

    // Used when T is a value type
    bool parse(StringRef value, size_t i, std::true_type) {
        return parser(value, i, this->get());
    }

    // Parses the given value and stores the result.
    virtual bool parse(StringRef value, size_t i) override {
        return parse(value, i, is_scalar());
    }

    // Used if the parser does not provide begin() and end().
    std::vector<StringRef> getValueNames(std::false_type) const {
        return std::vector<StringRef>();
    }

    // Used if the parser provides begin() and end().
    std::vector<StringRef> getValueNames(std::true_type) const {
        return std::vector<StringRef>(getParser().begin(), getParser().end());
    }

    // Returns a list of the values for this option.
    virtual std::vector<StringRef> getValueNames() const override {
        return getValueNames(HasBeginEnd<ParserT>());
    }
};

// Construct a new Option with a default constructed parser
template <class T, class... An>
auto makeOption(An&&... an) -> Option<T>
{
    return Option<T>(std::forward<An>(an)...);
}

// Construct a new Option, initialize the parser with the given value
template <class T, class P, class... An>
auto makeOptionWithParser(P&& p, An&&... an) -> Option<T, Decay<P>>
{
    using R = Option<T, Decay<P>>;
    return R(typename R::WithParser(), std::forward<P>(p), std::forward<An>(an)...);
}

} // namespace cl
} // namespace support

#ifdef _MSC_VER
#pragma warning(pop)
#endif
