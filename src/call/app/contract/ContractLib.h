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
#include <call/basics/StringUtilities.h>

#include <string>
#include <vector>

namespace call
{
    static void call_push_string(lua_State *L, std::string k, std::string v);
    static void call_push_boolean(lua_State *L, std::string k, bool v);
    static void call_push_integer(lua_State *L, std::string k, lua_Integer v);
    static void call_push_number(lua_State *L, std::string k, lua_Number v);

    void RegisterContractLib(lua_State *L);

    // lz4 compress and hex
    static std::string code_compress(const std::vector<char> input);
    // unhex and uncompress
    static std::string code_uncompress(const std::string input);

} // namespace call
#endif
