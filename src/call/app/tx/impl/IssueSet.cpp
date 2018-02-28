#include <BeastConfig.h>
#include <call/app/tx/impl/IssueSet.h>
#include <call/app/paths/CallCalc.h>
#include <call/basics/Log.h>
#include <call/core/Config.h>
#include <call/protocol/st.h>
#include <call/protocol/TxFlags.h>
#include <call/protocol/JsonFields.h>
#include <call/protocol/Feature.h>
#include <call/protocol/Quality.h>
#include <call/protocol/Indexes.h>
#include <call/protocol/st.h>
#include <call/ledger/View.h>
namespace call {

	
	
		TER
			IssueSet::preflight(PreflightContext const& ctx)
		{
			auto const ret = preflight1(ctx);
			if (!isTesSuccess(ret))
				return ret;
			return tesSUCCESS;
		}

	
		TER
			IssueSet::preclaim(PreclaimContext const& ctx)
		{
			return tesSUCCESS;
		}
	TER IssueSet::doApply()
	{
		TER terResult = tesSUCCESS;
		//account_;
                std::uint32_t const uTxFlags =ctx_.tx.getFlags();
                bool enaddtion = uTxFlags & tfEnaddition;
		//auto srcAccount= ctx_.tx.getAccountID(sfAccount);
		STAmount satotal = ctx_.tx.getFieldAmount(sfTotal);
		Currency currency = satotal.getCurrency();
		auto viewJ = ctx_.app.journal("View");
		if (satotal.native())
		{
			return temBAD_CURRENCY;
		}
		SLE::pointer sleIssueRoot= view().peek(
			keylet::issuet(account_,currency));
		if (!sleIssueRoot)
		{
			uint256 index(getIssueIndex(account_, currency));
			JLOG(j_.trace()) <<
				"doTrustSet: Creating IssueRoot: " <<
				to_string(index);
			terResult = AccountIssuerCreate(view(),
				account_,
				satotal,
			    index,
				viewJ);

		}
		else
		{
			auto oldtotal = sleIssueRoot->getFieldAmount(sfTotal);
                        if (!enaddtion)
			{
			    return tecNO_AUTH;
			}
			else if (satotal <= oldtotal)
			{
				return  tecBADTOTAL;
			}
			else
			{
				sleIssueRoot->setFieldAmount(sfTotal, satotal);
				view().update(sleIssueRoot);
				JLOG(j_.trace()) << "apendent the total";
			}

		}
		return terResult;
	}
	
}
