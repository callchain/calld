//------------------------------------------------------------------------------
/*
  This file is part of calld: https://github.com/callchain/calld
  Copyright (c) 2018-2022 Callchain Foundation.

  This file is part of calld: https://github.com/ripple/rippled
  Copyright (c) 2012-2015 Ripple Labs Inc.

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

#ifndef CALL_TEST_JTX_ACCOUNT_H_INCLUDED
#define CALL_TEST_JTX_ACCOUNT_H_INCLUDED

#include <call/protocol/SecretKey.h>
#include <call/protocol/UintTypes.h>
#include <call/crypto/KeyType.h>
#include <call/beast/hash/uhash.h>
#include <unordered_map>
#include <string>

namespace call {
namespace test {
namespace jtx {

class IOU;

/** Immutable cryptographic account descriptor. */
class Account
{
private:
    // Tag for access to private contr
    struct privateCtorTag{};
public:
    /** The master account. */
    static Account const master;

    Account() = default;
    Account (Account&&) = default;
    Account (Account const&) = default;
    Account& operator= (Account const&) = default;
    Account& operator= (Account&&) = default;

    /** Create an account from a simple string name. */
    /** @{ */
    Account (std::string name,
        KeyType type = KeyType::secp256k1);

    Account (char const* name,
        KeyType type = KeyType::secp256k1)
        : Account(std::string(name), type)
    {
    }

    // This constructor needs to be public so `std::pair` can use it when emplacing
    // into the cache. However, it is logically `private`. This is enforced with the
    // `privateTag` parameter.
    Account (std::string name,
             std::pair<PublicKey, SecretKey> const& keys, Account::privateCtorTag);

    /** @} */

    /** Return the name */
    std::string const&
    name() const
    {
        return name_;
    }

    /** Return the public key. */
    PublicKey const&
    pk() const
    {
        return pk_;
    }

    /** Return the secret key. */
    SecretKey const&
    sk() const
    {
        return sk_;
    }

    /** Returns the Account ID.

        The Account ID is the uint160 hash of the public key.
    */
    AccountID
    id() const
    {
        return id_;
    }

    /** Returns the human readable public key. */
    std::string const&
    human() const
    {
        return human_;
    }

    /** Implicit conversion to AccountID.

        This allows passing an Account
        where an AccountID is expected.
    */
    operator AccountID() const
    {
        return id_;
    }

    /** Returns an IOU for the specified gateway currency. */
    IOU
    operator[](std::string const& s) const;

private:
    static std::unordered_map<
        std::pair<std::string, KeyType>,
            Account, beast::uhash<>> cache_;


    // Return the account from the cache & add it to the cache if needed
    static Account fromCache(std::string name, KeyType type);

    std::string name_;
    PublicKey pk_;
    SecretKey sk_;
    AccountID id_;
    std::string human_; // base58 public key string
};

inline
bool
operator== (Account const& lhs,
    Account const& rhs) noexcept
{
    return lhs.id() == rhs.id();
}

template <class Hasher>
void
hash_append (Hasher& h,
    Account const& v) noexcept
{
    hash_append(h, v.id());
}

inline
bool
operator< (Account const& lhs,
    Account const& rhs) noexcept
{
    return lhs.id() < rhs.id();
}

} // jtx
} // test
} // call

#endif
