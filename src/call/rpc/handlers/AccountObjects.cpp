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
    Copyright (c) 2012-2014 Ripple Labs Inc.

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
#include <call/json/json_writer.h>
#include <call/app/main/Application.h>
#include <call/ledger/ReadView.h>
#include <call/net/RPCErr.h>
#include <call/protocol/ErrorCodes.h>
#include <call/protocol/Indexes.h>
#include <call/protocol/JsonFields.h>
#include <call/protocol/LedgerFormats.h>
#include <call/resource/Fees.h>
#include <call/rpc/Context.h>
#include <call/rpc/impl/RPCHelpers.h>
#include <call/rpc/impl/Tuning.h>

#include <string>
#include <sstream>

namespace call {

/** General RPC command that can retrieve objects in the account root.
    {
      account: <account>|<account_public_key>
      ledger_hash: <string> // optional
      ledger_index: <string | unsigned integer> // optional
      type: <string> // optional, defaults to all account objects types
      limit: <integer> // optional
      marker: <opaque> // optional, resume previous query
    }
*/

Json::Value doAccountObjects (RPC::Context& context)
{
    auto const& params = context.params;
    if (! params.isMember (jss::account))
        return RPC::missing_field_error (jss::account);

    std::shared_ptr<ReadView const> ledger;
    auto result = RPC::lookupLedger (ledger, context);
    if (ledger == nullptr)
        return result;

    AccountID accountID;
    {
        auto const strIdent = params[jss::account].asString ();
        if (auto jv = RPC::accountFromString (accountID, strIdent))
        {
            for (auto it = jv.begin (); it != jv.end (); ++it)
                result[it.memberName ()] = *it;

            return result;
        }
    }

    if (! ledger->exists(keylet::account (accountID)))
        return rpcError (rpcACT_NOT_FOUND);

    auto type = RPC::chooseLedgerEntryType(params);
    if (type.first)
    {
        result.clear();
        type.first.inject(result);
        return result;
    }

    unsigned int limit;
    if (auto err = readLimitField(limit, RPC::Tuning::accountObjects, context))
        return *err;

    uint256 dirIndex;
    uint256 entryIndex;
    if (params.isMember (jss::marker))
    {
        auto const& marker = params[jss::marker];
        if (! marker.isString ())
            return RPC::expected_field_error (jss::marker, "string");

        std::stringstream ss (marker.asString ());
        std::string s;
        if (!std::getline(ss, s, ','))
            return RPC::invalid_field_error (jss::marker);

        if (! dirIndex.SetHex (s))
            return RPC::invalid_field_error (jss::marker);

        if (! std::getline (ss, s, ','))
            return RPC::invalid_field_error (jss::marker);

        if (! entryIndex.SetHex (s))
            return RPC::invalid_field_error (jss::marker);
    }

    if (! RPC::getAccountObjects (*ledger, accountID, type.second,
        dirIndex, entryIndex, limit, result))
    {
        result[jss::account_objects] = Json::arrayValue;
    }

    result[jss::account] = context.app.accountIDCache().toBase58 (accountID);
    context.loadType = Resource::feeMediumBurdenRPC;
    return result;
}

} // call
