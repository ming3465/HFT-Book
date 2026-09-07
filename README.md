# HFT-LOB

A C++ limit order book with AVL-balanced price levels, FIFO matching, LOBSTER replay, and per-symbol sharded benchmarks.


## Project layout

```
Limit_Order_Book/   Book, Limit, Order (the core)
Generate_Book/      Synthetic order-flow generator (Poisson-ish)
IO/                 LOBSTER CSV reader + replay
src/main.cpp        lob_smoke entrypoint
bench/bench_main.cpp lob_bench entrypoint
src/BST.cpp,        standalone learning files, not part of any target
src/double_linked_list.cpp
```
**Verified: 20,000,000 resting orders in ~2.1 GB RSS (~110 B/order), 1.84 M ops/sec single-threaded populate, ~7 M ops/sec aggregate across 4 sharded books.**
```cpp
Order
    int idNumber;
    bool buyOrSell; // true -> buy, false -> sell
    int shares;
    int limit;
    long long entryTime;
    long long eventTime;
    Order *nextOrder;
    Order *prevOrder;
    Limit *parentLimit;

Limit  // representing a single limit price
    int limitPrice;
    int size;
    int totalVolume;
    Limit *parent;
    Limit *leftChild;
    Limit *rightChild;
    Order *headOrder;
    Order *tailOrder;

Book
    Limit *buyTree;
    Limit *sellTree;
    Limit *lowestSell;
    Limit *highestBuy;
```

The idea is to have a binary tree of Limit objects sorted by limitPrice, each of which is itself a doubly linked list of Order objects. Each side of the book, the buy Limits and the sell Limits, should be in separate trees so that the inside of the book corresponds to the end and beginning of the buy Limit tree and sell Limit tree, respectively.  Each order is also an entry in a map keyed off idNumber, and each Limit is also an entry in a map keyed off limitPrice.

A variation on this structure is to store the Limits in a sparse array instead of a tree. This will give O(1) always for add operations, but at the cost of making deletion/execution of the last order at the inside limit O(M) as Book.lowestSell/highestBuy have to be updated (for a non-sparse book you will usually get much better than O(M) though). If you store the Limits in a sparse array and linked together in a list then adds become O(log M) again while deletes/executions stay O(1). These are all good implementations; which one is best depends mainly on the sparsity of the book (sparsity being the average distance in cents between limits that have volume, which is generally positively correlated with the instrument price).

With this implementation the operation to be noted is as below : 
```cpp
Add – O(log M) for the first order at a limit, O(1) for all others
Cancel – O(1)
Execute – O(1)
GetVolumeAtLimit – O(1)
GetBestBid/Offer – O(1)
```
where M is the number of price Limits (generally << N the number of orders). AVL tree should be used because the nature of markets is such that orders will be being removed from one side of the tree as they’re being added to the other they need to keep self-balancing the price limit. 

Goal :

To give some idea of the data volumes, the Nasdaq TotalView ITCH feed, which is every event in every instrument traded on the Nasdaq, can have data rates of 20+ gigabytes/day with spikes of 3 megabytes/second or more. The individual messages average about 20 bytes each so this means handling 100,000-200,000 messages per second during high volume periods.

---

## Build

CMake (MSYS2 UCRT64 GCC, Ninja generator):

```powershell
cmake --preset Ninja-UCRT64
cmake --build out/build/Ninja-UCRT64
```

Produces three executables under `out/build/Ninja-UCRT64/`:
- `lob_smoke.exe` — deterministic correctness asserts
- `lob_bench.exe` — capacity + throughput benchmark
- `Limit-Order-Book-Simulator.exe` — alias of `lob_smoke` (kept for the legacy target name)

## Testing

### 1. Correctness (run this first, always)

```powershell
out/build/Ninja-UCRT64/lob_smoke.exe
```

Exercises crossing-spread trades, edge fixup on cancel, empty-level removal, and a 1024-order AVL stress test (sequential ascending prices then sequential cancels — would chain a plain BST). Ends with `all asserts passed` on success; aborts with non-zero exit on any `assert()` failure.

Or via CTest:

```powershell
ctest --test-dir out/build/Ninja-UCRT64 --output-on-failure
```

### 2. Capacity (the 20M-order claim)

`lob_bench` takes three positional arguments:

```
lob_bench.exe <populate_count> <stream_count> <threads>
```

| Argument | Meaning |
|---|---|
| `<populate_count>` | Number of resting orders to seed before any churn |
| `<stream_count>` | Number of mixed messages (40 % cancel / 10 % execute / 50 % add) streamed on top — set to `0` for a pure capacity test |
| `<threads>` | Number of independent `Book` instances run in parallel (per-symbol sharding) |

Capacity run — 20 M resting orders, no churn, single thread:



### 3. Throughput + sharding

Single-threaded mixed flow (add/cancel/execute):

```powershell
out/build/Ninja-UCRT64/lob_bench.exe 1000000 1000000 1
#                                    <pop>   <str>   <thr>
```

Multi-threaded sharded (one independent `Book` per thread):

```powershell
out/build/Ninja-UCRT64/lob_bench.exe 1000000 1000000 4
#                                    <pop>   <str>   <thr>
```



## Interactive simulator

For a hands-on demo with **no data file at all**:

```bash
out/build/Ninja-UCRT64/lob_sim.exe
```
## References

[How to Build a Fast Limit Order Book - wkselph](https://web.archive.org/web/20110219163448/http://howtohft.wordpress.com/2011/02/15/how-to-build-a-fast-limit-order-book/)

[Millions of Orders per Second Matching Engine Testing - Alex Zus](https://habr.com/en/articles/581170/)
The simulator manages **one [Book](Limit_Order_Book/Book.hpp) per stock symbol** — say `MSFT` once and that book is created; say `MSFT` again and your order goes to the same book. Cross-symbol orders don't interact (MSFT buys never match AAPL sells), which is the per-symbol sharding pattern used throughout the project.

