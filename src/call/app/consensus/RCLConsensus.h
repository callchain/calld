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

#ifndef CALL_APP_CONSENSUS_RCLCONSENSUS_H_INCLUDED
#define CALL_APP_CONSENSUS_RCLCONSENSUS_H_INCLUDED

#include <BeastConfig.h>
#include <call/app/consensus/RCLCxLedger.h>
#include <call/app/consensus/RCLCxPeerPos.h>
#include <call/app/consensus/RCLCxTx.h>
#include <call/app/misc/FeeVote.h>
#include <call/basics/CountedObject.h>
#include <call/basics/Log.h>
#include <call/beast/utility/Journal.h>
#include <call/consensus/Consensus.h>
#include <call/core/JobQueue.h>
#include <call/overlay/Message.h>
#include <call/protocol/CallLedgerHash.h>
#include <call/protocol/STValidation.h>
#include <call/shamap/SHAMap.h>
#include <atomic>
#include <mutex>

namespace call {

class InboundTransactions;
class LocalTxs;
class LedgerMaster;
class ValidatorKeys;

/** Manages the generic consensus algorithm for use by the RCL.
*/
class RCLConsensus
{
    // Implements the Adaptor template interface required by Consensus.
    class Adaptor
    {
        Application& app_;
        std::unique_ptr<FeeVote> feeVote_;
        LedgerMaster& ledgerMaster_;
        LocalTxs& localTxs_;
        InboundTransactions& inboundTransactions_;
        beast::Journal j_;

        NodeID const nodeID_;
        PublicKey const valPublic_;
        SecretKey const valSecret_;

        // Ledger we most recently needed to acquire
        LedgerHash acquiringLedger_;
        ConsensusParms parms_;

        // The timestamp of the last validation we used
        NetClock::time_point lastValidationTime_;

        // These members are queried via public accesors and are atomic for
        // thread safety.
        std::atomic<bool> validating_{false};
        std::atomic<std::size_t> prevProposers_{0};
        std::atomic<std::chrono::milliseconds> prevRoundTime_{
            std::chrono::milliseconds{0}};
        std::atomic<ConsensusMode> mode_{ConsensusMode::observing};

    public:
        using Ledger_t = RCLCxLedger;
        using NodeID_t = NodeID;
        using TxSet_t = RCLTxSet;
        using PeerPosition_t = RCLCxPeerPos;

        using Result = ConsensusResult<Adaptor>;

        Adaptor(
            Application& app,
            std::unique_ptr<FeeVote>&& feeVote,
            LedgerMaster& ledgerMaster,
            LocalTxs& localTxs,
            InboundTransactions& inboundTransactions,
            ValidatorKeys const & validatorKeys,
            beast::Journal journal);

        bool
        validating() const
        {
            return validating_;
        }

        std::size_t
        prevProposers() const
        {
            return prevProposers_;
        }

        std::chrono::milliseconds
        prevRoundTime() const
        {
            return prevRoundTime_;
        }

        ConsensusMode
        mode() const
        {
            return mode_;
        }

        /** Called before kicking off a new consensus round.

            @param prevLedger Ledger that will be prior ledger for next round
            @return Whether we enter the round proposing
        */
        bool
        preStartRound(RCLCxLedger const & prevLedger);

        /** Consensus simulation parameters
         */
        ConsensusParms const&
        parms() const
        {
            return parms_;
        }

    private:
        //---------------------------------------------------------------------
        // The following members implement the generic Consensus requirements
        // and are marked private to indicate ONLY Consensus<Adaptor> will call
        // them (via friendship). Since they are callled only from Consenus<Adaptor>
        // methods and since RCLConsensus::consensus_ should only be accessed
        // under lock, these will only be called under lock.
        //
        // In general, the idea is that there is only ONE thread that is running
        // consensus code at anytime. The only special case is the dispatched
        // onAccept call, which does not take a lock and relies on Consensus not
        // changing state until a future call to startRound.
        friend class Consensus<Adaptor>;

        /** Attempt to acquire a specific ledger.

            If not available, asynchronously acquires from the network.

            @param ledger The ID/hash of the ledger acquire
            @return Optional ledger, will be seated if we locally had the ledger
        */
        boost::optional<RCLCxLedger>
        acquireLedger(LedgerHash const& ledger);

