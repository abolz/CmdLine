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

} // namespace support
