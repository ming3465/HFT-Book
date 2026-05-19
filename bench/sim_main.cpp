// Interactive order-book simulator.
//
// No external data — you drive a multi-symbol book by hand from a menu.
// Each stock symbol gets its own Book (per-symbol sharding pattern), and
// orders matched within a book go through the same AVL/matching engine
// as lob_smoke and lob_bench.

#include "Book.hpp"

#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

class Simulator {
    std::unordered_map<std::string, std::unique_ptr<Book>> books;
    int nextOrderId = 1;

public:
    void run() {
        std::cout << "=== Limit Order Book Simulator ===\n";
        while (true) {
            printMenu();
            int choice = readInt("> ");
            switch (choice) {
                case 1: doAdd();           break;
                case 2: doBestBidOffer();  break;
                case 3: doVolumeAtLimit(); break;
                case 4: doView();          break;
                case 5: std::cout << "Goodbye.\n"; return;
                default: std::cout << "Invalid choice. Pick 1-5.\n";
            }
        }
    }

private:
    void printMenu() {
        std::cout << "\n1) Add\n"
                  << "2) Get Best Bid / Offer\n"
                  << "3) Get Volume at Limit\n"
                  << "4) View\n"
                  << "5) Exit\n";
    }

    int readInt(const std::string& prompt) {
        while (true) {
            std::cout << prompt;
            int v;
            if (std::cin >> v) return v;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "  (please enter a number)\n";
        }
    }

    std::string readWord(const std::string& prompt) {
        std::cout << prompt;
        std::string s;
        std::cin >> s;
        return s;
    }

    bool readSide(const std::string& prompt) {
        while (true) {
            std::string s = readWord(prompt);
            if (s == "b" || s == "B" || s == "buy"  || s == "BUY")  return true;
            if (s == "s" || s == "S" || s == "sell" || s == "SELL") return false;
            std::cout << "  (please type 'b' or 's')\n";
        }
    }

    // --- option 1 -------------------------------------------------
    void doAdd() {
        std::string symbol = readWord("What type of Stock do you want to add? ");
        bool        buy    = readSide("Buy or sell (b/s)? ");
        int         shares = readInt ("How many? ");
        int         price  = readInt ("Price? ");

        if (shares <= 0 || price <= 0) {
            std::cout << "  (shares and price must be positive)\n";
            return;
        }
        auto& slot = books[symbol];
        if (!slot) {
            slot = std::make_unique<Book>();
            std::cout << "  (created new book for " << symbol << ")\n";
        }
        std::vector<Trade> trades;
        slot->addOrder(nextOrderId, buy, shares, price, trades);

        std::cout << "Order #" << nextOrderId << " (" << (buy ? "buy" : "sell")
                  << " " << shares << " " << symbol << " @ " << price << ") submitted.\n";

        if (!trades.empty()) {
            int filled = 0;
            for (const auto& t : trades) filled += t.shares;
            std::cout << ">>> MATCHED " << trades.size() << " trade"
                      << (trades.size() == 1 ? "" : "s")
                      << ", " << filled << " shares filled instantly:\n";
            for (const auto& t : trades) {
                std::cout << "    " << t.shares << " @ " << t.price
                          << "   buyer #" << t.buyId << " <- seller #" << t.sellId << "\n";
            }
            int residual = shares - filled;
            if (residual > 0)
                std::cout << "    (residual " << residual
                          << " shares posted at " << price << ")\n";
            else
                std::cout << "    (no residual; order fully filled)\n";
        }
        ++nextOrderId;
    }

    // Returns the chosen symbol, or empty string if no selection was made.
    std::string pickBook(const std::string& header) {
        if (books.empty()) {
            std::cout << "  (no books exist yet — use 'Add' first)\n";
            return "";
        }
        std::cout << header << "\n";
        std::vector<std::string> keys;
        int i = 1;
        for (const auto& [k, _] : books) {
            std::cout << "  " << i++ << ") " << k << "\n";
            keys.push_back(k);
        }
        int pick = readInt("Select: ");
        if (pick < 1 || pick > static_cast<int>(keys.size())) {
            std::cout << "  (out of range)\n";
            return "";
        }
        return keys[pick - 1];
    }

