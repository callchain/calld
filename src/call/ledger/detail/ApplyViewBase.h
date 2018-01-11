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

#ifndef CALL_LEDGER_APPLYVIEWBASE_H_INCLUDED
#define CALL_LEDGER_APPLYVIEWBASE_H_INCLUDED

#include <call/ledger/ApplyView.h>
#include <call/ledger/CashDiff.h>
#include <call/ledger/OpenView.h>
#include <call/ledger/ReadView.h>
#include <call/ledger/detail/ApplyStateTable.h>
#include <call/protocol/CALLAmount.h>

namespace call {
namespace detail {

class ApplyViewBase
    : public ApplyView
    , public RawView
{
public:
    ApplyViewBase() = delete;
    ApplyViewBase (ApplyViewBase const&) = delete;
    ApplyViewBase& operator= (ApplyViewBase&&) = delete;
    ApplyViewBase& operator= (ApplyViewBase const&) = delete;


    ApplyViewBase (ApplyViewBase&&) = default;

    ApplyViewBase(
        ReadView const* base, ApplyFlags flags);

    // ReadView
    bool
    open() const override;

    LedgerInfo const&
    info() const override;

    Fees const&
    fees() const override;

    Rules const&
    rules() const override;

    bool
    exists (Keylet const& k) const override;

    boost::optional<key_type>
    succ (key_type const& key, boost::optional<
        key_type> const& last = boost::none) const override;

    std::shared_ptr<SLE const>
    read (Keylet const& k) const override;

    std::unique_ptr<sles_type::iter_base>
    slesBegin() const override;

    std::unique_ptr<sles_type::iter_base>
    slesEnd() const override;

    std::unique_ptr<sles_type::iter_base>
    slesUpperBound(uint256 const& key) const override;

    std::unique_ptr<txs_type::iter_base>
    txsBegin() const override;

    std::unique_ptr<txs_type::iter_base>
    txsEnd() const override;

    bool
    txExists (key_type const& key) const override;

    tx_type
    txRead (key_type const& key) const override;

    // ApplyView

    ApplyFlags
    flags() const override;

    std::shared_ptr<SLE>
    peek (Keylet const& k) override;

    void
    erase (std::shared_ptr<
        SLE> const& sle) override;

    void
    insert (std::shared_ptr<
        SLE> const& sle) override;

    void
    update (std::shared_ptr<
        SLE> const& sle) override;

    // RawView

    void
    rawErase (std::shared_ptr<
        SLE> const& sle) override;

    void
    rawInsert (std::shared_ptr<
        SLE> const& sle) override;

    void
    rawReplace (std::shared_ptr<
        SLE> const& sle) override;

    void
    rawDestroyCALL (
        CALLAmount const& feeDrops) override;

    friend
    CashDiff cashFlowDiff (
        CashFilter lhsFilter, ApplyViewBase const& lhs,
        CashFilter rhsFilter, ApplyViewBase const& rhs);

protected:
    ApplyFlags flags_;
    ReadView const* base_;
    detail::ApplyStateTable items_;
};

} // detail
} // call

#endif