        /** Relay the given proposal to all peers

            @param peerPos The peer position to relay.
         */
        void
        relay(RCLCxPeerPos const& peerPos);

        /** Relay disputed transacction to peers.

            Only relay if the provided transaction hasn't been shared recently.

            @param tx The disputed transaction to relay.
        */
        void
        relay(RCLCxTx const& tx);

        /** Acquire the transaction set associated with a proposal.

            If the transaction set is not available locally, will attempt
            acquire it from the network.

            @param setId The transaction set ID associated with the proposal
            @return Optional set of transactions, seated if available.
       */
        boost::optional<RCLTxSet>
        acquireTxSet(RCLTxSet::ID const& setId);

        /** Whether the open ledger has any transactions
         */
        bool
        hasOpenTransactions() const;

        /** Number of proposers that have vallidated the given ledger

            @param h The hash of the ledger of interest
            @return the number of proposers that validated a ledger
        */
        std::size_t
        proposersValidated(LedgerHash const& h) const;

        /** Number of proposers that have validated a ledger descended from
           requested ledger.

            @param h The hash of the ledger of interest.
            @return The number of validating peers that have validated a ledger
                    succeeding the one provided.
        */
        std::size_t
        proposersFinished(LedgerHash const& h) const;

        /** Propose the given position to my peers.

            @param proposal Our proposed position
        */
        void
        propose(RCLCxPeerPos::Proposal const& proposal);

        /** Relay the given tx set to peers.

            @param set The TxSet to share.
        */
        void
        relay(RCLTxSet const& set);

        /** Get the ID of the previous ledger/last closed ledger(LCL) on the
           network

            @param ledgerID ID of previous ledger used by consensus
            @param ledger Previous ledger consensus has available
            @param mode Current consensus mode
            @return The id of the last closed network

            @note ledgerID may not match ledger.id() if we haven't acquired
                  the ledger matching ledgerID from the network
         */
        uint256
        getPrevLedger(
            uint256 ledgerID,
            RCLCxLedger const& ledger,
            ConsensusMode mode);

        /** Notified of change in consensus mode

            @param before The prior consensus mode
            @param after The new consensus mode
        */
        void
        onModeChange(
            ConsensusMode before,
            ConsensusMode after);

        /** Close the open ledger and return initial consensus position.

           @param ledger the ledger we are changing to
           @param closeTime When consensus closed the ledger
           @param mode Current consensus mode
           @return Tentative consensus result
        */
        Result
        onClose(
            RCLCxLedger const& ledger,
            NetClock::time_point const& closeTime,
            ConsensusMode mode);

        /** Process the accepted ledger.

            @param result The result of consensus
            @param prevLedger The closed ledger consensus worked from
            @param closeResolution The resolution used in agreeing on an
                                   effective closeTime
            @param rawCloseTimes The unrounded closetimes of ourself and our
                                 peers
            @param mode Our participating mode at the time consensus was
                        declared
            @param consensusJson Json representation of consensus state
        */
        void
        onAccept(
            Result const& result,
            RCLCxLedger const& prevLedger,
            NetClock::duration const& closeResolution,
            ConsensusCloseTimes const& rawCloseTimes,
            ConsensusMode const& mode,
            Json::Value&& consensusJson);

        /** Process the accepted ledger that was a result of simulation/force
            accept.

            @ref onAccept
        */
        void
        onForceAccept(
            Result const& result,
            RCLCxLedger const& prevLedger,
            NetClock::duration const& closeResolution,
            ConsensusCloseTimes const& rawCloseTimes,
            ConsensusMode const& mode,
            Json::Value&& consensusJson);

        /** Notify peers of a consensus state change

            @param ne Event type for notification
            @param ledger The ledger at the time of the state change
            @param haveCorrectLCL Whether we believ we have the correct LCL.
        */
        void
        notify(
            protocol::NodeEvent ne,
            RCLCxLedger const& ledger,
            bool haveCorrectLCL);

        /** Accept a new ledger based on the given transactions.

            @ref onAccept
         */
        void
        doAccept(
            Result const& result,
            RCLCxLedger const& prevLedger,
            NetClock::duration closeResolution,
            ConsensusCloseTimes const& rawCloseTimes,
            ConsensusMode const& mode,
            Json::Value&& consensusJson);

