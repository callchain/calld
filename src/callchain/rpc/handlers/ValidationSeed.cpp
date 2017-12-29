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
#include <callchain/app/main/Application.h>
#include <callchain/core/Config.h>
#include <callchain/net/RPCErr.h>
#include <callchain/protocol/ErrorCodes.h>
#include <callchain/protocol/JsonFields.h>
#include <callchain/rpc/Context.h>
#include <callchain/basics/make_lock.h>
#include <iostream>

namespace callchain {

// {
//   secret: <string>
// }
Json::Value doValidationSeed (RPC::Context& context)
{
    // TODO: This feature is temporarily disabled since we
    // cannot modify the config object. We should consider
    // whether we want the ability to change the validator
    // keys at runtime.
    return rpcError (rpcNOT_IMPL);

    // auto lock = make_lock(context.app.getMasterMutex());
    // Json::Value obj (Json::objectValue);

    // if (!context.params.isMember (jss::secret))
    // {
    //     std::cerr << "Unset validation seed." << std::endl;

    //     context.app.config().VALIDATION_SEED.clear ();
    //     context.app.config().VALIDATION_PUB.clear ();
    //     context.app.config().VALIDATION_PRIV.clear ();
    // }
    // else if (!context.app.config().VALIDATION_SEED.setSeedGeneric (
    //     context.params[jss::secret].asString ()))
    // {
    //     context.app.config().VALIDATION_PUB.clear ();
    //     context.app.config().VALIDATION_PRIV.clear ();

    //     return rpcError (rpcBAD_SEED);
    // }
    // else
    // {
    //     auto& seed = context.app.config().VALIDATION_SEED;
    //     auto& pub = context.app.config().VALIDATION_PUB;

    //     pub = RippleAddress::createNodePublic (seed);
    //     context.app.config().VALIDATION_PRIV = RippleAddress::createNodePrivate (seed);

    //     obj[jss::validation_public_key] = pub.humanNodePublic ();
    //     obj[jss::validation_seed] = seed.humanSeed ();
    //     obj[jss::validation_key] = seed.humanSeed1751 ();
    // }

    // return obj;
}

} // callchain
