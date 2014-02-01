// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#include "Support/StringRef.h"
#include "Support/StringRefStream.h"
#include "Support/Utility.h"

#include <functional>
#include <iomanip>
#include <map>
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
    Prefix,                 // Must this option directly prefix its value?
    MayPrefix,              // Can this option directly prefix its value?
    Grouping,               // Can this option group with other options?
    Positional,             // Is a positional argument, no '-' required
};

enum MiscFlags : unsigned char {
    None                    = 0,
    CommaSeparated          = 0x01, // Should this list split between commas?
    Hidden                  = 0x02, // Do not show this option in the usage
    ConsumeAfter            = 0x04, // Handle all following arguments as positional arguments
};

//--------------------------------------------------------------------------------------------------
// CmdLine
//

class OptionBase;
class OptionGroup;

class CmdLine
{
    friend class OptionBase;
    friend class OptionGroup;

public:
    using OptionMap = std::map<StringRef, OptionBase*>;
    using OptionGroups = std::map<StringRef, OptionGroup*>;
    using OptionVector = std::vector<OptionBase*>;
    using ConstOptionVector = std::vector<OptionBase const*>;
    using StringVector = std::vector<std::string>;

private:
    // List of options
    OptionMap options_;
    // List of option groups and associated options
    OptionGroups groups_;
    // List of positional options
    OptionVector positionals_;
    // The length of the longest prefix option
    size_t maxPrefixLength_;

public:
    // Constructor.
    CmdLine();

    // Destructor.
    ~CmdLine();

    // Adds the given option to the command line
    void add(OptionBase* opt);

    // Parse the given command line arguments
    void parse(StringVector const& argv);

    // Expand response files and parse the command line arguments
    void expandAndParse(StringVector argv);

    // Returns the list of (unique) options, sorted by name.
    ConstOptionVector options() const;

    // Returns a list of the positional options.
    ConstOptionVector positionals() const;

    // Check if all required options have been specified
    void check();

private:
    OptionBase* findOption(StringRef name) const;

    void expandResponseFile(StringVector& argv, size_t i);
    void expandResponseFiles(StringVector& argv);

    void handleArg(bool& dashdash, StringVector const& argv, size_t& i, OptionVector::iterator& pos);

    void handlePositional(StringRef curr, StringVector const& argv, size_t& i, OptionVector::iterator& pos);

    bool handleOption(StringRef curr, StringVector const& argv, size_t& i);
    bool handlePrefix(StringRef curr, size_t i);
    bool handleGroup(StringRef curr, StringVector const& argv, size_t& i);

    void addOccurrence(OptionBase* opt, StringRef name, StringVector const& argv, size_t& i);
    void addOccurrence(OptionBase* opt, StringRef name, StringRef arg, size_t i);

    void parse(OptionBase* opt, StringRef name, StringRef arg, size_t i);

    void check(OptionBase const* opt);
};

//--------------------------------------------------------------------------------------------------
// ArgName
//

struct ArgName
{
    std::string value;

    explicit ArgName(std::string value) : value(std::move(value))
    {
    }
};

//--------------------------------------------------------------------------------------------------
// Desc
//

struct Desc
{
    std::string value;

    explicit Desc(std::string x) : value(std::move(x))
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

    explicit Initializer(T x) : value(std::forward<T>(x))
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
    void operator()(StringRef name, StringRef arg, size_t /*i*/, T& value) const
    {
        StringRefStream stream(arg);

        if (!(stream >> std::setbase(0) >> value) || !stream.eof())
        {
            throw std::runtime_error("invalid argument '" + arg + "' for option '" + name + "'");
        }
    }
};

template <>
struct Parser<bool>
{
    void operator()(StringRef name, StringRef arg, size_t /*i*/, bool& value) const
    {
        if (arg.empty() || arg == "1" || arg == "true" || arg == "on")
            value = true;
        else if (arg == "0" || arg == "false" || arg == "off")
            value = false;
        else
            throw std::runtime_error("invalid argument '" + arg + "' for option '" + name + "'");
    }
};

template <>
struct Parser<std::string>
{
    void operator()(StringRef /*name*/, StringRef arg, size_t /*i*/, std::string& value) const {
        value.assign(arg.data(), arg.size());
    }
};

