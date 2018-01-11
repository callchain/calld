//------------------------------------------------------------------------------
/*
    This file is part of calld: https://github.com/call/calld
    Copyright (c) 2012, 2013 Call Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef CALL_PROTOCOL_CALLAMOUNT_H_INCLUDED
#define CALL_PROTOCOL_CALLAMOUNT_H_INCLUDED

#include <call/basics/contract.h>
#include <call/protocol/SystemParameters.h>
#include <call/beast/utility/Zero.h>
#include <boost/operators.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <cstdint>
#include <string>
#include <type_traits>

using beast::zero;

namespace call {

class CALLAmount
    : private boost::totally_ordered <CALLAmount>
    , private boost::additive <CALLAmount>
{
private:
    std::int64_t drops_;

public:
    CALLAmount () = default;
    CALLAmount (CALLAmount const& other) = default;
    CALLAmount& operator= (CALLAmount const& other) = default;

    CALLAmount (beast::Zero)
        : drops_ (0)
    {
    }

    CALLAmount&
    operator= (beast::Zero)
    {
        drops_ = 0;
        return *this;
    }

    template <class Integer,
        class = typename std::enable_if_t <
            std::is_integral<Integer>::value>>
    CALLAmount (Integer drops)
        : drops_ (static_cast<std::int64_t> (drops))
    {
    }

    template <class Integer,
        class = typename std::enable_if_t <
            std::is_integral<Integer>::value>>
    CALLAmount&
    operator= (Integer drops)
    {
        drops_ = static_cast<std::int64_t> (drops);
        return *this;
    }

    CALLAmount&
    operator+= (CALLAmount const& other)
    {
        drops_ += other.drops_;
        return *this;
    }

    CALLAmount&
    operator-= (CALLAmount const& other)
    {
        drops_ -= other.drops_;
        return *this;
    }

    CALLAmount
    operator- () const
    {
        return { -drops_ };
    }

    bool
    operator==(CALLAmount const& other) const
    {
        return drops_ == other.drops_;
    }

    bool
    operator<(CALLAmount const& other) const
    {
        return drops_ < other.drops_;
    }

    /** Returns true if the amount is not zero */
    explicit
    operator bool() const noexcept
    {
        return drops_ != 0;
    }

    /** Return the sign of the amount */
    int
    signum() const noexcept
    {
        return (drops_ < 0) ? -1 : (drops_ ? 1 : 0);
    }

    /** Returns the number of drops */
    std::int64_t
    drops () const
    {
        return drops_;
    }
};

inline
std::string
to_string (CALLAmount const& amount)
{
    return std::to_string (amount.drops ());
}

inline
CALLAmount
mulRatio (
    CALLAmount const& amt,
    std::uint32_t num,
    std::uint32_t den,
    bool roundUp)
{
    using namespace boost::multiprecision;

    if (!den)
        Throw<std::runtime_error> ("division by zero");

    int128_t const amt128 (amt.drops ());
    auto const neg = amt.drops () < 0;
    auto const m = amt128 * num;
    auto r = m / den;
    if (m % den)
    {
        if (!neg && roundUp)
            r += 1;
        if (neg && !roundUp)
            r -= 1;
    }
    if (r > std::numeric_limits<std::int64_t>::max ())
        Throw<std::overflow_error> ("CALL mulRatio overflow");
    return CALLAmount (r.convert_to<std::int64_t> ());
}

/** Returns true if the amount does not exceed the initial CALL in existence. */
inline
bool isLegalAmount (CALLAmount const& amount)
{
    return amount.drops () <= SYSTEM_CURRENCY_START;
}

}

#endif
