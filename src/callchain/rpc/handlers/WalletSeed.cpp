//------------------------------------------------------------------------------
/*
    This file is part of callchaind: https://github.com/callchain/callchaind
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
#include <callchain/json/json_value.h>
#include <callchain/net/RPCErr.h>
#include <callchain/protocol/ErrorCodes.h>
#include <callchain/protocol/JsonFields.h>
#include <callchain/protocol/Seed.h>
#include <callchain/rpc/Context.h>

namespace callchain {

// {
//   secret: <string>
// }
Json::Value doWalletSeed (RPC::Context& context)
{
    boost::optional<Seed> seed;

    bool bSecret = context.params.isMember (jss::secret);

    if (bSecret)
        seed = parseGenericSeed (context.params[jss::secret].asString ());
    else
        seed = randomSeed ();

    if (!seed)
        return rpcError (rpcBAD_SEED);

    Json::Value obj (Json::objectValue);
    obj[jss::seed]     = toBase58(*seed);
    obj[jss::key]      = seedAs1751(*seed);
    obj[jss::deprecated] = "Use wallet_propose instead";
    return obj;
}

} // callchain
