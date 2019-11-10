//------------------------------------------------------------------------------
/*
    This file is part of calld: https://github.com/callchain/calld
    Copyright (c) 2018, 2019 Callchain Foundation.

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

#ifndef CALL_PROTOCOL_INVOICE_H_INCLUDED
#define CALL_PROTOCOL_INVOICE_H_INCLUDED

#include <cassert>
#include <functional>
#include <type_traits>

#include <call/protocol/UintTypes.h>

namespace call {

/** A invoice token
*/
class Invoice
{
public:
    uint256 id;
    Blob info;

    Invoice ()
    {
    }

    Invoice (uint256 const& i, Blob const& b)
            : id (i), info (b)
    {
    }
};



//------------------------------------------------------------------------------
inline bool const& newInvoice (Invoice const& i)
{
    return !i.info.empty();
}


}

#endif
