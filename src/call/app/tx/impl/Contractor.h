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

#ifndef CALL_TX_CONTRACTOR_H_INCLUDED
#define CALL_TX_CONTRACTOR_H_INCLUDED

#include <call/app/paths/CallCalc.h>
#include <call/app/tx/impl/Transactor.h>
#include <call/basics/Log.h>
#include <call/protocol/TxFlags.h>

namespace call {


class Contractor
    : public Transactor
{

public:
    Contractor (ApplyContext& ctx)
        : Transactor(ctx)
    {
    }

    // for lua glue functions
    TER doTransfer(AccountID const& toAccountID, STAmount const& amount);
    TER doIssueSet(STAmount const & total, std::uint32_t rate, Blob const& info);
    void doContractPrint(std::string const& data);

protected:
    virtual void preCompute();

    virtual TER doApply () = 0;
};

} // call

#endif
