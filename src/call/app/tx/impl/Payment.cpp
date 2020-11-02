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

#include <BeastConfig.h>
#include <call/app/tx/impl/Payment.h>
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

// See https://dev.callchain.live/wiki/Transaction_Format#Payment_.280.29

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
    bool const partialPaymentAllowed = uTxFlags & tfPartialPayment;
    auto const paths = ctx.tx.isFieldPresent(sfPaths);
    auto const sendMax = ctx.tx[~sfSendMax];

    AccountID const srcAccountID(ctx.tx[sfAccount]);
    AccountID const uDstAccountID(ctx.tx[sfDestination]);
    STAmount const saDstAmount(ctx.tx[sfAmount]);

    // Check currency's issue set
    // TODO, fix when dstAmount's issuer is source account
    TER ter1 = checkIssue(ctx, saDstAmount, false);
    if (ter1 != tesSUCCESS)
    {
        JLOG(ctx.j.trace()) << "doPayment: preclaim, issue set not exists or not valid, dstAmount: " 
            << saDstAmount.getFullText();
        return ter1;
    }

    // sendMax not support invoice token
    if (sendMax)
    {
        // TODO, when sendMax issuer is account_
        TER ter2 = checkIssue(ctx, *sendMax, true);
        if (ter2 != tesSUCCESS)
        {
            JLOG(ctx.j.trace()) << "doPayment: preclaim, issue set not exists or not valid, sendMax: " 
                << (*sendMax).getFullText();
            return ter2;
        }
    }

    // check invoice id
    if (!saDstAmount.native())
    {
        auto const issueRoot = ctx.view.read(keylet::issuet(saDstAmount));
        std::uint32_t const issueFlags = issueRoot->getFieldU32(sfFlags);
        if ((issueFlags & tfInvoiceEnable) != 0)
        {
            if (!ctx.tx.isFieldPresent(sfInvoiceID))
            {
                JLOG(ctx.j.trace()) << "doPayment: preclaim, invoice id not present";
                return temBAD_INVOICEID;
            }
            auto const issued = issueRoot->getFieldAmount(sfIssued);
            auto const id = ctx.tx.getFieldH256 (sfInvoiceID);
            auto const sleInvoice = ctx.view.read(keylet::invoicet(id, issued.issue().account, issued.issue().currency));
            if (issued.issue().account == srcAccountID)
            {
                // issue, check invoice field present
                if (!ctx.tx.isFieldPresent(sfInvoice))
                {
                JLOG(ctx.j.trace()) << "doPayment: issue invoice but invoice field not present";
                return temBAD_INVOICE;
                }
                // check invoice not exists
                if (sleInvoice)
                {
                JLOG(ctx.j.trace()) << "doPayment: invoice id exists already, id=" << to_string(id);
                return temID_EXISTED;
                }
            }
            else
            {
                // transfer, check invoice id exists
                if (!sleInvoice)
                {
                    JLOG(ctx.j.trace()) << "doPayment, invoice not exists, id=" << to_string(id);
                    return temINVOICE_NOT_EXISTS;
                }
            }
        }       
    }

    auto const srck = keylet::account(srcAccountID);
    auto const sleSrc = ctx.view.read(srck);

    auto const k = keylet::account(uDstAccountID);
    auto const sleDst = ctx.view.read(k);

    if (!sleDst)
    {
        // Destination account does not exist.
        if (!saDstAmount.native())
        {
            JLOG(ctx.j.trace()) << "Delay transaction: Destination account does not exist.";
            // Another transaction could create the account and then this
            // transaction would succeed.
            return tecNO_DST;
        }
        else if (ctx.view.open() && partialPaymentAllowed)
        {
            // You cannot fund an account with a partial payment.
            // Make retry work smaller, by rejecting this.
            JLOG(ctx.j.trace()) << "Delay transaction: Partial payment not allowed to create account.";
            // Another transaction could create the account and then this
            // transaction would succeed.
            return telNO_DST_PARTIAL;
        }
        else if (saDstAmount < STAmount(ctx.view.fees().accountReserve(0)))
        {
            // accountReserve is the minimum amount that an account can have.
            // Reserve is not scaled by load.
            JLOG(ctx.j.trace()) <<
                "Delay transaction: Destination account does not exist. " <<
                "Insufficent payment to create account.";

            // TODO: dedupe
            // Another transaction could create the account and then this
            // transaction would succeed.
            return tecNO_DST_INSUF_CALL;
        }
    } 
    else if ((sleDst->getFlags() & lsfRequireDestTag) && !ctx.tx.isFieldPresent(sfDestinationTag))
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

    AccountID const uDstAccountID (ctx_.tx.getAccountID (sfDestination));
    STAmount const saDstAmount (ctx_.tx.getFieldAmount (sfAmount));
    STAmount maxSourceAmount;
    STAmount deliveredAmount = saDstAmount;

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
        sleDst->setAccountID(sfInviter, account_); // set account inviter
        view().insert(sleDst);
    }
    else
    {
        // Tell the engine that we are intending to change the destination
        // account.  The source account gets always charged a fee so it's always
        // marked as modified.
        view().update (sleDst);
    }

    TER terResult = tesSUCCESS;
    bool const bCall = paths || sendMax || !saDstAmount.native ();
    // XXX Should sendMax be sufficient to imply call?

    // 1. do payment
    if (bCall)
    {
        auto const sleIssue = view().peek(keylet::issuet(saDstAmount));
        // not send back to issuer and AutoTrust is flags set, do autotrust
        if (saDstAmount.getIssuer() != uDstAccountID && (sleDst->getFieldU32 (sfFlags) & lsfAutoTrust) != 0)
        {
            auto const sleState = view().peek(keylet::line(saDstAmount.getIssuer(), uDstAccountID, saDstAmount.getCurrency()));

            assert(sleState);

            terResult = auto_trust(view(), uDstAccountID, sleIssue->getFieldAmount(sfTotal), j_);
            if (!isTesSuccess(terResult)) return terResult;
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
            else {
                ctx_.deliver (rc.actualAmountOut);
                deliveredAmount = rc.actualAmountOut;
            }
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

        // TODO, moved to view.cpp
        // if invoice, process invoice create or transfer
        auto const uIssueFlags = sleIssue->getFieldU32(sfFlags);
        if (isTesSuccess(terResult) && (uIssueFlags & tfInvoiceEnable) != 0)
        {
            uint256 id = ctx_.tx.getFieldH256(sfInvoiceID);
            uint256 index = getInvoiceIndex(id, saDstAmount.issue().account, saDstAmount.issue().currency);
            if (account_ == saDstAmount.issue().account) // issue invoice
            {
                // create invoice
                Blob invoice = ctx_.tx.getFieldVL(sfInvoice);
                JLOG(j_.trace()) << "doPayment: create invoice root, index=" << to_string(index);
                terResult = invoiceCreate(view(), uDstAccountID, id, invoice, index, saDstAmount, j_);
            }
            else
            {
                // transfer invoice
                terResult = invoiceTransfer(view(), account_, uDstAccountID, index,
                        uDstAccountID == saDstAmount.issue().account, j_);
            }
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

    // 2. call account code
    if (!isTesSuccess(terResult))
        return terResult;

    // success but no do call code
    if ((uTxFlags & tfNoCodeCall) != 0)
    {
        return terResult; // should be tesSUCCESS
    }

    if (sleDst->isFieldPresent(sfCode))
    {
        try
        {
            terResult = doCodeCall(deliveredAmount);
        }
        catch (const char * msg)
        {
            JLOG(j_.warn()) << "doCodeCall fee run out exception=" << msg;
            return tecCODE_FEE_OUT;
        }
        catch (std::exception &e)
        {
            JLOG(j_.warn()) << "doCodeCall exception=" << e.what();
            return tecINTERNAL;
        }
    }

    return terResult;
}

TER
Payment::doCodeCall(STAmount const& amount)
{
    TER terResult = tesSUCCESS;
    
    AccountID const uDstAccountID (ctx_.tx.getAccountID (sfDestination));

    Blob code = view().read(keylet::account(uDstAccountID))->getFieldVL(sfCode);
    std::string codeS = strCopy(code);
    std::string bytecode = UncompressData(codeS);

    lua_State *L = luaL_newstate();
    lua_atpanic(L, panic_handler);
    luaL_openlibs(L);
    RegisterContractLib(L); // register cpp functions for lua contract

    auto const beforeFeeLimit = feeLimit();
    std::int64_t drops = beforeFeeLimit.drops();
    lua_setdrops(L, drops);

    // load and call code
    int lret = luaL_loadbuffer(L, bytecode.data(), bytecode.size(), "") || lua_pcall(L, 0, 0, 0);
    if (lret != LUA_OK)
    {
        JLOG(j_.warn()) << "Fail to call load account code, error=" << lret;
        drops = lua_getdrops(L);
        lua_close(L);
        return isFeeRunOut(drops) ? tecCODE_FEE_OUT : tecCODE_LOAD_FAILED;
    }

    lua_getglobal(L, "main");
    // push parameters, collect parameters if exists
    lua_newtable(L);
    if (ctx_.tx.isFieldPresent(sfArgs))
    {
        auto const& args = ctx_.tx.getFieldArray(sfArgs);
        call_push_args(L, args);
    }

    // set currency transactor in registry table
    lua_pushlightuserdata(L, (void *)&ctx_.app);
    ContractData cd;
    cd.contractor = this;
    cd.address = uDstAccountID;
    lua_pushlightuserdata(L, (void *)&cd);
    lua_settable(L, LUA_REGISTRYINDEX);

    // set global parameters for lua contract
    lua_newtable(L); // for msg
    call_push_string(L, "address", to_string(uDstAccountID));
    call_push_string(L, "sender", to_string(account_));
    call_push_integer(L, "block", ctx_.app.getLedgerMaster().getCurrentLedgerIndex());
    if (amount.native())
    {
        call_push_string(L, "value", to_string(amount.call()));
    }
    else
    {
        lua_pushstring(L, "value");
        lua_newtable(L);
        call_push_string(L, "value", amount.getText());
        call_push_string(L, "currency", to_string(amount.getCurrency()));
        call_push_string(L, "issuer", to_string(amount.getIssuer()));
        lua_settable(L, -3);
    }
    lua_setglobal(L, "msg");

    // restore lua contract variable
    RestoreLuaTable(L, uDstAccountID);

    lret = lua_pcall(L, 1, 1, 0);
    if (lret != LUA_OK)
    {
        JLOG(j_.warn()) << "Fail to call account code main, error=" << lret;
        drops = lua_getdrops(L);
        lua_close(L);
        return isFeeRunOut(drops) ? tecCODE_FEE_OUT : tecCODE_CALL_FAILED;
    }

    // get result
    terResult = TER(lua_tointeger(L, -1));
    lua_pop(L, 1);
    if (isNotSuccess(terResult))
    {
        int r = terResult;
        terResult = TER(r + 1000);
    }

    drops = lua_getdrops(L);
    terResult = isFeeRunOut(drops) ? tecCODE_FEE_OUT : terResult;

    // save lua contract variable
    if (isTesSuccess(terResult))
    {
        terResult = SaveLuaTable(L, uDstAccountID);
    }

    // close lua state
    lua_close(L);
    return terResult;
}

}  // call
