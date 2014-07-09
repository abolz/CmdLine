// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

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
