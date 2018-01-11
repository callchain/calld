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

#include <BeastConfig.h>
#include <call/app/paths/Credit.h>
#include <call/app/paths/Flow.h>
#include <call/app/paths/impl/AmountSpec.h>
#include <call/app/paths/impl/StrandFlow.h>
#include <call/app/paths/impl/Steps.h>
#include <call/basics/Log.h>
#include <call/protocol/IOUAmount.h>
#include <call/protocol/CALLAmount.h>

#include <boost/container/flat_set.hpp>

#include <numeric>
#include <sstream>

namespace call {

template<class FlowResult>
static
auto finishFlow (PaymentSandbox& sb,
    Issue const& srcIssue, Issue const& dstIssue,
    FlowResult&& f)
{
    path::CallCalc::Output result;
    if (f.ter == tesSUCCESS)
        f.sandbox->apply (sb);
    else
        result.removableOffers = std::move (f.removableOffers);

    result.setResult (f.ter);
    result.actualAmountIn = toSTAmount (f.in, srcIssue);
    result.actualAmountOut = toSTAmount (f.out, dstIssue);

    return result;
};

path::CallCalc::Output
flow (
    PaymentSandbox& sb,
    STAmount const& deliver,
    AccountID const& src,
    AccountID const& dst,
    STPathSet const& paths,
    bool defaultPaths,
    bool partialPayment,
    bool ownerPaysTransferFee,
    bool offerCrossing,
    boost::optional<Quality> const& limitQuality,
    boost::optional<STAmount> const& sendMax,
    beast::Journal j,
    path::detail::FlowDebugInfo* flowDebugInfo)
{
    Issue const srcIssue = [&] {
        if (sendMax)
            return sendMax->issue ();
        if (!isCALL (deliver.issue ().currency))
            return Issue (deliver.issue ().currency, src);
        return callIssue ();
    }();

    Issue const dstIssue = deliver.issue ();

    boost::optional<Issue> sendMaxIssue;
    if (sendMax)
        sendMaxIssue = sendMax->issue ();

    // convert the paths to a collection of strands. Each strand is the collection
    // of account->account steps and book steps that may be used in this payment.
    auto sr = toStrands (sb, src, dst, dstIssue, limitQuality, sendMaxIssue,
        paths, defaultPaths, ownerPaysTransferFee, offerCrossing, j);

    if (sr.first != tesSUCCESS)
    {
        path::CallCalc::Output result;
        result.setResult (sr.first);
        return result;
    }

    auto& strands = sr.second;

    if (j.trace())
    {
        j.trace() << "\nsrc: " << src << "\ndst: " << dst
            << "\nsrcIssue: " << srcIssue << "\ndstIssue: " << dstIssue;
        j.trace() << "\nNumStrands: " << strands.size ();
        for (auto const& curStrand : strands)
        {
            j.trace() << "NumSteps: " << curStrand.size ();
            for (auto const& step : curStrand)
            {
                j.trace() << '\n' << *step << '\n';
            }
        }
    }

    const bool srcIsCALL = isCALL (srcIssue.currency);
    const bool dstIsCALL = isCALL (dstIssue.currency);

    auto const asDeliver = toAmountSpec (deliver);

    // The src account may send either call or iou. The dst account may receive
    // either call or iou. Since CALL and IOU amounts are represented by different
    // types, use templates to tell `flow` about the amount types.
    if (srcIsCALL && dstIsCALL)
    {
        return finishFlow (sb, srcIssue, dstIssue,
            flow<CALLAmount, CALLAmount> (
                sb, strands, asDeliver.call, partialPayment, offerCrossing,
                limitQuality, sendMax, j, flowDebugInfo));
    }

    if (srcIsCALL && !dstIsCALL)
    {
        return finishFlow (sb, srcIssue, dstIssue,
            flow<CALLAmount, IOUAmount> (
                sb, strands, asDeliver.iou, partialPayment, offerCrossing,
                limitQuality, sendMax, j, flowDebugInfo));
    }

    if (!srcIsCALL && dstIsCALL)
    {
        return finishFlow (sb, srcIssue, dstIssue,
            flow<IOUAmount, CALLAmount> (
                sb, strands, asDeliver.call, partialPayment, offerCrossing,
                limitQuality, sendMax, j, flowDebugInfo));
    }

    assert (!srcIsCALL && !dstIsCALL);
    return finishFlow (sb, srcIssue, dstIssue,
        flow<IOUAmount, IOUAmount> (
            sb, strands, asDeliver.iou, partialPayment, offerCrossing,
            limitQuality, sendMax, j, flowDebugInfo));

}

} // call
