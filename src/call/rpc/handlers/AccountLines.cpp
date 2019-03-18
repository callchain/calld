//------------------------------------------------------------------------------
/*
    This file is part of calld: https://github.com/callchain/calld
    Copyright (c) 2018, 2019 Callchain Fundation.

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
    Copyright (c) 2012-2014 Ripple Labs Inc.

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
#include <call/app/main/Application.h>
#include <call/app/paths/CallState.h>
#include <call/ledger/ReadView.h>
#include <call/net/RPCErr.h>
#include <call/protocol/ErrorCodes.h>
#include <call/protocol/JsonFields.h>
#include <call/resource/Fees.h>
#include <call/rpc/Context.h>
#include <call/rpc/impl/RPCHelpers.h>
#include <call/rpc/impl/Tuning.h>
#include <call/basics/StringUtilities.h>
namespace call
{

struct VisitData
{
	std::vector<CallState::pointer> items;
	AccountID const &accountID;
	bool hasPeer;
	AccountID const &raPeerAccount;
};
struct IssueData
{
	std::vector<IssueRoot::pointer> items;
	AccountID const &accountID;
};
struct TokenData
{
	std::vector<std::shared_ptr<SLE const> > items;
	AccountID const &accountID;
};

void addIssue(Json::Value &jsonIssued, IssueRoot &issued, STAmount const &freeze)
{
	STAmount saTotal(issued.getTotal());
	STAmount saIssued(issued.getissued());
	std::uint64_t fans = issued.getFans();
	std::uint32_t flags = issued.getFlags();
	Json::Value &jPeer(jsonIssued.append(Json::objectValue));

	jPeer["Total"] = saTotal.getJson(0);
	jPeer["Issued"] = saIssued.getJson(0);
	jPeer["Freeze"] = freeze.getJson(0);
	jPeer["Flags"] = boost::lexical_cast<std::string>(flags);
	jPeer["Fans"] = boost::lexical_cast<std::string>(fans);
}

void addToken(Json::Value &jsonToken, std::shared_ptr<SLE const> sleToken)
{
	Json::Value &jPeer(jsonToken.append(Json::objectValue));

	STAmount number = sleToken->getFieldAmount(sfAmount);
	uint256 tokenId = sleToken->getFieldH256(sfTokenID);
	Blob metaInfo = sleToken->getFieldVL(sfMetaInfo);

	jPeer["Amount"] = number.getJson(0);
	jPeer["TokenID"] = call::to_string(tokenId);
	jPeer["MetaInfo"] = strHex(metaInfo);
}

void addLine(Json::Value &jsonLines, CallState const &line, STAmount const &freeze, std::shared_ptr<ReadView const> &ledger)
{
	STAmount const &saBalance(line.getBalance());
	STAmount const &saLimit(line.getLimit());
	STAmount const &saLimitPeer(line.getLimitPeer());
	Json::Value &jPeer(jsonLines.append(Json::objectValue));

	jPeer[jss::account] = to_string(line.getAccountIDPeer());
	jPeer["freeze"] = freeze.getText();

	auto const sleAccepted = ledger->read(keylet::account(line.getAccountIDPeer()));
	if (sleAccepted)
	{
		if (sleAccepted->isFieldPresent(sfNickName))
		{
			Json::Value jvAccepted;
			RPC::injectSLE(jvAccepted, *sleAccepted);
			jPeer[jss::NickName] = jvAccepted[jss::NickName];
		}
	}
	// Amount reported is positive if current account holds other
	// account's IOUs.
	//
	// Amount reported is negative if other account holds current
	// account's IOUs.
	jPeer[jss::balance] = saBalance.getText();
	jPeer[jss::currency] = to_string(saBalance.issue().currency);
	jPeer[jss::limit] = saLimit.getText();
	jPeer[jss::limit_peer] = saLimitPeer.getText();
	jPeer[jss::quality_in] = line.getQualityIn().value;
	jPeer[jss::quality_out] = line.getQualityOut().value;
	if (line.getAuth())
		jPeer[jss::authorized] = true;
	if (line.getAuthPeer())
		jPeer[jss::peer_authorized] = true;
	if (line.getNoCall())
		jPeer[jss::no_call] = true;
	if (line.getNoCallPeer())
		jPeer[jss::no_call_peer] = true;
	if (line.getFreeze())
		jPeer[jss::freeze] = true;
	if (line.getFreezePeer())
		jPeer[jss::freeze_peer] = true;
}

// {
//   account: <account>|<account_public_key>
//   ledger_hash : <ledger>
//   ledger_index : <ledger_index>
//   limit: integer                 // optional
//   marker: opaque                 // optional, resume previous query
// }
Json::Value doAccountLines(RPC::Context &context)
{
	auto const &params(context.params);
	if (!params.isMember(jss::account))
		return RPC::missing_field_error(jss::account);

	std::shared_ptr<ReadView const> ledger;
	auto result = RPC::lookupLedger(ledger, context);
	if (!ledger)
		return result;

	//    Json::Value jvAccepted(result["call_balance"] = Json::objectValue);
	std::string strIdent(params[jss::account].asString());
	AccountID accountID;

	if (auto jv = RPC::accountFromString(accountID, strIdent))
	{
		for (auto it = jv.begin(); it != jv.end(); ++it)
			result[it.memberName()] = *it;
		return result;
	}

	if (!ledger->exists(keylet::account(accountID)))
		return rpcError(rpcACT_NOT_FOUND);

	std::string strPeer;
	if (params.isMember(jss::peer))
		strPeer = params[jss::peer].asString();
	auto hasPeer = !strPeer.empty();

	AccountID raPeerAccount;
	if (hasPeer)
	{
		if (auto jv = RPC::accountFromString(raPeerAccount, strPeer))
		{
			for (auto it = jv.begin(); it != jv.end(); ++it)
				result[it.memberName()] = *it;
			return result;
		}
	}

	//get offers
	std::vector<std::shared_ptr<SLE const>> offers;
	forEachItem(*ledger, accountID, [&offers](std::shared_ptr<SLE const> const &sle) {
		if (sle->getType() == ltOFFER)
			offers.emplace_back(sle);
	});

	Json::Value &jsonLines(result[jss::lines] = Json::arrayValue);
	Json::Value &callBalance(result["call_info"] = Json::objectValue);
	auto const sleAccepted = ledger->read(keylet::account(accountID));
	if (sleAccepted)
	{
		//Json::Value& jPeer(jsonLines.append(Json::objectValue));

		//RPC::injectSLE(jvAccepted, *sleAccepted);
		STAmount freeze;

		std::uint32_t ownercount = sleAccepted->getFieldU32(sfOwnerCount);
		STAmount callbalance = sleAccepted->getFieldAmount(sfBalance);
		freeze = ledger->fees().accountReserve(ownercount);

		for (auto const &offer : offers)
		{
			STAmount takergets = offer->getFieldAmount(sfTakerGets);
			if ((freeze.getCurrency() == takergets.getCurrency()) && (freeze.getIssuer() == takergets.getIssuer()))
			{
				freeze += takergets;
			}
		}

		//  jPeer[jss::account] = "";
		callBalance[jss::balance] = callbalance.getText();
		callBalance[jss::currency] = to_string(callbalance.issue().currency);
		//	jPeer[jss::limit] = "0";
		//	jPeer[jss::limit_peer] = "0";
		callBalance["freeze"] = freeze.getText();

		if (sleAccepted->isFieldPresent(sfNickName))
		{
			Json::Value jvAccepted;
			RPC::injectSLE(jvAccepted, *sleAccepted);
			result[jss::NickName] = jvAccepted[jss::NickName];
		}
	}

	/*if(sleAccepted)
    {
        if (sleAccepted->isFieldPresent(sfNickName))
            {
                //Blob nick = sleAccepted->getFieldVL(sfNickName);
                //std::string strnick = strCopy(nick);
                Json::Value jvAccepted;
				RPC::injectSLE(jvAccepted, *sleAccepted);
                result[jss::NickName] = jvAccepted[jss::NickName];
            }
 
    }*/
	unsigned int limit;
	if (auto err = readLimitField(limit, RPC::Tuning::accountLines, context))
		return *err;

	VisitData visitData = {{}, accountID, hasPeer, raPeerAccount};
	unsigned int reserve(limit);
	uint256 startAfter;
	std::uint64_t startHint;

	if (params.isMember(jss::marker))
	{
		// We have a start point. Use limit - 1 from the result and use the
		// very last one for the resume.
		Json::Value const &marker(params[jss::marker]);

		if (!marker.isString())
			return RPC::expected_field_error(jss::marker, "string");

		startAfter.SetHex(marker.asString());
		auto const sleLine = ledger->read({ltCALL_STATE, startAfter});

		if (!sleLine)
			return rpcError(rpcINVALID_PARAMS);

		if (sleLine->getFieldAmount(sfLowLimit).getIssuer() == accountID)
			startHint = sleLine->getFieldU64(sfLowNode);
		else if (sleLine->getFieldAmount(sfHighLimit).getIssuer() == accountID)
			startHint = sleLine->getFieldU64(sfHighNode);
		else
			return rpcError(rpcINVALID_PARAMS);

		// Caller provided the first line (startAfter), add it as first result
		auto const line = CallState::makeItem(accountID, sleLine);
		if (line == nullptr)
			return rpcError(rpcINVALID_PARAMS);

		//get freeze
		STAmount const &saBalance(line->getBalance());
		Currency currency = saBalance.getCurrency();
		STAmount const &saLimit(line->getLimit());
		AccountID peerid(line->getAccountIDPeer());
		STAmount freeze;
		freeze = zero;
		if (saLimit > zero)
		{
			Issue issue(currency, peerid);
			freeze.setIssue(issue);
		}
		else
		{
			Issue issue(currency, accountID);
			freeze.setIssue(issue);
		}

		for (auto const &offer : offers)
		{
			STAmount takergets = offer->getFieldAmount(sfTakerGets);
			if ((freeze.getCurrency() == takergets.getCurrency()) && (freeze.getIssuer() == takergets.getIssuer()))
			{
				freeze += takergets;
			}
		}

		addLine(jsonLines, *line, freeze, ledger);
		visitData.items.reserve(reserve);
	}
	else
	{
		startHint = 0;
		// We have no start point, limit should be one higher than requested.
		visitData.items.reserve(++reserve);
	}

	{
		if (!forEachItemAfter(*ledger, accountID, startAfter, startHint, reserve,
				[&visitData](std::shared_ptr<SLE const> const &sleCur) {
			auto const line = CallState::makeItem(visitData.accountID, sleCur);
			if (line != nullptr && (!visitData.hasPeer || visitData.raPeerAccount == line->getAccountIDPeer()))
			{
				visitData.items.emplace_back(line);
				return true;
			}
			return false;
		}))
		{
			return rpcError(rpcINVALID_PARAMS);
		}
	}

	if (visitData.items.size() == reserve)
	{
		result[jss::limit] = limit;

		CallState::pointer line(visitData.items.back());
		result[jss::marker] = to_string(line->key());
		visitData.items.pop_back();
	}

	result[jss::account] = context.app.accountIDCache().toBase58(accountID);

	for (auto const &item : visitData.items)
	{
		auto line = *item.get();
		STAmount const &saBalance(line.getBalance());
		Currency currency = saBalance.getCurrency();
		STAmount const &saLimit(line.getLimit());
		AccountID peerid(line.getAccountIDPeer());
		STAmount freeze;
		freeze = zero;
		if (saLimit > zero)
		{
			Issue issue(currency, peerid);
			freeze.setIssue(issue);
		}
		else
		{
			Issue issue(currency, accountID);
			freeze.setIssue(issue);
		}

		for (auto const &offer : offers)
		{
			STAmount takergets = offer->getFieldAmount(sfTakerGets);
			if ((freeze.getCurrency() == takergets.getCurrency()) && (freeze.getIssuer() == takergets.getIssuer()))
			{
				freeze += takergets;
			}
		}

		addLine(jsonLines, *item.get(), freeze, ledger);
	}
	// for (auto const& item : visitData.items)
	//        addLine (jsonLines, *item.get ());

	context.loadType = Resource::feeMediumBurdenRPC;
	return result;
}

