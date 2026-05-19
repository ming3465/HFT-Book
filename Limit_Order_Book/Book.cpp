#include "Book.hpp"
#include "Order.hpp"
#include "Limit.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>

static long long bookNowNs() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

Book::Book() : buyTree(nullptr), sellTree(nullptr), lowestSell(nullptr), highestBuy(nullptr){}

void Book::deleteSubtree(Limit* root)
{
    if (root == nullptr) return;
    deleteSubtree(root->leftChild);
    deleteSubtree(root->rightChild);
    delete root;
}

Book::~Book()
{
    for (auto& [id, order] : orderMap) {
        delete order;
    }
    orderMap.clear();

    deleteSubtree(buyTree);
    deleteSubtree(sellTree);
    limitBuyMap.clear();
    limitSellMap.clear();
}

// ---------- AVL helpers ----------

int Book::height(Limit* n) const
{
    return n ? n->height : 0;
}

int Book::balanceFactor(Limit* n) const
{
    return n ? height(n->leftChild) - height(n->rightChild) : 0;
}

void Book::updateHeight(Limit* n)
{
    if (n) n->height = 1 + std::max(height(n->leftChild), height(n->rightChild));
}

Limit* Book::rotateLeft(Limit* n)
{
    Limit* r  = n->rightChild;
    Limit* rl = r->leftChild;

    r->leftChild  = n;
    n->rightChild = rl;

    r->parent = n->parent;
    n->parent = r;
    if (rl) rl->parent = n;

    updateHeight(n);
    updateHeight(r);
    return r;
}

Limit* Book::rotateRight(Limit* n)
{
    Limit* l  = n->leftChild;
    Limit* lr = l->rightChild;

    l->rightChild = n;
    n->leftChild  = lr;

    l->parent = n->parent;
    n->parent = l;
    if (lr) lr->parent = n;

    updateHeight(n);
    updateHeight(l);
    return l;
}

Limit* Book::rebalance(Limit* n)
{
    if (n == nullptr) return nullptr;
    updateHeight(n);
    int bf = balanceFactor(n);

    if (bf > 1 && balanceFactor(n->leftChild) >= 0) {
        return rotateRight(n);                              // LL
    }
    if (bf > 1 && balanceFactor(n->leftChild) < 0) {
        n->leftChild = rotateLeft(n->leftChild);
        if (n->leftChild) n->leftChild->parent = n;
        return rotateRight(n);                              // LR
    }
    if (bf < -1 && balanceFactor(n->rightChild) <= 0) {
        return rotateLeft(n);                               // RR
    }
    if (bf < -1 && balanceFactor(n->rightChild) > 0) {
        n->rightChild = rotateRight(n->rightChild);
        if (n->rightChild) n->rightChild->parent = n;
        return rotateLeft(n);                               // RL
    }
    return n;
}

Limit* Book::minNode(Limit* n) const
{
    while (n && n->leftChild) n = n->leftChild;
    return n;
}

Limit* Book::maxNode(Limit* n) const
{
    while (n && n->rightChild) n = n->rightChild;
    return n;
}

Limit* Book::insertAvl(Limit* root, Limit* limit, Limit* parent)
{
    if (root == nullptr) {
        limit->parent     = parent;
        limit->leftChild  = nullptr;
        limit->rightChild = nullptr;
        limit->height     = 1;
        return limit;
    }
    if (limit->limitPrice < root->limitPrice) {
        root->leftChild = insertAvl(root->leftChild, limit, root);
        if (root->leftChild) root->leftChild->parent = root;
    } else if (limit->limitPrice > root->limitPrice) {
        root->rightChild = insertAvl(root->rightChild, limit, root);
        if (root->rightChild) root->rightChild->parent = root;
    } else {
        return root;                                        // duplicate price (shouldn't happen)
    }
    return rebalance(root);
}

// Detach the leftmost node from `root`; rebalance up. Output the detached node.
Limit* Book::removeMinFromSubtree(Limit* root, Limit*& removed)
{
    if (root == nullptr) { removed = nullptr; return nullptr; }
    if (root->leftChild == nullptr) {
        removed = root;
        Limit* right = root->rightChild;
        if (right) right->parent = root->parent;
        return right;
    }
    root->leftChild = removeMinFromSubtree(root->leftChild, removed);
    if (root->leftChild) root->leftChild->parent = root;
    return rebalance(root);
}

