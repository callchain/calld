//------------------------------------------------------------------------------
/*
    This file is part of callchaind: https://github.com/callchain/callchaind
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
#include <callchain/protocol/Quality.h>
#include <callchain/app/paths/cursor/RippleLiquidity.h>
#include <callchain/basics/Log.h>

namespace callchain {
namespace path {

// Compute how much might flow for the node for the pass. Does not actually
// adjust balances.
//
// uQualityIn -> uQualityOut
//   saPrvReq -> saCurReq
//   sqPrvAct -> saCurAct
//
// This is a minimizing routine: moving in reverse it propagates the send limit
// to the sender, moving forward it propagates the actual send toward the
// receiver.
//
// When this routine works backwards, saCurReq is the driving variable: it
// calculates previous wants based on previous credit limits and current wants.
//
// When this routine works forwards, saPrvReq is the driving variable: it
//   calculates current deliver based on previous delivery limits and current
//   wants.
//
// This routine is called one or two times for a node in a pass. If called once,
// it will work and set a rate.  If called again, the new work must not worsen
// the previous rate.

void callchainLiquidity (
    RippleCalc& callchainCalc,
    Rate const& qualityIn,
    Rate const& qualityOut,
    STAmount const& saPrvReq,   // --> in limit including fees, <0 = unlimited
    STAmount const& saCurReq,   // --> out limit
    STAmount& saPrvAct,  // <-> in limit including achieved so far: <-- <= -->
    STAmount& saCurAct,  // <-> out limit including achieved so far: <-- <= -->
    std::uint64_t& uRateMax)
{
    JLOG (callchainCalc.j_.trace())
        << "callchainLiquidity>"
        << " qualityIn=" << qualityIn
        << " qualityOut=" << qualityOut
        << " saPrvReq=" << saPrvReq
        << " saCurReq=" << saCurReq
        << " saPrvAct=" << saPrvAct
        << " saCurAct=" << saCurAct;

    // saCurAct was once zero in a production server.
    assert (saCurReq != zero);
    assert (saCurReq > zero);

    assert (saPrvReq.getCurrency () == saCurReq.getCurrency ());
    assert (saPrvReq.getCurrency () == saPrvAct.getCurrency ());
    assert (saPrvReq.getIssuer () == saPrvAct.getIssuer ());

    const bool bPrvUnlimited = (saPrvReq < zero);  // -1 means unlimited.

    // Unlimited stays unlimited - don't do calculations.

    // How much could possibly flow through the previous node?
    const STAmount saPrv = bPrvUnlimited ? saPrvReq : saPrvReq - saPrvAct;

    // How much could possibly flow through the current node?
    const STAmount  saCur = saCurReq - saCurAct;

    JLOG (callchainCalc.j_.trace())
        << "callchainLiquidity: "
        << " bPrvUnlimited=" << bPrvUnlimited
        << " saPrv=" << saPrv
        << " saCur=" << saCur;

    // If nothing can flow, we might as well not do any work.
    if (saPrv == zero || saCur == zero)
        return;

    if (qualityIn >= qualityOut)
    {
        // You're getting better quality than you asked for, so no fee.
        JLOG (callchainCalc.j_.trace()) << "callchainLiquidity: No fees";

        // Only process if the current rate, 1:1, is not worse than the previous
        // rate, uRateMax - otherwise there is no flow.
        if (!uRateMax || STAmount::uRateOne <= uRateMax)
        {
            // Limit amount to transfer if need - the minimum of amount being
            // paid and the amount that's wanted.
            STAmount saTransfer = bPrvUnlimited ? saCur
                : std::min (saPrv, saCur);

            // In reverse, we want to propagate the limited cur to prv and set
            // actual cur.
            //
            // In forward, we want to propagate the limited prv to cur and set
            // actual prv.
            //
            // This is the actual flow.
            saPrvAct += saTransfer;
            saCurAct += saTransfer;

            // If no rate limit, set rate limit to avoid combining with
            // something with a worse rate.
            if (uRateMax == 0)
                uRateMax = STAmount::uRateOne;
        }
    }
    else
    {
        // If the quality is worse than the previous
        JLOG (callchainCalc.j_.trace()) << "callchainLiquidity: Fee";

        std::uint64_t const uRate = getRate (
            STAmount (qualityOut.value),
            STAmount (qualityIn.value));

        // If the next rate is at least as good as the current rate, process.
        if (!uRateMax || uRate <= uRateMax)
        {
            // current actual = current request * (quality out / quality in).
            auto numerator = multiplyRound (saCur, qualityOut, true);
            // True means "round up" to get best flow.

            STAmount saCurIn = divideRound (numerator, qualityIn, true);

            JLOG (callchainCalc.j_.trace())
                << "callchainLiquidity:"
                << " bPrvUnlimited=" << bPrvUnlimited
                << " saPrv=" << saPrv
                << " saCurIn=" << saCurIn;

            if (bPrvUnlimited || saCurIn <= saPrv)
            {
                // All of current. Some amount of previous.
                saCurAct += saCur;
                saPrvAct += saCurIn;
                JLOG (callchainCalc.j_.trace())
                    << "callchainLiquidity:3c:"
                    << " saCurReq=" << saCurReq
                    << " saPrvAct=" << saPrvAct;
            }
            else
            {
                // There wasn't enough money to start with - so given the
                // limited input, how much could we deliver?
                // current actual = previous request
                //                  * (quality in / quality out).
                // This is inverted compared to the code above because we're
                // going the other way
                auto numerator = multiplyRound (saPrv,
                    qualityIn, saCur.issue(), true);
                // A part of current. All of previous. (Cur is the driver
                // variable.)
                STAmount saCurOut = divideRound (numerator,
                    qualityOut, saCur.issue(), true);

                JLOG (callchainCalc.j_.trace())
                    << "callchainLiquidity:4: saCurReq=" << saCurReq;

                saCurAct += saCurOut;
                saPrvAct = saPrvReq;
            }
            if (!uRateMax)
                uRateMax = uRate;
        }
    }

    JLOG (callchainCalc.j_.trace())
        << "callchainLiquidity<"
        << " qualityIn=" << qualityIn
        << " qualityOut=" << qualityOut
        << " saPrvReq=" << saPrvReq
        << " saCurReq=" << saCurReq
        << " saPrvAct=" << saPrvAct
        << " saCurAct=" << saCurAct;
}

static
Rate
callchainQuality (
    ReadView const& view,
    AccountID const& destination,
    AccountID const& source,
    Currency const& currency,
    SField const& sfLow,
    SField const& sfHigh)
{
    if (destination == source)
        return parityRate;

    auto const& sfField = destination < source ? sfLow : sfHigh;

    auto const sle = view.read(
        keylet::line(destination, source, currency));

    if (!sle || !sle->isFieldPresent (sfField))
        return parityRate;

    auto quality = sle->getFieldU32 (sfField);

    // Avoid divide by zero. NIKB CHECKME: if we
    // allow zero qualities now, then we shouldn't.
    if (quality == 0)
        quality = 1;

    return Rate{ quality };
}

Rate
quality_in (
    ReadView const& view,
    AccountID const& uToAccountID,
    AccountID const& uFromAccountID,
    Currency const& currency)
{
    return callchainQuality (view, uToAccountID, uFromAccountID, currency,
        sfLowQualityIn, sfHighQualityIn);
}

Rate
quality_out (
    ReadView const& view,
    AccountID const& uToAccountID,
    AccountID const& uFromAccountID,
    Currency const& currency)
{
    return callchainQuality (view, uToAccountID, uFromAccountID, currency,
        sfLowQualityOut, sfHighQualityOut);
}

} // path
} // callchain
