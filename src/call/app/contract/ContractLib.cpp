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
#include <lz4/lib/lz4.h>

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

static int call_ledger_closed(lua_State *L)
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

static int call_account_info(lua_State *L)
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

static int call_do_transfer(lua_State *L)
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
    
    lua_getglobal(L, "__APPLY_CONTEXT_FOR_CALL_CODE");
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

static int __call_set_value(lua_State *L, std::string key, Blob value)
{
    lua_getglobal(L, "__APPLY_CONTEXT_FOR_CALL_CODE");
    Transactor *transactor = reinterpret_cast<Transactor *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    ApplyView& view = transactor->view();

    lua_getglobal(L, "msg");
    lua_getfield(L, -1, "address");
    std::string contractS = lua_tostring(L, -1);
    lua_pop(L, 2);

    auto const index = getParamIndex(contractS, key);
    auto const sle = view.peek(keylet::paramt(index));
    if (sle) {
        sle->setFieldVL(sfInfo, value);
        view.update(sle);
    } else {
        auto const sle = std::make_shared<SLE>(ltPARAMROOT, index);
        sle->setFieldVL(sfInfo, value);
        view.insert(sle);
    }

    lua_pushstring(L, key.c_str());
    lua_pushinteger(L, tesSUCCESS);
    return 2;
}

static int call_set_value(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 2) { // key, value
        return call_error(L, tedINVALID_PARAM_NUMS);
    }

    if (!lua_isstring(L, 1)) {
        return call_error(L, tedINVALID_PARAM_TYPE);
    }

    if (!lua_isstring(L, 1)) {
        return call_error(L, tedINVALID_KEY_TYPE);
    }
    if (!lua_isstring(L, 2)) {
        return call_error(L, tedINVALID_KEY_TYPE);
    }
    const char *key= lua_tostring(L, 1);
    std::string keyS = key;
    const char *value = lua_tostring(L, 2);
    std::string valueS = value;

    return __call_set_value(L, key, strCopy(valueS));
}

static int call_get_value(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) { // key
        return call_error(L, tedINVALID_PARAM_NUMS);
    }

    if (!lua_isstring(L, 1)) {
        return call_error(L, tedINVALID_PARAM_TYPE);
    }

    const char* key = lua_tostring(L, 1);
    std::string keyS = key;

    lua_getglobal(L, "__APPLY_CONTEXT_FOR_CALL_CODE");
    Transactor *transactor = reinterpret_cast<Transactor *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    ApplyView& view = transactor->view();

    lua_getglobal(L, "msg");
    lua_getfield(L, -1, "address");
    std::string contractS = lua_tostring(L, -1);
    lua_pop(L, 2);

    auto const index = getParamIndex(contractS, key);
    auto const sle = view.peek(keylet::paramt(index));

    if (!sle) {
        return call_error(L, tedNO_SUCH_VALUE);
    }
    Blob const value = sle->getFieldVL(sfInfo);

    lua_pushstring(L, strCopy(value).c_str());
    lua_pushinteger(L, tesSUCCESS);
    return 2;
}

static int call_del_value(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) { // key
        return call_error(L, tedINVALID_PARAM_NUMS);
    }

    if (!lua_isstring(L, 1)) {
        return call_error(L, tedINVALID_PARAM_TYPE);
    }

    const char* key = lua_tostring(L, 1);
    std::string keyS = key;

    lua_getglobal(L, "__APPLY_CONTEXT_FOR_CALL_CODE");
    Transactor *transactor = reinterpret_cast<Transactor *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    ApplyView& view = transactor->view();

    lua_getglobal(L, "msg");
    lua_getfield(L, -1, "address");
    std::string contractS = lua_tostring(L, -1);
    lua_pop(L, 2);

    auto const index = getParamIndex(contractS, key);
    auto const sle = view.peek(keylet::paramt(index));

    if (!sle) {
        return call_error(L, tedNO_SUCH_VALUE);
    }

    view.erase(sle);

    lua_pushstring(L, keyS.c_str());
    lua_pushinteger(L, tesSUCCESS);
    return 2;
}

void RegisterContractLib(lua_State *L)
{
    lua_register(L, "call_ledger_closed", call_ledger_closed);
    lua_register(L, "call_account_info",  call_account_info );

    lua_register(L, "call_do_transfer",   call_do_transfer  );

    lua_register(L, "call_set_value",     call_set_value    );
    lua_register(L, "call_get_value",     call_get_value    );
    lua_register(L, "call_del_value",     call_del_value    );
}

int hexchar2int(char c)
{
    if (c >= '0' && c <= '9') return (c - '0');
    if (c >= 'A' && c <= 'F') return (c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (c - 'a' + 10);
    return 0;
}

void hex2bytes(const std::string s, std::vector<char> &bytes)
{
    int sz = s.length();
    int size = sz / 2;
    char buf[size];
    for (int i = 0 ; i < sz ; i += 2) {
        buf[i/2] = (char) ((hexchar2int(s.at(i)) << 4) | hexchar2int(s.at(i+1)));
    }
    bytes.reserve(size);
    std::copy(buf, buf + size, bytes.begin());
}

std::string bytes2hex(char* bytes, int length)
{
    std::string str("");
    std::string hex("0123456789abcdef");
    for (int i = 0; i < length; i++)
    {
        int b;
        b = 0x0f & (bytes[i] >> 4);
        str.append(1, hex.at(b));
        b = 0x0f & bytes[i];
        str.append(1, hex.at(b));
    }
    return str;
}

std::string code_compress(const std::vector<char> input)
{
    int max_size = LZ4_compressBound(input.size());
    char output_buf[max_size];
    int actual_size = LZ4_compress_default(&input[0], output_buf, input.size(), max_size);
    std::string result = bytes2hex(output_buf, actual_size);
    return result;
}

void code_uncompress(const std::string input, std::vector<char> &output)
{
    std::vector<char> bytes(input.size()*2);
    hex2bytes(input, bytes);
    int size = bytes.size();
    int max_size = size*2 + 8;
    char output_buf[max_size];
    int actual_size = LZ4_decompress_safe(&bytes[0], output_buf, size, max_size);
    output.reserve(actual_size);
    std::copy(output_buf, output_buf + actual_size, output.begin());
}

}