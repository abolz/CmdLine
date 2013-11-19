// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

namespace support
{
namespace cl
{

//--------------------------------------------------------------------------------------------------
// WithIndex
//

template <class T>
struct WithIndex
{
    // The value of the option
    T value;
    // The index where the option appeared on the command line
    int index;

    WithIndex() : value(), index(-1)
    {
    }

    template <class U, class = typename std::enable_if<std::is_constructible<T, U>::value>::type>
    WithIndex(U&& value)
        : value(std::forward<U>(value))
        , index(-1)
    {
    }
};

//--------------------------------------------------------------------------------------------------
// Parser
//

template <class T>
struct Parser<WithIndex<T>>
{
    bool operator()(StringRef value, size_t i, WithIndex<T>& result) const
    {
        result.index = static_cast<int>(i);
        return Parser<T>()(value, i, result.value);
    }
};

} // namespace cl
} // namespace support
