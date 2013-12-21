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
    None = 0,
    CommaSeparated = 0x01,  // Should this list split between commas?
    Hidden = 0x02,          // Do not show this option in the usage
    ConsumeAfter = 0x04,    // Handle all following arguments as positional arguments
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
    using OptionMap         = std::map<StringRef, OptionBase*>;
    using OptionGroups      = std::map<StringRef, OptionGroup*>;
    using OptionVector      = std::vector<OptionBase*>;
    using ConstOptionVector = std::vector<OptionBase const*>;
    using StringVector      = std::vector<std::string>;

private:
    // List of options
    OptionMap options;
    // List of option groups and associated options
    OptionGroups groups;
    // List of positional options
    OptionVector positionals;
    // List of error message
    StringVector errors;
    // List of unknown command line arguments
    StringVector unknowns;
    // The length of the longest prefix option
    size_t maxPrefixLength;

public:
    explicit CmdLine();

    // Adds the given option to the command line
    bool add(OptionBase* opt);

    // Parse the given command line arguments
    bool parse(StringVector const& argv, bool ignoreUnknowns = false);

    // Returns the list of errors
    StringVector const& getErrors() const { return errors; }

    // Returns the list of unknown command line arguments
    StringVector const& getUnknowns() const { return unknowns; }

    // Returns the list of (unique) options, sorted by name.
    ConstOptionVector getOptions(bool SkipHidden = true) const;

    // Returns a list of the positional options.
    ConstOptionVector getPositionalOptions() const;

private:
    OptionBase* findOption(StringRef name) const;

    bool handleArg(StringVector const& argv, size_t& i, OptionVector::iterator& pos, bool& dashdash, bool ignoreUnknowns);

    bool handlePositional(StringRef arg, size_t i, OptionVector::iterator& pos);

    bool handleOption(bool& success, StringRef arg, size_t& i, StringVector const& argv);
    bool handlePrefix(bool& success, StringRef arg, size_t i);
    bool handleGroup(bool& success, StringRef arg, size_t i);

    bool addOccurrence(OptionBase* opt, StringRef spec, StringRef value, size_t i);

    bool check();
    bool check(OptionBase const* opt);
    bool check(OptionGroup const* g);

    bool error(std::string str);
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
    bool operator()(StringRef /*spec*/, StringRef value, size_t /*i*/, T& result) const
    {
        StringRefStream stream(value);
        return (stream >> std::setbase(0) >> result) && stream.eof();
    }
};

template <>
struct Parser<bool>
{
    bool operator ()(StringRef /*spec*/, StringRef value, size_t /*i*/, bool& result) const
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
    bool operator ()(StringRef /*spec*/, StringRef value, size_t /*i*/, std::string& result) const
    {
        result.assign(value.data(), value.size());
        return true;
    }
};

template <class P>
void allowedValues(P const& /*parser*/, std::vector<StringRef>& /*vec*/)
{
}

template <class P>
void allowedValues(std::reference_wrapper<P> const& parser, std::vector<StringRef>& vec)
{
    allowedValues(parser.get(), vec);
}

template <class P>
void descriptions(P const& /*parser*/, std::vector<StringRef>& /*vec*/)
{
}

template <class P>
void descriptions(std::reference_wrapper<P> const& parser, std::vector<StringRef>& vec)
{
    descriptions(parser.get(), vec);
}

//--------------------------------------------------------------------------------------------------
// MapParser
//

template <class T>
struct MapParser
{
    using MapType       = std::map<std::string, std::pair<T, std::string>>;
    using MapValueType  = typename MapType::value_type;

    struct Init : MapValueType
    {
        Init(std::string name, T value, std::string desc = "")
            : MapValueType{std::move(name), {std::move(value), std::move(desc)}}
        {
        }
    };

    MapType map;

    explicit MapParser(std::initializer_list<Init> ilist)
        : map(ilist.begin(), ilist.end())
    {
    }

    bool operator ()(StringRef spec, StringRef value, size_t /*i*/, T& result) const
    {
        // If the value is null, the option is specified by spec
        auto I = map.find(value.data() ? value.str() : spec.str());

        if (I != map.end())
        {
            result = I->second.first;
            return true;
        }

        return false;
    }
};

template <class T>
void allowedValues(MapParser<T> const& parser, std::vector<StringRef>& vec)
{
    for (auto const& I : parser.map)
        vec.emplace_back(I.first);
}

template <class T>
void descriptions(MapParser<T> const& parser, std::vector<StringRef>& vec)
{
    for (auto const& I : parser.map)
        vec.emplace_back(I.second.second);
}

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
    bool check() const;

    // Returns a description for this group
    std::string desc() const;

