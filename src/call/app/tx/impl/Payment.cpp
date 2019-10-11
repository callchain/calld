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
    auto issueRoot = ctx.view.read(keylet::issuet(saDstAmount));
    if (issueRoot)
    {
        std::uint32_t const issueFlags = issueRoot->getFieldU32(sfFlags);
        if ((issueFlags & tfNonFungible) != 0 && !ctx.tx.isFieldPresent(sfInvoiceID))
        {
            JLOG(ctx.j.trace()) << "doPayment: preclaim, invoice id not present";
            return temBAD_INVOICEID;
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
    bool issuing = false;

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

    // do call check call before
    if (sleDst->isFieldPresent(sfCode))
    {
        terResult = doCodeCheckCall();
        if (terResult != tesSUCCESS)
            return terResult;
    }

    bool const bCall = paths || sendMax || !saDstAmount.native ();
    // XXX Should sendMax be sufficient to imply call?

    if (bCall)
    {
        // Call payment with at least one intermediate step and uses
        AccountID AIssuer = saDstAmount.getIssuer();
        Currency currency = saDstAmount.getCurrency();

        SLE::pointer sleIssueRoot = view().peek(keylet::issuet(saDstAmount));
        STAmount issued = sleIssueRoot->getFieldAmount(sfIssued);
        std::uint32_t const uIssueFlags = sleIssueRoot->getFieldU32(sfFlags);
        bool const is_nft = ((uIssueFlags & tfNonFungible) != 0);
       
        // source account is issue account, its issuing operation
        if (AIssuer == account_ && issued.issue() == saDstAmount.issue())
        {
            // update issue set
            if (issued + saDstAmount > sleIssueRoot->getFieldAmount(sfTotal))
            {
                JLOG(j_.trace()) << "doPayment: issue over amount: " << saDstAmount.getFullText();
                return tecOVERISSUED_AMOUNT;
            }

            // when issuer issue new one, check if id exists already return error else create one
            if (is_nft)
            {
                issuing = true;
                uint256 id = ctx_.tx.getFieldH256 (sfInvoiceID);
                SLE::pointer sleInvoiceRoot = view().peek(keylet::invoicet(id, account_, currency));
                // issue should not create same id invoice
                if (sleInvoiceRoot) 
                {
                    JLOG(j_.trace()) << "doPayment: invoice id exists " << id;
                    return temID_EXISTED;
                }
                auto const isInvoice = ctx_.tx.isFieldPresent(sfInvoice);
                if (!isInvoice)
                {
                    JLOG(j_.trace()) << "doPayment: issue invoice, but invoice not present";
                    return temBAD_INVOICE;
                }
 
                Blob invoice = ctx_.tx.getFieldVL(sfInvoice);
                uint256 uCIndex(getInvoiceIndex(id, account_, currency)); // issuer, currency, id
	            JLOG(j_.trace()) << "doPayment: Creating InvoiceRoot: " << to_string(uCIndex);
	            terResult = invoiceCreate(view(), uDstAccountID, id, invoice, uCIndex, saDstAmount, j_);                    
                if (terResult != tesSUCCESS)
                {
                    return terResult;
                }
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

        // update token owner, when not issue
        if (terResult == tesSUCCESS && is_nft && !issuing)
        {
            uint256 id = ctx_.tx.getFieldH256 (sfInvoiceID);
            uint256 uCIndex(getInvoiceIndex(id, AIssuer, currency));
            terResult = invoiceTransfer(view(), account_, uDstAccountID, 
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

    // before enter code
    if (terResult != tesSUCCESS || (uTxFlags & tfNoCodeCall) != 0)
        return terResult;
    bool hasCode = sleDst->isFieldPresent(sfCode);
    if (!hasCode) return terResult;

    return doCodeCall(deliveredAmount);
}

TER
Payment::doCodeCheckCall()
{
    TER terResult = tesSUCCESS;
    AccountID const uDstAccountID (ctx_.tx.getAccountID (sfDestination));

    Blob code = view().read(keylet::account(uDstAccountID))->getFieldVL(sfCode);
    std::string codeS = strCopy(code);
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    RegisterContractLib(L); // register cpp functions for lua contract

    auto const uSrcSLE = view().read(keylet::account(account_));
    auto const uBalance = uSrcSLE->getFieldAmount(sfBalance).call();
    auto const uOwnerCount = uSrcSLE->getFieldU32 (sfOwnerCount);
    // This is the total reserve in drops.
    auto const reserve = view().fees().accountReserve(uOwnerCount);
    auto const feeLimit = uBalance - reserve;
    unsigned long long drops = boost::lexical_cast<unsigned long long>(feeLimit.drops());
    lua_setdrops(L, drops);

    // load and call code
    int lret = luaL_dostring(L, codeS.c_str());
    if (lret != LUA_OK)
    {
        JLOG(j_.warn()) << "Fail to call load account code, error=" << lret;
        lua_close(L);
        return terCODE_LOAD_FAILED;
    }

    lua_getglobal(L, "check");
    if (!lua_isfunction(L, -1))
    {
        lua_close(L);
        return terResult;
    }

    // set global parameters for later lua glue code internal
    lua_pushlightuserdata(L, this);
    lua_setglobal(L, "__APPLY_CONTEXT_FOR_CALL_CODE");

    // set global parameters for lua contract
    lua_newtable(L); // for msg
    call_push_string(L, "address", to_string(uDstAccountID));
    call_push_string(L, "sender", to_string(account_));
    call_push_string(L, "value", "0");
    lua_setglobal(L, "msg");

    lua_newtable(L); // for block
    call_push_integer(L, "height", ctx_.app.getLedgerMaster().getCurrentLedgerIndex());
    lua_setglobal(L, "block");

    lret = lua_pcall(L, 0, 1, 0);
    if (lret != LUA_OK)
    {
        JLOG(j_.warn()) << "Fail to call account code check, error=" << lret;
        lua_close(L);
        return terCODE_CALL_FAILED;
    }

    // get result
    terResult = TER(lua_tointeger(L, -1));
    lua_pop(L, 1);

    // pay contract feee
    drops = lua_getdrops(L);
    CALLAmount finalAmount (drops);
    auto const feeAmount = feeLimit - finalAmount;
    ctx_.setExtraFee(feeAmount);
    payContractFee(feeAmount);

    // close lua state
    lua_close(L);

    return terResult;
}

TER
Payment::doCodeCall(STAmount const& deliveredAmount)
{
    TER terResult = tesSUCCESS;
    
    AccountID const uDstAccountID (ctx_.tx.getAccountID (sfDestination));

    Blob code = view().read(keylet::account(uDstAccountID))->getFieldVL(sfCode);
    std::string codeS = strCopy(code);
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    RegisterContractLib(L); // register cpp functions for lua contract

    auto const uSrcSLE = view().read(keylet::account(account_));
    auto const uBalance = uSrcSLE->getFieldAmount(sfBalance).call();
    auto const uOwnerCount = uSrcSLE->getFieldU32 (sfOwnerCount);
    // This is the total reserve in drops.
    auto const reserve = view().fees().accountReserve(uOwnerCount);
    auto const feeLimit = uBalance - reserve;
    unsigned long long drops = boost::lexical_cast<unsigned long long>(feeLimit.drops());
    lua_setdrops(L, drops);

    // load and call code
    int lret = luaL_dostring(L, codeS.c_str());
    if (lret != LUA_OK)
    {
        JLOG(j_.warn()) << "Fail to call load account code, error=" << lret;
        lua_close(L);
        return terCODE_LOAD_FAILED;
    }

    lua_getglobal(L, "main");
    // push parameters, collect parameters if exists
    lua_newtable(L);
    if (ctx_.tx.isFieldPresent(sfMemos))
    {
        auto const& memos = ctx_.tx.getFieldArray(sfMemos);
        int n = 0;
        for (auto const& memo : memos)
        {
            auto memoObj = dynamic_cast <STObject const*> (&memo);
            if (!memoObj->isFieldPresent(sfMemoData))
            {
                continue;
            }
            std::string data = strCopy(memoObj->getFieldVL(sfMemoData));
            lua_pushstring(L, data.c_str());
            lua_rawseti(L, -2, n);
            ++n;
        }
    }

    // set global parameters for later lua glue code internal
    lua_pushlightuserdata(L, this);
    lua_setglobal(L, "__APPLY_CONTEXT_FOR_CALL_CODE");

    // set global parameters for lua contract
    lua_newtable(L); // for msg
    call_push_string(L, "address", to_string(uDstAccountID));
    call_push_string(L, "sender", to_string(account_));
    call_push_string(L, "value", deliveredAmount.getJson(0).asString());
    lua_setglobal(L, "msg");

    lua_newtable(L); // for block
    call_push_integer(L, "height", ctx_.app.getLedgerMaster().getCurrentLedgerIndex());
    lua_setglobal(L, "block");

    lret = lua_pcall(L, 1, 1, 0);
    if (lret != LUA_OK)
    {
        JLOG(j_.warn()) << "Fail to call account code main, error=" << lret;
        lua_close(L);
        return terCODE_CALL_FAILED;
    }

    // get result
    terResult = TER(lua_tointeger(L, -1));
    lua_pop(L, 1);

    // pay contract feee
    drops = lua_getdrops(L);
    CALLAmount finalAmount (drops);
    auto const feeAmount = feeLimit - finalAmount;
    ctx_.setExtraFee(feeAmount);
    payContractFee(feeAmount);

    // close lua state
    lua_close(L);

    return terResult;
}

void
Payment::payContractFee(CALLAmount const& feeAmount)
{
    auto const sle = view().peek(keylet::account(account_));

    auto feesle = view().peek(keylet::txfee());
	if (!feesle)
	{
		auto const feeindex = getFeesIndex();
		auto const feesle = std::make_shared<SLE>(ltFeeRoot, feeindex);
		feesle->setFieldAmount(sfBalance, feeAmount);
		view().insert(feesle);
	}
	else
	{
		view().update(feesle);
		auto fee = feesle->getFieldAmount(sfBalance) + feeAmount;
		feesle->setFieldAmount(sfBalance, fee);
	}
    sle->setFieldAmount (sfBalance, sle->getFieldAmount(sfBalance) - feeAmount);
}


TER 
Payment::doTransfer(AccountID const& toAccountID, STAmount const& amount)
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
        if ((uIssueFlags & tfNonFungible) != 0)
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

    TER terResult;
    if (!amount.native())
    {
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

        terResult = tesSUCCESS;
    }

    return terResult;
}

}  // call
