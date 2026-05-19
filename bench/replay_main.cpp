// Replay a LOBSTER message CSV through the order book.
//
// Usage:
//   lob_replay <path-to-message.csv> [max_messages]
//
// `max_messages` (optional) caps the number of messages replayed; useful for
// quick smoke runs against a multi-million-row file.

#include "Book.hpp"
#include "LobsterReader.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "usage: lob_replay <path-to-message.csv> [max_messages]\n";
        return 1;
    }
    std::string path = argv[1];
    std::size_t maxMsgs = (argc > 2) ? static_cast<std::size_t>(std::atoll(argv[2])) : 0;

    std::cout << "loading " << path << " ...\n";
    auto t0   = std::chrono::steady_clock::now();
    auto msgs = lobster::loadMessages(path);
    auto t1   = std::chrono::steady_clock::now();

    if (msgs.empty()) {
        std::cerr << "no messages loaded (open failure or empty file)\n";
        return 2;
    }
    if (maxMsgs > 0 && msgs.size() > maxMsgs) {
        msgs.resize(maxMsgs);
    }

    std::chrono::duration<double> parseSec = t1 - t0;
    std::cout << "loaded " << msgs.size() << " messages in "
              << parseSec.count() << " s ("
              << (msgs.size() / parseSec.count() / 1e6) << " M/s parse rate)\n";

    Book book;
    auto t2     = std::chrono::steady_clock::now();
    auto stats  = lobster::replay(book, msgs);
    auto t3     = std::chrono::steady_clock::now();
    std::chrono::duration<double> replaySec = t3 - t2;

    std::cout << "replay: " << replaySec.count() << " s ("
              << (msgs.size() / replaySec.count() / 1e6) << " M/s)\n";
    std::cout << "  submits=" << stats.submits
              << " cancels="  << stats.cancels
              << " executes=" << stats.executes
              << " skipped="  << stats.skipped << "\n";
    std::cout << "final state:"
              << " bestBid=" << (book.hasBid() ? book.bestBid() : 0)
              << " bestAsk=" << (book.hasAsk() ? book.bestAsk() : 0)
              << " orderCount=" << book.orderCount() << "\n";
    return 0;
}