template <class P>
std::vector<StringRef> allowedValues(P const& /*parser*/) {
    return {};
}

template <class P>
std::vector<StringRef> allowedValues(std::reference_wrapper<P> const& parser) {
    return allowedValues(parser.get());
}

template <class P>
std::vector<StringRef> descriptions(P const& /*parser*/) {
    return {};
}

template <class P>
std::vector<StringRef> descriptions(std::reference_wrapper<P> const& parser) {
    return descriptions(parser.get());
}

//--------------------------------------------------------------------------------------------------
// MapParser
//

template <class T>
struct MapParser
{
    using MapType = std::map<std::string, std::pair<T, std::string>>;
    using MapValueType = typename MapType::value_type;

    struct Init : MapValueType
    {
        Init(std::string name, T value, std::string desc = "")
            : MapValueType{std::move(name), {std::move(value), std::move(desc)}}
        {
            assert(!this->first.empty() && "empty keys are not allowed");
        }
    };

    MapType map;

    explicit MapParser(std::initializer_list<Init> ilist)
        : map(ilist.begin(), ilist.end())
    {
    }

    void operator()(StringRef name, StringRef arg, size_t /*i*/, T& value) const
    {
        // If the arg is empty, the option is specified by name
        auto I = map.find(arg.empty() ? name.str() : arg.str());

        if (I == map.end())
            throw std::runtime_error("invalid argument '" + arg + "' for option '" + name + "'");

        value = I->second.first;
    }
};

template <class T>
std::vector<StringRef> allowedValues(MapParser<T> const& parser)
{
    std::vector<StringRef> vec;

    for (auto const& I : parser.map)
        vec.emplace_back(I.first);

    return vec;
}

template <class T>
std::vector<StringRef> descriptions(MapParser<T> const& parser)
{
    std::vector<StringRef> vec;

    for (auto const& I : parser.map)
        vec.emplace_back(I.second.second);

    return vec;
}

//--------------------------------------------------------------------------------------------------
// Traits
//

template <class ElementT, class InserterT>
struct TraitsBase
{
    using element_type = ElementT;
    using inserter_type = InserterT;
};

namespace details
{
    struct Any {
        template <class... T> Any(T&&...) {}
    };

    struct Inserter
    {
        template <class C, class V>
        void operator()(C& c, V&& v) const {
            c.insert(c.end(), std::forward<V>(v));
        }
    };

    template <class T>
    inline auto HasInsert(T&& t)
        -> decltype(Inserter()(t, std::declval<typename T::value_type>()), std::true_type());

    inline auto HasInsert(Any)
        -> std::false_type;

    template <class T, class = decltype(HasInsert(std::declval<T>()))>
    struct DefaultTraits
        : TraitsBase<RemoveCVRec<typename T::value_type>, Inserter>
    {
    };

    template <class T>
    struct DefaultTraits<T, std::false_type>
        : TraitsBase<T, void>
    {
    };
}

template <class T>
struct Traits : details::DefaultTraits<T>
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
// OptionGroup
//

class OptionGroup
{
    friend class OptionBase;

public:
    enum Type {
        Default,    // No restrictions (zero or more...)
        Zero,       // No options in this group may be specified
        ZeroOrOne,  // At most one option of this group must be specified
        One,        // Exactly one option of this group must be specified
        OneOrMore,  // At least one option must be specified
        All,        // All options of this group must be specified
        ZeroOrAll,  // If any option in this group is specified, the others must be specified, too.
    };

public:
    explicit OptionGroup(CmdLine& cmd, std::string name, Type type = Default);

    // Checks whether this group is valid
    void check() const;

private:
    // The name of this option group
    std::string name_;
    // The type of this group
    Type type_;
    // The list of options in this group
    std::vector<OptionBase*> options_;
};

//--------------------------------------------------------------------------------------------------
// OptionBase
//

class OptionBase
{
    friend class CmdLine;

