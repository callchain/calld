//------------------------------------------------------------------------------
/*
    This file is part of calld: https://github.com/call/calld
    Copyright (c) 2012, 2013 Call Labs Inc.

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

#ifndef CALL_APP_LEDGER_TRANSACTIONSTATESF_H_INCLUDED
#define CALL_APP_LEDGER_TRANSACTIONSTATESF_H_INCLUDED

#include <call/app/ledger/AbstractFetchPackContainer.h>
#include <call/shamap/SHAMapSyncFilter.h>
#include <call/shamap/Family.h>
#include <cstdint>

namespace call {

// This class is only needed on add functions
// sync filter for transactions tree during ledger sync
class TransactionStateSF
    : public SHAMapSyncFilter
{
private:
    Family& f_;
    AbstractFetchPackContainer& fp_;

public:
    explicit
    TransactionStateSF(Family&, AbstractFetchPackContainer&);

    // Note that the nodeData is overwritten by this call
    void gotNode (bool fromFilter,
                  SHAMapHash const& nodeHash,
                  Blob&& nodeData,
                  SHAMapTreeNode::TNType) const override;

    boost::optional<Blob>
    getNode(SHAMapHash const& nodeHash) const override;
};

} // call

#endif
