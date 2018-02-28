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

#ifndef CALL_PROTOCOL_TXFLAGS_H_INCLUDED
#define CALL_PROTOCOL_TXFLAGS_H_INCLUDED

#include <cstdint>

namespace call {

//
// Transaction flags.
//

/** Transaction flags.

    These flags modify the behavior of an operation.

    @note Changing these will create a hard fork
    @ingroup protocol
*/
class TxFlag
{
public:
    static std::uint32_t const requireDestTag = 0x00010000;
};
// VFALCO TODO Move all flags into this container after some study.

// Universal Transaction flags:
const std::uint32_t tfFullyCanonicalSig    = 0x80000000;
const std::uint32_t tfUniversal            = tfFullyCanonicalSig;
const std::uint32_t tfUniversalMask        = ~ tfUniversal;

// AccountSet flags:
// VFALCO TODO Javadoc comment every one of these constants
//const std::uint32_t TxFlag::requireDestTag       = 0x00010000;
const std::uint32_t tfOptionalDestTag      = 0x00020000;
const std::uint32_t tfRequireAuth          = 0x00040000;
const std::uint32_t tfOptionalAuth         = 0x00080000;
const std::uint32_t tfDisallowCALL          = 0x00100000;
const std::uint32_t tfAllowCALL             = 0x00200000;
const std::uint32_t tfAccountSetMask       = ~ (tfUniversal | TxFlag::requireDestTag | tfOptionalDestTag
                                             | tfRequireAuth | tfOptionalAuth
                                             | tfDisallowCALL | tfAllowCALL);

// AccountSet SetFlag/ClearFlag values
const std::uint32_t asfRequireDest         = 1;
const std::uint32_t asfRequireAuth         = 2;
const std::uint32_t asfDisallowCALL         = 3;
const std::uint32_t asfDisableMaster       = 4;
const std::uint32_t asfAccountTxnID        = 5;
const std::uint32_t asfNoFreeze            = 6;
const std::uint32_t asfGlobalFreeze        = 7;
const std::uint32_t asfDefaultCall       = 8;

// OfferCreate flags:
const std::uint32_t tfPassive              = 0x00010000;
const std::uint32_t tfImmediateOrCancel    = 0x00020000;
const std::uint32_t tfFillOrKill           = 0x00040000;
const std::uint32_t tfSell                 = 0x00080000;
const std::uint32_t tfOfferCreateMask      = ~ (tfUniversal | tfPassive | tfImmediateOrCancel | tfFillOrKill | tfSell);

// Payment flags:
const std::uint32_t tfNoCallDirect       = 0x00010000;
const std::uint32_t tfPartialPayment       = 0x00020000;
const std::uint32_t tfLimitQuality         = 0x00040000;
const std::uint32_t tfPaymentMask          = ~ (tfUniversal | tfPartialPayment | tfLimitQuality | tfNoCallDirect);

// TrustSet flags:
const std::uint32_t tfSetfAuth             = 0x00010000;
const std::uint32_t tfSetNoCall          = 0x00020000;
const std::uint32_t tfClearNoCall        = 0x00040000;
const std::uint32_t tfSetFreeze            = 0x00100000;
const std::uint32_t tfClearFreeze          = 0x00200000;
const std::uint32_t tfTrustSetMask         = ~ (tfUniversal | tfSetfAuth | tfSetNoCall | tfClearNoCall
                                             | tfSetFreeze | tfClearFreeze);

// EnableAmendment flags:
const std::uint32_t tfGotMajority          = 0x00010000;
const std::uint32_t tfLostMajority         = 0x00020000;

// PaymentChannel flags:
const std::uint32_t tfRenew                = 0x00010000;
const std::uint32_t tfClose                = 0x00020000;

//IssueSet flags:
const std::uint32_t tfEnaddition  = 0x00010000;
const std::uint32_t tfIssueSetMask = ~tfEnaddition;
} // call

#endif
