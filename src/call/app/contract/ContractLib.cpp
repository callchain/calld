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
#include <call/app/contract/ContractLib.h>
#include <call/app/main/Application.h>
#include <call/basics/StringUtilities.h>
#include <call/basics/base_uint.h>

namespace call
{

namespace 
{
    static void call_push_string(lua_State *L, std::string k, std::string v)
    {
        lua_pushstring(L, k.c_str());
        lua_pushstring(L, v.c_str());
        lua_settable(L, -3);
    }

    static void call_push_boolean(lua_State *L, std::string k, bool v)
    {
        lua_pushstring(L, k.c_str());
        lua_pushboolean(L, v);
        lua_settable(L, -3);
    }

    static void call_push_integer(lua_State *L, std::string k, lua_Integer v)
    {
        lua_pushstring(L, k.c_str());
        lua_pushinteger(L, v);
        lua_settable(L, -3);
    }

    static void call_push_number(lua_State *L, std::string k, lua_Number v)
    {
        lua_pushstring(L, k.c_str());
        lua_pushinteger(L, v);
        lua_settable(L, -3);
    }
}

static int call_ledger_closed(lua_State *L)
{
    lua_newtable(L);
    auto const ledger = getApp().getLedgerMaster().getClosedLedger();
    auto const info = ledger->info();

    lua_getglobal(L, "globalhello");
    const char* hello = lua_tostring(L, -1);
    lua_pop(L, -1);

    call_push_integer(L, "seq", info.seq);
    call_push_integer(L, "parentCloseTime", info.parentCloseTime.time_since_epoch().count());
    call_push_string(L, "hash", to_string(hello));
    call_push_string(L, "txHash", to_string(info.txHash));
    call_push_string(L, "accountHash", to_string(info.accountHash));
    call_push_string(L, "parentHash", to_string(info.parentHash));
    call_push_string(L, "drops", to_string(info.drops));
    call_push_boolean(L, "validated", info.validated);
    call_push_boolean(L, "accepted", info.accepted);
    call_push_integer(L, "closeFlags", info.closeFlags);
    call_push_integer(L, "closeTimeResolution", info.closeTimeResolution.count());
    call_push_integer(L, "closeTime", info.parentCloseTime.time_since_epoch().count());

    return 1;
}

static int call_ledger_info(lua_State *L)
{
    return 0;
}

static int call_account_info(lua_State *L)
{
    return 0;
}

static int call_do_transfer(lua_State *L)
{
    return 0;
}

static int call_set_value(lua_State *L)
{
    return 0;
}

static int call_get_value(lua_State *L)
{
    return 0;
}

static int call_del_value(lua_State *L)
{
    return 0;
}

void RegisterContractLib(lua_State *L)
{
    lua_register(L, "call_ledger_closed", call_ledger_closed);
    lua_register(L, "call_ledger_info", call_ledger_info);

    lua_register(L, "call_account_info", call_account_info);
    
    lua_register(L, "call_do_transfer", call_do_transfer);
    
    lua_register(L, "call_set_value", call_set_value);
    lua_register(L, "call_get_value", call_get_value);
    lua_register(L, "call_del_value", call_del_value);
}

}