    // The name of this option
    std::string name_;
    // The name of the value of this option
    std::string argName_;
    // The help message for this option
    std::string desc_;
    // Controls how often the option must/may be specified on the command line
    NumOccurrences numOccurrences_;
    // Controls whether the option expects a value
    NumArgs numArgs_;
    // Controls how the option might be specified
    Formatting formatting_;
    // Other flags
    MiscFlags miscFlags_;
    // The number of times this option was specified on the command line
    unsigned count_;

protected:
    // Constructor.
    OptionBase();

public:
    // Destructor.
    virtual ~OptionBase();

public:
    // Returns the name of this option
    std::string const& name() const {
        return name_;
    }

    // Return name of the value
    std::string const& argName() const {
        return argName_;
    }

    // Resturns the description of this option
    std::string const& desc() const {
        return desc_;
    }

    // Returns how often the option must/may be specified on the command line
    NumOccurrences numOccurrences() const {
        return numOccurrences_;
    }

    // Returns whether the option expects a value
    NumArgs numArgs() const {
        return numArgs_;
    }

    // Returns how the option might be specified
    Formatting formatting() const {
        return formatting_;
    }

    // Returns other flags
    MiscFlags flags() const {
        return miscFlags_;
    }

    // Returns the number of times this option has been specified on the command line
    unsigned count() const {
        return count_;
    }

    // Returns whether this option may be specified multiple times.
    bool isUnbounded() const;

    // Returns whether this option must be specified on the command line
    bool isRequired() const;

    // Returns whether this is a Prefix or MayPrefix option.
    bool isPrefix() const;

    // Returns a list of allowed values for this option
    virtual std::vector<StringRef> allowedValues() const = 0;

    // Returns a list of descriptions for any allowed value for this option
    virtual std::vector<StringRef> descriptions() const = 0;

protected:
    void apply(std::string x)       { name_ = std::move(x); }
    void apply(ArgName x)           { argName_ = std::move(x.value); }
    void apply(Desc x)              { desc_ = std::move(x.value); }
    void apply(NumOccurrences x)    { numOccurrences_ = x; }
    void apply(NumArgs x)           { numArgs_ = x; }
    void apply(Formatting x)        { formatting_ = x; }
    void apply(MiscFlags x)         { miscFlags_ = static_cast<MiscFlags>(miscFlags_ | x); }

    void apply(OptionGroup& x) {
        x.options_.push_back(this); // FIXME: Check for duplicates
    }

    template <class U>
    void apply(Initializer<U>) {
        // NOTE: this is handled in the ctors of BasicOption...
    }

    template <class A, class... An>
    void applyRec(A&& a, An&&... an)
    {
        apply(std::forward<A>(a));
        applyRec(std::forward<An>(an)...);
    }

    template <class... An>
    void applyRec(CmdLine& cmd, An&&... an)
    {
        applyRec(std::forward<An>(an)...);
        cmd.add(this);
    }

    void applyRec(); // End recursion - check for valid flags

private:
    // Returns the name of this option for use in error messages
    StringRef displayName() const;

    bool isOccurrenceAllowed() const;
    bool isOccurrenceRequired() const;

    // Parses the given value and stores the result.
    virtual void parse(StringRef spec, StringRef value, size_t i) = 0;
};

//--------------------------------------------------------------------------------------------------
// BasicOption<T>
//

template <class T>
class BasicOption : public OptionBase
{
    using BaseType = OptionBase;

    T value_;

protected:
    BasicOption()
        : BaseType()
        , value_{} // NOTE: error here if T is a reference type and not initialized using init(T)
    {
    }

    template <class... An, class X = T, class = EnableIf<std::is_reference<X>>>
    BasicOption(Initializer<T> x, An&&...)
        : BaseType()
        , value_(x)
    {
    }

    template <class U, class... An, class X = T, class = DisableIf<std::is_reference<X>>>
    BasicOption(Initializer<U> x, An&&...)
        : BaseType()
        , value_(x)
    {
    }

    template <class A, class... An>
    BasicOption(A&&, An&&... an)
        : BasicOption(std::forward<An>(an)...)
    {
    }

public:
    using value_type = RemoveReference<T>;

    // Returns the value
    value_type& value() {
        return value_;
    }

    // Returns the value
    value_type const& value() const {
        return value_;
    }

    // Returns a pointer to the value
    value_type* operator->() {
        return std::addressof(value_);
    }

    // Returns a pointer to the value
    value_type const* operator->() const {
        return std::addressof(value_);
    }
};

