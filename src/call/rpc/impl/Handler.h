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

#ifndef CALL_RPC_HANDLER_H_INCLUDED
#define CALL_RPC_HANDLER_H_INCLUDED

#include <call/core/Config.h>
#include <call/rpc/RPCHandler.h>
#include <call/rpc/Status.h>

namespace Json {
class Object;
}

namespace call {
namespace RPC {

// Under what condition can we call this RPC?
enum Condition {
    NO_CONDITION     = 0,
    NEEDS_NETWORK_CONNECTION  = 1,
    NEEDS_CURRENT_LEDGER  = 2 + NEEDS_NETWORK_CONNECTION,
    NEEDS_CLOSED_LEDGER   = 4 + NEEDS_NETWORK_CONNECTION,
};

struct Handler
{
    template <class JsonValue>
    using Method = std::function <Status (Context&, JsonValue&)>;

    const char* name_;
    Method<Json::Value> valueMethod_;
    Role role_;
    RPC::Condition condition_;
};

const Handler* getHandler (std::string const&);

/** Return a Json::objectValue with a single entry. */
template <class Value>
Json::Value makeObjectValue (
    Value const& value, Json::StaticString const& field = jss::message)
{
    Json::Value result (Json::objectValue);
    result[field] = value;
    return result;
}

} // RPC
} // call

#endif
