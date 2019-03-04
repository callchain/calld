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
        JLOG(j.trace()) <<
            "Malformed transaction: Invalid flags set.";
        return temINVALID_FLAG;
    }

	return tesSUCCESS;
}

TER IssueSet::preclaim(PreclaimContext const &ctx)
{
	return tesSUCCESS;
}

TER IssueSet::doApply()
{
	TER terResult = tesSUCCESS;
	std::uint32_t const uTxFlags = ctx_.tx.getFlags();
	STAmount satotal = ctx_.tx.getFieldAmount(sfTotal);
	Currency currency = satotal.getCurrency();
	auto viewJ = ctx_.app.journal("View");
	if (satotal.native())
	{
		return temBAD_CURRENCY;
	}

	// account only allowed to issue self currency
	if (satotal.getIssuer() != account_)
	{
		return tecNO_AUTH;
	}

	SLE::pointer sleIssueRoot = view().peek(keylet::issuet(account_, currency));
	// not isused yet
	if (!sleIssueRoot)
	{
		uint256 uCIndex(getIssueIndex(account_, currency));
		JLOG(j_.trace()) << "doTrustSet: Creating IssueRoot: " << to_string(uCIndex);
		terResult = AccountIssuerCreate(view(), account_, satotal, uTxFlags, uCIndex, viewJ);
		if (terResult == tesSUCCESS) {
			SLE::pointer sleRoot = view().peek (keylet::account(account_));
			adjustOwnerCount(view(), sleRoot, 1, viewJ);
		}
	}
	// old issue setting
	else
	{
		auto oldtotal = sleIssueRoot->getFieldAmount(sfTotal);
		std::uint32_t const flags = sleIssueRoot->getFieldU32(sfFlags);
		// not allow to edit
		if ((flags & tfEnaddition) == 0)
		{
			return tecNO_AUTH;
		}

		// not allow to update total amount of issue smaller than current amount
		if (satotal <= oldtotal)
		{
			return tecBADTOTAL;
		}

		sleIssueRoot->setFieldAmount(sfTotal, satotal);
		view().update(sleIssueRoot);
		JLOG(j_.trace()) << "apendent the total";
	}
	return terResult;
}

} // namespace call
