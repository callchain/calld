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
#include <call/app/tx/impl/IssueSet.h>

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
#include <lua-vm/src/lua.hpp>

namespace call
{

TER IssueSet::preflight(PreflightContext const &ctx)
{
	auto const ret = preflight1(ctx);
	if (!isTesSuccess(ret))
		return ret;

	auto& tx = ctx.tx;
	auto& j = ctx.j;

	std::uint32_t const uTxFlags = tx.getFlags ();
	if (uTxFlags & tfIssueSetMask)
    {
        JLOG(j.trace()) << "Malformed transaction: Invalid flags set.";
        return temINVALID_FLAG;
    }

	// TransferRate
    if (tx.isFieldPresent (sfTransferRate))
    {
        std::uint32_t uRate = tx.getFieldU32 (sfTransferRate);

        if (uRate && (uRate < QUALITY_ONE))
        {
            JLOG(j.trace()) << "Malformed transaction: Transfer rate too small.";
            return temBAD_TRANSFER_RATE;
        }

        if (ctx.rules.enabled(fix1201) && (uRate > 2 * QUALITY_ONE))
        {
            JLOG(j.trace()) << "Malformed transaction: Transfer rate too large.";
            return temBAD_TRANSFER_RATE;
        }
    }

	return tesSUCCESS;
}

TER IssueSet::preclaim(PreclaimContext const &ctx)
{
	STAmount totalAmount = ctx.tx.getFieldAmount(sfTotal);
	if (totalAmount.native() || !ctx.tx.isFieldPresent(sfTotal))
	{
		return temBAD_TOTAL_AMOUNT;
	}
	// only allow issue owner token
	AccountID uSrcAccount = ctx.tx.getAccountID(sfAccount);
	if (totalAmount.getIssuer() != uSrcAccount)
	{
		return tecNO_AUTH;
	}

	auto const sle = ctx.view.read(keylet::issuet(totalAmount));
	if (!sle) return tesSUCCESS;

	std::uint32_t const uFlagsIn = sle->getFieldU32(sfFlags);
	auto const totalAmountIn = sle->getFieldAmount(sfTotal);
	std::uint32_t const uTxFlags = ctx.tx.getFlags();

	// no transfer rate for invoice
	if ((uTxFlags & tfInvoiceEnable) != 0 && ctx.tx.isFieldPresent(sfTransferRate))
	{
		return temINVOICE_NO_FEE;
	}

	// not allow additional want to set be additional
	if ((uTxFlags & tfAdditional) != 0 && (uFlagsIn & lsfAdditional) ==0)
	{
		return temNOT_ADDITIONAL;
	}

	// not allow additional issue more amount
	if ((uFlagsIn & lsfAdditional) == 0 && (totalAmount > totalAmountIn))
	{
		return temNOT_ADDITIONAL;
	}

	/*
	 * TODO, add back next version
	// forbidden double code set when code fixed
	if (ctx.tx.isFieldPresent(sfCode) && (uFlagsIn & lsfCodeFixed) != 0)
	{
		return temCODE_FIXED;
	}

	// when set code fixed, but no code present
	if ((uTxFlags & tfCodeFixed) != 0 
			&& (!ctx.tx.isFieldPresent(sfCode) && !sle->isFieldPresent(sfCode)))
	{
		return temNO_CODE;
	}

	// check code account
    if (ctx.tx.isFieldPresent(sfCode))
    {
        std::string code = strCopy(ctx.tx.getFieldVL(sfCode));
        lua_State *L = luaL_newstate();
        luaL_openlibs(L);
        lua_setdrops(L, (unsigned long long)-1);
        // check code
        int lret = luaL_dostring(L, code.c_str());
        if (lret != LUA_OK)
        {
            JLOG(ctx.j.warn()) << "invalid issue code, error=" << lret;
            return temINVALID_CODE;
        }
        lua_close(L);
    }
	*/

	return tesSUCCESS;
}

TER IssueSet::doApply()
{
	TER terResult = tesSUCCESS;
	std::uint32_t const uTxFlags = ctx_.tx.getFlags();
	STAmount totalAmount = ctx_.tx.getFieldAmount(sfTotal);

	auto viewJ = ctx_.app.journal("View");

	SLE::pointer sle = view().peek(keylet::issuet(totalAmount));
	if (!sle)
	{
		uint256 index = getIssueIndex(account_, totalAmount.getCurrency());
		JLOG(j_.trace()) << "IssueSet: Creating IssueRoot " << to_string(index);
		std::uint32_t rate = ctx_.tx.getFieldU32(sfTransferRate);
		Blob info = ctx_.tx.getFieldVL(sfInfo);
		/*
		 * TODO, add back next version
		Blob code = ctx_.tx.getFieldVL(sfCode);
		*/
		terResult = issueSetCreate(view(), account_, totalAmount, rate, uTxFlags, index, info, viewJ);
	}
	else
	{
		std::uint32_t const uFlagsIn = sle->getFieldU32(sfFlags);
		std::uint32_t uFlagsOut = uFlagsIn;
		auto saOldTotal = sle->getFieldAmount(sfTotal);
		
		if ((uFlagsIn & tfAdditional) != 0 && totalAmount > saOldTotal)
		{
			sle->setFieldAmount(sfTotal, totalAmount);
		}
		
		// if (ctx_.tx.isFieldPresent(sfCode))
		// {
		// 	sle->setFieldVL(sfCode, ctx_.tx.getFieldVL(sfCode));
		// }

		if ((uFlagsIn & lsfNonFungible) == 0 && ctx_.tx.isFieldPresent(sfTransferRate))
		{
			sle->setFieldU32(sfTransferRate, ctx_.tx.getFieldU32(sfTransferRate));
		}
		// Additional -> non additional
		if ((uFlagsIn & lsfAdditional) != 0 && (uTxFlags & tfAdditional) == 0)
		{
			uFlagsOut &= ~lsfAdditional;
		}

		/*
		 * TODO, add back next version
		// non code fixed -> code fixed
		if ((uFlagsIn & lsfCodeFixed) == 0 && (uTxFlags & tfCodeFixed) != 0)
		{
			uFlagsOut |= lsfCodeFixed;
		}
		*/
		
		if (uFlagsIn != uFlagsOut)
		{
			sle->setFieldU32 (sfFlags, uFlagsOut);
		}

		/**
		 *  code global variable
		 *  expired time
		 *  enable payment
		 *  enable offer
		 *
		 */
		/*
		 * TODO, add back next version
		if (ctx_.tx.isFieldPresent (sfCode))
		{
			// TODO code owner reserve
			auto uCode = ctx_.tx[sfCode];
			sle->setFieldVL(sfCode, uCode);
		}
		*/
		
		view().update(sle);
	}

	return terResult;
}

} // namespace call