// Three-case AVL delete by splicing the in-order successor (no payload copy),
// so external pointers in limitBuy/SellMap to the removed node are the only
// references invalidated.
Limit* Book::removeLimit(Limit* root, int price)
{
    if (root == nullptr) return nullptr;

    if (price < root->limitPrice) {
        root->leftChild = removeLimit(root->leftChild, price);
        if (root->leftChild) root->leftChild->parent = root;
    } else if (price > root->limitPrice) {
        root->rightChild = removeLimit(root->rightChild, price);
        if (root->rightChild) root->rightChild->parent = root;
    } else {
        if (root->leftChild == nullptr || root->rightChild == nullptr) {
            Limit* child = root->leftChild ? root->leftChild : root->rightChild;
            if (child) child->parent = root->parent;
            delete root;
            return child;
        }
        Limit* succ    = nullptr;
        Limit* newRight = removeMinFromSubtree(root->rightChild, succ);
        succ->leftChild  = root->leftChild;
        if (succ->leftChild)  succ->leftChild->parent  = succ;
        succ->rightChild = newRight;
        if (succ->rightChild) succ->rightChild->parent = succ;
        succ->parent = root->parent;
        delete root;
        root = succ;
    }

    return rebalance(root);
}

// ---------- Edge maintenance ----------

void Book::updateBookEdge(Limit* newLimit, bool buyOrSell)
{
    if (buyOrSell) {
        if (highestBuy == nullptr || newLimit->limitPrice > highestBuy->limitPrice) {
            highestBuy = newLimit;
        }
    } else {
        if (lowestSell == nullptr || newLimit->limitPrice < lowestSell->limitPrice) {
            lowestSell = newLimit;
        }
    }
}

void Book::shrinkBookEdge(bool buyOrSell)
{
    if (buyOrSell) {
        highestBuy = buyTree ? maxNode(buyTree) : nullptr;
    } else {
        lowestSell = sellTree ? minNode(sellTree) : nullptr;
    }
}

// ---------- Public mutators ----------

void Book::addOrder(int orderId, bool buyOrSell, int shares, int limitPrice)
{
    std::vector<Trade> sink;
    addOrder(orderId, buyOrSell, shares, limitPrice, sink);
}

void Book::addOrder(int orderId, bool buyOrSell, int shares, int limitPrice,
                    std::vector<Trade>& outTrades)
{
    std::lock_guard<std::mutex> g(bookMutex);

    int residual = shares;
    if (buyOrSell && lowestSell != nullptr && limitPrice >= lowestSell->limitPrice) {
        residual = matchUnlocked(orderId, true, shares, limitPrice, outTrades);
    } else if (!buyOrSell && highestBuy != nullptr && limitPrice <= highestBuy->limitPrice) {
        residual = matchUnlocked(orderId, false, shares, limitPrice, outTrades);
    }
    if (residual == 0) return;

    Order* newOrder = new Order(orderId, buyOrSell, residual, limitPrice);
    orderMap.emplace(orderId, newOrder);

    auto& limitMap = buyOrSell ? limitBuyMap : limitSellMap;
    if (limitMap.find(limitPrice) == limitMap.end()) {
        addLimit(limitPrice, buyOrSell);
    }
    limitMap.at(limitPrice)->append(newOrder);
}

void Book::addLimit(int limitPrice, bool buyOrSell)
{
    auto& limitMap = buyOrSell ? limitBuyMap : limitSellMap;
    auto& tree     = buyOrSell ? buyTree : sellTree;

    Limit* newLimit = new Limit(limitPrice);
    limitMap.emplace(limitPrice, newLimit);

    if (tree == nullptr) {
        tree = newLimit;
        if (buyOrSell) highestBuy = newLimit;
        else            lowestSell = newLimit;
    } else {
        tree = insertAvl(tree, newLimit, nullptr);
        if (tree) tree->parent = nullptr;
        updateBookEdge(newLimit, buyOrSell);
    }
}

Limit* Book::insert(Limit* root, Limit* limit, Limit* parent)
{
    return insertAvl(root, limit, parent);
}

void Book::cancelOrder(int orderId)
{
    std::lock_guard<std::mutex> g(bookMutex);
    cancelOrderUnlocked(orderId);
}

void Book::cancelOrderUnlocked(int orderId)
{
    auto it = orderMap.find(orderId);
    if (it == orderMap.end()) return;

    Order* order = it->second;
    Limit* L     = order->parentLimit;
    bool   side  = order->buyOrSell;

    L->removeOrder(order);
    delete order;
    orderMap.erase(it);

    if (L->size == 0) {
        int  price  = L->limitPrice;
        bool isEdge = side ? (L == highestBuy) : (L == lowestSell);
        auto& tree  = side ? buyTree : sellTree;
        auto& map   = side ? limitBuyMap : limitSellMap;

        tree = removeLimit(tree, price);
        if (tree) tree->parent = nullptr;
        map.erase(price);

        if (isEdge) shrinkBookEdge(side);
    }
}

std::vector<Trade> Book::executeOrder(int orderId, bool side, int shares, int limitPrice)
{
    std::lock_guard<std::mutex> g(bookMutex);
    std::vector<Trade> trades;
    matchUnlocked(orderId, side, shares, limitPrice, trades);
    return trades;
}

