//------------------------------------------------------------------------------
/*
    This file is part of callchaind: https://github.com/callchain/callchaind
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

#ifndef CALLCHAIN_PROTOCOL_SYSTEMPARAMETERS_H_INCLUDED
#define CALLCHAIN_PROTOCOL_SYSTEMPARAMETERS_H_INCLUDED

#include <cstdint>
#include <string>

namespace callchain {

// Various protocol and system specific constant globals.

/* The name of the system. */
static inline
std::string const&
systemName ()
{
    static std::string const name = "callchain";
    return name;
}

/** Configure the native currency. */
static
std::uint64_t const
SYSTEM_CURRENCY_GIFT = 5000;

static
std::uint64_t const
SYSTEM_CURRENCY_USERS = 100000000;

/** Number of drops per 1 XRP */
static
std::uint64_t const
SYSTEM_CURRENCY_PARTS = 1000000;

/** Number of drops in the genesis account. */
static
std::uint64_t const
SYSTEM_CURRENCY_START = SYSTEM_CURRENCY_GIFT * SYSTEM_CURRENCY_USERS * SYSTEM_CURRENCY_PARTS*0.001;

/* The currency code for the native currency. */
static inline
std::string const&
systemCurrencyCode ()
{
    static std::string const code = "CALL";
    return code;
}

} // callchain

#endif
