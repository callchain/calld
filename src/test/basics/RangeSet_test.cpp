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
#include <callchain/basics/RangeSet.h>
#include <callchain/beast/unit_test.h>

namespace callchain
{
class RangeSet_test : public beast::unit_test::suite
{
public:
    void
    testPrevMissing()
    {
        testcase("prevMissing");

        // Set will include:
        // [ 0, 5]
        // [10,15]
        // [20,25]
        // etc...

        RangeSet<std::uint32_t> set;
        for (std::uint32_t i = 0; i < 10; ++i)
            set.insert(range(10 * i, 10 * i + 5));

        for (std::uint32_t i = 1; i < 100; ++i)
        {
            boost::optional<std::uint32_t> expected;
            // no prev missing in domain for i <= 6
            if (i > 6)
            {
                std::uint32_t const oneBelowRange = (10 * (i / 10)) - 1;

                expected = ((i % 10) > 6) ? (i - 1) : oneBelowRange;
            }
            BEAST_EXPECT(prevMissing(set, i) == expected);
        }
    }

    void
    testToString()
    {
        testcase("toString");

        RangeSet<std::uint32_t> set;
        BEAST_EXPECT(to_string(set) == "empty");

        set.insert(1);
        BEAST_EXPECT(to_string(set) == "1");

        set.insert(range(4u, 6u));
        BEAST_EXPECT(to_string(set) == "1,4-6");

        set.insert(2);
        BEAST_EXPECT(to_string(set) == "1-2,4-6");

        set.erase(range(4u, 5u));
        BEAST_EXPECT(to_string(set) == "1-2,6");
    }
    void
    run()
    {
        testPrevMissing();
        testToString();
    }
};

BEAST_DEFINE_TESTSUITE(RangeSet, callchain_basics, callchain);

}  // namespace callchain