//============================================
Json::Value doAccountTokens(RPC::Context &context)
{
	auto const &params(context.params);
	if (!params.isMember(jss::account))
		return RPC::missing_field_error(jss::account);

	std::shared_ptr<ReadView const> ledger;
	auto result = RPC::lookupLedger(ledger, context);
	if (!ledger)
		return result;

	std::string strIdent(params[jss::account].asString());
	AccountID accountID;

	if (auto jv = RPC::accountFromString(accountID, strIdent))
	{
		for (auto it = jv.begin(); it != jv.end(); ++it)
			result[it.memberName()] = *it;
		return result;
	}

	if (!ledger->exists(keylet::account(accountID)))
		return rpcError(rpcACT_NOT_FOUND);

	auto const sleAccepted = ledger->read(keylet::account(accountID));
	if (sleAccepted)
	{
		if (sleAccepted->isFieldPresent(sfNickName))
		{
			Json::Value jvAccepted;
			RPC::injectSLE(jvAccepted, *sleAccepted);
			result[jss::NickName] = jvAccepted[jss::NickName];
		}
	}

	unsigned int limit;
	if (auto err = readLimitField(limit, RPC::Tuning::accountLines, context))
		return *err;

	Json::Value &jsonTokens(result[jss::lines] = Json::arrayValue);
	TokenData visitData = {{}, accountID};
	unsigned int reserve(limit);
	uint256 startAfter;
	std::uint64_t startHint;

	if (params.isMember(jss::marker))
	{
		// We have a start point. Use limit - 1 from the result and use the
		// very last one for the resume.
		Json::Value const &marker(params[jss::marker]);

		if (!marker.isString())
			return RPC::expected_field_error(jss::marker, "string");

		startAfter.SetHex(marker.asString());
		auto const sleToken = ledger->read({ltTOKEN_ROOT, startAfter});

		if (!sleToken)
			return rpcError(rpcINVALID_PARAMS);

		startHint = sleToken->getFieldU64(sfLowNode);

		addToken(jsonTokens, sleToken);
		visitData.items.reserve(reserve);
	}
	else
	{
		startHint = 0;
		// We have no start point, limit should be one higher than requested.
		visitData.items.reserve(++reserve);
	}

	{
		if (!forEachItemAfter(*ledger, accountID, startAfter, startHint, reserve,
				[&visitData](std::shared_ptr<SLE const> const &sleCur) {
			if (sleCur != NULL && sleCur->getType() == ltTOKEN_ROOT)
			{
				visitData.items.emplace_back(sleCur);
				return true;
			}
			return false;
		}))
		{
			return rpcError(rpcINVALID_PARAMS);
		}
	}

	if (visitData.items.size() == reserve)
	{
		result[jss::limit] = limit;

		std::shared_ptr<SLE const> last = visitData.items.back();
		result[jss::marker] = to_string(last->key());
		visitData.items.pop_back();
	}

	result[jss::account] = context.app.accountIDCache().toBase58(accountID);

	for (auto const &item : visitData.items)
	{
		addToken(jsonTokens, item);
	}

	context.loadType = Resource::feeMediumBurdenRPC;
	return result;
}

