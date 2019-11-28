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

#include <BeastConfig.h>
#include <call/app/tx/impl/Contractor.h>
#include <call/app/tx/impl/ApplyContext.h>
#include <call/app/paths/CallCalc.h>
#include <call/app/contract/ContractLib.h>
#include <call/app/ledger/LedgerMaster.h>
#include <call/basics/Log.h>
#include <call/core/Config.h>
#include <call/protocol/st.h>
#include <call/protocol/TxFlags.h>
#include <call/protocol/JsonFields.h>
#include <call/basics/StringUtilities.h>
#include <lua-vm/src/lua.hpp>

namespace call {

void
Contractor::preCompute ()
{
    Transactor::preCompute();
}

// --- callchain sytem call for lua contract ---
TER 
Contractor::doTransfer(AccountID const& toAccountID, STAmount const& amount)
{
    // check issue
    if (!amount.native())
    {
        std::shared_ptr<SLE const> sle = view().read(keylet::issuet(amount));
        if (!sle)
        {
            return temCURRENCY_NOT_ISSUE;
        }
        std::uint32_t const uIssueFlags = sle->getFieldU32(sfFlags);
        if ((uIssueFlags & tfInvoiceEnable) != 0)
        {
            // not support invoice now
            return temNOT_SUPPORT;
        }
    }

    // from=contract, to, amount
    AccountID const uContractID (ctx_.tx.getAccountID (sfDestination));
    SLE::pointer sleSrc = view().peek (keylet::account(uContractID));
    if (!sleSrc) {
        return temBAD_SRC_ACCOUNT;
    }

    SLE::pointer sleDst = view().peek (keylet::account(toAccountID));
    if (!sleDst)
    {
        if (!amount.native())
        {
            return tecNO_DST;
        }
        if (amount < STAmount(view().fees().accountReserve(0)))
        {
            return tecNO_DST_INSUF_CALL;
        }

        // Create the account.
        sleDst = std::make_shared<SLE>(keylet::account(toAccountID));
        sleDst->setAccountID(sfAccount, toAccountID);
        sleDst->setFieldAmount(sfBalance, 0);
        sleDst->setFieldU32(sfSequence, 1);
        view().insert(sleDst);
    }
    else
    {
        view().update (sleDst);
    }

    TER terResult = tesSUCCESS;
    if (!amount.native())
    {
        // not send back to issuer and AutoTrust is flags set, do autotrust
        if (amount.getIssuer() != toAccountID && (sleDst->getFieldU32 (sfFlags) & lsfAutoTrust) != 0)
        {
            auto const sleState = view().peek(keylet::line(amount.getIssuer(), toAccountID, amount.getCurrency()));
            if (!sleState)
            {
                auto const sleIssue = view().peek(keylet::issuet(amount));
                terResult = auto_trust(view(), toAccountID, sleIssue->getFieldAmount(sfTotal), j_);
                if (!isTesSuccess(terResult))
                {
                    return terResult;
                }
            }
        }

        path::CallCalc::Input rcInput;
        rcInput.partialPaymentAllowed = false;
        rcInput.defaultPathsAllowed = true;
        rcInput.limitQuality = false;
        rcInput.isLedgerOpen = view().open();
        STPathSet const empty{};

        path::CallCalc::Output rc;
        {
            PaymentSandbox pv(&view());
            rc = path::CallCalc::callCalculate (
                pv,
                amount,
                amount,
                toAccountID,
                uContractID,
                empty,
                ctx_.app.logs(),
                &rcInput);
            pv.apply(ctx_.rawView());
        }
        terResult = rc.result();
    }
    else
    {
        // Direct CALL payment.
        auto const uOwnerCount = sleSrc->getFieldU32 (sfOwnerCount);
        auto const reserve = view().fees().accountReserve(uOwnerCount);
        auto const mmm = reserve;
        auto mPriorBalance = sleSrc->getFieldAmount (sfBalance);
        if (mPriorBalance < amount.call() + mmm)
        {
            return tecUNFUNDED_PAYMENT;
        }
        sleSrc->setFieldAmount (sfBalance, mPriorBalance - amount);
        sleDst->setFieldAmount (sfBalance, sleDst->getFieldAmount (sfBalance) + amount);
    }

    return terResult;
}

TER
Contractor::doIssueSet(STAmount const & total, std::uint32_t rate, Blob const& info)
{
    SLE::pointer sle = view().peek(keylet::issuet(total));
    if (sle)
    {
        return temCURRENCY_ISSUED;
    }

    /**
     * not allowed additional, not non-nft
     */
    auto const& issue = total.issue();
    uint256 index = getIssueIndex(issue.account, issue.currency);
	return issueSetCreate(view(), issue.account, total, rate, 0, index, info, j_);
}

void
Contractor::doContractPrint(std::string const& data)
{
    // only print in trace log level
    JLOG(j_.trace()) << "Contract Data: " << data;
}


}  // call
