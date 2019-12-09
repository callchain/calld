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
//   NickName: <string>,
// }

// TODO(tom): what is that "default"?
Json::Value	doNicknameInfo(RPC::Context& context)
{
	std::string nick;
	Json::Value result;
	auto& params = context.params;
	if (params.isMember(jss::NickName))
	{
		nick = params[jss::NickName].asString();
	}
	else
	{
		RPC::inject_error(rpcINVALID_PARAMS, result);
		return result;
	}

	std::shared_ptr<ReadView const> ledger;
	result = RPC::lookupLedger(ledger, context);

	if (!ledger)
		return result;
	
	Blob blobname = strCopy(nick);
    std::shared_ptr<SLE const> sle = cachedRead(*ledger, getNicknameIndex(blobname), ltNICKNAME);

	if (!sle)
	{
		RPC::inject_error(rpcNICKACCOUNT_NOT_FOUND, result);
		return result;
	}

	auto nickaccount = toBase58(sle->getAccountID(sfAccount));
    context.params[jss::account] = nickaccount;
	result = doAccountInfo(context);
	return result;
}

} // call
