//------------------------------------------------------------------------------
/*
    This file is part of calld: https://github.com/callchain/calld
    Copyright (c) 2018, 2019 Callchain Fundation.

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

    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

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

#ifndef CALL_PATH_IMPL_AMOUNTSPEC_H_INCLUDED
#define CALL_PATH_IMPL_AMOUNTSPEC_H_INCLUDED

#include <call/protocol/IOUAmount.h>
#include <call/protocol/CALLAmount.h>
#include <call/protocol/STAmount.h>

namespace call {

struct AmountSpec
{
    bool native;
    union
    {
        CALLAmount call;
        IOUAmount iou;
    };
    boost::optional<AccountID> issuer;
    boost::optional<Currency> currency;

    friend
    std::ostream&
    operator << (
        std::ostream& stream,
        AmountSpec const& amt)
    {
        if (amt.native)
            stream << to_string (amt.call);
        else
            stream << to_string (amt.iou);
        if (amt.currency)
            stream << "/(" << *amt.currency << ")";
        if (amt.issuer)
            stream << "/" << *amt.issuer << "";
        return stream;
    }
};

struct EitherAmount
{
#ifndef NDEBUG
    bool native = false;
#endif

    union
    {
        IOUAmount iou;
        CALLAmount call;
    };

    EitherAmount () = default;

    explicit
    EitherAmount (IOUAmount const& a)
            :iou(a)
    {
    }

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
    // ignore warning about half of iou amount being uninitialized
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
    explicit
    EitherAmount (CALLAmount const& a)
            :call(a)
    {
#ifndef NDEBUG
        native = true;
#endif
    }
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

    explicit
    EitherAmount (AmountSpec const& a)
    {
#ifndef NDEBUG
        native = a.native;
#endif
        if (a.native)
            call = a.call;
        else
            iou = a.iou;
    }
};

template <class T>
T&
get (EitherAmount& amt)
{
    static_assert(sizeof(T) == -1, "Must used specialized function");
    return T(0);
}

template <>
inline
IOUAmount&
get<IOUAmount> (EitherAmount& amt)
{
    assert (!amt.native);
    return amt.iou;
}

template <>
inline
CALLAmount&
get<CALLAmount> (EitherAmount& amt)
{
    assert (amt.native);
    return amt.call;
}

template <class T>
T const&
get (EitherAmount const& amt)
{
    static_assert(sizeof(T) == -1, "Must used specialized function");
    return T(0);
}

template <>
inline
IOUAmount const&
get<IOUAmount> (EitherAmount const& amt)
{
    assert (!amt.native);
    return amt.iou;
}

template <>
inline
CALLAmount const&
get<CALLAmount> (EitherAmount const& amt)
{
    assert (amt.native);
    return amt.call;
}

inline
AmountSpec
toAmountSpec (STAmount const& amt)
{
    assert (amt.mantissa () < std::numeric_limits<std::int64_t>::max ());
    bool const isNeg = amt.negative ();
    std::int64_t const sMant =
        isNeg ? - std::int64_t (amt.mantissa ()) : amt.mantissa ();
    AmountSpec result;

    result.native = isCALL (amt);
    if (result.native)
    {
        result.call = CALLAmount (sMant);
    }
    else
    {
        result.iou = IOUAmount (sMant, amt.exponent ());
        result.issuer = amt.issue().account;
        result.currency = amt.issue().currency;
    }

    return result;
}

inline
EitherAmount
toEitherAmount (STAmount const& amt)
{
    if (isCALL (amt))
        return EitherAmount{amt.call()};
    return EitherAmount{amt.iou()};
}

inline
AmountSpec
toAmountSpec (
    EitherAmount const& ea,
    boost::optional<Currency> const& c)
{
    AmountSpec r;
    r.native = (!c || isCALL (*c));
    r.currency = c;
    assert (ea.native == r.native);
    if (r.native)
    {
        r.call = ea.call;
    }
    else
    {
        r.iou = ea.iou;
    }
    return r;
}

}

#endif
