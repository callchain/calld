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
#include <call/app/paths/impl/AmountSpec.h>
#include <call/app/paths/impl/Steps.h>
#include <call/app/paths/impl/StepChecks.h>
#include <call/basics/Log.h>
#include <call/ledger/PaymentSandbox.h>
#include <call/protocol/IOUAmount.h>
#include <call/protocol/Quality.h>
#include <call/protocol/CALLAmount.h>

#include <boost/container/flat_set.hpp>

#include <numeric>
#include <sstream>

namespace call {

template <class TDerived>
class CALLEndpointStep : public StepImp<
    CALLAmount, CALLAmount, CALLEndpointStep<TDerived>>
{
private:
    AccountID acc_;
    bool const isLast_;
    beast::Journal j_;

    // Since this step will always be an endpoint in a strand
    // (either the first or last step) the same cache is used
    // for cachedIn and cachedOut and only one will ever be used
    boost::optional<CALLAmount> cache_;

    boost::optional<EitherAmount>
    cached () const
    {
        if (!cache_)
            return boost::none;
        return EitherAmount (*cache_);
    }

public:
    CALLEndpointStep (
        StrandContext const& ctx,
        AccountID const& acc)
            : acc_(acc)
            , isLast_(ctx.isLast)
            , j_ (ctx.j) {}

    AccountID const& acc () const
    {
        return acc_;
    };

    boost::optional<std::pair<AccountID,AccountID>>
    directStepAccts () const override
    {
        if (isLast_)
            return std::make_pair(callAccount(), acc_);
        return std::make_pair(acc_, callAccount());
    }

    boost::optional<EitherAmount>
    cachedIn () const override
    {
        return cached ();
    }

    boost::optional<EitherAmount>
    cachedOut () const override
    {
        return cached ();
    }

    boost::optional<Quality>
    qualityUpperBound(ReadView const& v, bool& redeems) const override;

    std::pair<CALLAmount, CALLAmount>
    revImp (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        CALLAmount const& out);

    std::pair<CALLAmount, CALLAmount>
    fwdImp (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        CALLAmount const& in);

    std::pair<bool, EitherAmount>
    validFwd (
        PaymentSandbox& sb,
        ApplyView& afView,
        EitherAmount const& in) override;

    // Check for errors and violations of frozen constraints.
    TER check (StrandContext const& ctx) const;

protected:
    CALLAmount
    callLiquidImpl (ReadView& sb, std::int32_t reserveReduction) const
    {
        return call::callLiquid (sb, acc_, reserveReduction, j_);
    }

    std::string logStringImpl (char const* name) const
    {
        std::ostringstream ostr;
        ostr <<
            name << ": " <<
            "\nAcc: " << acc_;
        return ostr.str ();
    }

private:
    template <class P>
    friend bool operator==(
        CALLEndpointStep<P> const& lhs,
        CALLEndpointStep<P> const& rhs);

    friend bool operator!=(
        CALLEndpointStep const& lhs,
        CALLEndpointStep const& rhs)
    {
        return ! (lhs == rhs);
    }

    bool equal (Step const& rhs) const override
    {
        if (auto ds = dynamic_cast<CALLEndpointStep const*> (&rhs))
        {
            return *this == *ds;
        }
        return false;
    }
};

//------------------------------------------------------------------------------

// Flow is used in two different circumstances for transferring funds:
//  o Payments, and
//  o Offer crossing.
// The rules for handling funds in these two cases are almost, but not
// quite, the same.

// Payment CALLEndpointStep class (not offer crossing).
class CALLEndpointPaymentStep : public CALLEndpointStep<CALLEndpointPaymentStep>
{
public:
    using CALLEndpointStep<CALLEndpointPaymentStep>::CALLEndpointStep;

    CALLAmount
    callLiquid (ReadView& sb) const
    {
        return callLiquidImpl (sb, 0);;
    }

