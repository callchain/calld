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

#ifndef CALL_TX_CONTRACTLIB_H_INCLUDED
#define CALL_TX_CONTRACTLIB_H_INCLUDED

#include <lua-vm/src/lua.hpp>
#include <call/app/tx/impl/Contractor.h>
#include <call/basics/StringUtilities.h>
#include <call/protocol/AccountID.h>
#include <call/protocol/STArray.h>
#include <call/protocol/TER.h>

#include <string>
#include <vector>

#define LEDGER_INFO_DROP_COST    10
#define ACCOUNT_INFO_DROP_COST   10
#define CALLSTATE_INFO_DROP_COST 10
#define PRINT_DROP_COST          5
#define TRANSFER_DROP_COST       100
#define ADDRESS_CHECK_DROP_COST  5

#define CODE_MAX_SIZE            8192
#define CODE_UNIT_SIZE           16
#define CODE_DATA_UNIT_SIZE      32
#define CODE_DATA_MAX_SIZE       16384
#define MAX_TABLE_NESTED_SIZE    10

namespace call
{
    typedef struct {
        Contractor *contractor;
        AccountID &address;
    } ContractData;

    static void call_push_string(lua_State *L, std::string k, std::string v);
    static void call_push_boolean(lua_State *L, std::string k, bool v);
    static void call_push_integer(lua_State *L, std::string k, lua_Integer v);
    static void call_push_number(lua_State *L, std::string k, lua_Number v);

    static void call_push_args(lua_State *L, const STArray& args);

    void RegisterContractLib(lua_State *L);

    // using snappy compress and uncompress data
    static std::string CompressData(const std::string input);
    static std::string UncompressData(const std::string input);

    static TER SaveLuaTable(lua_State *L, AccountID const &contract_address);
    static void RestoreLuaTable(lua_State *L, AccountID const &contract_address);

    static int panic_handler(lua_State *L);

} // namespace call
#endif
