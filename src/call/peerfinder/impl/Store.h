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

#ifndef CALL_PEERFINDER_STORE_H_INCLUDED
#define CALL_PEERFINDER_STORE_H_INCLUDED

namespace call {
namespace PeerFinder {

/** Abstract persistence for PeerFinder data. */
class Store
{
public:
    virtual ~Store () { }

    // load the bootstrap cache
    using load_callback = std::function <void (beast::IP::Endpoint, int)>;
    virtual std::size_t load (load_callback const& cb) = 0;

    // save the bootstrap cache
    struct Entry
    {
        beast::IP::Endpoint endpoint;
        int valence;
    };
    virtual void save (std::vector <Entry> const& v) = 0;
};

}
}

#endif
