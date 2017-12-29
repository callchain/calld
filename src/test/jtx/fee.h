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

#ifndef CALLCHAIN_TEST_JTX_FEE_H_INCLUDED
#define CALLCHAIN_TEST_JTX_FEE_H_INCLUDED

#include <test/jtx/Env.h>
#include <test/jtx/tags.h>
#include <callchain/protocol/STAmount.h>
#include <callchain/basics/contract.h>
#include <boost/optional.hpp>

namespace callchain {
namespace test {
namespace jtx {

/** Set the fee on a JTx. */
class fee
{
private:
    bool manual_ = true;
    boost::optional<STAmount> amount_;

public:
    explicit
    fee (autofill_t)
        : manual_(false)
    {
    }

    explicit
    fee (none_t)
    {
    }

    explicit
    fee (STAmount const& amount)
        : amount_(amount)
    {
        if (! isXRP(*amount_))
            Throw<std::runtime_error> (
                "fee: not XRP");
    }

    void
    operator()(Env&, JTx& jt) const;
};

} // jtx
} // test
} // callchain

#endif
