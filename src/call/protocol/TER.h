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

#ifndef CALL_PROTOCOL_TER_H_INCLUDED
#define CALL_PROTOCOL_TER_H_INCLUDED

#include <boost/optional.hpp>
#include <string>

namespace call {

// See https://call.com/wiki/Transaction_errors
//
// "Transaction Engine Result"
// or Transaction ERror.
//
enum TER
{
    // Note: Range is stable.  Exact numbers are currently unstable.  Use tokens.

    // -399 .. -300: L Local error (transaction fee inadequate, exceeds local limit)
    // Only valid during non-consensus processing.
    // Implications:
    // - Not forwarded
    // - No fee check
    telLOCAL_ERROR  = -399,
    telBAD_DOMAIN,
    telBAD_PATH_COUNT,
    telBAD_PUBLIC_KEY,
    telFAILED_PROCESSING,
    telINSUF_FEE_P,
    telNO_DST_PARTIAL,
    telCAN_NOT_QUEUE,
    telCAN_NOT_QUEUE_BALANCE,
    telCAN_NOT_QUEUE_BLOCKS,
    telCAN_NOT_QUEUE_BLOCKED,
    telCAN_NOT_QUEUE_FEE,
    telCAN_NOT_QUEUE_FULL,

    // -299 .. -200: M Malformed (bad signature)
    // Causes:
    // - Transaction corrupt.
    // Implications:
    // - Not applied
    // - Not forwarded
    // - Reject
    // - Can not succeed in any imagined ledger.
    temMALFORMED    = -299,

    temBAD_AMOUNT,
    temBAD_CURRENCY,
    temBAD_EXPIRATION,
    temBAD_FEE,
    temBAD_ISSUER,
    temBAD_LIMIT,
    temBAD_OFFER,
    temBAD_PATH,
    temBAD_PATH_LOOP,
    temBAD_SEND_CALL_LIMIT,
    temBAD_SEND_CALL_MAX,
    temBAD_SEND_CALL_NO_DIRECT,
    temBAD_SEND_CALL_PARTIAL,
    temBAD_SEND_CALL_PATHS,
    temBAD_SEQUENCE,
    temBAD_SIGNATURE,
    temBAD_SRC_ACCOUNT,
    temBAD_TRANSFER_RATE,
    temDST_IS_SRC,
    temDST_NEEDED,
    temINVALID,
    temINVALID_FLAG,
    temREDUNDANT,
    temCALL_EMPTY,
    temDISABLED,
    temBAD_SIGNER,
    temBAD_QUORUM,
    temBAD_WEIGHT,
    temBAD_TICK_SIZE,

    // An intermediate result used internally, should never be returned.
    temUNCERTAIN,
    temUNKNOWN,
    temNICKNAMEEXISTED,
    temBAD_FUNDS,
    
    temID_EXISTED,
    temBAD_INVOICEID,
    temBAD_INVOICE,
    temNOT_SUPPORT,
    temINVOICE_NO_FEE,
    temCURRENCY_NOT_ISSUE,
    temSELF_TRUST,
    temNOT_ADDITIONAL,
    temBAD_INVOICE_AMOUNT,
    temINVOICE_NOT_ACCOUNT,
    temINVOICE_NOT_EXISTS,
    temNO_CODE,
    temCODE_ACCOUNT,
    temBAD_TOTAL_AMOUNT,
    temCODE_FIXED,
    temPRAMETER_KEYWORD,
    temNO_CODE_ENTRY,
    temINVALID_CODE,
    
    // -199 .. -100: F
    //    Failure (sequence number previously used)
    //
    // Causes:
    // - Transaction cannot succeed because of ledger state.
    // - Unexpected ledger state.
    // - C++ exception.
    //
    // Implications:
    // - Not applied
    // - Not forwarded
    // - Could succeed in an imagined ledger.
    tefFAILURE      = -199,
    tefALREADY,
    tefBAD_ADD_AUTH,
    tefBAD_AUTH,
    tefBAD_LEDGER,
    tefCREATED,
    tefEXCEPTION,
    tefINTERNAL,
    tefNO_AUTH_REQUIRED,    // Can't set auth if auth is not required.
    tefPAST_SEQ,
    tefWRONG_PRIOR,
    tefMASTER_DISABLED,
    tefMAX_LEDGER,
    tefBAD_SIGNATURE,
    tefBAD_QUORUM,
    tefNOT_MULTI_SIGNING,
    tefBAD_AUTH_MASTER,
    tefINVARIANT_FAILED,
    terCODE_LOAD_FAILED,
    terCODE_CALL_FAILED,

