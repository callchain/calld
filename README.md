# Callchain Pan-Entertainment Project

Callchain is project based on Ripple with lua virtual machine and pan-entertainment features.

---

## Why Callchain not use Ripple directly

- Ripple does not smart contract. Smart contract is most important feature in current blockchain system, ripple is lack this feature for long time. When user need some new feature, user have to add new transaction type. We need one general method, not endless transction type.

- Ripple has not asset features. In Ripple user can issue any amount of IOU. Although each trust line is limited by user's trust, but the overall amount it not limited. Also there is not asset feature any more other than trust limit amount. In real application, crypto assets are various not unitary.
 
- Ripple does not support more efficient blockchain government. There is no validator gain, no user's native token participation in blockchain, all is done by Ripple Lab Inc. User is not owner of blockchain, but Ripple is the only owner.


## Why Lua not EVM, WASM, V8, Docker or others

- We need only to handle asset in native blockchain. Callchain inherits Ripple's native asset issue in blockchain, user has not to use smart contract to issue assets. Smart contract is used to asset application not for general applications.
- Lua is so small and goes along well with C++ application. We welcome all Lua programmers in game programming.
- We need configurable smart contract not general smart contract. We need be convenient to configure assets in blockchain, not general computing ability in blockchain. First and last, Callchain is asset based blockchain.


## Implementation

### Completed

- Asset features have been done in first version.
- Support junior non fungible asset, namely invoice.
- Add Lua virtual machine to system, and provide some function system api for developer.

### Current Work

- Implement invoice in call path, not in payment post process.
- Support invoice in offer transaction.
- Support invoice in smart contract syscall api.
- overall system test work.

### Next Steps

- More efficient blockchain government.

## Contribution

Anyone is welcome, please contact me with email to landoyjx@163.com.
