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
#include <call/app/tx/impl/InvokeContract.h>

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

TER InvokeContract::preflight(PreflightContext const &ctx)
{
	auto const ret = preflight1(ctx);
    if (!isTesSuccess(ret))
        return ret;
    
    // check fields present
    auto const hasDestination = ctx.tx.isFieldPresent(sfDestination);
    if (!hasDestination)
    {
        JLOG(ctx.j.trace()) << "CreateContract::prefligh missing destination";
        return temDST_NEEDED;
    }
    auto const hasFunction = ctx.tx.isFieldPresent(sfFunction);
    if (!hasFunction)
    {
        JLOG(ctx.j.trace()) << "CreateContract::prefligh missing function";
        return temBAD_FUNCTION;
    }

	return tesSUCCESS;
}

TER InvokeContract::preclaim(PreclaimContext const &ctx)
{
	return tesSUCCESS;
}

TER InvokeContract::doApply()
{
	TER terResult = tesSUCCESS;
	
	return terResult;
}

} // namespace call
