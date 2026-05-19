#include "LobsterReader.hpp"
#include "Book.hpp"

#include <fstream>
#include <sstream>
#include <string>

namespace lobster {

std::vector<Message> loadMessages(const std::string& path)
{
    std::vector<Message> out;
    std::ifstream in(path);
    if (!in.is_open()) return out;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string tok;
        Message m{};
        int field = 0;
        bool ok = true;
        while (std::getline(ss, tok, ',')) {
            try {
                switch (field) {
                    case 0: m.time      = std::stod(tok); break;
                    case 1: m.type      = std::stoi(tok); break;
                    case 2: m.orderId   = std::stoi(tok); break;
                    case 3: m.shares    = std::stoi(tok); break;
                    case 4: m.price     = std::stoi(tok); break;
                    case 5: m.direction = std::stoi(tok); break;
                    default: break;
                }
            } catch (...) { ok = false; break; }
            ++field;
        }
        if (ok && field >= 6) out.push_back(m);
    }
    return out;
}

ReplayStats replay(Book& book, const std::vector<Message>& msgs)
{
    ReplayStats s{};
    for (const auto& m : msgs) {
        bool buy = (m.direction == 1);
        switch (m.type) {
            case 1:
                book.addOrder(m.orderId, buy, m.shares, m.price);
                ++s.submits;
                break;
            case 2: {
                // Partial cancel: emulate as full cancel + re-post residual.
                // Without the resting order's original size we can't compute
                // exact residual, so we treat the message as a hint and cancel
                // outright. Talking-point gap; a real `modify` preserves queue
                // position.
                book.cancelOrder(m.orderId);
                ++s.cancels;
                break;
            }
            case 3:
                book.cancelOrder(m.orderId);
                ++s.cancels;
                break;
            case 4: {
                // Visible execution against a resting order. LOBSTER's
                // direction column reports the SIDE OF THE RESTING ORDER, so
                // an aggressive crossing order is the opposite side.
                book.addOrder(-m.orderId, !buy, m.shares, m.price);
                ++s.executes;
                break;
            }
            case 5: // hidden execution — no visible book change
            case 7: // halt
            default:
                ++s.skipped;
                break;
        }
    }
    return s;
}

} // namespace lobster
