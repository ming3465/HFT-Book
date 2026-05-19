#include "GenerateBook.hpp"
#include "Book.hpp"

#include <algorithm>
#include <cmath>

GenerateBook::GenerateBook(int _midPrice, int _spread, double _pCancel,
                           double _pExecute, std::uint64_t seed)
    : midPrice(_midPrice), spread(_spread), pCancel(_pCancel), pExecute(_pExecute),
      rng(seed),
      priceDist(0.0, std::max(1, _spread) * 2.0),
      sizeDist(1, 100),
      sideDist(0.0, 1.0),
      opDist(0.0, 1.0)
{
    liveIds.reserve(1 << 16);
}

void GenerateBook::populate(Book& book, int numOrders)
{
    liveIds.reserve(liveIds.size() + numOrders);
    for (int i = 0; i < numOrders; ++i) {
        bool buy   = sideDist(rng) < 0.5;
        int offset = static_cast<int>(std::lround(std::abs(priceDist(rng)))) + 1;
        int price  = buy ? (midPrice - offset) : (midPrice + offset);
        int shares = sizeDist(rng);
        int id     = nextOrderId++;
        book.addOrder(id, buy, shares, price);
        liveIds.push_back(id);
    }
}

void GenerateBook::stream(Book& book, int numMessages)
{
    std::uniform_int_distribution<size_t> liveDist;
    for (int i = 0; i < numMessages; ++i) {
        double r = opDist(rng);
        if (!liveIds.empty() && r < pCancel) {
            liveDist.param(std::uniform_int_distribution<size_t>::param_type(0, liveIds.size() - 1));
            size_t idx = liveDist(rng);
            int    id  = liveIds[idx];
            book.cancelOrder(id);
            // Swap-pop; we don't track whether the cancel actually succeeded.
            liveIds[idx] = liveIds.back();
            liveIds.pop_back();
        } else if (r < pCancel + pExecute) {
            // Crossing order: aggressive buy at high price (or sell at low) walks the opposite side.
            bool buy   = sideDist(rng) < 0.5;
            int  price = buy ? (midPrice + spread * 4) : (midPrice - spread * 4);
            int  shares = sizeDist(rng);
            int  id     = nextOrderId++;
            book.addOrder(id, buy, shares, price);
            liveIds.push_back(id);
        } else {
            bool buy   = sideDist(rng) < 0.5;
            int offset = static_cast<int>(std::lround(std::abs(priceDist(rng)))) + 1;
            int price  = buy ? (midPrice - offset) : (midPrice + offset);
            int shares = sizeDist(rng);
            int id     = nextOrderId++;
            book.addOrder(id, buy, shares, price);
            liveIds.push_back(id);
        }
    }
}