Json::Value doAccountIssues(RPC::Context &context)
{
	auto const &params(context.params);
	if (!params.isMember(jss::account))
		return RPC::missing_field_error(jss::account);

	std::shared_ptr<ReadView const> ledger;
	auto result = RPC::lookupLedger(ledger, context);
	if (!ledger)
		return result;

	std::string strIdent(params[jss::account].asString());
	AccountID accountID;

	if (auto jv = RPC::accountFromString(accountID, strIdent))
	{
		for (auto it = jv.begin(); it != jv.end(); ++it)
			result[it.memberName()] = *it;
		return result;
	}

	if (!ledger->exists(keylet::account(accountID)))
		return rpcError(rpcACT_NOT_FOUND);

	auto const sleAccepted = ledger->read(keylet::account(accountID));
	if (sleAccepted)
	{
		if (sleAccepted->isFieldPresent(sfNickName))
		{
			Json::Value jvAccepted;
			RPC::injectSLE(jvAccepted, *sleAccepted);
			result[jss::NickName] = jvAccepted[jss::NickName];
		}
	}

	//get offers
	std::vector<std::shared_ptr<SLE const>> offers;
	forEachItem(*ledger, accountID, [&offers](std::shared_ptr<SLE const> const &sle) {
		if (sle->getType() == ltOFFER) 
		{
			offers.emplace_back(sle);
		}
	});

	unsigned int limit;
	if (auto err = readLimitField(limit, RPC::Tuning::accountLines, context))
		return *err;

	Json::Value &jsonIssues(result[jss::lines] = Json::arrayValue);
	IssueData visitData = {{}, accountID};
	unsigned int reserve(limit);
	uint256 startAfter;
	std::uint64_t startHint;

	if (params.isMember(jss::marker))
	{
		// We have a start point. Use limit - 1 from the result and use the
		// very last one for the resume.
		Json::Value const &marker(params[jss::marker]);

		if (!marker.isString())
			return RPC::expected_field_error(jss::marker, "string");

		startAfter.SetHex(marker.asString());
		auto const sleIssue = ledger->read({ltISSUEROOT, startAfter});

		if (!sleIssue)
			return rpcError(rpcINVALID_PARAMS);

		startHint = sleIssue->getFieldU64(sfLowNode);

		// Caller provided the first line (startAfter), add it as first result
		auto const issueroot = IssueRoot::makeItem(accountID, sleIssue);
		if (issueroot == nullptr)
			return rpcError(rpcINVALID_PARAMS);

		STAmount const &saBalance(issueroot->getTotal());
		Currency currency = saBalance.getCurrency();
		Issue issue(currency, accountID);

		STAmount freeze;
		freeze = zero;
		freeze.setIssue(issue);

		for (auto const &offer : offers)
		{
			STAmount takergets = offer->getFieldAmount(sfTakerGets);
			if ((freeze.getCurrency() == takergets.getCurrency()) && (freeze.getIssuer() == takergets.getIssuer()))
			{
				freeze += takergets;
			}
		}

		addIssue(jsonIssues, *issueroot, freeze);
		visitData.items.reserve(reserve);
	}
	else
	{
		startHint = 0;
		// We have no start point, limit should be one higher than requested.
		visitData.items.reserve(++reserve);
	}

	{
		if (!forEachItemAfter(*ledger, accountID, startAfter, startHint, reserve,
				[&visitData](std::shared_ptr<SLE const> const &sleCur) {
			auto const line = IssueRoot::makeItem(visitData.accountID, sleCur);
			if (line != nullptr)
			{
				visitData.items.emplace_back(line);
				return true;
			}
			return false;
		}))
		{
			return rpcError(rpcINVALID_PARAMS);
		}
	}

	if (visitData.items.size() == reserve)
	{
		result[jss::limit] = limit;

		IssueRoot::pointer Issue(visitData.items.back());
		result[jss::marker] = to_string(Issue->key());
		visitData.items.pop_back();
	}

	result[jss::account] = context.app.accountIDCache().toBase58(accountID);

	for (auto const &item : visitData.items)
	{
		STAmount const &saBalance(item->getTotal());
		Currency currency = saBalance.getCurrency();
		Issue issue(currency, accountID);

		STAmount freeze;
		freeze = zero;
		freeze.setIssue(issue);

		for (auto const &offer : offers)
		{
			STAmount takergets = offer->getFieldAmount(sfTakerGets);
			if ((freeze.getCurrency() == takergets.getCurrency()) && (freeze.getIssuer() == takergets.getIssuer()))
			{
				freeze += takergets;
			}
		}
		addIssue(jsonIssues, *item.get(), freeze);
	}

	//	for (auto const& item : visitData.items)
	//		addIssue(jsonIssues, *item.get());

	context.loadType = Resource::feeMediumBurdenRPC;
	return result;
}

} // namespace call