    // -99 .. -1: R Retry
    //   sequence too high, no funds for txn fee, originating -account
    //   non-existent
    //
    // Cause:
    //   Prior application of another, possibly non-existent, transaction could
    //   allow this transaction to succeed.
    //
    // Implications:
    // - Not applied
    // - May be forwarded
    //   - Results indicating the txn was forwarded: terQUEUED
    //   - All others are not forwarded.
    // - Might succeed later
    // - Hold
    // - Makes hole in sequence which jams transactions.
    terRETRY        = -99,
    terFUNDS_SPENT,      // This is a free transaction, so don't burden network.
    terINSUF_FEE_B,      // Can't pay fee, therefore don't burden network.
    terNO_ACCOUNT,       // Can't pay fee, therefore don't burden network.
    terNO_AUTH,          // Not authorized to hold IOUs.
    terNO_LINE,          // Internal flag.
    terOWNERS,           // Can't succeed with non-zero owner count.
    terPRE_SEQ,          // Can't pay fee, no point in forwarding, so don't
                         // burden network.
    terLAST,             // Process after all other transactions
    terNO_CALL,        // Rippling not allowed
    terQUEUED,           // Transaction is being held in TxQ until fee drops
    terBADTRUST,
    // 0: S Success (success)
    // Causes:
    // - Success.
    // Implications:
    // - Applied
    // - Forwarded
    tesSUCCESS      = 0,

    // 100 .. 159 C
    //   Claim fee only (call transaction with no good paths, pay to
    //   non-existent account, no path)
    //
    // Causes:
    // - Success, but does not achieve optimal result.
    // - Invalid transaction or no effect, but claim fee to use the sequence
    //   number.
    //
    // Implications:
    // - Applied
    // - Forwarded
    //
    // Only allowed as a return code of appliedTransaction when !tapRetry.
    // Otherwise, treated as terRETRY.
    //
    // DO NOT CHANGE THESE NUMBERS: They appear in ledger meta data.
    tecCLAIM                    = 100,
    tecPATH_PARTIAL             = 101,
    tecUNFUNDED_ADD             = 102,
    tecUNFUNDED_OFFER           = 103,
    tecUNFUNDED_PAYMENT         = 104,
    tecFAILED_PROCESSING        = 105,
    tecDIR_FULL                 = 121,
    tecINSUF_RESERVE_LINE       = 122,
    tecINSUF_RESERVE_OFFER      = 123,
    tecNO_DST                   = 124,
    tecNO_DST_INSUF_CALL        = 125,
    tecNO_LINE_INSUF_RESERVE    = 126,
    tecNO_LINE_REDUNDANT        = 127,
    tecPATH_DRY                 = 128,
    tecUNFUNDED                 = 129,  // Deprecated, old ambiguous unfunded.
    tecNO_ALTERNATIVE_KEY       = 130,
    tecNO_REGULAR_KEY           = 131,
    tecOWNERS                   = 132,
    tecNO_ISSUER                = 133,
    tecNO_AUTH                  = 134,
    tecNO_LINE                  = 135,
    tecINSUFF_FEE               = 136,
    tecFROZEN                   = 137,
    tecNO_TARGET                = 138,
    tecNO_PERMISSION            = 139,
    tecNO_ENTRY                 = 140,
    tecINSUFFICIENT_RESERVE     = 141,
    tecNEED_MASTER_KEY          = 142,
    tecDST_TAG_NEEDED           = 143,
    tecINTERNAL                 = 144,
    tecOVERSIZE                 = 145,
    tecCRYPTOCONDITION_ERROR    = 146,
    tecINVARIANT_FAILED         = 147,
    tecOVERISSUED_AMOUNT        = 148, 
    tecBADTOTAL                 = 149,
    tecINSUFFICINET_FUND        = 150,
    tecDIR_NOT_EXISTS           = 151,
    tecUNFUNDED_CONTRACT        = 152,

    // [500-1000) reserved for map error < tesSUCCESS
    // for error + 1000 -> contract error

    tedINVALID_PARAM_NUMS       = 1000,
    tedINVALID_PARAM_TYPE       = 1001,
    tedINVALID_PARAM_ACCOUNT    = 1002,
    tedACCOUNT_NOT_FOUND        = 1003,
    tedINVALID_KEY_TYPE         = 1004,
    tedINVALID_VALUE_TYPE       = 1005,
    tedINVALID_AMOUNT           = 1006,
    tedSEND_CONTRACT_SELF       = 1007,
    tedINVALID_DESTINATION      = 1008,
    tedNO_SUCH_VALUE            = 1009,
    tedCODE_FEE_OUT             = 1010,
};

inline bool isTelLocal(TER x)
{
    return ((x) >= telLOCAL_ERROR && (x) < temMALFORMED);
}

inline bool isTemMalformed(TER x)
{
    return ((x) >= temMALFORMED && (x) < tefFAILURE);
}

inline bool isTefFailure(TER x)
{
    return ((x) >= tefFAILURE && (x) < terRETRY);
}

inline bool isTerRetry(TER x)
{
    return ((x) >= terRETRY && (x) < tesSUCCESS);
}

inline bool isTesSuccess(TER x)
{
    return ((x) == tesSUCCESS);
}

inline bool isTecClaim(TER x)
{
    return ((x) >= tecCLAIM);
}

inline bool isNotSuccess(TER x)
{
    return ((x) < tesSUCCESS);
}

inline bool isMapError(TER x)
{
    return ((x) < 1000) && ((x) >= 500);
}

// VFALCO TODO group these into a shell class along with the defines above.
extern
bool
transResultInfo (TER code, std::string& token, std::string& text);

extern
std::string
transToken (TER code);

extern
std::string
transHuman (TER code);

extern
boost::optional<TER>
transCode(std::string const& token);

} // call

#endif
