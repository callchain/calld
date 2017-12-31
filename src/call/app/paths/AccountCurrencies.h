//------------------------------------------------------------------------------
/*
    This file is part of calld: https://github.com/call/calld
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

#ifndef CALL_APP_PATHS_ACCOUNTCURRENCIES_H_INCLUDED
#define CALL_APP_PATHS_ACCOUNTCURRENCIES_H_INCLUDED

#include <call/app/paths/RippleLineCache.h>
#include <call/protocol/types.h>

namespace call {

hash_set<Currency>
accountDestCurrencies(
    AccountID const& account,
        std::shared_ptr<RippleLineCache> const& cache,
            bool includeXRP);

hash_set<Currency>
accountSourceCurrencies(
    AccountID const& account,
        std::shared_ptr<RippleLineCache> const& lrLedger,
             bool includeXRP);

} // call

#endif