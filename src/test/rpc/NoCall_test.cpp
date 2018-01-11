//------------------------------------------------------------------------------
/*
    This file is part of calld: https://github.com/call/calld
    Copyright (c) 2016 Call Labs Inc.

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

#include <call/protocol/Feature.h>
#include <call/protocol/JsonFields.h>
#include <test/jtx.h>

namespace call {

namespace test {

class NoCall_test : public beast::unit_test::suite
{
public:
    void
    testSetAndClear()
    {
        testcase("Set and clear nocall");

        using namespace jtx;
        Env env(*this);

        auto const gw = Account("gateway");
        auto const alice = Account("alice");

        env.fund(CALL(10000), gw, alice);

        auto const USD = gw["USD"];

        Json::Value account_gw;
        account_gw[jss::account] = gw.human();
        Json::Value account_alice;
        account_alice[jss::account] = alice.human();

        for (auto SetOrClear : {true,false})
        {
            // Create a trust line with no-call flag setting
            env( trust(gw, USD(100), alice, SetOrClear ? tfSetNoCall
                                                       : tfClearNoCall));
            env.close();

            // Check no-call flag on sender 'gateway'
            auto lines = env.rpc("json", "account_lines", to_string(account_gw));
            auto const& gline0 = lines[jss::result][jss::lines][0u];
            BEAST_EXPECT(gline0[jss::no_call].asBool() == SetOrClear);

            // Check no-call peer flag on destination 'alice'
            lines = env.rpc("json", "account_lines", to_string(account_alice));
            auto const& aline0 = lines[jss::result][jss::lines][0u];
            BEAST_EXPECT(aline0[jss::no_call_peer].asBool() == SetOrClear);
        }
    }

    void testNegativeBalance(std::initializer_list<uint256> fs)
    {
        testcase("Set nocall on a line with negative balance");

        using namespace jtx;
        Env env(*this, with_features(fs));

        auto const gw = Account("gateway");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const carol = Account("carol");

        env.fund(CALL(10000), gw, alice, bob, carol);

        env.trust(alice["USD"](100), bob);
        env.trust(bob["USD"](100), carol);
        env.close();

        env(pay(alice, carol, carol["USD"](50)), path(bob));

        env(trust(alice, bob["USD"](100), bob, tfSetNoCall));
        env(trust(bob, carol["USD"](100), carol, tfSetNoCall));
        env.close();

        Json::Value params;
        params[jss::source_account] = alice.human();
        params[jss::destination_account] = carol.human();
        params[jss::destination_amount] = [] {
            Json::Value dest_amt;
            dest_amt[jss::currency] = "USD";
            dest_amt[jss::value] = "1";
            dest_amt[jss::issuer] = Account("carol").human();
            return dest_amt;
        }();

        auto const resp = env.rpc("json", "call_path_find", to_string(params));
        BEAST_EXPECT(resp[jss::result][jss::alternatives].size()==1);

        Json::Value account_alice;
        account_alice[jss::account] = alice.human();
        auto const res = env.rpc("json", "account_lines", to_string(account_alice));
        auto const& lines = res[jss::result][jss::lines];
        BEAST_EXPECT(lines.size() == 1);
        BEAST_EXPECT(!lines[0u].isMember(jss::no_call));
    }

    void testPairwise(std::initializer_list<uint256> fs)
    {
        testcase("pairwise NoCall");

        using namespace jtx;
        Env env(*this, with_features(fs));

        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const carol = Account("carol");

        env.fund(CALL(10000), alice, bob, carol);

        env(trust(bob, alice["USD"](100)));
        env(trust(carol, bob["USD"](100)));

        env(trust(bob, alice["USD"](100), alice, tfSetNoCall));
        env(trust(bob, carol["USD"](100), carol, tfSetNoCall));
        env.close();

        Json::Value params;
        params[jss::source_account] = alice.human();
        params[jss::destination_account] = carol.human();
        params[jss::destination_amount] = [] {
            Json::Value dest_amt;
            dest_amt[jss::currency] = "USD";
            dest_amt[jss::value] = "1";
            dest_amt[jss::issuer] = Account("carol").human();
            return dest_amt;
        }();

        auto const resp = env.rpc("json", "call_path_find", to_string(params));
        BEAST_EXPECT(resp[jss::result][jss::alternatives].size() == 0);

        env(pay(alice, carol, bob["USD"](50)), ter(tecPATH_DRY));
    }

    void testDefaultCall(std::initializer_list<uint256> fs)
    {
        testcase("Set default call on an account and check new trustlines");

        using namespace jtx;
        Env env(*this, with_features(fs));

        auto const gw = Account("gateway");
        auto const alice = Account("alice");
        auto const bob =   Account("bob");

        env.fund(CALL(10000), gw, nocall(alice, bob));

        env(fset(bob, asfDefaultCall));

        auto const USD = gw["USD"];

        env(trust(gw, USD(100), alice, 0));
        env(trust(gw, USD(100), bob, 0));

        {
            Json::Value params;
            params[jss::account] = gw.human();
            params[jss::peer] = alice.human();

            auto lines = env.rpc("json", "account_lines", to_string(params));
            auto const& line0 = lines[jss::result][jss::lines][0u];
            BEAST_EXPECT(line0[jss::no_call_peer].asBool() == true);
        }
        {
            Json::Value params;
            params[jss::account] = alice.human();
            params[jss::peer] = gw.human();

            auto lines = env.rpc("json", "account_lines", to_string(params));
            auto const& line0 = lines[jss::result][jss::lines][0u];
            BEAST_EXPECT(line0[jss::no_call].asBool() == true);
        }
        {
            Json::Value params;
            params[jss::account] = gw.human();
            params[jss::peer] = bob.human();

            auto lines = env.rpc("json", "account_lines", to_string(params));
            auto const& line0 = lines[jss::result][jss::lines][0u];
            BEAST_EXPECT(line0[jss::no_call].asBool() == false);
        }
        {
            Json::Value params;
            params[jss::account] = bob.human();
            params[jss::peer] = gw.human();

            auto lines = env.rpc("json", "account_lines", to_string(params));
            auto const& line0 = lines[jss::result][jss::lines][0u];
            BEAST_EXPECT(line0[jss::no_call_peer].asBool() == false);
        }
    }

    void run ()
    {
        testSetAndClear();

        auto withFeatsTests = [this](std::initializer_list<uint256> fs) {
            testNegativeBalance(fs);
            testPairwise(fs);
            testDefaultCall(fs);
        };
        withFeatsTests({});
        withFeatsTests({featureFlow});
        withFeatsTests({featureFlow, fix1373});
        withFeatsTests({featureFlow, fix1373, featureFlowCross});
    }
};

BEAST_DEFINE_TESTSUITE(NoCall,app,call);

} // RPC
} // call

