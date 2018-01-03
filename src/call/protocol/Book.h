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

#ifndef CALL_PROTOCOL_BOOK_H_INCLUDED
#define CALL_PROTOCOL_BOOK_H_INCLUDED

#include <call/protocol/Issue.h>
#include <boost/utility/base_from_member.hpp>

namespace call {

/** Specifies an order book.
    The order book is a pair of Issues called in and out.
    @see Issue.
*/
class Book
{
public:
    Issue in;
    Issue out;

    Book ()
    {
    }

    Book (Issue const& in_, Issue const& out_)
        : in (in_)
        , out (out_)
    {
    }
};

bool
isConsistent (Book const& book);

std::string
to_string (Book const& book);

std::ostream&
operator<< (std::ostream& os, Book const& x);

template <class Hasher>
void
hash_append (Hasher& h, Book const& b)
{
    using beast::hash_append;
    hash_append(h, b.in, b.out);
}

Book
reversed (Book const& book);

/** Ordered comparison. */
int
compare (Book const& lhs, Book const& rhs);

/** Equality comparison. */
/** @{ */
bool
operator== (Book const& lhs, Book const& rhs);
bool
operator!= (Book const& lhs, Book const& rhs);
/** @} */

/** Strict weak ordering. */
/** @{ */
bool
operator< (Book const& lhs,Book const& rhs);
bool
operator> (Book const& lhs, Book const& rhs);
bool
operator>= (Book const& lhs, Book const& rhs);
bool
operator<= (Book const& lhs, Book const& rhs);
/** @} */

}

//------------------------------------------------------------------------------

namespace std {

template <>
struct hash <call::Issue>
    : private boost::base_from_member <std::hash <call::Currency>, 0>
    , private boost::base_from_member <std::hash <call::AccountID>, 1>
{
private:
    using currency_hash_type = boost::base_from_member <
        std::hash <call::Currency>, 0>;
    using issuer_hash_type = boost::base_from_member <
        std::hash <call::AccountID>, 1>;

public:
    using value_type = std::size_t;
    using argument_type = call::Issue;

    value_type operator() (argument_type const& value) const
    {
        value_type result (currency_hash_type::member (value.currency));
        if (!isXRP (value.currency))
            boost::hash_combine (result,
                issuer_hash_type::member (value.account));
        return result;
    }
};

//------------------------------------------------------------------------------

template <>
struct hash <call::Book>
{
private:
    using hasher = std::hash <call::Issue>;

    hasher m_hasher;

public:
    using value_type = std::size_t;
    using argument_type = call::Book;

    value_type operator() (argument_type const& value) const
    {
        value_type result (m_hasher (value.in));
        boost::hash_combine (result, m_hasher (value.out));
        return result;
    }
};

}

//------------------------------------------------------------------------------

namespace boost {

template <>
struct hash <call::Issue>
    : std::hash <call::Issue>
{
    using Base = std::hash <call::Issue>;
    // VFALCO NOTE broken in vs2012
    //using Base::Base; // inherit ctors
};

template <>
struct hash <call::Book>
    : std::hash <call::Book>
{
    using Base = std::hash <call::Book>;
    // VFALCO NOTE broken in vs2012
    //using Base::Base; // inherit ctors
};

}

#endif
