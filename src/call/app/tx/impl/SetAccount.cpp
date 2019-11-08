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

    This file is part of rippled: https://github.com/ripple/rippled
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
#include <call/app/tx/impl/SetAccount.h>
#include <call/basics/Log.h>
#include <call/core/Config.h>
#include <call/protocol/Feature.h>
#include <call/protocol/Indexes.h>
#include <call/protocol/PublicKey.h>
#include <call/protocol/Quality.h>
#include <call/protocol/st.h>
#include <call/ledger/View.h>
#include <call/basics/StringUtilities.h>
#include <call/app/contract/ContractLib.h>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <lua-vm/src/lua.hpp>

namespace call {

bool
SetAccount::affectsSubsequentTransactionAuth(STTx const& tx)
{
    auto const uTxFlags = tx.getFlags();
    if(uTxFlags & (tfRequireAuth | tfOptionalAuth))
        return true;

    auto const uSetFlag = tx[~sfSetFlag];
    if(uSetFlag && (*uSetFlag == asfRequireAuth || *uSetFlag == asfDisableMaster || *uSetFlag == asfAccountTxnID))
        return true;

    auto const uClearFlag = tx[~sfClearFlag];
    return uClearFlag 
        && (*uClearFlag == asfRequireAuth || *uClearFlag == asfDisableMaster || *uClearFlag == asfAccountTxnID);
}

TER
SetAccount::preflight (PreflightContext const& ctx)
{
    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    auto& tx = ctx.tx;
    auto& j = ctx.j;

    std::uint32_t const uTxFlags = tx.getFlags ();

    if (uTxFlags & tfAccountSetMask)
    {
        JLOG(j.trace()) << "Malformed transaction: Invalid flags set.";
        return temINVALID_FLAG;
    }

    std::uint32_t const uSetFlag = tx.getFieldU32 (sfSetFlag);
    std::uint32_t const uClearFlag = tx.getFieldU32 (sfClearFlag);

    if ((uSetFlag != 0) && (uSetFlag == uClearFlag))
    {
        JLOG(j.trace()) << "Malformed transaction: Set and clear same flag.";
        return temINVALID_FLAG;
    }

    //
    // RequireAuth
    //
    bool bSetRequireAuth   = (uTxFlags & tfRequireAuth) || (uSetFlag == asfRequireAuth);
    bool bClearRequireAuth = (uTxFlags & tfOptionalAuth) || (uClearFlag == asfRequireAuth);

    if (bSetRequireAuth && bClearRequireAuth)
    {
        JLOG(j.trace()) << "Malformed transaction: Contradictory flags set.";
        return temINVALID_FLAG;
    }

    //
    // RequireDestTag
    //
    bool bSetRequireDest   = (uTxFlags & TxFlag::requireDestTag) || (uSetFlag == asfRequireDest);
    bool bClearRequireDest = (uTxFlags & tfOptionalDestTag) || (uClearFlag == asfRequireDest);

    if (bSetRequireDest && bClearRequireDest)
    {
        JLOG(j.trace()) << "Malformed transaction: Contradictory flags set.";
        return temINVALID_FLAG;
    }

    //
    // DisallowCALL
    //
    bool bSetDisallowCALL   = (uTxFlags & tfDisallowCALL) || (uSetFlag == asfDisallowCALL);
    bool bClearDisallowCALL = (uTxFlags & tfAllowCALL) || (uClearFlag == asfDisallowCALL);

    if (bSetDisallowCALL && bClearDisallowCALL)
    {
        JLOG(j.trace()) << "Malformed transaction: Contradictory flags set.";
        return temINVALID_FLAG;
    }

    // TransferRate
    if (tx.isFieldPresent (sfTransferRate))
    {
        std::uint32_t uRate = tx.getFieldU32 (sfTransferRate);

        if (uRate && (uRate < QUALITY_ONE))
        {
            JLOG(j.trace()) << "Malformed transaction: Transfer rate too small.";
            return temBAD_TRANSFER_RATE;
        }

        if (ctx.rules.enabled(fix1201) && (uRate > 2 * QUALITY_ONE))
        {
            JLOG(j.trace()) << "Malformed transaction: Transfer rate too large.";
            return temBAD_TRANSFER_RATE;
        }
    }

    // TickSize
    if (tx.isFieldPresent (sfTickSize))
    {
        if (!ctx.rules.enabled(featureTickSize))
            return temDISABLED;

        auto uTickSize = tx[sfTickSize];
        if (uTickSize &&
            ((uTickSize < Quality::minTickSize) ||
            (uTickSize > Quality::maxTickSize)))
        {
            JLOG(j.trace()) << "Malformed transaction: Bad tick size.";
            return temBAD_TICK_SIZE;
        }
    }

    if (auto const mk = tx[~sfMessageKey])
    {
        if (mk->size() && ! publicKeyType ({mk->data(), mk->size()}))
        {
            JLOG(j.trace()) << "Invalid message key specified.";
            return telBAD_PUBLIC_KEY;
        }
    }

    auto const domain = tx[~sfDomain];
    if (domain && domain->size() > DOMAIN_BYTES_MAX)
    {
        JLOG(j.trace()) << "domain too long";
        return telBAD_DOMAIN;
    }

    return preflight2(ctx);
}

TER
SetAccount::preclaim(PreclaimContext const& ctx)
{
    auto const id = ctx.tx[sfAccount];

    std::uint32_t const uTxFlags = ctx.tx.getFlags();

    auto const sle = ctx.view.read(keylet::account(id));

    std::uint32_t const uFlagsIn = sle->getFieldU32(sfFlags);

    std::uint32_t const uSetFlag = ctx.tx.getFieldU32(sfSetFlag);

    // legacy AccountSet flags
    bool bSetRequireAuth = (uTxFlags & tfRequireAuth) || (uSetFlag == asfRequireAuth);

    //
    // RequireAuth
    //
    if (bSetRequireAuth && !(uFlagsIn & lsfRequireAuth))
    {
        if (!dirIsEmpty(ctx.view,
            keylet::ownerDir(id)))
        {
            JLOG(ctx.j.trace()) << "Retry: Owner directory not empty.";
            return (ctx.flags & tapRETRY) ? terOWNERS : tecOWNERS;
        }
    }

    bool bCodeAccount  = (uTxFlags & tfCodeAccount) || (uSetFlag == asfCodeAccount);
    if (bCodeAccount && (!sle->isFieldPresent(sfCode) && !ctx.tx.isFieldPresent(sfCode)))
    {
        JLOG(ctx.j.trace()) << "when set code account, code should present";
        return temNO_CODE;
    }

    if (bCodeAccount && (uFlagsIn & lsfCodeAccount))
    {
        JLOG(ctx.j.trace()) << "Account is already code account";
        return temCODE_ACCOUNT;
    }

    return tesSUCCESS;
}

TER
SetAccount::doApply ()
{
    std::uint32_t const uTxFlags = ctx_.tx.getFlags ();

    auto const sle = view().peek(keylet::account(account_));

    std::uint32_t const uFlagsIn = sle->getFieldU32 (sfFlags);
    std::uint32_t uFlagsOut = uFlagsIn;

    std::uint32_t const uSetFlag = ctx_.tx.getFieldU32 (sfSetFlag);
    std::uint32_t const uClearFlag = ctx_.tx.getFieldU32 (sfClearFlag);

    // legacy AccountSet flags
    bool bSetRequireDest   = (uTxFlags & TxFlag::requireDestTag) || (uSetFlag == asfRequireDest);
    bool bClearRequireDest = (uTxFlags & tfOptionalDestTag) || (uClearFlag == asfRequireDest);
    bool bSetRequireAuth   = (uTxFlags & tfRequireAuth) || (uSetFlag == asfRequireAuth);
    bool bClearRequireAuth = (uTxFlags & tfOptionalAuth) || (uClearFlag == asfRequireAuth);
    bool bSetDisallowCALL   = (uTxFlags & tfDisallowCALL) || (uSetFlag == asfDisallowCALL);
    bool bClearDisallowCALL = (uTxFlags & tfAllowCALL) || (uClearFlag == asfDisallowCALL);
    bool bCodeAccount       = (uTxFlags & tfCodeAccount) || (uSetFlag == asfCodeAccount); // no clear

    bool sigWithMaster = false;

    {
        auto const spk = ctx_.tx.getSigningPubKey();

        if (publicKeyType (makeSlice (spk)))
        {
            PublicKey const signingPubKey (makeSlice (spk));

            if (calcAccountID(signingPubKey) == account_)
                sigWithMaster = true;
        }
    }

    //
    // RequireAuth
    //
    if (bSetRequireAuth && !(uFlagsIn & lsfRequireAuth))
    {
        JLOG(j_.trace()) << "Set RequireAuth.";
        uFlagsOut |= lsfRequireAuth;
    }

    if (bClearRequireAuth && (uFlagsIn & lsfRequireAuth))
    {
        JLOG(j_.trace()) << "Clear RequireAuth.";
        uFlagsOut &= ~lsfRequireAuth;
    }


	//
	//nickname set
	//
	if (ctx_.tx.isFieldPresent(sfNickName))
	{
		Blob name = ctx_.tx.getFieldVL(sfNickName);
        Blob nameHex = strCopy(strHex(name));
		auto const nameSLE = view().peek(keylet::nick(nameHex));
		if (nameSLE)
		{
			return temNICKNAMEEXISTED;
		}
		else
		{
			if (sle->isFieldPresent(sfNickName))
			{
				Blob oldname = sle->getFieldVL(sfNickName);
				auto oldnameSLE = view().peek(keylet::nick(strCopy(strHex(oldname))));
                if(oldnameSLE)
                {
                    JLOG(j_.trace()) << "nick name account: " << toBase58(oldnameSLE->getAccountID(sfAccount));
                    view().erase(oldnameSLE);
                }
				auto newindex = getNicknameIndex(nameHex);
				auto const newnameSLE = std::make_shared<SLE>(ltNICKNAME, newindex);
				newnameSLE->setAccountID(sfAccount, sle->getAccountID(sfAccount));
				sle->setFieldVL(sfNickName, name);
				view().insert(newnameSLE);	
			}
			else
			{
				auto index = getNicknameIndex(nameHex);
				auto const nameSLE2 = std::make_shared<SLE>(ltNICKNAME, index);
				nameSLE2->setAccountID(sfAccount,sle->getAccountID(sfAccount));
				sle->setFieldVL(sfNickName, name);
				view().insert(nameSLE2);
			}
		}
	}

    //
    // RequireDestTag
    //

    if (bSetRequireDest && !(uFlagsIn & lsfRequireDestTag))
    {
        JLOG(j_.trace()) << "Set lsfRequireDestTag.";
        uFlagsOut |= lsfRequireDestTag;
    }

    if (bClearRequireDest && (uFlagsIn & lsfRequireDestTag))
    {
        JLOG(j_.trace()) << "Clear lsfRequireDestTag.";
        uFlagsOut &= ~lsfRequireDestTag;
    }

    //
    // DisallowCALL
    //
    if (bSetDisallowCALL && !(uFlagsIn & lsfDisallowCALL))
    {
        JLOG(j_.trace()) << "Set lsfDisallowCALL.";
        uFlagsOut |= lsfDisallowCALL;
    }

    if (bClearDisallowCALL && (uFlagsIn & lsfDisallowCALL))
    {
        JLOG(j_.trace()) << "Clear lsfDisallowCALL.";
        uFlagsOut &= ~lsfDisallowCALL;
    }

    //
    // DisableMaster
    //
    if ((uSetFlag == asfDisableMaster) && !(uFlagsIn & lsfDisableMaster))
    {
        if (!sigWithMaster)
        {
            JLOG(j_.trace()) << "Must use master key to disable master key.";
            return tecNEED_MASTER_KEY;
        }

        if ((!sle->isFieldPresent (sfRegularKey)) &&
            (!view().peek (keylet::signers (account_))))
        {
            // Account has no regular key or multi-signer signer list.

            // Prevent transaction changes until we're ready.
            if (view().rules().enabled(featureMultiSign))
                return tecNO_ALTERNATIVE_KEY;

            return tecNO_REGULAR_KEY;
        }

        JLOG(j_.trace()) << "Set lsfDisableMaster.";
        uFlagsOut |= lsfDisableMaster;
    }

    if ((uClearFlag == asfDisableMaster) && (uFlagsIn & lsfDisableMaster))
    {
        JLOG(j_.trace()) << "Clear lsfDisableMaster.";
        uFlagsOut &= ~lsfDisableMaster;
    }

    //
    // DefaultCall
    //
    if (uSetFlag == asfDefaultCall)
    {
        uFlagsOut   |= lsfDefaultCall;
    }
    else if (uClearFlag == asfDefaultCall)
    {
        uFlagsOut   &= ~lsfDefaultCall;
    }

    //
    // NoFreeze
    //
    if (uSetFlag == asfNoFreeze)
    {
        if (!sigWithMaster && !(uFlagsIn & lsfDisableMaster))
        {
            JLOG(j_.trace()) << "Can't use regular key to set NoFreeze.";
            return tecNEED_MASTER_KEY;
        }

        JLOG(j_.trace()) << "Set NoFreeze flag";
        uFlagsOut |= lsfNoFreeze;
    }

    // Anyone may set global freeze
    if (uSetFlag == asfGlobalFreeze)
    {
        JLOG(j_.trace()) << "Set GlobalFreeze flag";
        uFlagsOut |= lsfGlobalFreeze;
    }

    // If you have set NoFreeze, you may not clear GlobalFreeze
    // This prevents those who have set NoFreeze from using
    // GlobalFreeze strategically.
    if ((uSetFlag != asfGlobalFreeze) && (uClearFlag == asfGlobalFreeze) &&
        ((uFlagsOut & lsfNoFreeze) == 0))
    {
        JLOG(j_.trace()) << "Clear GlobalFreeze flag";
        uFlagsOut &= ~lsfGlobalFreeze;
    }

    //
    // Track transaction IDs signed by this account in its root
    //
    if ((uSetFlag == asfAccountTxnID) && !sle->isFieldPresent (sfAccountTxnID))
    {
        JLOG(j_.trace()) << "Set AccountTxnID";
        sle->makeFieldPresent (sfAccountTxnID);
    }

    if ((uClearFlag == asfAccountTxnID) && sle->isFieldPresent (sfAccountTxnID))
    {
        JLOG(j_.trace()) << "Clear AccountTxnID";
        sle->makeFieldAbsent (sfAccountTxnID);
    }

    //
    // EmailHash
    //
    if (ctx_.tx.isFieldPresent (sfEmailHash))
    {
        uint128 const uHash = ctx_.tx.getFieldH128 (sfEmailHash);

        if (!uHash)
        {
            JLOG(j_.trace()) << "unset email hash";
            sle->makeFieldAbsent (sfEmailHash);
        }
        else
        {
            JLOG(j_.trace()) << "set email hash";
            sle->setFieldH128 (sfEmailHash, uHash);
        }
    }

    //
    // WalletLocator
    //
    if (ctx_.tx.isFieldPresent (sfWalletLocator))
    {
        uint256 const uHash = ctx_.tx.getFieldH256 (sfWalletLocator);

        if (!uHash)
        {
            JLOG(j_.trace()) << "unset wallet locator";
            sle->makeFieldAbsent (sfWalletLocator);
        }
        else
        {
            JLOG(j_.trace()) << "set wallet locator";
            sle->setFieldH256 (sfWalletLocator, uHash);
        }
    }

    //
    // MessageKey
    //
    if (ctx_.tx.isFieldPresent (sfMessageKey))
    {
        Blob const messageKey = ctx_.tx.getFieldVL (sfMessageKey);

        if (messageKey.empty ())
        {
            JLOG(j_.debug()) << "set message key";
            sle->makeFieldAbsent (sfMessageKey);
        }
        else
        {
            JLOG(j_.debug()) << "set message key";
            sle->setFieldVL (sfMessageKey, messageKey);
        }
    }

    //
    // Domain
    //
    if (ctx_.tx.isFieldPresent (sfDomain))
    {
        Blob const domain = ctx_.tx.getFieldVL (sfDomain);

        if (domain.empty ())
        {
            JLOG(j_.trace()) << "unset domain";
            sle->makeFieldAbsent (sfDomain);
        }
        else
        {
            JLOG(j_.trace()) << "set domain";
            sle->setFieldVL (sfDomain, domain);
        }
    }

    //
    // TransferRate
    //
    if (ctx_.tx.isFieldPresent (sfTransferRate))
    {
        std::uint32_t uRate = ctx_.tx.getFieldU32 (sfTransferRate);

        if (uRate == 0 || uRate == QUALITY_ONE)
        {
            JLOG(j_.trace()) << "unset transfer rate";
            sle->makeFieldAbsent (sfTransferRate);
        }
        else
        {
            JLOG(j_.trace()) << "set transfer rate";
            sle->setFieldU32 (sfTransferRate, uRate);
        }
    }

    //
    // TickSize
    //
    if (ctx_.tx.isFieldPresent (sfTickSize))
    {
        auto uTickSize = ctx_.tx[sfTickSize];
        if ((uTickSize == 0) || (uTickSize == Quality::maxTickSize))
        {
            JLOG(j_.trace()) << "unset tick size";
            sle->makeFieldAbsent (sfTickSize);
        }
        else
        {
            JLOG(j_.trace()) << "set tick size";
            sle->setFieldU8 (sfTickSize, uTickSize);
        }
    }

    // code account flag
    if (bCodeAccount && !(uFlagsIn & lsfCodeAccount))
    {
        JLOG(j_.trace()) << "Set lsfCodeAccount.";
        uFlagsOut |= lsfCodeAccount;
    }

    // code account code
    if (ctx_.tx.isFieldPresent (sfCode))
    {
        TER terResult = doInitCall();
        if (!isTesSuccess(terResult)) return terResult;
        else
        {
            std::string code = strCopy(ctx_.tx.getFieldVL(sfCode));
            auto uCode = strCopy(code);
            std::int32_t diff_size = uCode.size();
            if (sle->isFieldPresent(sfCode))
            {
                auto uOldCode = sle->getFieldVL(sfCode);
                diff_size = diff_size - uOldCode.size();
            }
            std::int32_t increment = view().fees().increment;
            std::int32_t codeCount = diff_size / increment;
            sle->setFieldU32(sfOwnerCount, sle->getFieldU32(sfOwnerCount) + codeCount);
            sle->setFieldVL(sfCode, uCode);
        }
    }

    if (uFlagsIn != uFlagsOut)
        sle->setFieldU32 (sfFlags, uFlagsOut);

    return tesSUCCESS;
}

TER
SetAccount::doInitCall ()
{
    std::string code = strCopy(ctx_.tx.getFieldVL(sfCode));
    std::string uncompress_code = UncompressData(code);
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    // set fee limit
    auto const beforeFeeLimit = feeLimit();
    std::int64_t drops = beforeFeeLimit.drops();
    lua_setdrops(L, drops);

    // check code main function exists
    int lret = luaL_loadbuffer(L, uncompress_code.data(), uncompress_code.size(), "") || lua_pcall(L, 0, 0, 0);
    if (lret != LUA_OK)
    {
        JLOG(j_.warn()) << "invalid account code, error=" << lret;
        drops = lua_getdrops(L);
        lua_close(L);
        return isFeeRunOut(drops) ? tedCODE_FEE_OUT : temINVALID_CODE;
    }
    lua_getglobal(L, "main");
    lret = lua_type(L, -1);
    if (lret != LUA_TFUNCTION)
    {
        JLOG(j_.warn()) << "no code entry, type=" << lret;
        drops = lua_getdrops(L);
        lua_close(L);
        return isFeeRunOut(drops) ? tedCODE_FEE_OUT : temNO_CODE_ENTRY;
    }
    lua_pop(L, 1);

    // call init
    lua_getglobal(L, "init");
    if (lua_isfunction(L, -1))
    {
        // push parameters, collect parameters if exists
        lua_newtable(L);
        if (ctx_.tx.isFieldPresent(sfMemos))
        {
            auto const& memos = ctx_.tx.getFieldArray(sfMemos);
            int n = 0;
            for (auto const& memo : memos)
            {
                auto memoObj = dynamic_cast <STObject const*> (&memo);
                if (!memoObj->isFieldPresent(sfMemoData))
                {
                    continue;
                }
                std::string data = strCopy(memoObj->getFieldVL(sfMemoData));
                lua_pushstring(L, data.c_str());
                lua_rawseti(L, -2, n);
                ++n;
            }
        }
        // set currency transactor in registry table
        lua_pushlightuserdata(L, (void *)&ctx_.app);
        lua_pushlightuserdata(L, this);
        lua_settable(L, LUA_REGISTRYINDEX);

        // set global parameters for lua contract
        lua_newtable(L); // for msg
        call_push_string(L, "address", to_string(account_));
        call_push_string(L, "sender", to_string(account_));
        call_push_string(L, "value", "0");
        lua_setglobal(L, "msg");

        lua_newtable(L); // for block
        call_push_integer(L, "height", ctx_.app.getLedgerMaster().getCurrentLedgerIndex());
        lua_setglobal(L, "block");

        lret = lua_pcall(L, 1, 1, 0);
        if (lret != LUA_OK)
        {
            JLOG(j_.warn()) << "Fail to call account code main, error=" << lret;
            drops = lua_getdrops(L);
            lua_close(L);
            return isFeeRunOut(drops) ? tedCODE_FEE_OUT : terCODE_CALL_FAILED;
        }
        // get result
        TER terResult = TER(lua_tointeger(L, -1));
        lua_pop(L, 1);
        if (isNotSuccess(terResult))
        {
            int r = terResult;
            terResult = TER(r + 1000);
        }

        drops = lua_getdrops(L);
        terResult = isFeeRunOut(drops) ? tedCODE_FEE_OUT : terResult;

        if (isTesSuccess(terResult))
        {
            // save lua contract variable
            SaveLuaTable(L, account_);
        }
        else
        {
            lua_close(L);
            return terResult;
        }
    }
    lua_close(L);

    return tesSUCCESS;
}

}