private:
    // The name of this option group
    std::string name;
    // The type of this group
    Type type;
    // The list of options in this group
    std::vector<OptionBase*> options;
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
    // Constructor.
    explicit OptionBase();

public:
    // Destructor.
    virtual ~OptionBase();

public:
    // Returns the name of this option
    std::string const& getName() const { return name; }

    // Return name of the value
    std::string const& getArgName() const { return argName; }

    // Resturns the description of this option
    std::string const& getDesc() const { return desc; }

    // Returns the number of times this option has been specified on the command line
    unsigned getCount() const { return count; }

protected:
    void apply(std::string x)       { name = std::move(x); }
    void apply(ArgName x)           { argName = std::move(x.value); }
    void apply(Desc x)              { desc = std::move(x.value); }
    void apply(NumOccurrences x)    { numOccurrences = x; }
    void apply(NumArgs x)           { numArgs = x; }
    void apply(Formatting x)        { formatting = x; }
    void apply(MiscFlags x)         { miscFlags = static_cast<MiscFlags>(miscFlags | x); }

    void apply(OptionGroup& x) {
        // FIXME: Check for duplicates
        x.options.push_back(this);
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

        if (!cmd.add(this))
            throw std::runtime_error("failed to register option");
    }

    void applyRec(); // End recursion - check for valid flags

private:
    // Returns the name of this option for use in error messages
    StringRef displayName() const;

    bool isOccurrenceAllowed() const;
    bool isOccurrenceRequired() const;
    bool isUnbounded() const;
    bool isOptional() const;
    bool isRequired() const;
    bool isPrefix() const;

    // Parses the given value and stores the result.
    virtual bool parse(StringRef spec, StringRef value, size_t i) = 0;

    // Returns a list of valid arguments for this option
    virtual void allowedValues(std::vector<StringRef>& vec) const = 0;

    // Returns a list of the descriptions for each valid argument
    virtual void descriptions(std::vector<StringRef>& vec) const = 0;
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
        , value(x)
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
};

//--------------------------------------------------------------------------------------------------
// Option
//

template <class T, class ParserT = Parser<typename Traits<T>::value_type>>
class Option : public BasicOption<T>
{
    using BaseType = BasicOption<T>;
    using StringRefVector = std::vector<StringRef>;

    ParserT parser;

public:
    using parser_type = ParserT;
    using traits_type = Traits<T>;

    using value_type        = typename traits_type::value_type;
    using inserter_type     = typename traits_type::inserter_type;
    using is_scalar         = typename std::is_void<inserter_type>::type;

    static_assert(is_scalar::value || std::is_default_constructible<value_type>::value,
        "elements of containers must be default constructible");

public:
    template <class P, class... An>
    explicit Option(P&& p, An&&... an)
        : BaseType(std::forward<An>(an)...)
        , parser(std::forward<P>(p))
    {
        this->apply(is_scalar::value ? Optional : ZeroOrMore);
        this->applyRec(std::forward<An>(an)...);
    }

    // Returns the parser
    ParserT& getParser() { return parser; }

    // Returns the parser
    ParserT const& getParser() const { return parser; }

private:
    bool parse(StringRef spec, StringRef value, size_t i, std::false_type)
    {
        value_type t;

        if (parser(spec, value, i, t))
        {
            inserter_type()(this->get(), std::move(t));
            return true;
        }

        return false;
    }

    bool parse(StringRef spec, StringRef value, size_t i, std::true_type) {
        return parser(spec, value, i, this->get());
    }

    bool parse(StringRef spec, StringRef value, size_t i) override {
        return parse(spec, value, i, is_scalar());
    }

    void allowedValues(StringRefVector& vec) const override final
    {
        using cl::allowedValues;
        allowedValues(getParser(), vec);
    }

    void descriptions(StringRefVector& vec) const override final
    {
        using cl::descriptions;
        descriptions(getParser(), vec);
    }
};

// Construct a new Option with a default constructed parser
template <class T, class... An>
auto makeOption(An&&... an) -> Option<T>
{
    using R = Option<T>;
    return R(typename R::parser_type(), std::forward<An>(an)...);
}

// Construct a new Option, initialize the parser with the given value
template <class T, class P, class... An>
auto makeOptionWithParser(P&& p, An&&... an) -> Option<T, Decay<P>>
{
    return Option<T, Decay<P>>(std::forward<P>(p), std::forward<An>(an)...);
}

} // namespace cl
} // namespace support

#ifdef _MSC_VER
#pragma warning(pop)
#endif
