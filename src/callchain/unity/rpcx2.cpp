//------------------------------------------------------------------------------
/*
    This file is part of callchaind: https://github.com/callchain/callchaind
    Copyright (c) 2012, 2013 Ripple Labs Inc.

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

// This has to be included early to prevent an obscure MSVC compile error
#include <boost/asio/deadline_timer.hpp>

#include <callchain/protocol/JsonFields.h>
#include <callchain/rpc/RPCHandler.h>
#include <callchain/rpc/handlers/Handlers.h>

#include <callchain/rpc/handlers/PathFind.cpp>
#include <callchain/rpc/handlers/PayChanClaim.cpp>
#include <callchain/rpc/handlers/Peers.cpp>
#include <callchain/rpc/handlers/Ping.cpp>
#include <callchain/rpc/handlers/Print.cpp>
#include <callchain/rpc/handlers/Random.cpp>
#include <callchain/rpc/handlers/RipplePathFind.cpp>
#include <callchain/rpc/handlers/ServerInfo.cpp>
#include <callchain/rpc/handlers/ServerState.cpp>
#include <callchain/rpc/handlers/SignFor.cpp>
#include <callchain/rpc/handlers/SignHandler.cpp>
#include <callchain/rpc/handlers/Stop.cpp>
#include <callchain/rpc/handlers/Submit.cpp>
#include <callchain/rpc/handlers/SubmitMultiSigned.cpp>
#include <callchain/rpc/handlers/Subscribe.cpp>
#include <callchain/rpc/handlers/TransactionEntry.cpp>
#include <callchain/rpc/handlers/Tx.cpp>
#include <callchain/rpc/handlers/TxHistory.cpp>
#include <callchain/rpc/handlers/UnlList.cpp>
#include <callchain/rpc/handlers/Unsubscribe.cpp>
#include <callchain/rpc/handlers/ValidationCreate.cpp>
#include <callchain/rpc/handlers/ValidationSeed.cpp>
#include <callchain/rpc/handlers/Validators.cpp>
#include <callchain/rpc/handlers/ValidatorListSites.cpp>
#include <callchain/rpc/handlers/WalletPropose.cpp>
#include <callchain/rpc/handlers/WalletSeed.cpp>

#include <callchain/rpc/impl/Handler.cpp>
#include <callchain/rpc/impl/LegacyPathFind.cpp>
#include <callchain/rpc/impl/Role.cpp>
#include <callchain/rpc/impl/RPCHandler.cpp>
#include <callchain/rpc/impl/RPCHelpers.cpp>
#include <callchain/rpc/impl/ServerHandlerImp.cpp>
#include <callchain/rpc/impl/Status.cpp>
#include <callchain/rpc/impl/TransactionSign.cpp>