int Book::matchUnlocked(int incomingId, bool side, int shares, int limitPrice,
                        std::vector<Trade>& trades)
{
    long long ts = bookNowNs();
    while (shares > 0) {
        Limit* L = side ? lowestSell : highestBuy;
        if (L == nullptr) break;
        if (side  && limitPrice < L->limitPrice) break;
        if (!side && limitPrice > L->limitPrice) break;

        Order* resting = L->headOrder;
        if (resting == nullptr) break;

        int fill = std::min(shares, resting->shares);
        trades.push_back(Trade{
            side ? incomingId        : resting->idNumber,
            side ? resting->idNumber : incomingId,
            fill, L->limitPrice, ts
        });

        L->totalVolume -= fill;
        shares         -= fill;
        resting->shares -= fill;
        resting->eventTime = ts;

        if (resting->shares == 0) {
            int restingId = resting->idNumber;
            cancelOrderUnlocked(restingId);
        }
    }
    return shares;
}

// ---------- Read accessors ----------

void Book::printLimit(int limitPrice, bool buyOrSell) const
{
    auto& limitMap = buyOrSell ? limitBuyMap : limitSellMap;
    if (limitMap.find(limitPrice) != limitMap.end()) {
        std::cout << (buyOrSell ? "Buy " : "Sell ") << "limit forwards:" << std::endl;
        limitMap.at(limitPrice)->printForward();
        std::cout << (buyOrSell ? "Buy " : "Sell ") << "limit backwards:" << std::endl;
        limitMap.at(limitPrice)->printBackward();
        limitMap.at(limitPrice)->print();
    } else {
        std::cout << "No " << (buyOrSell ? "buy " : "sell ") << "limit at " << limitPrice << std::endl;
    }
}

void Book::printOrder(int orderId) const
{
    if (orderMap.find(orderId) != orderMap.end()) {
        orderMap.at(orderId)->print();
    }
}

Order* Book::searchOrderMap(int orderId) const
{
    auto it = orderMap.find(orderId);
    if (it != orderMap.end()) return it->second;
    std::cout << "No order number " << orderId << std::endl;
    return nullptr;
}

Limit* Book::searchLimitBuyMap(int limitPrice) const
{
    auto it = limitBuyMap.find(limitPrice);
    if (it != limitBuyMap.end()) return it->second;
    std::cout << "No buy limit at " << limitPrice << std::endl;
    return nullptr;
}

Limit* Book::searchLimitSellMap(int limitPrice) const
{
    auto it = limitSellMap.find(limitPrice);
    if (it != limitSellMap.end()) return it->second;
    std::cout << "No sell limit at " << limitPrice << std::endl;
    return nullptr;
}

int    Book::bestBid() const     { return highestBuy ? highestBuy->limitPrice : 0; }
int    Book::bestAsk() const     { return lowestSell ? lowestSell->limitPrice : 0; }
bool   Book::hasBid() const      { return highestBuy != nullptr; }
bool   Book::hasAsk() const      { return lowestSell != nullptr; }
size_t Book::orderCount() const  { return orderMap.size(); }

bool Book::checkBalanced(Limit* n) const
{
    if (n == nullptr) return true;
    int lh = n->leftChild  ? n->leftChild->height  : 0;
    int rh = n->rightChild ? n->rightChild->height : 0;
    if (std::abs(lh - rh) > 1) return false;
    return checkBalanced(n->leftChild) && checkBalanced(n->rightChild);
}

bool Book::isBalanced() const
{
    return checkBalanced(buyTree) && checkBalanced(sellTree);
}

long long Book::volumeAt(int price, bool buyOrSell) const
{
    std::lock_guard<std::mutex> g(bookMutex);
    const auto& map = buyOrSell ? limitBuyMap : limitSellMap;
    auto it = map.find(price);
    if (it == map.end()) return 0;
    return it->second->totalVolume;
}

std::vector<Book::LevelInfo> Book::levels(bool buyOrSell) const
{
    std::lock_guard<std::mutex> g(bookMutex);
    const auto& map = buyOrSell ? limitBuyMap : limitSellMap;
    std::vector<LevelInfo> out;
    out.reserve(map.size());
    for (const auto& [price, L] : map) {
        out.push_back(LevelInfo{price, L->totalVolume, L->size});
    }
    // Buys descending (best first), sells ascending (best first).
    if (buyOrSell) {
        std::sort(out.begin(), out.end(),
                  [](const LevelInfo& a, const LevelInfo& b){ return a.price > b.price; });
    } else {
        std::sort(out.begin(), out.end(),
                  [](const LevelInfo& a, const LevelInfo& b){ return a.price < b.price; });
    }
    return out;
}
