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
#include <call/app/tx/impl/ApplyContext.h>
#include <call/app/tx/impl/Payment.h>
#include <call/app/main/Application.h>
#include <call/app/paths/CallCalc.h>
#include <call/basics/StringUtilities.h>
#include <call/basics/base_uint.h>
#include <call/json/json_value.h>
#include <call/json/json_reader.h>
#include <call/json/json_writer.h>
#include <call/rpc/impl/RPCHelpers.h>
#include <call/protocol/AccountID.h>
#include <call/protocol/Indexes.h>
#include <call/protocol/Issue.h>
#include <call/protocol/TxFlags.h>
#include <call/protocol/TER.h>
#include <snappy.h>

namespace call
{

int call_error(lua_State *L, int err)
{
    lua_pushnil(L);
    lua_pushinteger(L, err);
    return 2;
}

void call_push_string(lua_State *L, std::string k, std::string v)
{
    lua_pushstring(L, k.c_str());
    lua_pushstring(L, v.c_str());
    lua_settable(L, -3);
}

void call_push_boolean(lua_State *L, std::string k, bool v)
{
    lua_pushstring(L, k.c_str());
    lua_pushboolean(L, v);
    lua_settable(L, -3);
}

void call_push_integer(lua_State *L, std::string k, lua_Integer v)
{
    lua_pushstring(L, k.c_str());
    lua_pushinteger(L, v);
    lua_settable(L, -3);
}

void call_push_number(lua_State *L, std::string k, lua_Number v)
{
    lua_pushstring(L, k.c_str());
    lua_pushinteger(L, v);
    lua_settable(L, -3);
}

static int syscall_ledger(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) {
        return call_error(L, tedINVALID_PARAM_NUMS);
    }

    auto const ledger = getApp().getLedgerMaster().getClosedLedger();
    auto const info = ledger->info();

    lua_newtable(L);
    call_push_integer(L, "Height",              info.seq);
    call_push_integer(L, "ParentCloseTime",     info.parentCloseTime.time_since_epoch().count());
    call_push_string (L, "Hash",                to_string(info.hash));
    call_push_string (L, "ParentHash",          to_string(info.parentHash));
    call_push_integer(L, "CloseFlags",          info.closeFlags);
    call_push_integer(L, "CloseTimeResolution", info.closeTimeResolution.count());
    call_push_integer(L, "CloseTime",           info.parentCloseTime.time_since_epoch().count());

    lua_pushinteger(L, tesSUCCESS);

    return 2;
}

static int syscall_account(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) { // account
        return call_error(L, tedINVALID_PARAM_NUMS);
    }

    if (!lua_isstring(L, 1)) {
        return call_error(L, tedINVALID_PARAM_TYPE);
    }

    const char* account = lua_tostring(L, 1);
    std::string accountS = account;
    auto const accountID = RPC::accountFromStringStrict(accountS);
    if (!accountID) {
        return call_error(L, tedINVALID_PARAM_ACCOUNT);
    }

    auto const ledger = getApp().getLedgerMaster().getClosedLedger();
    auto const sleAccount = ledger->read(keylet::account(accountID.get()));
    if (!sleAccount) {
        return call_error(L, tedACCOUNT_NOT_FOUND);
    }

    lua_newtable(L);
    call_push_string (L, "Account",    account);
    call_push_string (L, "Balance",    to_string(sleAccount->getFieldAmount(sfBalance).call().drops()));
    call_push_integer(L, "Flags",      sleAccount->getFieldU32(sfFlags));
    call_push_integer(L, "OwnerCount", sleAccount->getFieldU32(sfOwnerCount));
    call_push_integer(L, "Sequence",   sleAccount->getFieldU32(sfSequence));

    lua_pushinteger(L, tesSUCCESS);

    return 2;
}

