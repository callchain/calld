#ifndef CALL_TX_ISSUESET_H_INCLUDED
#define CALL_TX_ISSUESET_H_INCLUDED

#include <call/app/paths/CallCalc.h>
#include <call/app/tx/impl/Transactor.h>
#include <call/basics/Log.h>
#include <call/protocol/TxFlags.h>
namespace call
{
class IssueSet : public Transactor
{
  public:
	static TER preflight(PreflightContext const &ctx);

	static TER preclaim(PreclaimContext const &ctx);

	IssueSet(ApplyContext &ctx) : Transactor(ctx)
	{
	}

	TER doApply() override;
};
} // namespace call
#endif
