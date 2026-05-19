#ifndef BOOK_HPP
#define BOOK_HPP

#include <mutex>
#include <unordered_map>
#include <vector>

class Limit;
class Order;

struct Trade {
    int buyId;
    int sellId;
    int shares;
    int price;
    long long ts;
};

class Book {
private:
    Limit *buyTree;
    Limit *sellTree;
    Limit *lowestSell;
    Limit *highestBuy;
    std::unordered_map<int, Order*> orderMap;
    std::unordered_map<int, Limit*> limitBuyMap;
    std::unordered_map<int, Limit*> limitSellMap;
    mutable std::mutex bookMutex;

    int    height(Limit* n) const;
    int    balanceFactor(Limit* n) const;
    void   updateHeight(Limit* n);
    Limit* rotateLeft(Limit* n);
    Limit* rotateRight(Limit* n);
    Limit* rebalance(Limit* n);
    Limit* insertAvl(Limit* root, Limit* limit, Limit* parent);
    Limit* removeLimit(Limit* root, int price);
    Limit* removeMinFromSubtree(Limit* root, Limit*& removed);
    Limit* minNode(Limit* n) const;
    Limit* maxNode(Limit* n) const;
    bool   checkBalanced(Limit* n) const;
    void   deleteSubtree(Limit* root);

    void shrinkBookEdge(bool buyOrSell);
    void cancelOrderUnlocked(int orderId);
    int  matchUnlocked(int incomingId, bool side, int shares, int limitPrice,
                       std::vector<Trade>& trades);

public:
    Book();
    ~Book();
    void addOrder(int orderId, bool buyOrSell, int shares, int limitPrice);
    void addLimit(int limitPrice, bool buyOrSell);
    Limit* insert(Limit* root, Limit* limit, Limit* parent=nullptr);
    void updateBookEdge(Limit* newLimit, bool buyOrSell);
    void cancelOrder(int orderId);
    std::vector<Trade> executeOrder(int orderId, bool side, int shares, int limitPrice);
    void printLimit(int limitPrice, bool buyOrSell) const;
    void printOrder(int orderId) const;
    Order* searchOrderMap(int orderId) const;
    Limit* searchLimitBuyMap(int limitPrice) const;
    Limit* searchLimitSellMap(int limitPrice) const;

    int   bestBid() const;
    int   bestAsk() const;
    bool  hasBid() const;
    bool  hasAsk() const;
    size_t orderCount() const;
    bool  isBalanced() const;
};

#endif