static int syscall_callstate(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 3) { // account, issuer, currency
        return call_error(L, tedINVALID_PARAM_NUMS);
    }
    if (!lua_isstring(L, 1) || !lua_isstring(L, 2) || !lua_isstring(L, 3))
    {
        return call_error(L, tedINVALID_PARAM_TYPE);
    }
    const char* account = lua_tostring(L, 1);
    std::string accountS = account;
    auto const accountID = RPC::accountFromStringStrict(accountS).get();
    if (!accountID) {
        return call_error(L, tedINVALID_PARAM_ACCOUNT);
    }
    const char* issuer = lua_tostring(L, 2);
    std::string issuerS = issuer;
    auto const issuerID = RPC::accountFromStringStrict(issuerS).get();
    if (!issuerID) {
        return call_error(L, tedINVALID_PARAM_ISSUER);
    }
    if (accountID == issuerID) {
        return call_error(L, temDST_IS_SRC);
    }
    const char* currency = lua_tostring(L, 2);
    std::string currencyS = currency;
    auto const currencyObj = to_currency(currencyS);

    lua_pushlightuserdata(L, (void *)&getApp());
    lua_gettable(L, LUA_REGISTRYINDEX);
    Payment *payment = reinterpret_cast<Payment *>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    auto const sle = payment->view().peek(keylet::line(issuerID, accountID, currencyObj));
    if (!sle) {
        return call_error(L, terNO_LINE);
    }

    lua_newtable(L);
    call_push_string (L, "Account",    account);
    if (accountID > issuerID)
    {
        auto amount = sle->getFieldAmount(sfBalance);
        amount.negate();
        call_push_string (L, "Balance", amount.getText());
        call_push_string (L, "Limit",   sle->getFieldAmount(sfHighLimit).getText());
    }
    else
    {
        call_push_string (L, "Balance", sle->getFieldAmount(sfBalance).getText());
        call_push_string (L, "Limit",   sle->getFieldAmount(sfLowLimit).getText());
    }

    lua_pushinteger(L, tesSUCCESS);

    return 2;
}

static int syscall_transfer(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 2) { // to, amount
        return call_error(L, tedINVALID_PARAM_NUMS);
    }

    if (!lua_isstring(L, 1)) { // to
        return call_error(L, tedINVALID_PARAM_TYPE);
    }
    const char* to = lua_tostring(L, 1);
    std::string toS = to;

    if (!lua_isstring(L, 2) && !lua_istable(L, 2)) { // amount
        return call_error(L, tedINVALID_PARAM_TYPE);
    }
    
    STAmount amount;
    if (lua_isstring(L, 2)) {
        std::string valueS = lua_tostring(L, 2);
        CALLAmount callAmount(stoi(valueS));
        amount = callAmount;
    } else {
        std::string currency, value, issuer;
        lua_pushstring(L, "currency");
        lua_gettable(L, 2);
        currency = lua_tostring(L, -1);
        lua_pop(L, 1);
        lua_pushstring(L, "value");
        lua_gettable(L, 2);
        value = lua_tostring(L, -1);
        lua_pop(L, 1);
        lua_pushstring(L, "issuer");
        lua_gettable(L, 2);
        issuer = lua_tostring(L, -1);
        lua_pop(L, 1);

        Json::Value json;
        json["currency"] = currency;
        json["value"] = value;
        json["issuer"] = issuer;
        if (!amountFromJsonNoThrow(amount, json))
        {
            return call_error(L, tedINVALID_AMOUNT);
        }
    }

    lua_getglobal(L, "msg");
    lua_getfield(L, -1, "address");
    std::string contractS = lua_tostring(L, -1);
    lua_pop(L, 2);
    if (toS == contractS) {
        return call_error(L, tedSEND_CONTRACT_SELF);
    }
    
    lua_pushlightuserdata(L, (void *)&getApp());
    lua_gettable(L, LUA_REGISTRYINDEX);
    Payment *payment = reinterpret_cast<Payment *>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    auto const uDstAccountID = RPC::accountFromStringStrict(toS);
    if (!uDstAccountID) {
        return call_error(L, tedINVALID_DESTINATION);
    }

    TER terResult = payment->doTransfer(uDstAccountID.get(), amount);

    lua_pushnil(L); // may other useful result
    lua_pushinteger(L, terResult);
    return 2;
}

void RegisterContractLib(lua_State *L)
{
    lua_register(L, "syscall_ledger",    syscall_ledger  );
    lua_register(L, "syscall_account",   syscall_account );
    lua_register(L, "syscall_callstate", syscall_callstate);

    lua_register(L, "syscall_transfer",  syscall_transfer);
}

std::string CompressData(const std::string input)
{
    std::string output;
    snappy::Compress(input.data(), input.size(), &output);
    return output;
}

