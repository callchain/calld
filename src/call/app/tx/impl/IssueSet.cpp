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
	bool enaddtion = uTxFlags & tfEnaddition;
	STAmount satotal = ctx_.tx.getFieldAmount(sfTotal);
	Currency currency = satotal.getCurrency();
	auto viewJ = ctx_.app.journal("View");
	if (satotal.native())
	{
		return temBAD_CURRENCY;
	}

	SLE::pointer sleIssueRoot = view().peek(keylet::issuet(account_, currency));
	if (!sleIssueRoot)
	{
		uint256 currency_index(getIssueIndex(account_, currency));
		JLOG(j_.trace()) << "doTrustSet: Creating IssueRoot: " << to_string(currency_index);
		terResult = AccountIssuerCreate(view(), account_, satotal, currency_index, viewJ);
	}
	else
	{
		auto oldtotal = sleIssueRoot->getFieldAmount(sfTotal);
		if (!enaddtion)
		{
			return tecNO_AUTH;
		}
		else if (satotal <= oldtotal)
		{
			return tecBADTOTAL;
		}
		else
		{
			sleIssueRoot->setFieldAmount(sfTotal, satotal);
			view().update(sleIssueRoot);
			JLOG(j_.trace()) << "apendent the total";
		}
	}
	return terResult;
}

} // namespace call