//--------------------------------------------------------------------------------------------------
// InitParser
//

template <class T>
struct InitParser
{
    T value;

    explicit InitParser(T x) : value(std::forward<T>(x))
    {
    }

    operator T() { // extract
        return std::forward<T>(value);
    }
};

template <class T>
inline auto initParser(T&& value) -> InitParser<T&&>
{
    return InitParser<T&&>(std::forward<T>(value));
}

//--------------------------------------------------------------------------------------------------
// DefaultInitParser
//

struct DefaultInitParser {};

//--------------------------------------------------------------------------------------------------
// Option
//

template <class T, class TraitsT = Traits<T>, class ParserT = Parser<typename TraitsT::element_type>>
class Option : public BasicOption<T>
{
    using BaseType = BasicOption<T>;
    using StringRefVector = std::vector<StringRef>;

    ParserT parser_;

public:
    using parser_type = ParserT;
    using traits_type = TraitsT;
    using element_type = typename traits_type::element_type;
    using inserter_type = typename traits_type::inserter_type;

private:
    using IsScalar = typename std::is_void<inserter_type>::type;

    static_assert(IsScalar::value || std::is_default_constructible<element_type>::value,
        "elements of containers must be default constructible");

public:
    template <class... Args>
    explicit Option(DefaultInitParser, Args&&... args)
        : BaseType(std::forward<Args>(args)...)
    {
        this->applyRec(IsScalar::value ? Optional : ZeroOrMore, std::forward<Args>(args)...);
    }

    template <class P, class... Args>
    explicit Option(InitParser<P> p, Args&&... args)
        : BaseType(std::forward<Args>(args)...)
        , parser_(p)
    {
        this->applyRec(IsScalar::value ? Optional : ZeroOrMore, std::forward<Args>(args)...);
    }

    // Returns the parser
    parser_type& parser() {
        return parser_;
    }

    // Returns the parser
    parser_type const& parser() const {
        return parser_;
    }

    // Returns a list of allowed values for this option
    virtual std::vector<StringRef> allowedValues() const override final
    {
        using cl::allowedValues;
        return allowedValues(parser());
    }

    // Returns a list of descriptions for any allowed value for this option
    virtual std::vector<StringRef> descriptions() const override final
    {
        using cl::descriptions;
        return descriptions(parser());
    }

private:
    void parse(StringRef spec, StringRef value, size_t i, std::false_type)
    {
        element_type t;

        // Parse...
        parser_(spec, value, i, t);

        // and insert into the container
        inserter_type()(this->value(), std::move(t));
    }

    void parse(StringRef spec, StringRef value, size_t i, std::true_type) {
        parser_(spec, value, i, this->value());
    }

    virtual void parse(StringRef spec, StringRef value, size_t i) override final {
        parse(spec, value, i, IsScalar());
    }
};

//--------------------------------------------------------------------------------------------------
// make[Scalar]Option
//

// Construct a new Option with a default constructed parser
template <class T, class... Args>
auto makeOption(Args&&... args)
    -> Option<T>
{
    return Option<T>(DefaultInitParser(), std::forward<Args>(args)...);
}

// Construct a new Option with a default constructed parser
template <class T, class... Args>
auto makeScalarOption(Args&&... args)
    -> Option<T, TraitsBase<T, void>>
{
    return Option<T, TraitsBase<T, void>>(DefaultInitParser(), std::forward<Args>(args)...);
}

// Construct a new Option, initialize the parser with the given value
template <class T, class P, class... Args>
auto makeOption(InitParser<P> p, Args&&... args)
    -> Option<T, Traits<T>, Decay<P>>
{
    return Option<T, Traits<T>, Decay<P>>(std::move(p), std::forward<Args>(args)...);
}

// Construct a new Option, initialize the parser with the given value
template <class T, class P, class... Args>
auto makeScalarOption(InitParser<P> p, Args&&... args)
    -> Option<T, TraitsBase<T, void>, Decay<P>>
{
    return Option<T, TraitsBase<T, void>, Decay<P>>(std::move(p), std::forward<Args>(args)...);
}

} // namespace cl
} // namespace support

#ifdef _MSC_VER
#pragma warning(pop)
#endif
