// This file is distributed under the MIT license.
// See the LICENSE file for details.


#pragma once


#include <iterator>
#include <ostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4512) // assignment operator could not be generated
#endif


namespace support
{


    namespace details
    {
    namespace pp
    {

        using std::begin;
        using std::end;

        template<class T>
        void prettyPrint(std::ostream& stream, T const& object);

        // Overload to print strings using double quotation marks...
        template<class C, class T, class A>
        void prettyPrint(std::ostream& stream, std::basic_string<C,T,A> const& object)
        {
            stream << "\"" << object << "\"";
        }

        struct IsContainerImpl
        {
            template<class U>
            static auto test(U&& u) -> typename std::is_convertible<decltype(begin(u) == end(u)), bool>::type;
            static auto test(...) -> std::false_type;
        };

        template<class T>
        struct IsContainer : decltype( IsContainerImpl::test(std::declval<T>()) )
        {
        };

        template<class C, class T, class A>
        struct IsContainer<std::basic_string<C, T, A>> : std::false_type
        {
        };

        template<class T>
        struct IsTuple : std::false_type
        {
        };

        template<class T1, class T2>
        struct IsTuple<std::pair<T1, T2>> : std::true_type
        {
        };

        template<class ...Ts>
        struct IsTuple<std::tuple<Ts...>> : std::true_type
        {
        };

        template<class T>
        void printTuple(std::ostream& /*stream*/, T const& /*object*/, std::integral_constant<size_t, 0>)
        {
        }

        template<class T>
        void printTuple(std::ostream& stream, T const& object, std::integral_constant<size_t, 1>)
        {
            // Print last element of the tuple
            prettyPrint(stream, std::get<std::tuple_size<T>::value - 1>(object));
        }

        template<class T, size_t N>
        void printTuple(std::ostream& stream, T const& object, std::integral_constant<size_t, N>)
        {
            // Print the current element of the tuple
            prettyPrint(stream, std::get<std::tuple_size<T>::value - N>(object));

            // Separate the tuple elements
            stream << ", ";

            // Print the remaining tuple elements
            printTuple(stream, object, std::integral_constant<size_t, N - 1>());
        }

        template<class T>
        void dispatchTuple(std::ostream& stream, T const& object, std::true_type)
        {
            printTuple(stream, object, typename std::tuple_size<T>::type());
        }

        template<class T>
        void dispatchTuple(std::ostream& stream, T const& object, std::false_type)
        {
            stream << object;
        }

        template<class T>
        void dispatchContainer(std::ostream& stream, T const& object, std::true_type)
        {
            stream << "[";

            auto I = begin(object);
            auto E = end(object);

            if (I != E)
            {
                for (;;)
                {
                    prettyPrint(stream, *I);

                    if (++I != E)
                        stream << ", ";
                    else
                        break;
                }
            }

            stream << "]";
        }

        template<class T>
        void dispatchContainer(std::ostream& stream, T const& object, std::false_type)
        {
            dispatchTuple(stream, object, typename IsTuple<T>::type());
        }

        template<class T>
        void prettyPrint(std::ostream& stream, T const& object)
        {
            dispatchContainer(stream, object, typename IsContainer<T>::type());
        }

        template<class T>
        struct PrettyPrinter
        {
            T const& object;

            explicit PrettyPrinter(T const& object) : object(object)
            {
            }
        };

        template<class T>
        inline std::ostream& operator <<(std::ostream& stream, PrettyPrinter<T> const& x)
        {
            prettyPrint(stream, x.object);
            return stream;
        }

    } // namespace pp
    } // namespace details


    template<class T>
    inline details::pp::PrettyPrinter<T> pretty(T const& object)
    {
        return details::pp::PrettyPrinter<T>(object);
    }


} // namespace support


#ifdef _MSC_VER
#pragma warning(pop)
#endif
