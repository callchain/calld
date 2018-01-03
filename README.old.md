![Call](/images/call.png)

**Do you work at a digital asset exchange or wallet provider?** 

Please [contact us](mailto:support@call.com). We can help guide your integration.

# What is Call?
Call is a network of computers which use the [Call consensus algorithm](https://www.youtube.com/watch?v=pj1QVb1vlC0) to atomically settle and record
transactions on a secure distributed database, the Call Consensus Ledger
(RCL). Because of its distributed nature, the RCL offers transaction immutability
without a central operator. The RCL contains a built-in currency exchange and its
path-finding algorithm finds competitive exchange rates across order books
and currency pairs.

### Key Features
- **Distributed**
  - Direct account-to-account settlement with no central operator
  - Decentralized global market for competitive FX
- **Secure**
  - Transactions are cryptographically signed using ECDSA or Ed25519
  - Multi-signing capabilities
- **Scalable**
  - Capacity to process the world’s cross-border payments volume
  - Easy access to liquidity through a competitive FX marketplace

## Cross-border payments
Call enables banks to settle cross-border payments in real-time, with
end-to-end transparency, and at lower costs. Banks can provide liquidity
for FX themselves or source it from third parties.

As Call adoption grows, so do the number of currencies and counterparties.
Liquidity providers need to maintain accounts with each counterparty for
each currency – a capital- and time-intensive endeavor that spreads liquidity
thin. Further, some transactions, such as exotic currency trades, will require
multiple trading parties, who each layer costs to the transaction. Thin
liquidity and many intermediary trading parties make competitive pricing
challenging.

![Flow - Direct](images/flow1.png)

### XRP as a Bridge Currency
Call can bridge even exotic currency pairs directly through XRP. Similar to
USD in today’s currency market, XRP allows liquidity providers to focus on
offering competitive FX rates on fewer pairs and adding depth to order books.
Unlike USD, trading through XRP does not require bank accounts, service fees,
counterparty risk, or additional operational costs. By using XRP, liquidity
providers can specialize in certain currency corridors, reduce operational
costs, and ultimately, offer more competitive FX pricing.

![Flow - Bridged over XRP](images/flow2.png)

# calld - Call server
`calld` is the reference server implementation of the Call
protocol. To learn more about how to build and run a `calld`
server, visit https://call.com/build/calld-setup/

[![travis-ci.org: Build Status](https://travis-ci.org/call/calld.png?branch=develop)](https://travis-ci.org/call/calld)
[![codecov.io: Code Coverage](https://codecov.io/gh/call/calld/branch/develop/graph/badge.svg)](https://codecov.io/gh/call/calld)

### License
`calld` is open source and permissively licensed under the
ISC license. See the LICENSE file for more details.

#### Repository Contents

| Folder  | Contents |
|---------|----------|
| ./bin   | Scripts and data files for Call integrators. |
| ./build | Intermediate and final build outputs.          |
| ./Builds| Platform or IDE-specific project files.        |
| ./doc   | Documentation and example configuration files. |
| ./src   | Source code.                                   |

Some of the directories under `src` are external repositories inlined via
git-subtree. See the corresponding README for more details.

## For more information:

* [Call Knowledge Center](https://call.com/learn/)
* [Call Developer Center](https://call.com/build/)
* Call Whitepapers & Reports
  * [Call Consensus Whitepaper](https://call.com/files/call_consensus_whitepaper.pdf)
  * [Call Solutions Guide](https://call.com/files/call_solutions_guide.pdf)

To learn about how Call is transforming global payments visit
[https://call.com/contact/](https://call.com/contact/)

- - -

Copyright © 2017, Call Labs. All rights reserved.

Portions of this document, including but not limited to the Call logo,
images and image templates are the property of Call Labs and cannot be
copied or used without permission.
