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
  Copyright (c) 2012-2016 Ripple Labs Inc.

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

#include <call/app/tx/impl/InvariantCheck.h>
#include <call/basics/Log.h>

namespace call {

void
CALLNotCreated::visitEntry(
    uint256 const&,
    bool isDelete,
    std::shared_ptr <SLE const> const& before,
    std::shared_ptr <SLE const> const& after)
{
    if(before)
    {
        switch (before->getType())
        {
        case ltACCOUNT_ROOT:
            drops_ -= (*before)[sfBalance].call().drops();
            break;
        case ltPAYCHAN:
            drops_ -= ((*before)[sfAmount] - (*before)[sfBalance]).call().drops();
            break;
        case ltESCROW:
            drops_ -= (*before)[sfAmount].call().drops();
            break;
        default:
            break;
        }
    }

    if(after)
    {
        switch (after->getType())
        {
        case ltACCOUNT_ROOT:
            drops_ += (*after)[sfBalance].call().drops();
            break;
        case ltPAYCHAN:
            if (! isDelete)
                drops_ += ((*after)[sfAmount] - (*after)[sfBalance]).call().drops();
            break;
        case ltESCROW:
            if (! isDelete)
                drops_ += (*after)[sfAmount].call().drops();
            break;
        default:
            break;
        }
    }
}

bool
CALLNotCreated::finalize(STTx const& tx, TER /*tec*/, beast::Journal const& j)
{
    auto fee = tx.getFieldAmount(sfFee).call().drops();
    if(-1*fee <= drops_ && drops_ <= 0)
        return true;

    JLOG(j.fatal()) << "Invariant failed: CALL net change was " << drops_ <<
        " on a fee of " << fee;
    return false;
}

//------------------------------------------------------------------------------

void
CALLBalanceChecks::visitEntry(
    uint256 const&,
    bool,
    std::shared_ptr <SLE const> const& before,
    std::shared_ptr <SLE const> const& after)
{
    auto isBad = [](STAmount const& balance)
    {
        if (!balance.native())
            return true;

        auto const drops = balance.call().drops();

        // Can't have more than the number of drops instantiated
        // in the genesis ledger.
        if (drops > SYSTEM_CURRENCY_START)
            return true;

        // Can't have a negative balance (0 is OK)
        if (drops < 0)
            return true;

        return false;
    };

    if(before && before->getType() == ltACCOUNT_ROOT)
        bad_ |= isBad ((*before)[sfBalance]);

    if(after && after->getType() == ltACCOUNT_ROOT)
        bad_ |= isBad ((*after)[sfBalance]);
}

bool
CALLBalanceChecks::finalize(STTx const&, TER, beast::Journal const& j)
{
    if (bad_)
    {
        JLOG(j.fatal()) << "Invariant failed: incorrect account CALL balance";
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------

void
NoBadOffers::visitEntry(
    uint256 const&,
    bool isDelete,
    std::shared_ptr <SLE const> const& before,
    std::shared_ptr <SLE const> const& after)
{
    auto isBad = [](STAmount const& pays, STAmount const& gets)
    {
        // An offer should never be negative
        if (pays < beast::zero)
            return true;

        if (gets < beast::zero)
            return true;

        // Can't have an CALL to CALL offer:
        return pays.native() && gets.native();
    };

    if(before && before->getType() == ltOFFER)
        bad_ |= isBad ((*before)[sfTakerPays], (*before)[sfTakerGets]);

    if(after && after->getType() == ltOFFER)
        bad_ |= isBad((*after)[sfTakerPays], (*after)[sfTakerGets]);
}

bool
NoBadOffers::finalize(STTx const& tx, TER, beast::Journal const& j)
{
    if (bad_)
    {
        JLOG(j.fatal()) << "Invariant failed: offer with a bad amount";
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------

void
NoZeroEscrow::visitEntry(
    uint256 const&,
    bool isDelete,
    std::shared_ptr <SLE const> const& before,
    std::shared_ptr <SLE const> const& after)
{
    auto isBad = [](STAmount const& amount)
    {
        if (!amount.native())
            return true;

        if (amount.call().drops() <= 0)
            return true;

        if (amount.call().drops() >= SYSTEM_CURRENCY_START)
            return true;

        return false;
    };

    if(before && before->getType() == ltESCROW)
        bad_ |= isBad((*before)[sfAmount]);

    if(after && after->getType() == ltESCROW)
        bad_ |= isBad((*after)[sfAmount]);
}

bool
NoZeroEscrow::finalize(STTx const& tx, TER, beast::Journal const& j)
{
    if (bad_)
    {
        JLOG(j.fatal()) << "Invariant failed: escrow specifies invalid amount";
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------

void
AccountRootsNotDeleted::visitEntry(
    uint256 const&,
    bool isDelete,
    std::shared_ptr <SLE const> const& before,
    std::shared_ptr <SLE const> const&)
{
    if (isDelete && before && before->getType() == ltACCOUNT_ROOT)
        accountDeleted_ = true;
}

bool
AccountRootsNotDeleted::finalize(STTx const&, TER, beast::Journal const& j)
{
    if (! accountDeleted_)
        return true;

    JLOG(j.fatal()) << "Invariant failed: an account root was deleted";
    return false;
}

//------------------------------------------------------------------------------

void
LedgerEntryTypesMatch::visitEntry(
    uint256 const&,
    bool,
    std::shared_ptr <SLE const> const& before,
    std::shared_ptr <SLE const> const& after)
{
    if (before && after && before->getType() != after->getType())
        typeMismatch_ = true;

    if (after)
    {
        switch (after->getType())
        {
        case ltACCOUNT_ROOT:
        case ltDIR_NODE:
        case ltCALL_STATE:
        case ltTICKET:
        case ltSIGNER_LIST:
        case ltOFFER:
        case ltLEDGER_HASHES:
        case ltAMENDMENTS:
        case ltFEE_SETTINGS:
        case ltESCROW:
        case ltPAYCHAN:
        case ltNICKNAME:
        case ltISSUEROOT:
        case ltFeeRoot:
        case ltINVOICE:
            break;
        default:
            invalidTypeAdded_ = true;
            break;
        }
    }
}

bool
LedgerEntryTypesMatch::finalize(STTx const&, TER, beast::Journal const& j)
{
    if ((! typeMismatch_) && (! invalidTypeAdded_))
        return true;

    if (typeMismatch_)
    {
        JLOG(j.fatal()) << "Invariant failed: ledger entry type mismatch";
    }

    if (invalidTypeAdded_)
    {
        JLOG(j.fatal()) << "Invariant failed: invalid ledger entry type added";
    }

    return false;
}

//------------------------------------------------------------------------------

void
NoCALLTrustLines::visitEntry(
    uint256 const&,
    bool,
    std::shared_ptr <SLE const> const&,
    std::shared_ptr <SLE const> const& after)
{
    if (after && after->getType() == ltCALL_STATE)
    {
        // checking the issue directly here instead of
        // relying on .native() just in case native somehow
        // were systematically incorrect
        callTrustLine_ =
            after->getFieldAmount (sfLowLimit).issue() == callIssue() ||
            after->getFieldAmount (sfHighLimit).issue() == callIssue();
    }
}

bool
NoCALLTrustLines::finalize(STTx const&, TER, beast::Journal const& j)
{
    if (! callTrustLine_)
        return true;

    JLOG(j.fatal()) << "Invariant failed: an CALL trust line was created";
    return false;
}

}  // call

