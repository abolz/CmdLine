// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace support
{

//--------------------------------------------------------------------------------------------------
// Alias templates
//

template <class T, class U = void>
using EnableIf = typename std::enable_if<T::value, U>::type;

template <class T, class U = void>
using DisableIf = typename std::enable_if<!T::value, U>::type;

template <class T>
using Decay = typename std::decay<T>::type;

template <class T>
using RemoveReference = typename std::remove_reference<T>::type;

template <class T>
using RemoveCV = typename std::remove_cv<T>::type;

namespace details
{
    template <class T>
    struct RemoveCVRec
    {
        using type = RemoveCV<T>;
    };

    template <template <class...> class T, class... A>
    struct RemoveCVRec<T<A...>>
    {
        using type = T<typename RemoveCVRec<A>::type...>;
    };
}

template <class T>
using RemoveCVRec = typename details::RemoveCVRec<T>::type;

template <class T, class U>
using IsSame = typename std::is_same<T, U>::type;

template <class From, class To>
using IsConvertible = typename std::is_convertible<From, To>::type;

} // namespace support
