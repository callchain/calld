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
#include <call/app/tx/impl/Contractor.h>
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

void call_push_args(lua_State *L, const STArray& args)
{
    for (auto const& arg : args)
    {
        auto const& argObj = dynamic_cast <STObject const*> (&arg);
        std::string argType = strCopy(argObj->getFieldVL(sfArgType));
        std::string argName = strCopy(argObj->getFieldVL(sfArgName));
        lua_pushstring(L, argName.c_str());
        std::string value = strCopy(argObj->getFieldVL(sfArgValue));
        if (argType == "integer")
        {
            std::int32_t i = atoi(value.c_str());
            lua_pushinteger(L, i);
        }
        else if (argType == "boolean")
        {
            bool b;
            std::istringstream(value) >> b;
            lua_pushboolean(L, b);
        }
        else if (argType == "number")
        {
            double f = atof(value.c_str());
            lua_pushnumber(L, f);
        }
        else if (argType == "string")
        {
            std::string s = value;
            lua_pushstring(L, s.c_str());
        }
        else if (argType == "address")
        {
            std::string a = value;
            lua_pushstring(L, a.c_str());
        }
        else if (argType == "amount")
        {
            STAmount amount;
            amountFromContractNoThrow(amount, value);
            if (amount.native())
            {
                lua_pushinteger(L, amount.call().drops());
            }
            else
            {
                lua_newtable(L);
                call_push_string(L, "value", amount.getText());
                call_push_string(L, "currency", to_string(amount.getCurrency()));
                call_push_string(L, "issuer", to_string(amount.getIssuer()));
            }
        }
        else
        {
            // this should not occur
        }
        lua_settable(L, -3);
    }
}

static int syscall_ledger(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) {
        return call_error(L, tecINVALID_PARAM_NUMS);
    }

    long long left_drops = lua_getdrops(L);
    if (!lua_setdrops(L, left_drops - LEDGER_INFO_DROP_COST)) {
        return call_error(L, tecCODE_FEE_OUT);
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
        return call_error(L, tecINVALID_PARAM_NUMS);
    }

    if (!lua_isstring(L, 1)) {
        return call_error(L, tecINVALID_PARAM_TYPE);
    }

    const char* account = lua_tostring(L, 1);
    std::string accountS = account;
    auto const accountID = RPC::accountFromStringStrict(accountS);
    if (!accountID) {
        return call_error(L, tecINVALID_PARAM_ACCOUNT);
    }

    long long left_drops = lua_getdrops(L);
    if (!lua_setdrops(L, left_drops - ACCOUNT_INFO_DROP_COST)) {
        return call_error(L, tecCODE_FEE_OUT);
    }

    auto const ledger = getApp().getLedgerMaster().getClosedLedger();
    auto const sleAccount = ledger->read(keylet::account(accountID.get()));
    if (!sleAccount) {
        return call_error(L, tecACCOUNT_NOT_FOUND);
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
        return call_error(L, tecINVALID_PARAM_NUMS);
    }
    if (!lua_isstring(L, 1) || !lua_isstring(L, 2) || !lua_isstring(L, 3))
    {
        return call_error(L, tecINVALID_PARAM_TYPE);
    }
    const char* account = lua_tostring(L, 1);
    std::string accountS = account;
    auto const accountID = RPC::accountFromStringStrict(accountS).get();
    if (!accountID) {
        return call_error(L, tecINVALID_PARAM_ACCOUNT);
    }
    const char* issuer = lua_tostring(L, 2);
    std::string issuerS = issuer;
    auto const issuerID = RPC::accountFromStringStrict(issuerS).get();
    if (!issuerID) {
        return call_error(L, tecINVALID_PARAM_ISSUER);
    }
    if (accountID == issuerID) {
        return call_error(L, temDST_IS_SRC);
    }

    long long left_drops = lua_getdrops(L);
    if (!lua_setdrops(L, left_drops - CALLSTATE_INFO_DROP_COST)) {
        return call_error(L, tecCODE_FEE_OUT);
    }

    const char* currency = lua_tostring(L, 3);
    std::string currencyS = currency;
    auto const currencyObj = to_currency(currencyS);

    lua_pushlightuserdata(L, (void *)&getApp());
    lua_gettable(L, LUA_REGISTRYINDEX);
    ContractData cd = reinterpret_cast<ContractData *>(lua_touserdata(L, -1));
    Contractor *contractor = cd->contractor;
    lua_pop(L, 1);

    auto const sle = contractor->view().peek(keylet::line(issuerID, accountID, currencyObj));
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
        return call_error(L, tecINVALID_PARAM_NUMS);
    }

    if (!lua_isstring(L, 1)) { // to
        return call_error(L, tecINVALID_PARAM_TYPE);
    }
    const char* to = lua_tostring(L, 1);
    std::string toS = to;

    if (!lua_isstring(L, 2) && !lua_istable(L, 2)) { // amount
        return call_error(L, tecINVALID_PARAM_TYPE);
    }

    long long left_drops = lua_getdrops(L);
    if (!lua_setdrops(L, left_drops - TRANSFER_DROP_COST)) {
        return call_error(L, tecCODE_FEE_OUT);
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
            return call_error(L, tecINVALID_AMOUNT);
        }
    }

    lua_pushlightuserdata(L, (void *)&getApp());
    lua_gettable(L, LUA_REGISTRYINDEX);
    ContractData *cd = reinterpret_cast<ContractData *>(lua_touserdata(L, -1));
    Contractor * contractor = cd->contractor;
    std::string contractS = to_string(cd->address);
    lua_pop(L, 1);

    if (toS == contractS) {
        return call_error(L, tecSEND_CONTRACT_SELF);
    }

    auto const uDstAccountID = RPC::accountFromStringStrict(toS);
    if (!uDstAccountID) {
        return call_error(L, tecINVALID_DESTINATION);
    }

    TER terResult = contractor->doTransfer(uDstAccountID.get(), amount);

    lua_pushnil(L); // may other useful result
    lua_pushinteger(L, terResult);
    return 2;
}

