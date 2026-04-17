# HFT-Book
```cpp
Order
    int idNumber;
    bool buyOrSell; // true -> buy, false -> sell
    int shares;
    int limit;
    int entryTime;
    int eventTime;
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