std::string UncompressData(const std::string input)
{
    std::string output;
    snappy::Uncompress(input.data(), input.size(), &output);
    return output;
}

void __save_lua_table(lua_State *L, Json::Value &root)
{
    lua_pushnil(L);
    while (lua_next(L, -2) != 0)
    {
        // only save table data where key type is string
        // other data is ignore
        if (lua_type(L, -2) == LUA_TSTRING)
        {
            const char *key = lua_tostring(L, -2);
            if (lua_isstring(L, -1))
            {
                root[key] = lua_tostring(L, -1);
            }
            else if (lua_isnumber(L, -1))
            {
                root[key] = lua_tonumber(L, -1);
            }
            else if (lua_isboolean(L, -1))
            {
                root[key] = lua_toboolean(L, -1);
            }
            else if (lua_istable(L, -1))
            {
                Json::Value sub_root;
                __save_lua_table(L, sub_root);
                root[key] = sub_root;
            }
            // only save string->{string, number, boolean, table}
            // function, lightdata, thread is all ignore
        }
        lua_pop(L, 1);
    }
}

void SaveLuaTable(lua_State *L, AccountID const &contract_address)
{
    lua_getglobal(L, "contract"); // contract global variable
    if (!lua_istable(L, -1))  return; // no variable table

    Json::Value root;
    __save_lua_table(L, root);
    if (root.size() == 0) return; // empty table

    Json::FastWriter fastWriter;
    std::string output = fastWriter.write(root);
    std::string data = CompressData(output); // compress it

    // get transactor
    lua_pushlightuserdata(L, (void *)&getApp());
    lua_gettable(L, LUA_REGISTRYINDEX);
    Transactor *transactor = reinterpret_cast<Transactor *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    ApplyView& view = transactor->view();

    // save or update data
    auto const index = getParamIndex(contract_address);
    auto const sle = view.peek(keylet::paramt(index));
    if (sle) {
        if (root.size() == 0)
        {
            view.erase(sle); // when no fields in contract just delete it
        }
        else
        {
            sle->setFieldVL(sfInfo, strCopy(data));
            view.update(sle);
        }
    } else {
        auto const sle = std::make_shared<SLE>(ltPARAMROOT, index);
        sle->setFieldVL(sfInfo, strCopy(data));
        view.insert(sle);
    }
}

void __restore_lua_table(lua_State *L, Json::Value const &root)
{
    Json::Value::Members members = root.getMemberNames();
    for (auto it = members.begin(); it != members.end(); ++it)
    {
        // only for key is string
        std::string key = *it;
        if (root[*it].type() == Json::objectValue)
        {
            lua_pushstring(L, key.c_str());
            lua_newtable(L);
            __restore_lua_table(L, root[*it]);
            lua_settable(L, -3);
        }
        else if (root[*it].type() == Json::stringValue)
        {
            call_push_string(L, key, root[*it].asString());
        }
        else if (root[*it].type() == Json::realValue)
        {
            call_push_number(L, key, root[*it].asDouble());
        }
        else if (root[*it].type() == Json::uintValue)
        {
            // TODO, fix unsigned convert
            call_push_integer(L, key, root[*it].asUInt());
        }
        else if (root[*it].type() == Json::booleanValue)
        {
            call_push_boolean(L, key, root[*it].asBool());
        }
        else if (root[*it].type() == Json::intValue)
        {
            call_push_integer(L, key, root[*it].asInt());
        }
    }
}

void RestoreLuaTable(lua_State *L, AccountID const &contract_address)
{
    lua_pushlightuserdata(L, (void *)&getApp());
    lua_gettable(L, LUA_REGISTRYINDEX);
    Transactor *transactor = reinterpret_cast<Transactor *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    ApplyView& view = transactor->view();

    // read data from saved
    auto const index = getParamIndex(contract_address);
    auto const sle = view.peek(keylet::paramt(index));
    if (!sle || !sle->isFieldPresent(sfInfo)) return; // no contract data saved

    Blob data = sle->getFieldVL(sfInfo);
    std::string input = UncompressData(strCopy(data)); // uncompress it
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(input, root)) return; // invalid json data

    lua_newtable(L);
    __restore_lua_table(L, root); // root should not empty
    lua_setglobal(L, "contract");
}

}