static int syscall_issueset(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 4) { // total, currency
        return call_error(L, tecINVALID_PARAM_NUMS);
    }

    if (!lua_isnumber(L, 1)) { // total
        return call_error(L, tecINVALID_PARAM_TYPE);
    }
    lua_Number total = lua_tonumber(L, 1);

    if (!lua_isstring(L, 2)) { // currency
        return call_error(L, tecINVALID_PARAM_TYPE);
    }
    const char* currency = lua_tostring(L, 2);
    std::string currencyS = currency;

    if (!lus_isinteger(L, 3)) { // rate
        return call_error(L, tecINVALID_PARAM_TYPE);
    }
    int rate = lua_tointeger(L, 3);

    if (!lua_isstring(L, 4)) { // info
        return call_error(L, tecINVALID_PARAM_TYPE);
    }
    const char* info = lua_tostring(L, 4);
    std::string infoS = info;

    long long left_drops = lua_getdrops(L);
    if (!lua_setdrops(L, left_drops - TRANSFER_DROP_COST)) {
        return call_error(L, tecCODE_FEE_OUT);
    }

    lua_pushlightuserdata(L, (void *)&getApp());
    lua_gettable(L, LUA_REGISTRYINDEX);
    ContractData *cd = reinterpret_cast<ContractData *>(lua_touserdata(L, -1));
    Contractor * contractor = cd->contractor;
    std::string contractS = to_string(cd->address);
    lua_pop(L, 1);

    Json::Value json;
    json["value"] = total;
    json["currency"] = currencyS;
    json["issuer"] = contractS;

    STAmount amount;
    if (!amountFromJsonNoThrow(amount, json))
    {
        return call_error(L, tecINVALID_AMOUNT);
    }

    TER terResult = contractor->doIssueSet(amount, rate, strCopy(infoS));

    lua_pushnil(L); // may other useful result
    lua_pushinteger(L, terResult);
    return 2;
}

static int syscall_print(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) {
        return call_error(L, tecINVALID_PARAM_NUMS);
    }

    if (!lua_isstring(L, 1)) {
        return call_error(L, tecINVALID_PARAM_TYPE);
    }

    long long left_drops = lua_getdrops(L);
    if (!lua_setdrops(L, left_drops - PRINT_DROP_COST)) {
        return call_error(L, tecCODE_FEE_OUT);
    }

    const char* data = lua_tostring(L, 1);
    std::string dataS = data;

    lua_pushlightuserdata(L, (void *)&getApp());
    lua_gettable(L, LUA_REGISTRYINDEX);
    ContractData *cd = reinterpret_cast<ContractData *>(lua_touserdata(L, -1));
    Contractor * contractor = cd->contractor;
    lua_pop(L, 1);

    contractor->doContractPrint(data);

    lua_pushnil(L);
    lua_pushinteger(L, tesSUCCESS);
    return 2;
}

