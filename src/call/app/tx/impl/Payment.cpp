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

#include <BeastConfig.h>
#include <call/app/tx/impl/Payment.h>
#include <call/app/paths/CallCalc.h>
#include <call/basics/Log.h>
#include <call/core/Config.h>
#include <call/protocol/st.h>
#include <call/protocol/TxFlags.h>
#include <call/protocol/JsonFields.h>

namespace call {

// See https://call.com/wiki/Transaction_Format#Payment_.280.29

CALLAmount
Payment::calculateMaxSpend(STTx const& tx)
{
    if (tx.isFieldPresent(sfSendMax))
    {
        auto const& sendMax = tx[sfSendMax];
        return sendMax.native() ? sendMax.call() : beast::zero;
    }
    /* If there's no sfSendMax in CALL, and the sfAmount isn't
    in CALL, then the transaction can not send CALL. */
    auto const& saDstAmount = tx.getFieldAmount(sfAmount);
    return saDstAmount.native() ? saDstAmount.call() : beast::zero;
}

TER
Payment::preflight (PreflightContext const& ctx)
{
    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    auto& tx = ctx.tx;
    auto& j = ctx.j;

    std::uint32_t const uTxFlags = tx.getFlags ();

    if (uTxFlags & tfPaymentMask)
    {
        JLOG(j.trace()) << "Malformed transaction: " <<
            "Invalid flags set.";
        return temINVALID_FLAG;
    }

    bool const partialPaymentAllowed = uTxFlags & tfPartialPayment;
    bool const limitQuality = uTxFlags & tfLimitQuality;
    bool const defaultPathsAllowed = !(uTxFlags & tfNoCallDirect);
    bool const bPaths = tx.isFieldPresent (sfPaths);
    bool const bMax = tx.isFieldPresent (sfSendMax);

    STAmount const saDstAmount (tx.getFieldAmount (sfAmount));

    STAmount maxSourceAmount;
    auto const account = tx.getAccountID(sfAccount);

    if (bMax)
        maxSourceAmount = tx.getFieldAmount (sfSendMax);
    else if (saDstAmount.native ())
        maxSourceAmount = saDstAmount;
    else
        maxSourceAmount = STAmount (
            { saDstAmount.getCurrency (), account },
            saDstAmount.mantissa(), saDstAmount.exponent (),
            saDstAmount < zero);

    auto const& uSrcCurrency = maxSourceAmount.getCurrency ();
    auto const& uDstCurrency = saDstAmount.getCurrency ();

    // isZero() is CALL.  FIX!
    bool const bCALLDirect = uSrcCurrency.isZero () && uDstCurrency.isZero ();

    if (!isLegalNet (saDstAmount) || !isLegalNet (maxSourceAmount))
        return temBAD_AMOUNT;

    auto const uDstAccountID = tx.getAccountID (sfDestination);

    if (!uDstAccountID)
    {
        JLOG(j.trace()) << "Malformed transaction: " << "Payment destination account not specified.";
        return temDST_NEEDED;
    }
    if (bMax && maxSourceAmount <= zero)
    {
        JLOG(j.trace()) << "Malformed transaction: " << "bad max amount: " << maxSourceAmount.getFullText ();
        return temBAD_AMOUNT;
    }
    if (saDstAmount <= zero)
    {
        JLOG(j.trace()) << "Malformed transaction: " << "bad dst amount: " << saDstAmount.getFullText ();
        return temBAD_AMOUNT;
    }
    if (badCurrency() == uSrcCurrency || badCurrency() == uDstCurrency)
    {
        JLOG(j.trace()) <<"Malformed transaction: " << "Bad currency.";
        return temBAD_CURRENCY;
    }
    if (account == uDstAccountID && uSrcCurrency == uDstCurrency && !bPaths)
    {
        // You're signing yourself a payment.
        // If bPaths is true, you might be trying some arbitrage.
        JLOG(j.trace()) << "Malformed transaction: " <<
            "Redundant payment from " << to_string (account) <<
            " to self without path for " << to_string (uDstCurrency);
        return temREDUNDANT;
    }
    if (bCALLDirect && bMax)
    {
        // Consistent but redundant transaction.
        JLOG(j.trace()) << "Malformed transaction: " << "SendMax specified for CALL to CALL.";
        return temBAD_SEND_CALL_MAX;
    }
    if (bCALLDirect && bPaths)
    {
        // CALL is sent without paths.
        JLOG(j.trace()) << "Malformed transaction: " << "Paths specified for CALL to CALL.";
        return temBAD_SEND_CALL_PATHS;
    }
    if (bCALLDirect && partialPaymentAllowed)
    {
        // Consistent but redundant transaction.
        JLOG(j.trace()) << "Malformed transaction: " << "Partial payment specified for CALL to CALL.";
        return temBAD_SEND_CALL_PARTIAL;
    }
    if (bCALLDirect && limitQuality)
    {
        // Consistent but redundant transaction.
        JLOG(j.trace()) << "Malformed transaction: " << "Limit quality specified for CALL to CALL.";
        return temBAD_SEND_CALL_LIMIT;
    }
    if (bCALLDirect && !defaultPathsAllowed)
    {
        // Consistent but redundant transaction.
        JLOG(j.trace()) << "Malformed transaction: " << "No call direct specified for CALL to CALL.";
        return temBAD_SEND_CALL_NO_DIRECT;
    }

    auto const deliverMin = tx[~sfDeliverMin];
    if (deliverMin)
    {
        if (! partialPaymentAllowed)
        {
            JLOG(j.trace()) << "Malformed transaction: Partial payment not specified for " 
                << jss::DeliverMin.c_str() << ".";
            return temBAD_AMOUNT;
        }

        auto const dMin = *deliverMin;
        if (!isLegalNet(dMin) || dMin <= zero)
        {
            JLOG(j.trace()) << "Malformed transaction: Invalid " << jss::DeliverMin.c_str() 
                << " amount. " << dMin.getFullText();
            return temBAD_AMOUNT;
        }
        if (dMin.issue() != saDstAmount.issue())
        {
            JLOG(j.trace()) <<  "Malformed transaction: Dst issue differs "
                "from " << jss::DeliverMin.c_str() << ". " << dMin.getFullText();
            return temBAD_AMOUNT;
        }
        if (dMin > saDstAmount)
        {
            JLOG(j.trace()) << "Malformed transaction: Dst amount less than " <<
                jss::DeliverMin.c_str() << ". " << dMin.getFullText();
            return temBAD_AMOUNT;
        }
    }

    return preflight2 (ctx);
}

TER
Payment::preclaim(PreclaimContext const& ctx)
{

    // Call if source or destination is non-native or if there are paths.
    std::uint32_t const uTxFlags = ctx.tx.getFlags();
    // bool const partialPaymentAllowed = uTxFlags & tfPartialPayment;
    auto const paths = ctx.tx.isFieldPresent(sfPaths);
    auto const sendMax = ctx.tx[~sfSendMax];

    AccountID const srcAccountID(ctx.tx[sfAccount]);
    AccountID const uDstAccountID(ctx.tx[sfDestination]);
    STAmount const saDstAmount(ctx.tx[sfAmount]);

    auto const srck = keylet::account(srcAccountID);
    auto const sleSrc = ctx.view.read(srck);

    auto const k = keylet::account(uDstAccountID);
    auto const sleDst = ctx.view.read(k);

    // if (!sleDst)
    // {
    //     // Destination account does not exist.
    //     if (!saDstAmount.native())
    //     {
    //         JLOG(ctx.j.trace()) <<
    //             "Delay transaction: Destination account does not exist.";

    //         // Another transaction could create the account and then this
    //         // transaction would succeed.
    //         return tecNO_DST;
    //     }
    //     else if (ctx.view.open()
    //         && partialPaymentAllowed)
    //     {
    //         // You cannot fund an account with a partial payment.
    //         // Make retry work smaller, by rejecting this.
    //         JLOG(ctx.j.trace()) <<
    //             "Delay transaction: Partial payment not allowed to create account.";


    //         // Another transaction could create the account and then this
    //         // transaction would succeed.
    //         return telNO_DST_PARTIAL;
    //     }
    //     else if (saDstAmount < STAmount(ctx.view.fees().accountReserve(0)))
    //     {
    //         // accountReserve is the minimum amount that an account can have.
    //         // Reserve is not scaled by load.
    //         JLOG(ctx.j.trace()) <<
    //             "Delay transaction: Destination account does not exist. " <<
    //             "Insufficent payment to create account.";

    //         // TODO: dedupe
    //         // Another transaction could create the account and then this
    //         // transaction would succeed.
    //         return tecNO_DST_INSUF_CALL;
    //     }
    // }

    if (sleDst != NULL && (sleDst->getFlags() & lsfRequireDestTag) && !ctx.tx.isFieldPresent(sfDestinationTag))
    {
        // The tag is basically account-specific information we don't
        // understand, but we can require someone to fill it in.
        // We didn't make this test for a newly-formed account because there's
        // no way for this field to be set.
        JLOG(ctx.j.trace()) << "Malformed transaction: DestinationTag required.";

        return tecDST_TAG_NEEDED;
    }

    if (paths || sendMax || !saDstAmount.native())
    {
        // Call payment with at least one intermediate step and uses
        // transitive balances.

        // Copy paths into an editable class.
        STPathSet const spsPaths = ctx.tx.getFieldPathSet(sfPaths);

        auto pathTooBig = spsPaths.size() > MaxPathSize;

        if (!pathTooBig) {
            for (auto const& path : spsPaths) {
                if (path.size() > MaxPathLength)
                {
                    pathTooBig = true;
                    break;
                }
            }
        }

        if (ctx.view.open() && pathTooBig)
        {
            return telBAD_PATH_COUNT; // Too many paths for proposed ledger.
        }
    }

    return tesSUCCESS;
}


TER
Payment::doApply ()
{
    auto const deliverMin = ctx_.tx[~sfDeliverMin];

    // Call if source or destination is non-native or if there are paths.
    std::uint32_t const uTxFlags = ctx_.tx.getFlags ();
    bool const partialPaymentAllowed = uTxFlags & tfPartialPayment;
    bool const limitQuality = uTxFlags & tfLimitQuality;
    bool const defaultPathsAllowed = !(uTxFlags & tfNoCallDirect);
    auto const paths = ctx_.tx.isFieldPresent(sfPaths);
    auto const sendMax = ctx_.tx[~sfSendMax];
    bool issuing = false;

    AccountID const uDstAccountID (ctx_.tx.getAccountID (sfDestination));
    STAmount const saDstAmount (ctx_.tx.getFieldAmount (sfAmount));
    STAmount maxSourceAmount;

    // Check issue set exists
    if (!checkIssue(ctx_, saDstAmount, false))
    {
        return temBAD_FUNDS;
    }
    if (sendMax && !checkIssue(ctx_, *sendMax, false))
    {
        return temBAD_FUNDS;
    }

    if (sendMax)
        maxSourceAmount = *sendMax;
    else if (saDstAmount.native ())
        maxSourceAmount = saDstAmount;
    else
        maxSourceAmount = STAmount (
            {saDstAmount.getCurrency (), account_},
            saDstAmount.mantissa(), saDstAmount.exponent (),
            saDstAmount < zero);

    JLOG(j_.trace()) << "maxSourceAmount=" << maxSourceAmount.getFullText () 
        << " saDstAmount=" << saDstAmount.getFullText ();

    // Open a ledger for editing.
    auto const k = keylet::account(uDstAccountID);
    SLE::pointer sleDst = view().peek (k);

    if (!sleDst)
    {
        // Create the account.
        sleDst = std::make_shared<SLE>(k);
        sleDst->setAccountID(sfAccount, uDstAccountID);
        sleDst->setFieldAmount(sfBalance, 0);
        sleDst->setFieldU32(sfSequence, 1);
        view().insert(sleDst);
    }
    else
    {
        // Tell the engine that we are intending to change the destination
        // account.  The source account gets always charged a fee so it's always
        // marked as modified.
        view().update (sleDst);
    }

    TER terResult;

    bool const bCall = paths || sendMax || !saDstAmount.native ();
    // XXX Should sendMax be sufficient to imply call?

    if (bCall)
    {
        // Call payment with at least one intermediate step and uses
        AccountID AIssuer = saDstAmount.getIssuer();
        Currency currency = saDstAmount.getCurrency();

        SLE::pointer sleIssueRoot = view().peek(keylet::issuet(AIssuer, currency));
        STAmount issued = sleIssueRoot->getFieldAmount(sfIssued);
        std::uint32_t const uIssueFlags = sleIssueRoot->getFieldU32(sfFlags);

        bool const is_nft = ((uIssueFlags & tfNonFungible) != 0);
        if (is_nft)
        {
            // id not present
            if (!ctx_.tx.isFieldPresent(sfTokenID))
            {
                return temBAD_TOKENID;
            }
            // amount not 1
            STAmount one(saDstAmount.issue(), 1);
            if (saDstAmount != one)
            {
                return temBAD_AMOUNT;
            }
        }

        // source account is issue account
        if (AIssuer == account_ && issued.issue() == saDstAmount.issue())
        {
            // update issue set
            if (issued + saDstAmount > sleIssueRoot->getFieldAmount(sfTotal))
            {
                return tecOVERISSUED_AMOUNT;
            }
            // else add issused
            sleIssueRoot->setFieldAmount(sfIssued, issued + saDstAmount);
            view().update(sleIssueRoot);

            // when issuer issue new one, check if id exists already return error else create one
            if (is_nft)
            {
                issuing = true;
                uint256 id = ctx_.tx.getFieldH256 (sfTokenID);
                SLE::pointer sleTokenRoot = view().peek(keylet::tokent(id, account_, currency));
                // issue should not create same id token
                if (sleTokenRoot) 
                {
                    return temID_EXISTED;
                }
                auto const isMeta = ctx_.tx.isFieldPresent(sfMetaInfo);
                if (!isMeta)
                {
                    return temBAD_METAINFO;
                }
 
                Blob metaInfo = ctx_.tx.getFieldVL(sfMetaInfo);
                uint256 uCIndex(getTokenIndex(id, account_, currency));
	            JLOG(j_.trace()) << "doPayment: Creating TokenRoot: " << to_string(uCIndex);
	            terResult = AccountTokenCreate(view(), uDstAccountID, id, metaInfo, uCIndex, saDstAmount, j_);                    
                if (terResult != tesSUCCESS)
                {
                    return terResult;
                }
            }
        }

        //  update uDst balance
        if (mActivation != 0)
		{
		    sleDst->setFieldAmount(sfBalance, sleDst->getFieldAmount(sfBalance) + mActivation);
		}

	    STAmount total = sleIssueRoot->getFieldAmount(sfTotal);
        if (saDstAmount.getIssuer() != uDstAccountID)
        {
		    SLE::pointer sleCallState = view().peek(keylet::line(saDstAmount.getIssuer(), uDstAccountID, currency));
		    if (!sleCallState)
		    {
			    auto result = auto_trust(view(), uDstAccountID, total, j_);
			    if (result != tesSUCCESS)
                {
                    return result;
                }

                sleIssueRoot->setFieldU64(sfFans, sleIssueRoot->getFieldU64(sfFans) + 1);
                view().update(sleIssueRoot);
		    }
        }
        // Copy paths into an editable class.
        STPathSet spsPaths = ctx_.tx.getFieldPathSet (sfPaths);

        path::CallCalc::Input rcInput;
        rcInput.partialPaymentAllowed = partialPaymentAllowed;
        rcInput.defaultPathsAllowed = defaultPathsAllowed;
        rcInput.limitQuality = limitQuality;
        rcInput.isLedgerOpen = view().open();

        path::CallCalc::Output rc;
        {
            PaymentSandbox pv(&view());
            JLOG(j_.debug()) << "Entering CallCalc in payment: " << ctx_.tx.getTransactionID();
            rc = path::CallCalc::callCalculate (
                pv,
                maxSourceAmount,
                saDstAmount,
                uDstAccountID,
                account_,
                spsPaths,
                ctx_.app.logs(),
                &rcInput);
            // VFALCO NOTE We might not need to apply, depending on the TER. 
            // But always applying *should* be safe.
            pv.apply(ctx_.rawView());
        }

        // TODO: is this right?  If the amount is the correct amount, was
        // the delivered amount previously set?
        if (rc.result () == tesSUCCESS && rc.actualAmountOut != saDstAmount)
        {
            if (deliverMin && rc.actualAmountOut < *deliverMin)
                rc.setResult (tecPATH_PARTIAL);
            else
                ctx_.deliver (rc.actualAmountOut);
        }

        terResult = rc.result ();

        // Because of its overhead, if CallCalc
        // fails with a retry code, claim a fee
        // instead. Maybe the user will be more
        // careful with their path spec next time.
        if (isTerRetry (terResult)) 
        {
            terResult = tecPATH_DRY;
        }

        // update token owner, when not issue
        if (terResult == tesSUCCESS && is_nft && !issuing)
        {
            uint256 id = ctx_.tx.getFieldH256 (sfTokenID);
            uint256 uCIndex(getTokenIndex(id, AIssuer, currency));
            terResult = TokenTransfer(view(), account_, uDstAccountID, currency, 
                uCIndex, uDstAccountID == AIssuer, j_);
        }
    }
    else
    {
        assert (saDstAmount.native ());

        // Direct CALL payment.

        // uOwnerCount is the number of entries in this legder for this
        // account that require a reserve.
        auto const uOwnerCount = view().read(keylet::account(account_))->getFieldU32 (sfOwnerCount);

        // This is the total reserve in drops.
        auto const reserve = view().fees().accountReserve(uOwnerCount);

        // mPriorBalance is the balance on the sending account BEFORE the
        // fees were charged. We want to make sure we have enough reserve
        // to send. Allow final spend to use reserve for fee.
        auto const mmm = std::max(reserve, ctx_.tx.getFieldAmount (sfFee).call ());

        if (mPriorBalance < saDstAmount.call () + mmm)
        {
            // Vote no. However the transaction might succeed, if applied in
            // a different order.
            JLOG(j_.trace()) << "Delay transaction: Insufficient funds: " <<
                " " << to_string (mPriorBalance) <<
                " / " << to_string (saDstAmount.call () + mmm) <<
                " (" << to_string (reserve) << ")";

            terResult = tecUNFUNDED_PAYMENT;
        }
        else
        {
            // The source account does have enough money, so do the
            // arithmetic for the transfer and make the ledger change.
            view().peek(keylet::account(account_))->setFieldAmount (sfBalance, mSourceBalance - saDstAmount);
            sleDst->setFieldAmount (sfBalance, sleDst->getFieldAmount (sfBalance) + saDstAmount);

            // Re-arm the password change fee if we can and need to.
            if ((sleDst->getFlags () & lsfPasswordSpent))
            {
                sleDst->clearFlag (lsfPasswordSpent);
            }

            terResult = tesSUCCESS;
        }
    }

    return terResult;
}

}  // call
