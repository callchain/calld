//------------------------------------------------------------------------------
/*
    This file is part of calld: https://github.com/call/calld
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

#include <call/protocol/JsonFields.h>
#include <call/rpc/RPCHandler.h>
#include <call/rpc/handlers/Handlers.h>

#include <call/rpc/handlers/PathFind.cpp>
#include <call/rpc/handlers/PayChanClaim.cpp>
#include <call/rpc/handlers/Peers.cpp>
#include <call/rpc/handlers/Ping.cpp>
#include <call/rpc/handlers/Print.cpp>
#include <call/rpc/handlers/Random.cpp>
#include <call/rpc/handlers/RipplePathFind.cpp>
#include <call/rpc/handlers/ServerInfo.cpp>
#include <call/rpc/handlers/ServerState.cpp>
#include <call/rpc/handlers/SignFor.cpp>
#include <call/rpc/handlers/SignHandler.cpp>
#include <call/rpc/handlers/Stop.cpp>
#include <call/rpc/handlers/Submit.cpp>
#include <call/rpc/handlers/SubmitMultiSigned.cpp>
#include <call/rpc/handlers/Subscribe.cpp>
#include <call/rpc/handlers/TransactionEntry.cpp>
#include <call/rpc/handlers/Tx.cpp>
#include <call/rpc/handlers/TxHistory.cpp>
#include <call/rpc/handlers/UnlList.cpp>
#include <call/rpc/handlers/Unsubscribe.cpp>
#include <call/rpc/handlers/ValidationCreate.cpp>
#include <call/rpc/handlers/ValidationSeed.cpp>
#include <call/rpc/handlers/Validators.cpp>
#include <call/rpc/handlers/ValidatorListSites.cpp>
#include <call/rpc/handlers/WalletPropose.cpp>
#include <call/rpc/handlers/WalletSeed.cpp>

#include <call/rpc/impl/Handler.cpp>
#include <call/rpc/impl/LegacyPathFind.cpp>
#include <call/rpc/impl/Role.cpp>
#include <call/rpc/impl/RPCHandler.cpp>
#include <call/rpc/impl/RPCHelpers.cpp>
#include <call/rpc/impl/ServerHandlerImp.cpp>
#include <call/rpc/impl/Status.cpp>
#include <call/rpc/impl/TransactionSign.cpp>


