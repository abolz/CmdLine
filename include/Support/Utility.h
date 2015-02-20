// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#include <functional>
#include <iterator>
#include <type_traits>

namespace support
{

//--------------------------------------------------------------------------------------------------
// <type_traits>
//

using std::begin;
using std::end;
using std::swap;

// size

template <class Container>
inline auto size(Container const& cont) -> decltype(cont.size())
{
    return cont.size();
}

template <class T, size_t N>
inline auto size(T const (&)[N]) -> size_t
{
    return N;
}

// empty

template <class Container>
inline auto empty(Container const& cont) -> decltype(cont.empty())
{
    return cont.empty();
}

template <class T, size_t N>
inline auto empty(T const (&)[N]) -> bool
{
    return false;
}

// front

template <class Container>
inline auto front(Container& cont) -> decltype((cont.front()))
{
    return cont.front();
}

template <class Container>
inline auto front(Container const& cont) -> decltype((cont.front()))
{
    return cont.front();
}

template <class T, size_t N>
inline T& front(T (&arr)[N])
{
    return arr[0];
}

// back

template <class Container>
inline auto back(Container& cont) -> decltype((cont.back()))
{
    return cont.back();
}

template <class Container>
inline auto back(Container const& cont) -> decltype((cont.back()))
{
    return cont.back();
}

template <class T, size_t N>
inline T& back(T (&arr)[N])
{
    return arr[N - 1];
}

// data

template <class Container>
inline auto data(Container& cont) -> decltype((cont.data()))
{
    return cont.data();
}

template <class Container>
inline auto data(Container const& cont) -> decltype((cont.data()))
{
    return cont.data();
}

template <class T, size_t N>
inline T* data(T (&arr)[N])
{
    return arr;
}

//--------------------------------------------------------------------------------------------------
// <type_traits>
//

template <class T>
struct remove_cv_rec {
    using type = typename std::remove_cv<T>::type;
};

template <template <class...> class T, class... A>
struct remove_cv_rec<T<A...>> {
    using type = T<typename remove_cv_rec<A>::type...>;
};

template <class T>
struct unwrap_reference_wrapper {
    using type = T;
};

template <class T>
struct unwrap_reference_wrapper<std::reference_wrapper<T>> {
    using type = T;
};

// alias templates

template <bool T, class U = void>
using enable_if_t = typename std::enable_if<T, U>::type;

template <class T, class U = void>
using enable_if_value_t = typename std::enable_if<T::value, U>::type;

template <class T, class U = void>
using disable_if_value_t = typename std::enable_if<!T::value, U>::type;

template <class T>
using decay_t = typename std::decay<T>::type;

template <class T>
using remove_reference_t = typename std::remove_reference<T>::type;

template <class T>
using remove_cv_t = typename std::remove_cv<T>::type;

template <class T>
using remove_cv_rec_t = typename remove_cv_rec<T>::type;

template <class T, class U>
using is_same_t = typename std::is_same<T, U>::type;

template <class From, class To>
using is_convertible_t = typename std::is_convertible<From, To>::type;

template <class T>
using unwrap_reference_wrapper_t = typename unwrap_reference_wrapper<T>::type;

} // namespace support