        /** Build the new last closed ledger.

            Accept the given the provided set of consensus transactions and
            build the last closed ledger. Since consensus just agrees on which
            transactions to apply, but not whether they make it into the closed
            ledger, this function also populates retriableTxs with those that
            can be retried in the next round.

            @param previousLedger Prior ledger building upon
            @param set The set of transactions to apply to the ledger
            @param closeTime The the ledger closed
            @param closeTimeCorrect Whether consensus agreed on close time
            @param closeResolution Resolution used to determine consensus close
                                   time
            @param roundTime Duration of this consensus rorund
            @param retriableTxs Populate with transactions to retry in next
                                round
            @return The newly built ledger
      */
        RCLCxLedger
        buildLCL(
            RCLCxLedger const& previousLedger,
            RCLTxSet const& set,
            NetClock::time_point closeTime,
            bool closeTimeCorrect,
            NetClock::duration closeResolution,
            std::chrono::milliseconds roundTime,
            CanonicalTXSet& retriableTxs);

        /** Validate the given ledger and share with peers as necessary

            @param ledger The ledger to validate
            @param proposing Whether we were proposing transactions while
                             generating this ledger.  If we are not proposing,
                             a validation can still be sent to inform peers that
                             we know we aren't fully participating in consensus
                             but are still around and trying to catch up.
        */
        void
        validate(RCLCxLedger const& ledger, bool proposing);

    };

public:
    //! Constructor
    RCLConsensus(
        Application& app,
        std::unique_ptr<FeeVote>&& feeVote,
        LedgerMaster& ledgerMaster,
        LocalTxs& localTxs,
        InboundTransactions& inboundTransactions,
        Consensus<Adaptor>::clock_type const& clock,
        ValidatorKeys const & validatorKeys,
        beast::Journal journal);

    RCLConsensus(RCLConsensus const&) = delete;

    RCLConsensus&
    operator=(RCLConsensus const&) = delete;

    //! Whether we are validating consensus ledgers.
    bool
    validating() const
    {
        return adaptor_.validating();
    }

    //! Get the number of proposing peers that participated in the previous
    //! round.
    std::size_t
    prevProposers() const
    {
        return adaptor_.prevProposers();
    }

    /** Get duration of the previous round.

        The duration of the round is the establish phase, measured from closing
        the open ledger to accepting the consensus result.

        @return Last round duration in milliseconds
    */
    std::chrono::milliseconds
    prevRoundTime() const
    {
        return adaptor_.prevRoundTime();
    }

    //! @see Consensus::mode
    ConsensusMode
    mode() const
    {
        return adaptor_.mode();
    }

    //! @see Consensus::getJson
    Json::Value
    getJson(bool full) const;

    //! @see Consensus::startRound
    void
    startRound(
        NetClock::time_point const& now,
        RCLCxLedger::ID const& prevLgrId,
        RCLCxLedger const& prevLgr);

    //! @see Consensus::timerEntry
    void
    timerEntry(NetClock::time_point const& now);

    //! @see Consensus::gotTxSet
    void
    gotTxSet(NetClock::time_point const& now, RCLTxSet const& txSet);

    // @see Consensus::prevLedgerID
    RCLCxLedger::ID
    prevLedgerID() const
    {
        ScopedLockType _{mutex_};
        return consensus_.prevLedgerID();
    }

    //! @see Consensus::simulate
    void
    simulate(
        NetClock::time_point const& now,
        boost::optional<std::chrono::milliseconds> consensusDelay);

    //! @see Consensus::proposal
    bool
    peerProposal(
        NetClock::time_point const& now,
        RCLCxPeerPos const& newProposal);

    ConsensusParms const &
    parms() const
    {
        return adaptor_.parms();
    }

private:
    // Since Consensus does not provide intrinsic thread-safety, this mutex
    // guards all calls to consensus_. adaptor_ uses atomics internally
    // to allow concurrent access of its data members that have getters.
    mutable std::recursive_mutex mutex_;
    using ScopedLockType = std::lock_guard <std::recursive_mutex>;

    Adaptor adaptor_;
    Consensus<Adaptor> consensus_;
    beast::Journal j_;
};
}

#endif
