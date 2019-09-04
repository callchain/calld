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
	return tesSUCCESS;
}

TER IssueSet::doApply()
{
	TER terResult = tesSUCCESS;
	std::uint32_t const uTxFlags = ctx_.tx.getFlags();

	if (!ctx_.tx.isFieldPresent(sfTotal))
	{
		return temBAD_AMOUNT;
	}

	STAmount satotal = ctx_.tx.getFieldAmount(sfTotal);
	Currency currency = satotal.getCurrency();
	if (satotal.native())
	{
		return temBAD_CURRENCY;
	}
	if (satotal.getIssuer() != account_)
	{
		return tecNO_AUTH;
	}

	bool const is_nft = (uTxFlags & tfNonFungible) != 0;
	if (is_nft && ctx_.tx.isFieldPresent(sfTransferRate))
	{
		return temINVOICE_NO_FEE;
	}

	auto viewJ = ctx_.app.journal("View");

	SLE::pointer sleIssueRoot = view().peek(keylet::issuet(account_, currency));
	// not isused yet
	if (!sleIssueRoot)
	{
		uint256 uCIndex(getIssueIndex(account_, currency));
		JLOG(j_.trace()) << "IssueSet: Creating IssueRoot " << to_string(uCIndex);
		std::uint32_t rate = ctx_.tx.getFieldU32(sfTransferRate);
		terResult = issueSetCreate(view(), account_, satotal, rate, uTxFlags, uCIndex, viewJ);
		if (terResult == tesSUCCESS) {
			SLE::pointer sleRoot = view().peek (keylet::account(account_));
			adjustOwnerCount(view(), sleRoot, 1, viewJ);
		} else {
			return terResult;
		}
	}
	// old issue setting
	else
	{
		auto oldtotal = sleIssueRoot->getFieldAmount(sfTotal);
		std::uint32_t const flags = sleIssueRoot->getFieldU32(sfFlags);
		bool const editable = (flags & tfEnaddition) != 0;
		bool const is_nft2 = (flags & tfNonFungible) != 0;

		if (!editable && (satotal > oldtotal))
		{
			return temNOT_EDITABLE;
		}

		// allow to edit and total is bigger than old total
		if (editable && (satotal > oldtotal))
		{
			sleIssueRoot->setFieldAmount(sfTotal, satotal);
		}
		
		// if has transfer rate, update it
		const bool has_rate = ctx_.tx.isFieldPresent(sfTransferRate);
		if (has_rate && is_nft2) 
		{
			return temINVOICE_NO_FEE;
		}
		if (has_rate && !is_nft2) 
		{
			std::uint32_t rate = ctx_.tx.getFieldU32(sfTransferRate);
			sleIssueRoot->setFieldU32(sfTransferRate, rate);
		}
		
		view().update(sleIssueRoot);
		JLOG(j_.trace()) << "apendent the total";
	}
	return terResult;
}

} // namespace call
