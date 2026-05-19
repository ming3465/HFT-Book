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

Add – O(log M) for the first order at a limit, O(1) for all others
Cancel – O(1)
Execute – O(1)
GetVolumeAtLimit – O(1)
GetBestBid/Offer – O(1)

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

```powershell
out/build/Ninja-UCRT64/lob_bench.exe 20000000 0 1
```

Look for:
- `resident orders: 20000000` — every order made it in
- `RSS post-pop: ~2100 MB` — within ~10% is fine
- `~110 B/order` — the headline number

**Requires ~3 GB of free RAM.** If the machine starts paging, populate time blows up from ~11 s to minutes.

Fast sanity check (~0.5 s, ~110 MB):

```powershell
out/build/Ninja-UCRT64/lob_bench.exe 1000000 0 1
```

### 3. Throughput + sharding

Single-threaded mixed flow (add/cancel/execute):

```powershell
out/build/Ninja-UCRT64/lob_bench.exe 1000000 1000000 1
```

Multi-threaded sharded (one independent `Book` per thread):

```powershell
out/build/Ninja-UCRT64/lob_bench.exe 1000000 1000000 4
```

Per-thread `M/s` should stay close to the single-threaded number (within ~20%); aggregate should scale roughly linearly with thread count until memory bandwidth saturates.

### 4. After any edit

```powershell
cmake --build out/build/Ninja-UCRT64; if ($?) { out/build/Ninja-UCRT64/lob_smoke.exe }
```

If you touched matching or AVL, also run `lob_bench 1000000 1000000 1` — the random flow exercises millions of crossings, cancels-of-edge-orders, and emptied levels that the deterministic smoke test doesn't cover.

