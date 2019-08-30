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
*/
//==============================================================================
#include <BeastConfig.h>
#include <call/app/tx/impl/CreateContract.h>

#include <call/app/paths/CallCalc.h>
#include <call/basics/Log.h>
#include <call/core/Config.h>
#include <call/protocol/st.h>
#include <call/protocol/TxFlags.h>
#include <call/protocol/JsonFields.h>
#include <call/protocol/Feature.h>
#include <call/protocol/Quality.h>
#include <call/protocol/Indexes.h>
#include <call/protocol/st.h>
#include <call/ledger/View.h>
#include <call/basics/StringUtilities.h>
#include <lua.hpp>

namespace call
{

TER CreateContract::preflight(PreflightContext const &ctx)
{
	auto const ret = preflight1(ctx);
    if (!isTesSuccess(ret))
        return ret;
    
    // check fields present
    auto const amount = ctx.tx.getFieldAmount(sfAmount);
    if (!amount)
    {
        JLOG(ctx.j.trace()) << "CreateContract::prefligh missing amount";
        return temBAD_AMOUNT;
    }
    if (!amount.native())
    {
        JLOG(ctx.j.trace()) << "CreateContract::prefligh amount should be native amount";
        return temBAD_AMOUNT;
    }
    if (amount < zero)
    {
        JLOG(ctx.j.trace()) << "CreateContract::prefligh amount should be positive amount";
        return temBAD_AMOUNT;
    }
    auto const hasCode = ctx.tx.isFieldPresent(sfCode);
    if (!hasCode)
    {
        JLOG(ctx.j.trace()) << "CreateContract::prefligh missing code";
        return temBAD_CODE;
    }

	return tesSUCCESS;
}

TER CreateContract::preclaim(PreclaimContext const &ctx)
{
	return tesSUCCESS;
}

TER CreateContract::doApply()
{
	TER terResult = tesSUCCESS;
    STAmount const saAmount = ctx_.tx.getFieldAmount(sfAmount);
    auto const reserve = ctx_.view().fees().accountReserve(0);
    if (saAmount.call() < reserve)
    {
        JLOG(j_.trace()) << "CreateContract::amount too low for reserve";
        return tecINSUFFICIENT_RESERVE;
    }

    auto const srcAccount = view().read(keylet::account(account_));
    // uOwnerCount is the number of entries in this legder for this
    // account that require a reserve.
	auto const uOwnerCount = srcAccount->getFieldU32 (sfOwnerCount);

    // This is the total reserve in drops.
    auto const reserve = view().fees().accountReserve(uOwnerCount);

    // mPriorBalance is the balance on the sending account BEFORE the
    // fees were charged. We want to make sure we have enough reserve
    // to send. Allow final spend to use reserve for fee.
    auto const mmm = std::max(reserve, ctx_.tx.getFieldAmount (sfFee).call ());

    if (mPriorBalance < saAmount.call () + mmm)
    {
        // Vote no. However the transaction might succeed, if applied in
        // a different order.
        JLOG(j_.trace()) << "Delay transaction: Insufficient funds: " <<
            " " << to_string (mPriorBalance) <<
            " / " << to_string (saAmount.call () + mmm) <<
            " (" << to_string (reserve) << ")";
        return tecUNFUNDED_CONTRACT;
    }

    Blob const code = ctx_.tx.getFieldVL(sfCode);
    std::string codeText = strCopy(code);
    if (checkFuncNumber(codeText, "Main") != 1)
    {
        JLOG(j_.trace()) << "CreateContract Main function count: " << checkFuncNumber(codeText, "Main");
        return temBADCODE_MAIN_FUNCTION;
    }

    // call Main function
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    // set fee limit
    auto const feeLimit = ctx_.tx.getFieldAmount (sfFee);
    lua_setfee(L, boost::lexical_cast<unsigned long long>(feeLimit.getText()));

    

	return terResult;
}

} // namespace call
