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

#ifndef CALL_NET_SNTPCLOCK_H_INCLUDED
#define CALL_NET_SNTPCLOCK_H_INCLUDED

#include <call/beast/clock/abstract_clock.h>
#include <call/beast/utility/Journal.h>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace call {

/** A clock based on system_clock and adjusted for SNTP. */
class SNTPClock
    : public beast::abstract_clock<
        std::chrono::system_clock>
{
public:
    virtual
    void
    run (std::vector <std::string> const& servers) = 0;

    virtual
    duration
    offset() const = 0;
};

extern
std::unique_ptr<SNTPClock>
make_SNTPClock (beast::Journal);

} // call

#endif
