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

#ifndef CALL_TX_APPLYCONTEXT_H_INCLUDED
#define CALL_TX_APPLYCONTEXT_H_INCLUDED

#include <call/app/main/Application.h>
#include <call/ledger/ApplyViewImpl.h>
#include <call/core/Config.h>
#include <call/protocol/STTx.h>
#include <call/protocol/CALLAmount.h>
#include <call/beast/utility/Journal.h>
#include <boost/optional.hpp>
#include <utility>

namespace call {

// tx_enable_test

/** State information when applying a tx. */
class ApplyContext
{
public:
    explicit
    ApplyContext (Application& app, OpenView& base,
        STTx const& tx, TER preclaimResult,
            std::uint64_t baseFee, ApplyFlags flags,
                    beast::Journal = {});

    Application& app;
    STTx const& tx;
    TER const preclaimResult;
    std::uint64_t const baseFee;
    beast::Journal const journal;
    CALLAmount extraFee;

    ApplyView&
    view()
    {
        return *view_;
    }

    ApplyView const&
    view() const
    {
        return *view_;
    }

    // VFALCO Unfortunately this is necessary
    RawView&
    rawView()
    {
        return *view_;
    }

    /** Sets the DeliveredAmount field in the metadata */
    void
    deliver (STAmount const& amount)
    {
        view_->deliver(amount);
    }

    /** Discard changes and start fresh. */
    void
    discard();

    /** Apply the transaction result to the base. */
    void
    apply (TER);

    /** Get the number of unapplied changes. */
    std::size_t
    size ();

    /** Visit unapplied changes. */
    void
    visit (std::function <void (
        uint256 const& key,
        bool isDelete,
        std::shared_ptr <SLE const> const& before,
        std::shared_ptr <SLE const> const& after)> const& func);

    void
    destroyCALL (CALLAmount const& fee)
    {
        view_->rawDestroyCALL(fee);
    }

    TER
    checkInvariants(TER);

private:
    template<std::size_t... Is>
    TER checkInvariantsHelper(TER terResult, std::index_sequence<Is...>);

    OpenView& base_;
    ApplyFlags flags_;
    boost::optional<ApplyViewImpl> view_;
};

} // call

#endif