    // --- option 2 -------------------------------------------------
    void doBestBidOffer() {
        if (books.empty()) {
            std::cout << "  (no books exist yet — use 'Add' first)\n";
            return;
        }
        std::cout << "All books:\n";
        for (const auto& [sym, bp] : books) {
            std::cout << "  " << sym << "  ";
            if (bp->hasBid()) std::cout << "bestBid=" << bp->bestBid();
            else              std::cout << "bestBid=(none)";
            std::cout << "  ";
            if (bp->hasAsk()) std::cout << "bestAsk=" << bp->bestAsk();
            else              std::cout << "bestAsk=(none)";
            std::cout << "\n";
        }
    }

    // --- option 3 -------------------------------------------------
    void doVolumeAtLimit() {
        int limit = readInt("What is the limit? ");
        if (books.empty()) {
            std::cout << "  (no books exist yet — use 'Add' first)\n";
            return;
        }
        // Show only books that have something resting at the limit price,
        // either side.
        std::vector<std::string> candidates;
        std::cout << "Books with a level at price " << limit << ":\n";
        int i = 1;
        for (const auto& [k, bp] : books) {
            long long buyVol  = bp->volumeAt(limit, true);
            long long sellVol = bp->volumeAt(limit, false);
            if (buyVol + sellVol > 0) {
                std::cout << "  " << i++ << ") " << k
                          << "  (buy " << buyVol << " / sell " << sellVol << ")\n";
                candidates.push_back(k);
            }
        }
        if (candidates.empty()) {
            std::cout << "  (no book has any volume at price " << limit << ")\n";
            return;
        }
        int pick = readInt("Select: ");
        if (pick < 1 || pick > static_cast<int>(candidates.size())) {
            std::cout << "  (out of range)\n";
            return;
        }
        const std::string& sym = candidates[pick - 1];
        long long buyVol  = books[sym]->volumeAt(limit, true);
        long long sellVol = books[sym]->volumeAt(limit, false);
        std::cout << sym << " @ price " << limit
                  << ":  buyVolume=" << buyVol
                  << "  sellVolume=" << sellVol << "\n";
    }

    // --- option 4 -------------------------------------------------
    void printBookSummary(const std::string& sym, const Book& b) {
        std::cout << "  " << sym;
        if (b.hasBid()) std::cout << "  bestBid=" << b.bestBid();
        else            std::cout << "  bestBid=(none)";
        if (b.hasAsk()) std::cout << "  bestAsk=" << b.bestAsk();
        else            std::cout << "  bestAsk=(none)";
        if (b.hasBid() && b.hasAsk())
            std::cout << "  spread=" << (b.bestAsk() - b.bestBid());
        std::cout << "  orders=" << b.orderCount() << "\n";
    }

    void printBookFull(const std::string& sym, const Book& b) {
        std::cout << "=== " << sym << " ===\n";
        printBookSummary(sym, b);
        auto bids = b.levels(true);
        auto asks = b.levels(false);
        std::cout << "  Buy levels (best first):\n";
        if (bids.empty()) std::cout << "    (none)\n";
        for (const auto& lv : bids)
            std::cout << "    " << lv.price << "  volume=" << lv.volume
                      << "  orders=" << lv.size << "\n";
        std::cout << "  Sell levels (best first):\n";
        if (asks.empty()) std::cout << "    (none)\n";
        for (const auto& lv : asks)
            std::cout << "    " << lv.price << "  volume=" << lv.volume
                      << "  orders=" << lv.size << "\n";
    }

    void doView() {
        if (books.empty()) {
            std::cout << "  (no books exist yet — use 'Add' first)\n";
            return;
        }
        std::cout << "Available books:\n";
        std::vector<std::string> keys;
        int i = 1;
        for (const auto& [k, _] : books) {
            std::cout << "  " << i++ << ") " << k << "\n";
            keys.push_back(k);
        }
        std::string choice = readWord("Type 'all' or a book number: ");
        if (choice == "all" || choice == "ALL" || choice == "All") {
            for (const auto& [sym, bp] : books) printBookFull(sym, *bp);
            return;
        }
        // Try number; fall back to symbol match.
        int idx = 0;
        try { idx = std::stoi(choice); } catch (...) { idx = 0; }
        if (idx >= 1 && idx <= static_cast<int>(keys.size())) {
            const std::string& sym = keys[idx - 1];
            printBookFull(sym, *books[sym]);
            return;
        }
        auto it = books.find(choice);
        if (it != books.end()) {
            printBookFull(it->first, *it->second);
            return;
        }
        std::cout << "  (no match for '" << choice << "')\n";
    }
};

} // namespace

int main() {
    Simulator sim;
    sim.run();
    return 0;
}
