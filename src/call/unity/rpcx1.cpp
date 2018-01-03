//------------------------------------------------------------------------------
/*
    This file is part of calld: https://github.com/call/calld
    Copyright (c) 2012, 2013 Call Labs Inc.

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

#include <call/rpc/handlers/AccountCurrenciesHandler.cpp>
#include <call/rpc/handlers/AccountInfo.cpp>
#include <call/rpc/handlers/AccountLines.cpp>
#include <call/rpc/handlers/AccountChannels.cpp>
#include <call/rpc/handlers/AccountObjects.cpp>
#include <call/rpc/handlers/AccountOffers.cpp>
#include <call/rpc/handlers/AccountTx.cpp>
#include <call/rpc/handlers/AccountTxOld.cpp>
#include <call/rpc/handlers/AccountTxSwitch.cpp>
#include <call/rpc/handlers/BlackList.cpp>
#include <call/rpc/handlers/BookOffers.cpp>
#include <call/rpc/handlers/CanDelete.cpp>
#include <call/rpc/handlers/Connect.cpp>
#include <call/rpc/handlers/ConsensusInfo.cpp>
#include <call/rpc/handlers/Feature1.cpp>
#include <call/rpc/handlers/Fee1.cpp>
#include <call/rpc/handlers/FetchInfo.cpp>
#include <call/rpc/handlers/GatewayBalances.cpp>
#include <call/rpc/handlers/GetCounts.cpp>
#include <call/rpc/handlers/LedgerHandler.cpp>
#include <call/rpc/handlers/LedgerAccept.cpp>
#include <call/rpc/handlers/LedgerCleanerHandler.cpp>
#include <call/rpc/handlers/LedgerClosed.cpp>
#include <call/rpc/handlers/LedgerCurrent.cpp>
#include <call/rpc/handlers/LedgerData.cpp>
#include <call/rpc/handlers/LedgerEntry.cpp>
#include <call/rpc/handlers/LedgerHeader.cpp>
#include <call/rpc/handlers/LedgerRequest.cpp>
#include <call/rpc/handlers/LogLevel.cpp>
#include <call/rpc/handlers/LogRotate.cpp>
#include <call/rpc/handlers/NoCallCheck.cpp>
#include <call/rpc/handlers/OwnerInfo.cpp>
