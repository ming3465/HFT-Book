#include "Order.hpp"
#include "Limit.hpp"
#include <chrono>
#include <iostream>

static long long nowNs() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

Order::Order(int _idNumber, bool _buyOrSell, int _shares, int _limit)
    : Order(_idNumber, _buyOrSell, _shares, _limit, nowNs()) {}

Order::Order(int _idNumber, bool _buyOrSell, int _shares, int _limit, long long _entryTime)
    : idNumber(_idNumber), buyOrSell(_buyOrSell), shares(_shares), limit(_limit),
    entryTime(_entryTime), eventTime(_entryTime),
    nextOrder(nullptr), prevOrder(nullptr), parentLimit(nullptr) {}

Order::~Order() {}

// Legacy: pre-AVL cancel path. Book::cancelOrder now uses Limit::removeOrder instead.
void Order::cancel()
{
    if (prevOrder == nullptr)
    {
        parentLimit->headOrder = nextOrder;
    } else
    {
        prevOrder->nextOrder = nextOrder;
    }
    if (nextOrder == nullptr)
    {
        parentLimit->tailOrder = prevOrder;
    } else
    {
    nextOrder->prevOrder = prevOrder;
    }
}

void Order::print() const
{
    std::cout << "Order ID: " << idNumber
    << ", Order Type: " << (buyOrSell == 1 ? "buy" : "sell")
    << ", Order Size: " << shares
    << ", Order Limit: " << limit
    << std::endl;
    if (parentLimit != nullptr) {
        parentLimit->print();
    } else {
        std::cout << "No parent limit assigned" << std::endl;
    }
}
