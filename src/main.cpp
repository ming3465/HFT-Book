#include <cassert>
#include <iostream>

#include "Book.hpp"
#include "Limit.hpp"
#include "Order.hpp"

int main() {
    Book book;

    // Build a small book: buys 95..99, sells 101..105, each 50 shares.
    for (int i = 0; i < 5; ++i) {
        book.addOrder(/*id*/ 1 + i, /*buy*/ true,  /*shares*/ 50, /*price*/ 95 + i);
    }
    for (int i = 0; i < 5; ++i) {
        book.addOrder(/*id*/ 11 + i, /*buy*/ false, /*shares*/ 50, /*price*/ 101 + i);
    }

    std::cout << "[1] initial state\n";
    std::cout << "    bestBid=" << book.bestBid() << " bestAsk=" << book.bestAsk()
              << " orderCount=" << book.orderCount() << "\n";
    assert(book.bestBid() == 99);
    assert(book.bestAsk() == 101);
    assert(book.orderCount() == 10);
    assert(book.isBalanced());

    // ---- Test 1: crossing buy at 103, 120 shares ----
    // Should fully eat 101 (50) + 102 (50), partial 103 (20).
    auto trades = book.executeOrder(/*id*/ 100, /*buy*/ true, /*shares*/ 120, /*price*/ 103);
    std::cout << "[2] crossing buy 120@103 -> " << trades.size() << " trades\n";
    for (auto& t : trades) {
        std::cout << "    fill " << t.shares << "@" << t.price
                  << " buy=" << t.buyId << " sell=" << t.sellId << "\n";
    }
    assert(trades.size() == 3);
    int totalFilled = 0;
    for (auto& t : trades) totalFilled += t.shares;
    assert(totalFilled == 120);
    assert(trades[0].price == 101 && trades[0].shares == 50);
    assert(trades[1].price == 102 && trades[1].shares == 50);
    assert(trades[2].price == 103 && trades[2].shares == 20);

    // Levels 101, 102 should be gone; lowestSell at 103 with 30 shares left.
    assert(book.searchLimitSellMap(101) == nullptr);
    assert(book.searchLimitSellMap(102) == nullptr);
    assert(book.bestAsk() == 103);
    assert(book.isBalanced());

    // ---- Test 2: cancel the buy edge ----
    book.cancelOrder(/*id*/ 5);                              // was the 99 buy
    std::cout << "[3] cancel id=5 -> bestBid=" << book.bestBid() << "\n";
    assert(book.bestBid() == 98);
    assert(book.searchLimitBuyMap(99) == nullptr);
    assert(book.isBalanced());

    // ---- Test 3: aggressive sell at 96 sweeps three buy levels ----
    // Buy levels remaining: 95, 96, 97, 98 (50 shares each). Sell @96 shares=200
    // should consume 98 (50) + 97 (50) + 96 (50) = 150, then residual 50 posts
    // at price 96 on the sell side.
    book.addOrder(/*id*/ 200, /*buy*/ false, /*shares*/ 200, /*price*/ 96);
    std::cout << "[4] aggressive sell 200@96 -> bestBid=" << book.bestBid()
              << " bestAsk=" << book.bestAsk() << "\n";
    assert(book.bestBid() == 95);
    assert(book.bestAsk() == 96);
    assert(book.searchLimitBuyMap(96) == nullptr);
    assert(book.searchLimitBuyMap(97) == nullptr);
    assert(book.searchLimitBuyMap(98) == nullptr);
    assert(book.isBalanced());

    // ---- Test 4: cancel a non-existent order is a no-op ----
    book.cancelOrder(/*id*/ 9999);
    book.cancelOrder(/*id*/ 5);                              // already cancelled

    // ---- Test 5: stress AVL with sequential inserts (worst case for a plain BST) ----
    Book ladder;
    for (int p = 1; p <= 1024; ++p) {
        ladder.addOrder(p, /*buy*/ true, 1, p);
    }
    assert(ladder.bestBid() == 1024);
    assert(ladder.isBalanced());
    for (int p = 1; p <= 1024; ++p) {
        ladder.cancelOrder(p);
        assert(ladder.isBalanced());
    }
    assert(ladder.orderCount() == 0);
    std::cout << "[5] AVL stress (1024 sequential add+cancel) passed\n";

    std::cout << "all asserts passed\n";
    return 0;
}