    std::string logString () const override
    {
        return logStringImpl ("CALLEndpointPaymentStep");
    }
};

// Offer crossing CALLEndpointStep class (not a payment).
class CALLEndpointOfferCrossingStep :
    public CALLEndpointStep<CALLEndpointOfferCrossingStep>
{
private:

    // For historical reasons, offer crossing is allowed to dig further
    // into the CALL reserve than an ordinary payment.  (I believe it's
    // because the trust line was created after the CALL was removed.)
    // Return how much the reserve should be reduced.
    //
    // Note that reduced reserve only happens if the trust line does not
    // currently exist.
    static std::int32_t computeReserveReduction (
        StrandContext const& ctx, AccountID const& acc)
    {
        if (ctx.isFirst &&
            !ctx.view.read (keylet::line (acc, ctx.strandDeliver)))
                return -1;
        return 0;
    }

public:
    CALLEndpointOfferCrossingStep (
        StrandContext const& ctx, AccountID const& acc)
    : CALLEndpointStep<CALLEndpointOfferCrossingStep> (ctx, acc)
    , reserveReduction_ (computeReserveReduction (ctx, acc))
    {
    }

    CALLAmount
    callLiquid (ReadView& sb) const
    {
        return callLiquidImpl (sb, reserveReduction_);
    }

    std::string logString () const override
    {
        return logStringImpl ("CALLEndpointOfferCrossingStep");
    }

private:
    std::int32_t const reserveReduction_;
};

//------------------------------------------------------------------------------

template <class TDerived>
inline bool operator==(CALLEndpointStep<TDerived> const& lhs,
    CALLEndpointStep<TDerived> const& rhs)
{
    return lhs.acc_ == rhs.acc_ && lhs.isLast_ == rhs.isLast_;
}

template <class TDerived>
boost::optional<Quality>
CALLEndpointStep<TDerived>::qualityUpperBound(
    ReadView const& v, bool& redeems) const
{
    redeems = this->redeems(v, true);
    return Quality{STAmount::uRateOne};
}


template <class TDerived>
std::pair<CALLAmount, CALLAmount>
CALLEndpointStep<TDerived>::revImp (
    PaymentSandbox& sb,
    ApplyView& afView,
    boost::container::flat_set<uint256>& ofrsToRm,
    CALLAmount const& out)
{
    auto const balance = static_cast<TDerived const*>(this)->callLiquid (sb);

    auto const result = isLast_ ? out : std::min (balance, out);

    auto& sender = isLast_ ? callAccount() : acc_;
    auto& receiver = isLast_ ? acc_ : callAccount();
    auto ter   = accountSend (sb, sender, receiver, toSTAmount (result), j_);
    if (ter != tesSUCCESS)
        return {CALLAmount{beast::zero}, CALLAmount{beast::zero}};

    cache_.emplace (result);
    return {result, result};
}

template <class TDerived>
std::pair<CALLAmount, CALLAmount>
CALLEndpointStep<TDerived>::fwdImp (
    PaymentSandbox& sb,
    ApplyView& afView,
    boost::container::flat_set<uint256>& ofrsToRm,
    CALLAmount const& in)
{
    assert (cache_);
    auto const balance = static_cast<TDerived const*>(this)->callLiquid (sb);

    auto const result = isLast_ ? in : std::min (balance, in);

    auto& sender = isLast_ ? callAccount() : acc_;
    auto& receiver = isLast_ ? acc_ : callAccount();
    auto ter   = accountSend (sb, sender, receiver, toSTAmount (result), j_);
    if (ter != tesSUCCESS)
        return {CALLAmount{beast::zero}, CALLAmount{beast::zero}};

    cache_.emplace (result);
    return {result, result};
}

template <class TDerived>
std::pair<bool, EitherAmount>
CALLEndpointStep<TDerived>::validFwd (
    PaymentSandbox& sb,
    ApplyView& afView,
    EitherAmount const& in)
{
    if (!cache_)
    {
        JLOG (j_.error()) << "Expected valid cache in validFwd";
        return {false, EitherAmount (CALLAmount (beast::zero))};
    }

    assert (in.native);

    auto const& callIn = in.call;
    auto const balance = static_cast<TDerived const*>(this)->callLiquid (sb);

    if (!isLast_ && balance < callIn)
    {
        JLOG (j_.error()) << "CALLEndpointStep: Strand re-execute check failed."
            << " Insufficient balance: " << to_string (balance)
            << " Requested: " << to_string (callIn);
        return {false, EitherAmount (balance)};
    }

    if (callIn != *cache_)
    {
        JLOG (j_.error()) << "CALLEndpointStep: Strand re-execute check failed."
            << " ExpectedIn: " << to_string (*cache_)
            << " CachedIn: " << to_string (callIn);
    }
    return {true, in};
}

template <class TDerived>
TER
CALLEndpointStep<TDerived>::check (StrandContext const& ctx) const
{
    if (!acc_)
    {
        JLOG (j_.debug()) << "CALLEndpointStep: specified bad account.";
        return temBAD_PATH;
    }

    auto sleAcc = ctx.view.read (keylet::account (acc_));
    if (!sleAcc)
    {
        JLOG (j_.warn()) << "CALLEndpointStep: can't send or receive CALL from "
                             "non-existent account: "
                          << acc_;
        return terNO_ACCOUNT;
    }

    if (!ctx.isFirst && !ctx.isLast)
    {
        return temBAD_PATH;
    }

    auto& src = isLast_ ? callAccount () : acc_;
    auto& dst = isLast_ ? acc_ : callAccount();
    auto ter = checkFreeze (ctx.view, src, dst, callCurrency ());
    if (ter != tesSUCCESS)
        return ter;

    return tesSUCCESS;
}

//------------------------------------------------------------------------------

namespace test
{
// Needed for testing
bool callEndpointStepEqual (Step const& step, AccountID const& acc)
{
    if (auto xs =
        dynamic_cast<CALLEndpointStep<CALLEndpointPaymentStep> const*> (&step))
    {
        return xs->acc () == acc;
    }
    return false;
}
}

//------------------------------------------------------------------------------

std::pair<TER, std::unique_ptr<Step>>
make_CALLEndpointStep (
    StrandContext const& ctx,
    AccountID const& acc)
{
    TER ter = tefINTERNAL;
    std::unique_ptr<Step> r;
    if (ctx.offerCrossing)
    {
        auto offerCrossingStep =
            std::make_unique<CALLEndpointOfferCrossingStep> (ctx, acc);
        ter = offerCrossingStep->check (ctx);
        r = std::move (offerCrossingStep);
    }
    else // payment
    {
        auto paymentStep =
            std::make_unique<CALLEndpointPaymentStep> (ctx, acc);
        ter = paymentStep->check (ctx);
        r = std::move (paymentStep);
    }
    if (ter != tesSUCCESS)
        return {ter, nullptr};

    return {tesSUCCESS, std::move (r)};
}

} // call
