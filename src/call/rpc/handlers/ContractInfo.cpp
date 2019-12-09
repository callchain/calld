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

#include <call/app/main/Application.h>
#include <call/app/misc/TxQ.h>
#include <call/app/contract/ContractLib.h>
#include <call/json/json_value.h>
#include <call/ledger/ReadView.h>
#include <call/protocol/ErrorCodes.h>
#include <call/protocol/Indexes.h>
#include <call/protocol/JsonFields.h>
#include <call/protocol/types.h>
#include <call/rpc/Context.h>
#include <call/rpc/impl/RPCHelpers.h>
#include <call/basics/StringUtilities.h>
#include <call/json/json_value.h>
#include <call/json/json_reader.h>
#include <call/json/json_writer.h>

namespace call {

// {
//   account: <ident>,
//   ArgName: <string>,
//   strict: <bool>        // optional (default false)
//                         //   if true only allow public keys and addresses.
//   ledger_hash : <ledger>
//   ledger_index : <ledger_index>
//                         //   if true return SignerList(s).
//   queue : <bool>        // optional (default false)
//                         //   if true return information about transactions
//                         //   in the current TxQ, only if the requested
//                         //   ledger is open. Otherwise if true, returns an
//                         //   error.
// }

Json::Value doContractInfo (RPC::Context& context)
{
    auto& params = context.params;

    std::string strIdent;
    if (params.isMember(jss::account))
        strIdent = params[jss::account].asString();
    else if (params.isMember(jss::ident))
        strIdent = params[jss::ident].asString();
    else
        return RPC::missing_field_error (jss::account);
    
    std::string argname;
	Json::Value result;
	if (params.isMember(jss::ArgName))
	{
		argname = params[jss::ArgName].asString();
	}
	else
	{
		RPC::inject_error(rpcINVALID_PARAMS, result);
		return result;
	}

    std::shared_ptr<ReadView const> ledger;
    result = RPC::lookupLedger (ledger, context);

    if (!ledger)
        return result;

    bool bStrict = params.isMember (jss::strict) && params[jss::strict].asBool ();
    AccountID accountID;

    // Get info on account.
    auto jvAccepted = RPC::accountFromString (accountID, strIdent, bStrict);
    if (jvAccepted)
        return jvAccepted;

    auto const sleAccepted = ledger->read(keylet::paramt(getParamIndex(accountID)));
    if (sleAccepted)
    {
        Blob data = sleAccepted->getFieldVL(sfInfo);
        std::string input = UncompressData(strCopy(data));
        Json::Value root;
        Json::Reader reader;
        if (reader.parse(input, root))
        {
            auto const& value = root[argname];
            if (!value)
            {
                result[jss::account] = context.app.accountIDCache().toBase58 (accountID);
                result[jss::ArgName] = argname;
                RPC::inject_error (rpcPARAM_NOT_FOUND, result);
            }
            else
            {
                result[jss::account] = context.app.accountIDCache().toBase58 (accountID);
                result[jss::ArgName] = argname;
                result[jss::info] = value;
            }
        }
        else
        {
            result[jss::account] = context.app.accountIDCache().toBase58 (accountID);
            result[jss::ArgName] = argname;
            RPC::inject_error (rpcINVALID_CONTRACT_DATA, result);
        }
    }
    else
    {
        result[jss::account] = context.app.accountIDCache().toBase58 (accountID);
        result[jss::ArgName] = argname;
        RPC::inject_error (rpcACCOUNT_NO_DATA, result);
    }

    return result;
}


} // call
