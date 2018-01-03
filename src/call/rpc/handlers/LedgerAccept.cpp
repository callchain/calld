//------------------------------------------------------------------------------
/*
    This file is part of calld: https://github.com/call/calld
    Copyright (c) 2012-2014 Call Labs Inc.

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
#include <call/app/ledger/LedgerMaster.h>
#include <call/app/misc/NetworkOPs.h>
#include <call/app/main/Application.h>
#include <call/core/Config.h>
#include <call/net/RPCErr.h>
#include <call/protocol/ErrorCodes.h>
#include <call/protocol/Indexes.h>
#include <call/protocol/JsonFields.h>
#include <call/rpc/Context.h>
#include <call/basics/make_lock.h>

namespace call {

Json::Value doLedgerAccept (RPC::Context& context)
{
    auto lock = make_lock(context.app.getMasterMutex());
    Json::Value jvResult;

    if (!context.app.config().standalone())
    {
        jvResult[jss::error] = "notStandAlone";
    }
    else
    {
        context.netOps.acceptLedger ();

        jvResult[jss::ledger_current_index] =
            context.ledgerMaster.getCurrentLedgerIndex ();
    }

    return jvResult;
}

} // call
