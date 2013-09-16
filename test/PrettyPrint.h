// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace support {
namespace pp {

    struct IsContainerImpl
    {
        template<class U>
        static auto test(U&&) -> decltype(std::declval<typename U::const_iterator>(), std::true_type());
        static auto test(...) -> std::false_type;
    };

    template<class T>
    struct IsContainer : decltype( IsContainerImpl::test(std::declval<T>()) )
    {
    };

    template<class T, size_t N>
    struct IsContainer<T[N]> : std::true_type
    {
    };

    template<class E, class T, class A>
    struct IsContainer<std::basic_string<E, T, A>> : std::false_type
    {
    };

    struct IsTupleImpl
    {
        template<class U>
        static auto test(U&&) -> decltype(std::declval<typename std::tuple_element<0,U>::type>(), std::true_type());
        static auto test(...) -> std::false_type;
    };

    template<class T>
    struct IsTuple : decltype( IsTupleImpl::test(std::declval<T>()) )
    {
    };

    struct Formatter
    {
        template<class T>
        void value(std::ostream& stream, T const& object) const {
            stream << object;
        }

        template<class E, class T, class A>
        void value(std::ostream& stream, std::basic_string<E,T,A> const& object) const {
            stream << "\"" << object << "\"";
        }

        template<class T>
        void begin(std::ostream& stream, T const& /*object*/) const {
            stream << "[";
        }

        template<class T>
        void separator(std::ostream& stream, T const& /*object*/) const {
            stream << ", ";
        }

        template<class T>
        void end(std::ostream& stream, T const& /*object*/) const {
            stream << "]";
        }

        template<class T>
        void begin(std::ostream& stream, T const& /*object*/, std::true_type) const {
            stream << "{";
        }

        template<class T>
        void separator(std::ostream& stream, T const& /*object*/, std::true_type) const {
            stream << ", ";
        }

        template<class T>
        void end(std::ostream& stream, T const& /*object*/, std::true_type) const {
            stream << "}";
        }
    };

    template<class T>
    void dispatchTuple(std::ostream& stream, T const& object, std::true_type);

    template<class T>
    void dispatchTuple(std::ostream& stream, T const& object, std::false_type);

    template<class T>
    void dispatchContainer(std::ostream& stream, T const& object, std::true_type);

    template<class T>
    void dispatchContainer(std::ostream& stream, T const& object, std::false_type);

    template<class T, size_t N>
    void printTuple(std::ostream& stream, T const& object, std::integral_constant<size_t, N>);

    template<class T>
    void printContainer(std::ostream& stream, T const& object);

    template<class T>
    void printOther(std::ostream& stream, T const& object);

    template<class T>
    void printAny(std::ostream& stream, T const& object) {
        dispatchTuple(stream, object, typename IsTuple<T>::type());
    }

    template<class T>
    void dispatchTuple(std::ostream& stream, T const& object, std::true_type)
    {
        Formatter format;

        size_t const S = std::tuple_size<T>::value;

        // open
        format.begin(stream, object, std::true_type());

        // print tuple elements
        printTuple(stream, object, std::integral_constant<size_t, S>());

        // close
        format.end(stream, object, std::true_type());
    }

    template<class T>
    void dispatchTuple(std::ostream& stream, T const& object, std::false_type) {
        dispatchContainer(stream, object, typename IsContainer<T>::type());
    }

    template<class T>
    void dispatchContainer(std::ostream& stream, T const& object, std::true_type) {
        printContainer(stream, object);
    }

    template<class T>
    void dispatchContainer(std::ostream& stream, T const& object, std::false_type) {
        printOther(stream, object);
    }

    template<class T>
    void printTuple(std::ostream& /*stream*/, T const& /*object*/, std::integral_constant<size_t, 0>)
    {
    }

    template<class T>
    void printTuple(std::ostream& stream, T const& object, std::integral_constant<size_t, 1>)
    {
        size_t const S = std::tuple_size<T>::value;

        // print last element of the tuple
        printAny(stream, std::get<S - 1>(object));
    }

    template<class T, size_t N>
    void printTuple(std::ostream& stream, T const& object, std::integral_constant<size_t, N>)
    {
        Formatter format;

        size_t const S = std::tuple_size<T>::value;

        // print the current element of the tuple
        printAny(stream, std::get<S - N>(object));

        // separate the tuple elements
        format.separator(stream, object, std::true_type());

        // print the remaining tuple elements
        printTuple(stream, object, std::integral_constant<size_t, N - 1>());
    }

    template<class T>
    void printContainer(std::ostream& stream, T const& object)
    {
        using std::begin;
        using std::end;

        Formatter format;

        format.begin(stream, object);

        auto I = begin(object);
        auto E = end(object);

        if (I != E)
        {
            for (;;)
            {
                printAny(stream, *I);

                if (++I != E)
                    format.separator(stream, object);
                else
                    break;
            }
        }

        format.end(stream, object);
    }

    namespace fallback
    {
        template<class T>
        void prettyPrint(std::ostream& stream, T const& object)
        {
            Formatter format;

            format.value(stream, object);
        }
    }

    template<class T>
    void printOther(std::ostream& stream, T const& object)
    {
        using fallback::prettyPrint;

        prettyPrint(stream, object);
    }

    template<class T>
    struct PrettyPrinter
    {
        T const& object;

        explicit PrettyPrinter(T const& object) : object(object)
        {
        }

#ifdef _MSC_VER
    private:
        PrettyPrinter& operator =(PrettyPrinter const&) ; // C4512
#endif
    };

    template<class T>
    inline std::ostream& operator <<(std::ostream& stream, PrettyPrinter<T> const& x)
    {
        printAny(stream, x.object);
        return stream;
    }

} // namespace pp

template<class T>
inline pp::PrettyPrinter<T> pretty(T const& object) {
    return pp::PrettyPrinter<T>(object);
}

} // namespace support