static int syscall_is_address(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) {
        return call_error(L, tecINVALID_PARAM_NUMS);
    }

    if (!lua_isstring(L, 1)) {
        return call_error(L, tecINVALID_PARAM_TYPE);
    }

    long long left_drops = lua_getdrops(L);
    if (!lua_setdrops(L, left_drops - ADDRESS_CHECK_DROP_COST)) {
        return call_error(L, tecCODE_FEE_OUT);
    }

    const char* data = lua_tostring(L, 1);
    const std::string s = data;

    auto const account = parseBase58<AccountID>(s);
    bool ret = true;
    if (!account)
        ret = false;

    lua_pushboolean(L, ret);
    lua_pushinteger(L, tesSUCCESS);
    return 2;
}

void RegisterContractLib(lua_State *L)
{
    lua_register(L, "syscall_ledger",    syscall_ledger  );
    lua_register(L, "syscall_account",   syscall_account );
    lua_register(L, "syscall_callstate", syscall_callstate);

    lua_register(L, "syscall_transfer",  syscall_transfer);
    lua_register(L, "syscall_issueset",  syscall_issueset);

    lua_register(L, "syscall_print",     syscall_print);

    lua_register(L, "syscall_is_address",syscall_is_address);
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

bool __save_lua_table(lua_State *L, Json::Value &root, int nested)
{
    nested += 1;
    if (nested > MAX_TABLE_NESTED_SIZE)
        return false;

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
                if (!__save_lua_table(L, sub_root, nested))
                    return false;
                root[key] = sub_root;
            }
            // only save string->{string, number, boolean, table}
            // function, lightdata, thread is all ignore
        }
        lua_pop(L, 1);
    }
    return true;
}

TER SaveLuaTable(lua_State *L, AccountID const &contract_address)
{
    lua_getglobal(L, "contract"); // contract global variable
    if (!lua_istable(L, -1))  return tesSUCCESS; // no variable table

    Json::Value root;
    int nested = 0;
    if (!__save_lua_table(L, root, nested))
        return tecDATA_EXCEED_MAX_NEST;

    Json::FastWriter fastWriter;
    std::string output = fastWriter.write(root);
    std::string data = CompressData(output); // compress it

    // get transactor
    lua_pushlightuserdata(L, (void *)&getApp());
    lua_gettable(L, LUA_REGISTRYINDEX);
    ContractData *cd = reinterpret_cast<ContractData *>(lua_touserdata(L, -1));
    Contractor * contractor = cd->contractor;
    lua_pop(L, 1);
    ApplyView& view = contractor->view();

    // save or update data
    auto const index = getParamIndex(contract_address);
    auto const sle = view.peek(keylet::paramt(index));
    std::int32_t diff_size = data.size();
    if (diff_size > CODE_DATA_MAX_SIZE)
    {
        return tecDATA_EXCEED_MAX_SIZE;
    }

    if (sle) {
        auto const oldData = sle->getFieldVL(sfInfo);
        diff_size = diff_size - oldData.size();
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
        if (root.size() == 0) {
            // add empty data
            return tesSUCCESS;
        }
        auto const sle = std::make_shared<SLE>(ltPARAMROOT, index);
        sle->setFieldVL(sfInfo, strCopy(data));
        view.insert(sle);
    }

    // check reserve owner ok
    auto const sleAccount = view.peek(keylet::account(contract_address));
    std::int32_t dataCount = sleAccount->getFieldU32(sfOwnerCount) + (diff_size / CODE_DATA_UNIT_SIZE);
    auto const reserve = view.fees().accountReserve(dataCount);
    auto const balance = sleAccount->getFieldAmount(sfBalance);
    if (reserve > balance)
    {
        // reserved not enough
        return tecINSUFFICIENT_RESERVE;
    }
    sleAccount->setFieldU32(sfOwnerCount, dataCount);
    view.update(sleAccount);

    return tesSUCCESS;
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
    ContractData *cd = reinterpret_cast<ContractData *>(lua_touserdata(L, -1));
    Contractor * contractor = cd->contractor;
    lua_pop(L, 1);
    ApplyView& view = contractor->view();

    // read data from saved
    auto const index = getParamIndex(contract_address);
    auto const sle = view.peek(keylet::paramt(index));

    lua_newtable(L);
    if (sle && sle->isFieldPresent(sfInfo))
    {
        // have contract data saved
        Blob data = sle->getFieldVL(sfInfo);
        std::string input = UncompressData(strCopy(data)); // uncompress it
        Json::Value root;
        Json::Reader reader;
        if (reader.parse(input, root))
        {
            __restore_lua_table(L, root); // restore data
        }
    }
    lua_setglobal(L, "contract");
}

int panic_handler(lua_State *L)
{
    const char *msg = lua_tostring(L, -1);
    throw msg; // not string
}

}