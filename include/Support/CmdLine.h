// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#include "Support/StringRef.h"
#include "Support/StringRefStream.h"
#include "Support/Utility.h"

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
    ConsumeAfter            = 0x02, // Handle all following arguments as positional arguments
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

private:
    OptionBase* findOption(StringRef name) const;

    OptionVector getUniqueOptions() const;

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
    void check();
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
// Initializer
//

namespace details
{
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
}

template <class T>
inline auto init(T&& value) -> details::Initializer<T&&>
{
    return details::Initializer<T&&>(std::forward<T>(value));
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

//--------------------------------------------------------------------------------------------------
// allowedValues
//
// Returns a list of values the given parser can handle.
//
// NOTE:
//
// This function is never called using ADL!
//
// If your custom parser only allows a limited set of values, you can explicitly add an overload in
// the support::cl namespace. Do *NOT* add an overload for any of the types defined in namespace
// support or support::cl!!
//

template <class P>
std::vector<StringRef> allowedValues(P const& /*parser*/)
{
    return {};
}

//--------------------------------------------------------------------------------------------------
// MapParser
//

template <class T>
struct MapParser
{
    using map_type = std::map<std::string, T>;
    using map_value_type = typename map_type::value_type;

    map_type map;

    explicit MapParser(std::initializer_list<map_value_type> ilist)
        : map(ilist)
    {
    }

    void operator()(StringRef name, StringRef arg, size_t /*i*/, T& value) const
    {
        // If the arg is empty, the option is specified by name
        auto I = map.find(arg.empty() ? name.str() : arg.str());

        if (I == map.end())
            throw std::runtime_error("invalid argument '" + arg + "' for option '" + name + "'");

        value = I->second;
    }
};

template <class T>
std::vector<StringRef> allowedValues(MapParser<T> const& p)
{
    std::vector<StringRef> vec;

    for (auto const& I : p.map)
        vec.emplace_back(I.first);

    return vec;
}

//--------------------------------------------------------------------------------------------------
// Traits
//

template <class ElementT, class InserterT = void>
struct BasicTraits
{
    using element_type = ElementT;
    using inserter_type = InserterT;
};

namespace details
{
    struct Inserter
    {
        template <class C, class V>
        void operator()(C& c, V&& v) const {
            c.insert(c.end(), std::forward<V>(v));
        }
    };

    template <class T>
    inline auto HasInsert(T&& t)
        -> decltype(Inserter()(t, std::declval<typename T::value_type>()));

    inline auto HasInsert(AnyType)
        -> NotAType;

    template <class T, class = decltype(HasInsert(std::declval<T>()))>
    struct DefaultTraits
        : BasicTraits<RemoveCVRec<typename T::value_type>, Inserter>
    {
    };

    template <class T>
    struct DefaultTraits<T, NotAType>
        : BasicTraits<T>
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
struct Traits<std::string> : BasicTraits<std::string>
{
};

template <class T>
using ScalarType = BasicTraits<T>;

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

    // Returns the name of this option
    std::string const& name() const { return name_; }

    // Return name of the value
    std::string const& argName() const { return argName_; }

    // Returns the number of times this option has been specified on the command line
    unsigned count() const { return count_; }

protected:
    void apply(std::string x)       { name_ = std::move(x); }
    void apply(ArgName x)           { argName_ = std::move(x.value); }
    void apply(NumOccurrences x)    { numOccurrences_ = x; }
    void apply(NumArgs x)           { numArgs_ = x; }
    void apply(Formatting x)        { formatting_ = x; }
    void apply(MiscFlags x)         { miscFlags_ = static_cast<MiscFlags>(miscFlags_ | x); }

    void apply(OptionGroup& x)
    {
        // FIXME:
        // Check for duplicates
        x.options_.push_back(this);
    }

    template <class U>
    void apply(details::Initializer<U>)
    {
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

        //
        // NOTE:
        //
        // This might call the virtual function allowedValues(). Since applyRec is *only* called
        // in the body of the constructor of Option<> and this is where allowedValues() is actually
        // implemented, this is ok.
        //
        cmd.add(this);
    }

    void applyRec();

private:
    StringRef displayName() const;

    bool isOccurrenceAllowed() const;
    bool isOccurrenceRequired() const;
    bool isUnbounded() const;
    bool isRequired() const;
    bool isPrefix() const;

    // Parses the given value and stores the result.
    virtual void parse(StringRef spec, StringRef value, size_t i) = 0;

    // Returns a list of allowed values for this option
    virtual std::vector<StringRef> getAllowedValues() const = 0;
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
        , value_() // NOTE: error here if T is a reference type and not initialized using init(T)
    {
    }

    template <class... An, class X = T, class = EnableIf<std::is_reference<X>>>
    BasicOption(details::Initializer<T> x, An&&...)
        : BaseType()
        , value_(x)
    {
    }

    template <class U, class... An, class X = T, class = DisableIf<std::is_reference<X>>>
    BasicOption(details::Initializer<U> x, An&&...)
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
    value_type& value() { return value_; }

    // Returns the value
    value_type const& value() const { return value_; }

    // Returns a pointer to the value
    value_type* operator->() { return std::addressof(value_); }

    // Returns a pointer to the value
    value_type const* operator->() const { return std::addressof(value_); }
};

//--------------------------------------------------------------------------------------------------
// Option
//

enum DefaultInitParserTag { DefaultInitParser };
enum InitParserTag { InitParser };

template <class T, template <class> class TraitsT = Traits, class ParserT = Parser<typename TraitsT<T>::element_type>>
class Option : public BasicOption<T>
{
    using BaseType = BasicOption<T>;
    using StringRefVector = std::vector<StringRef>;

    using ElementType = typename TraitsT<T>::element_type;
    using InserterType = typename TraitsT<T>::inserter_type;
    using IsScalar = typename std::is_void<InserterType>::type;

    static_assert(IsScalar::value || std::is_default_constructible<ElementType>::value,
        "elements of containers must be default constructible");

    ParserT parser_;

public:
    using parser_type = UnwrapReferenceWrapper<ParserT>;

    template <class... An>
    explicit Option(DefaultInitParserTag, An&&... an)
        : BaseType(std::forward<An>(an)...)
    {
        this->applyRec(IsScalar::value ? Optional : ZeroOrMore, std::forward<An>(an)...);
    }

    template <class P, class... An>
    explicit Option(InitParserTag, P&& p, An&&... an)
        : BaseType(std::forward<An>(an)...)
        , parser_(std::forward<P>(p))
    {
        this->applyRec(IsScalar::value ? Optional : ZeroOrMore, std::forward<An>(an)...);
    }

    // Returns the parser
    parser_type& parser() { return parser_; }

    // Returns the parser
    parser_type const& parser() const { return parser_; }

private:
    void parse(StringRef spec, StringRef value, size_t i, std::false_type)
    {
        ElementType t;

        // Parse...
        parser()(spec, value, i, t);

        // and insert into the container
        InserterType()(this->value(), std::move(t));
    }

    void parse(StringRef spec, StringRef value, size_t i, std::true_type) {
        parser()(spec, value, i, this->value());
    }

    virtual void parse(StringRef spec, StringRef value, size_t i) override final {
        parse(spec, value, i, IsScalar());
    }

    virtual std::vector<StringRef> getAllowedValues() const override final {
        return cl::allowedValues(parser());
    }
};

//--------------------------------------------------------------------------------------------------
// makeOption
//

// Construct a new Option with a default constructed parser
template <class T, template <class> class TraitsT = Traits, class... An>
auto makeOption(An&&... an)
    -> Option<T, TraitsT>
{
    return Option<T, TraitsT>(DefaultInitParser, std::forward<An>(an)...);
}

// Construct a new Option, initialize the parser with the given value
template <class T, template <class> class TraitsT = Traits, class P, class... An>
auto makeOption(InitParserTag, P&& p, An&&... an)
    -> Option<T, TraitsT, Decay<P>>
{
    return Option<T, TraitsT, Decay<P>>(InitParser, std::forward<P>(p), std::forward<An>(an)...);
}

// Construct a new Option, initialize the a map-parser with the given values
template <class T, template <class> class TraitsT = Traits, class... An>
auto makeOption(std::initializer_list<typename MapParser<T>::map_value_type> ilist, An&&... an)
    -> Option<T, TraitsT, MapParser<T>>
{
    return Option<T, TraitsT, MapParser<T>>(InitParser, ilist, std::forward<An>(an)...);
}

} // namespace cl
} // namespace support

#ifdef _MSC_VER
#pragma warning(pop)
#endif
