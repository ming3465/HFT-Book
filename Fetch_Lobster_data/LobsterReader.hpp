#ifndef LOBSTER_READER_HPP
#define LOBSTER_READER_HPP

#include <cstddef>
#include <string>
#include <vector>

class Book;

namespace lobster {

// LOBSTER message file row (per https://lobsterdata.com/info/DataStructure.php):
//   time, type, orderId, shares, price, direction
// type: 1=submit, 2=cancel-partial, 3=cancel-full,
//       4=execute-visible, 5=execute-hidden, 7=trading halt
// direction: +1 buy, -1 sell
struct Message {
    double time;
    int    type;
    int    orderId;
    int    shares;
    int    price;
    int    direction;
};

// Parse a LOBSTER `*_message_*.csv` file. Returns empty vector on open failure.
std::vector<Message> loadMessages(const std::string& path);

// Replay messages into a Book.
struct ReplayStats {
    std::size_t submits   = 0;
    std::size_t cancels   = 0;
    std::size_t executes  = 0;
    std::size_t skipped   = 0;                              // halts, type-5 hidden, parse anomalies
};
ReplayStats replay(Book& book, const std::vector<Message>& msgs);

} // namespace lobster

#endif
