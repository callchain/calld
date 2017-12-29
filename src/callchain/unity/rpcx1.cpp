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

#include <callchain/rpc/handlers/AccountCurrenciesHandler.cpp>
#include <callchain/rpc/handlers/AccountInfo.cpp>
#include <callchain/rpc/handlers/AccountLines.cpp>
#include <callchain/rpc/handlers/AccountChannels.cpp>
#include <callchain/rpc/handlers/AccountObjects.cpp>
#include <callchain/rpc/handlers/AccountOffers.cpp>
#include <callchain/rpc/handlers/AccountTx.cpp>
#include <callchain/rpc/handlers/AccountTxOld.cpp>
#include <callchain/rpc/handlers/AccountTxSwitch.cpp>
#include <callchain/rpc/handlers/BlackList.cpp>
#include <callchain/rpc/handlers/BookOffers.cpp>
#include <callchain/rpc/handlers/CanDelete.cpp>
#include <callchain/rpc/handlers/Connect.cpp>
#include <callchain/rpc/handlers/ConsensusInfo.cpp>
#include <callchain/rpc/handlers/Feature1.cpp>
#include <callchain/rpc/handlers/Fee1.cpp>
#include <callchain/rpc/handlers/FetchInfo.cpp>
#include <callchain/rpc/handlers/GatewayBalances.cpp>
#include <callchain/rpc/handlers/GetCounts.cpp>
#include <callchain/rpc/handlers/LedgerHandler.cpp>
#include <callchain/rpc/handlers/LedgerAccept.cpp>
#include <callchain/rpc/handlers/LedgerCleanerHandler.cpp>
#include <callchain/rpc/handlers/LedgerClosed.cpp>
#include <callchain/rpc/handlers/LedgerCurrent.cpp>
#include <callchain/rpc/handlers/LedgerData.cpp>
#include <callchain/rpc/handlers/LedgerEntry.cpp>
#include <callchain/rpc/handlers/LedgerHeader.cpp>
#include <callchain/rpc/handlers/LedgerRequest.cpp>
#include <callchain/rpc/handlers/LogLevel.cpp>
#include <callchain/rpc/handlers/LogRotate.cpp>
#include <callchain/rpc/handlers/NoRippleCheck.cpp>
#include <callchain/rpc/handlers/OwnerInfo.cpp>
